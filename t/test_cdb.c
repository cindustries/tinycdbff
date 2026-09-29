/* Public domain. Part of the tinycdbff test tree.
 *
 * t/test_cdb.c — host-side behavioural tests for tinycdbff, run against the
 * in-memory FatFs shim in t/mem. Asserts only the observable contract
 * (return codes, recovered value bytes, dlen).
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cdb.h"

static int checks, failures;

#define CHECK(cond) do { \
  ++checks; \
  if (!(cond)) { ++failures; printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

#define S(x) (x), (unsigned)strlen(x)

/* --- helpers ----------------------------------------------------------- */

static void begin(FIL *f, struct cdb_make *m)
{
  f_open(f, "mem.cdb", FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
  CHECK(cdb_make_start(m, f) == 0);
}

/* finish the writer; returns cdb_make_finish() result */
static int finish(struct cdb_make *m) { return cdb_make_finish(m); }

/* 1 if key is found with exactly value (val,vlen), else 0 */
static int has(FIL *f, const char *key, unsigned klen, const void *val, unsigned vlen)
{
  unsigned dlen = 0xdeadbeef;
  unsigned char *b;
  int r = cdb_seek(f, key, klen, &dlen), ok;
  if (r != 1 || dlen != vlen) return 0;
  b = (unsigned char *)malloc(vlen ? vlen : 1);
  ok = cdb_bread(f, b, (int)vlen) == 0 && (vlen == 0 || memcmp(b, val, vlen) == 0);
  free(b);
  return ok;
}

static int hass(FIL *f, const char *key, const char *val)
{
  return has(f, key, (unsigned)strlen(key), val, (unsigned)strlen(val));
}

/* --- tests ------------------------------------------------------------- */

static void test_pack_unpack_hash(void)
{
  unsigned char b[4];
  unsigned v[] = { 0, 1, 255, 256, 0x01020304u, 0x80000000u, 0xffffffffu };
  unsigned i;
  for (i = 0; i < sizeof(v)/sizeof(v[0]); ++i) {
    cdb_pack(v[i], b);
    CHECK(cdb_unpack(b) == v[i]);
  }
  cdb_pack(0x01020304u, b);   /* little-endian on disk */
  CHECK(b[0] == 4 && b[1] == 3 && b[2] == 2 && b[3] == 1);

  CHECK(cdb_hash("", 0) == 5381u);           /* djb seed */
  CHECK(cdb_hash("a", 1) == (((5381u << 5) + 5381u) ^ 'a'));
  CHECK(cdb_hash("abc", 3) == cdb_hash("abc", 3));
  CHECK(cdb_hash("abc", 3) != cdb_hash("abd", 3));
}

static void test_roundtrip(void)
{
  FIL f; struct cdb_make m;
  unsigned char bin[] = { 0, 1, 2, 0, 255, 0, 128 };
  unsigned char *big = (unsigned char *)malloc(10000);
  unsigned i;
  char k[32], v[64];

  for (i = 0; i < 10000; ++i) big[i] = (unsigned char)(i * 7 + 3);
  begin(&f, &m);
  CHECK(cdb_make_add(&m, S("alpha"), S("one")) == 0);
  CHECK(cdb_make_add(&m, S("beta"), bin, sizeof(bin)) == 0);
  CHECK(cdb_make_add(&m, S("empty"), "", 0) == 0);
  CHECK(cdb_make_add(&m, "bin\0key", 7, S("binkey")) == 0);
  CHECK(cdb_make_add(&m, S("big"), big, 10000) == 0);  /* > 4096 write buffer */
  for (i = 0; i < 2000; ++i) {
    sprintf(k, "key%u", i); sprintf(v, "value-%u", i * 31u);
    CHECK(cdb_make_add(&m, S(k), S(v)) == 0);
  }
  CHECK(finish(&m) == 0);

  CHECK(hass(&f, "alpha", "one"));
  CHECK(has(&f, S("beta"), bin, sizeof(bin)));
  CHECK(has(&f, S("empty"), "", 0));
  CHECK(has(&f, "bin\0key", 7, S("binkey")));
  CHECK(cdb_seek(&f, "bin\0zzz", 7, NULL) == 0);
  CHECK(has(&f, S("big"), big, 10000));
  for (i = 0; i < 2000; ++i) {
    sprintf(k, "key%u", i); sprintf(v, "value-%u", i * 31u);
    if (!hass(&f, k, v)) { CHECK(0 && "bulk round-trip"); break; }
  }
  CHECK(cdb_seek(&f, S("key2000"), NULL) == 0);
  free(big);
  f_close(&f);
}

static void test_empty_db(void)
{
  FIL f; struct cdb_make m;
  begin(&f, &m);
  CHECK(finish(&m) == 0);
  CHECK(cdb_seek(&f, S("anything"), NULL) == 0);
  f_close(&f);
}

static void test_not_found(void)
{
  FIL f; struct cdb_make m; unsigned dlen = 77;
  begin(&f, &m);
  CHECK(cdb_make_add(&m, S("present"), S("v")) == 0);
  CHECK(finish(&m) == 0);
  CHECK(cdb_seek(&f, S("absent"), &dlen) == 0);
  CHECK(cdb_seek(&f, S("presen"), &dlen) == 0);    /* prefix */
  CHECK(cdb_seek(&f, S("presentx"), &dlen) == 0);  /* extension */
  CHECK(cdb_seek(&f, S("present"), &dlen) == 1 && dlen == 1);
  f_close(&f);
}

static void test_put_add(void)
{
  FIL f; struct cdb_make m;
  begin(&f, &m);
  CHECK(cdb_make_put(&m, S("k"), S("v1"), CDB_PUT_ADD) == 0);
  CHECK(cdb_make_put(&m, S("k"), S("v2"), CDB_PUT_ADD) == 0);  /* dup allowed */
  CHECK(m.cdb_rcnt == 2);
  CHECK(finish(&m) == 0);
  CHECK(cdb_seek(&f, S("k"), NULL) == 1);
  f_close(&f);
}

static void test_put_insert(void)
{
  FIL f; struct cdb_make m;
  begin(&f, &m);
  CHECK(cdb_make_put(&m, S("k"), S("first"), CDB_PUT_INSERT) == 0);
  errno = 0;
  CHECK(cdb_make_put(&m, S("k"), S("second"), CDB_PUT_INSERT) == 1);
  CHECK(errno == EEXIST);
  CHECK(cdb_make_put(&m, S("other"), S("o"), CDB_PUT_INSERT) == 0);
  CHECK(m.cdb_rcnt == 2);
  CHECK(finish(&m) == 0);
  CHECK(hass(&f, "k", "first"));
  CHECK(hass(&f, "other", "o"));
  f_close(&f);
}

static void test_put_replace(void)
{
  FIL f; struct cdb_make m;
  begin(&f, &m);
  CHECK(cdb_make_put(&m, S("a"), S("a-old"), CDB_PUT_REPLACE) == 0); /* new key */
  CHECK(cdb_make_put(&m, S("b"), S("b-val"), CDB_PUT_REPLACE) == 0);
  CHECK(cdb_make_put(&m, S("c"), S("c-val"), CDB_PUT_REPLACE) == 0);
  /* replace a record with later records behind it (exercises remove_record) */
  CHECK(cdb_make_put(&m, S("a"), S("a-new-longer"), CDB_PUT_REPLACE) == 1);
  CHECK(m.cdb_rcnt == 3);
  CHECK(finish(&m) == 0);
  CHECK(hass(&f, "a", "a-new-longer"));
  CHECK(hass(&f, "b", "b-val"));
  CHECK(hass(&f, "c", "c-val"));
  f_close(&f);
}

static void test_put_warn(void)
{
  FIL f; struct cdb_make m;
  begin(&f, &m);
  CHECK(cdb_make_put(&m, S("k"), S("v1"), CDB_PUT_WARN) == 0);
  CHECK(cdb_make_put(&m, S("k"), S("v2"), CDB_PUT_WARN) == 1);  /* exists, still added */
  CHECK(m.cdb_rcnt == 2);
  CHECK(finish(&m) == 0);
  CHECK(cdb_seek(&f, S("k"), NULL) == 1);
  f_close(&f);
}

static void test_put_replace0(void)
{
  FIL f; struct cdb_make m;
  begin(&f, &m);
  CHECK(cdb_make_put(&m, S("a"), S("a-old"), CDB_PUT_REPLACE0) == 0);
  CHECK(cdb_make_put(&m, S("b"), S("b-val"), CDB_PUT_REPLACE0) == 0);
  CHECK(cdb_make_put(&m, S("a"), S("a-new"), CDB_PUT_REPLACE0) == 1);
  CHECK(m.cdb_rcnt == 2);
  /* replacing the last record just rewinds dpos */
  CHECK(cdb_make_put(&m, S("a"), S("a-newer"), CDB_PUT_REPLACE0) == 1);
  CHECK(m.cdb_rcnt == 2);
  CHECK(finish(&m) == 0);
  CHECK(hass(&f, "a", "a-newer"));
  CHECK(hass(&f, "b", "b-val"));
  f_close(&f);
}

static void test_put_bad_mode(void)
{
  FIL f; struct cdb_make m;
  begin(&f, &m);
  errno = 0;
  CHECK(cdb_make_put(&m, S("k"), S("v"), (enum cdb_put_mode)99) == -1);
  CHECK(errno == EINVAL);
  CHECK(finish(&m) == 0);
  f_close(&f);
}

static void test_exists(void)
{
  FIL f; struct cdb_make m;
  begin(&f, &m);
  CHECK(cdb_make_add(&m, S("k"), S("v")) == 0);
  CHECK(cdb_make_exists(&m, S("k")) == 1);
  CHECK(cdb_make_exists(&m, S("nope")) == 0);
  CHECK(finish(&m) == 0);
  f_close(&f);
}

/* Error paths: a failing f_lseek must propagate (FRESULT is never < 0). */

static void test_err_seek(void)
{
  FIL f; struct cdb_make m; unsigned n;
  begin(&f, &m);
  CHECK(cdb_make_add(&m, S("k"), S("v")) == 0);
  CHECK(finish(&m) == 0);
  /* lseeks in a hit: TOC slot, hash slot, record — fail each in turn */
  for (n = 0; n < 3; ++n) {
    mem_fault_lseek_after(n);
    CHECK(cdb_seek(&f, S("k"), NULL) < 0);
    mem_fault_clear();
  }
  CHECK(cdb_seek(&f, S("k"), NULL) == 1);  /* healthy again */
  f_close(&f);
}

static void test_err_finish(void)
{
  FIL f; struct cdb_make m;
  begin(&f, &m);
  CHECK(cdb_make_add(&m, S("k"), S("v")) == 0);
  mem_fault_lseek_after(0);
  CHECK(cdb_make_finish(&m) < 0);
  mem_fault_clear();
  f_close(&f);
}

static void test_err_make_put(void)
{
  FIL f; struct cdb_make m;
  /* WARN on existing key: lseek #0 is match(), lseek #1 is findrec's final
   * reposition to dpos. Both failures must propagate as -1. */
  begin(&f, &m);
  CHECK(cdb_make_put(&m, S("k"), S("v1"), CDB_PUT_WARN) == 0);
  mem_fault_lseek_after(0);
  CHECK(cdb_make_put(&m, S("k"), S("v2"), CDB_PUT_WARN) == -1);
  mem_fault_clear();
  f_close(&f);

  begin(&f, &m);
  CHECK(cdb_make_put(&m, S("k"), S("v1"), CDB_PUT_WARN) == 0);
  mem_fault_lseek_after(1);
  CHECK(cdb_make_put(&m, S("k"), S("v2"), CDB_PUT_WARN) == -1);
  mem_fault_clear();
  f_close(&f);
}

/* Hardening: the injected fault leaves the file position where it was. Park it
 * (or build the db) so a swallowed error would read/write plausible data
 * instead of failing by accident at EOF. Only a real != FR_OK check returns -1. */

static void test_err_seek_stale_midfile(void)
{
  FIL f; struct cdb_make m; unsigned n, i;
  char k[32], target[32];
  target[0] = 0;
  /* a key in TOC bucket 0 so valid hash tables follow it in the file */
  for (i = 0; !target[0]; ++i) {
    sprintf(k, "t%u", i);
    if ((cdb_hash(k, (unsigned)strlen(k)) & 255) == 0) strcpy(target, k);
  }
  begin(&f, &m);
  CHECK(cdb_make_add(&m, S(target), S("tv")) == 0);
  for (i = 0; i < 300; ++i) {
    sprintf(k, "fill%u", i);
    CHECK(cdb_make_add(&m, S(k), S("x")) == 0);
  }
  CHECK(finish(&m) == 0);
  for (n = 0; n < 3; ++n) {
    mem_fault_lseek_after(n);
    CHECK(cdb_seek(&f, S(target), NULL) < 0);
    mem_fault_clear();
  }
  CHECK(hass(&f, target, "tv"));
  f_close(&f);
}

static void test_err_make_stale_midfile(void)
{
  FIL f; struct cdb_make m; unsigned n;

  /* match(): park the fd on the first record ("k", klen 1) so a swallowed
   * seek failure would read a valid matching header and report success. */
  begin(&f, &m);
  CHECK(cdb_make_put(&m, S("k"), S("v1"), CDB_PUT_WARN) == 0);
  CHECK(cdb_make_exists(&m, S("k")) == 1);        /* flushes buffer */
  CHECK(f_lseek(&f, 2048) == FR_OK);              /* park mid-file */
  mem_fault_lseek_after(0);
  CHECK(cdb_make_put(&m, S("k"), S("v2"), CDB_PUT_WARN) == -1);
  mem_fault_clear();
  f_close(&f);

  /* remove_record() via REPLACE: seeks #1 and #2 (after match's #0) */
  for (n = 1; n <= 2; ++n) {
    begin(&f, &m);
    CHECK(cdb_make_put(&m, S("a"), S("a-old"), CDB_PUT_REPLACE) == 0);
    CHECK(cdb_make_put(&m, S("b"), S("b-val"), CDB_PUT_REPLACE) == 0);
    CHECK(cdb_make_put(&m, S("c"), S("c-val"), CDB_PUT_REPLACE) == 0);
    mem_fault_lseek_after(n);
    CHECK(cdb_make_put(&m, S("a"), S("a-new"), CDB_PUT_REPLACE) == -1);
    mem_fault_clear();
    f_close(&f);
  }

  /* zerofill_record() via REPLACE0: seek #1 (after match's #0) */
  begin(&f, &m);
  CHECK(cdb_make_put(&m, S("a"), S("a-old"), CDB_PUT_REPLACE0) == 0);
  CHECK(cdb_make_put(&m, S("b"), S("b-val"), CDB_PUT_REPLACE0) == 0);
  mem_fault_lseek_after(1);
  CHECK(cdb_make_put(&m, S("a"), S("a-new"), CDB_PUT_REPLACE0) == -1);
  mem_fault_clear();
  f_close(&f);
}

/* --- driver ------------------------------------------------------------ */

static void run(const char *name, void (*fn)(void))
{
  int before = failures;
  fn();
  printf("%s %s\n", failures == before ? "PASS" : "FAIL", name);
}

int main(void)
{
  run("pack/unpack/hash", test_pack_unpack_hash);
  run("round-trip", test_roundtrip);
  run("empty db", test_empty_db);
  run("not-found returns 0", test_not_found);
  run("put ADD", test_put_add);
  run("put INSERT", test_put_insert);
  run("put REPLACE", test_put_replace);
  run("put WARN", test_put_warn);
  run("put REPLACE0", test_put_replace0);
  run("put bad mode", test_put_bad_mode);
  run("make_exists", test_exists);
  run("error: f_lseek fails in cdb_seek", test_err_seek);
  run("error: f_lseek fails in cdb_make_finish", test_err_finish);
  run("error: f_lseek fails in cdb_make_put", test_err_make_put);
  run("error: cdb_seek fault, stale mid-file position", test_err_seek_stale_midfile);
  run("error: cdb_make_put faults, stale mid-file position", test_err_make_stale_midfile);
  printf("\n%d checks, %d failed\n", checks, failures);
  return failures ? 1 : 0;
}
