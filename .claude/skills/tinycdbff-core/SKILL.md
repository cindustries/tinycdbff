---
name: tinycdbff-core
description: >-
  Load before touching any tinycdbff source — cdb.h, cdb_int.h or any cdb_*.c —
  or its tests. Covers the on-disk cdb format contract, the Chan FatFs I/O seam
  (f_read/f_write/f_lseek on a FIL*), the FRESULT-is-never-negative trap, the
  public API return conventions (cdb_seek, cdb_make_*, put modes), the
  embedded/no-POSIX portability rule, and the two-layer test model (opaque
  stub vs in-memory FatFs shim).
---

# tinycdbff core

tinycdbff is a port of Michael Tokarev's **TinyCDB** (a constant database, itself
based on djb's cdb) to **Chan FatFs**. It is a *source-drop module*: the consumer
vendors the `.c` files and links them against a real Chan FatFs that supplies the
genuine `ff.h`. tinycdbff itself is never built, installed or published. Public
domain (The Unlicense).

## The FatFs I/O seam — and the FRESULT trap

All I/O goes through Chan FatFs on a `FIL *` — `f_read`, `f_write`, `f_lseek` —
never POSIX `read`/`write`/`lseek` and never `mmap`. `FIL` is handled **only by
pointer**; cdb code never dereferences it or takes it by value.

`f_lseek`, `f_read` and `f_write` return `FRESULT`, an enum whose values are all
**non-negative** (`FR_OK == 0`). Therefore:

```c
if (f_lseek(fd, pos) != FR_OK) return -1;   /* correct */
if (f_lseek(fd, pos) < 0)      return -1;   /* DEAD — never true, error slips through */
```

Upstream cdb used POSIX calls that return `-1` on error, so any ported `< 0`
check on an `f_*` result is dead error handling. Always compare against `FR_OK`
(or `!= 0`, the style already used in `cdb_make_finish`). `f_read`/`f_write` also
signal short/zero transfers via the `*br`/`*bw` out-param — check both the
`FRESULT` and the byte count (`if (fr || br == 0) return -1;`).

`cdb_bread` is the one I/O helper with C-style semantics: it returns `0` on
success and `-1` on error, so `cdb_bread(...) < 0` is correct.

## The on-disk format is a compatibility contract

The byte layout is the cdb format and must not change — an index built here is
read by `cdb_seek` (and any other cdb reader) byte-for-byte:

- **TOC**: first 2048 bytes = 256 slots × 8 bytes, each `(hashtable_pos:4,
  hashtable_count:4)`.
- **Records**: `(klen:4, vlen:4, key[klen], val[vlen])`, appended from offset 2048.
- **Hash tables**: at the end, each slot 8 bytes `(hval:4, rpos:4)`.
- All 4-byte integers are **little-endian**, packed/unpacked only via `cdb_pack`
  / `cdb_unpack`.
- Hash is djb-derived: `hash = ((hash << 5) + hash) ^ c`, seed `5381` (`cdb_hash`).

Do not "modernise" the layout, the endianness or the hash. `cdb_pack` writing the
top byte as `num >> 8` after two shifts is deliberate and equivalent to the
canonical four-shift form — leave it.

## Public API (cdb.h)

- **Read**: `cdb_seek(FIL*, key, klen, &dlen)` → `1` found (file left positioned
  at the value, `dlen` set to value length), `0` not found, `-1` error. Then
  `cdb_bread(FIL*, buf, len)` reads the value bytes.
- **Write**: `cdb_make_start` → repeated `cdb_make_add` / `cdb_make_put` →
  `cdb_make_finish` (which builds the hash tables, rewrites the TOC, and frees
  the in-RAM record lists). `cdb_make_exists` / `cdb_make_find` probe existing
  records. Put modes: `CDB_PUT_ADD`, `CDB_PUT_REPLACE`, `CDB_PUT_INSERT`,
  `CDB_PUT_WARN`, `CDB_PUT_REPLACE0`.
- `CDB_STATIC_INIT` and `cdb_seqinit` are leftovers from full tinycdb's mmap
  reader, which this port does **not** include — treat them as dead, don't build
  on them.

The writer holds records in RAM (`struct cdb_rl` buckets, 254 recs each) plus a
4096-byte write buffer; `cdb_make_finish` allocates one buffer for the largest
hash table. The consumer is memory-constrained — keep allocations bounded and
freed on every exit path.

## Embedded target — no POSIX, no hosted extras

The real consumer is bare-metal / RTOS Chan FatFs. Use only libc that already
appears here: `string.h` (memcpy/memcmp/memset/memmove), `stdlib.h`
(malloc/free), `errno.h`. Do **not** introduce hosted-only headers such as
`<unistd.h>` — it may not exist on the target. Sources are C99 and must pass
`gcc -std=c99 -Wall -Wextra` clean.

Every source file keeps its public-domain dedication header (based on tinycdb by
Michael Tokarev; Chan FatFs port by Torsten Raudssus). Don't add code under an
incompatible licence.

## Two-layer test model

1. **compile-check** (`t/compile-check.sh`, run in CI): `gcc -fsyntax-only` over
   every source against `t/ff.h`, a **minimal opaque FatFs stub** (declares `FIL`
   as an incomplete type plus the three `f_*` functions). It proves the sources
   *compile*; it cannot run cdb behaviour, and a green compile-check says nothing
   about whether the index is correct.
2. **Behavioural** tests need either real FatFs+storage or an **in-memory FatFs
   shim** — a concrete `FIL` struct over a RAM buffer with real `f_read` /
   `f_write` / `f_lseek` implementations, kept in the test tree so a host can run
   `cdb_make` → `cdb_seek` round-trips (put modes, not-found, error paths)
   without hardware. `FIL` being pointer-only is what makes this shim clean. Keep
   any such harness self-contained — never vendor real Chan FatFs into `t/`.
