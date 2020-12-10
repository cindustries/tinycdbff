tinycdbff - CDB for Chan FatFs
==============================

This code is based on [TinyCDB](http://www.corpit.ru/mjt/tinycdb.html) by Michael Tokarev.

Requires [FatFs - Generic FAT File System Module](http://elm-chan.org/fsw/ff/00index_e.html)
to be also included, so that it has access to **ff.h**.

Sample Makefile integration parts

```make
TINYCDBFF_DIR       ?= ./tinycdbff
TINYCDBFF_INCLUDE    = $(TINYCDBFF_DIR)
TINYCDBFF_SOURCE_DIR = $(TINYCDBFF_DIR)/
TINYCDBFF_SOURCE     = \
  cdb_hash.c \
  cdb_make_add.c \
  cdb_make.c \
  cdb_make_put.c \
  cdb_seek.c \
  cdb_unpack.c

CFLAGS              += -I$(TINYCDBFF_INCLUDE)

EXTRA_SOURCE        += $(addprefix $(TINYCDBFF_SOURCE_DIR), $(TINYCDBFF_SOURCE))
```

Sample query usages:

```c
#include "ff.h"
#include "cdb.h"

FIL file;
f_open(&file, filename, FA_READ);
if (cdb_seek(&file, key, keylen, &datalen) > 0) {
  data = malloc(datalen + 1);
  cdb_bread(fd, data, datalen);
  data[datalen] = '\\0';
  printf("key=%s data=%s\\n", key, data);
} else {
  printf("key=%s not found\\n", key); 
}
f_close(fd);
```

Sample create usage:

```c
#include "ff.h"
#include "cdb.h"

FIL *fd;
FRESULT fr;
struct cdb_make cdbm;
char *key, *data;
unsigned keylen, datalen;

/* initialize the database */
fr = f_open(fd, "NEW.CDB", FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
if (fr) return -1; // error on opening
cdb_make_start(&cdbm, fd);

while(have_more_data()) {
  /* initialize key and data */
  if (cdb_make_exists(&cdbm, key, keylen) == 0) {
    cdb_make_add(&cdbm, key, keylen, data, datalen);
    /* or use cdb_make_put() with appropriate flags */
  }
}

/* finalize and close the database */
cdb_make_finish(&cdbm);
f_close(fd);
```

Support
-------

Support at **#hardware** on **irc.perl.org**

[Report issues here at GitHub](https://github.com/cindustries/tinycdbff/issues)
