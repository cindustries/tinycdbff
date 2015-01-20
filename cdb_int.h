/* cdb_int.h: internal cdb library declarations
 *
 * This file is based on the tinycdb package by Michael Tokarev, mjt@corpit.ru.
 * Public domain.
 *
 * Patched for Chan FatFs by Torsten Raudssus, <torsten@raudss.us>
 *
 */

#include "cdb.h"
#include <errno.h>
#include <string.h>
#include "ff.h" // ChanFs must be in include directory

#ifndef EPROTO
# define EPROTO EINVAL
#endif

#ifndef internal_function
# ifdef __GNUC__
#  define internal_function __attribute__((visibility("hidden")))
# else
#  define internal_function
# endif
#endif

struct cdb_rec {
  unsigned hval;
  unsigned rpos;
};

struct cdb_rl {
  struct cdb_rl *next;
  unsigned cnt;
  struct cdb_rec rec[254];
};

int _cdb_make_write(struct cdb_make *cdbmp,
		    const unsigned char *ptr, unsigned len);
int _cdb_make_fullwrite(FIL *fd, const unsigned char *buf, unsigned len);
int _cdb_make_flush(struct cdb_make *cdbmp);
int _cdb_make_add(struct cdb_make *cdbmp, unsigned hval,
                  const void *key, unsigned klen,
                  const void *val, unsigned vlen);
