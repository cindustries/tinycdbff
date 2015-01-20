/* cdb.h: public cdb include file
 *
 * This file is based on the tinycdb package by Michael Tokarev, mjt@corpit.ru.
 * Public domain.
 *
 * Patched for Chan FatFs by Torsten Raudssus, <torsten@raudss.us>
 *
 */

#include "ff.h" // ChanFs must be in include directory

#ifndef TINYCDBFF_VERSION /* Last version of TinyCDB its based on */
#define TINYCDBFF_VERSION 0.78

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int cdbi_t; /* compatibility */

/* common routines */
unsigned cdb_hash(const void *buf, unsigned len);
unsigned cdb_unpack(const unsigned char buf[4]);
void cdb_pack(unsigned num, unsigned char buf[4]);

#define CDB_STATIC_INIT {0,0,0,0,0,0,0,0}
#define cdb_seqinit(cptr, cdbp) ((*(cptr))=2048)

/* old simple interface */
/* open file using standard routine, then: */
int cdb_seek(FIL *fd, const void *key, unsigned klen, unsigned *dlenp);
int cdb_bread(FIL *fd, void *buf, int len);

/* cdb_make */

struct cdb_make {
  FIL *cdb_fd;			/* file descriptor */
  /* private */
  unsigned cdb_dpos;		/* data position so far */
  unsigned cdb_rcnt;		/* record count so far */
  unsigned char cdb_buf[4096];	/* write buffer */
  unsigned char *cdb_bpos;	/* current buf position */
  struct cdb_rl *cdb_rec[256];	/* list of arrays of record infos */
};

enum cdb_put_mode {
  CDB_PUT_ADD = 0,	/* add unconditionnaly, like cdb_make_add() */
#define CDB_PUT_ADD	CDB_PUT_ADD
  CDB_FIND = CDB_PUT_ADD,
  CDB_PUT_REPLACE,	/* replace: do not place to index OLD record */
#define CDB_PUT_REPLACE	CDB_PUT_REPLACE
  CDB_FIND_REMOVE = CDB_PUT_REPLACE,
  CDB_PUT_INSERT,	/* add only if not already exists */
#define CDB_PUT_INSERT	CDB_PUT_INSERT
  CDB_PUT_WARN,		/* add unconditionally but ret. 1 if exists */
#define CDB_PUT_WARN	CDB_PUT_WARN
  CDB_PUT_REPLACE0,	/* if a record exists, fill old one with zeros */
#define CDB_PUT_REPLACE0 CDB_PUT_REPLACE0
  CDB_FIND_FILL0 = CDB_PUT_REPLACE0
};

int cdb_make_start(struct cdb_make *cdbmp, FIL *fd);
int cdb_make_add(struct cdb_make *cdbmp,
                 const void *key, unsigned klen,
                 const void *val, unsigned vlen);
int cdb_make_exists(struct cdb_make *cdbmp,
                    const void *key, unsigned klen);
int cdb_make_find(struct cdb_make *cdbmp,
                  const void *key, unsigned klen,
                  enum cdb_put_mode mode);
int cdb_make_put(struct cdb_make *cdbmp,
                 const void *key, unsigned klen,
                 const void *val, unsigned vlen,
                 enum cdb_put_mode mode);
int cdb_make_finish(struct cdb_make *cdbmp);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* include guard */
