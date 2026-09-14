/* t/ff.h — minimal Chan FatFs stub for STANDALONE compile-checking only.
 *
 * tinycdbff is a source-drop module: real builds pull in Chan FatFs
 * (http://elm-chan.org/fsw/ff/) which provides the genuine ff.h. This stub
 * declares only the tiny FatFs surface tinycdbff actually uses (FIL as an
 * opaque handle plus f_read/f_write/f_lseek), so CI can run
 * `gcc -fsyntax-only` over the sources and catch breakage without vendoring
 * all of FatFs. It is NOT a FatFs implementation and must never be shipped or
 * linked into a real build.
 */
#ifndef TINYCDBFF_TEST_FF_H
#define TINYCDBFF_TEST_FF_H

typedef unsigned int  UINT;
typedef unsigned long FSIZE_t;

typedef enum { FR_OK = 0 } FRESULT;

/* Opaque: tinycdbff only ever handles FIL by pointer, never by value or field. */
typedef struct FIL FIL;

FRESULT f_read (FIL *fp, void *buff, UINT btr, UINT *br);
FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw);
FRESULT f_lseek(FIL *fp, FSIZE_t ofs);

#endif /* TINYCDBFF_TEST_FF_H */
