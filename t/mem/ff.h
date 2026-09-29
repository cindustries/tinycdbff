/* Public domain. Part of the tinycdbff test tree (in-memory FatFs shim). */
/* t/mem/ff.h — in-memory FatFs shim for HOST-SIDE behavioural tests.
 *
 * A concrete FIL over a RAM buffer with working f_open/f_read/f_write/f_lseek/
 * f_close, plus a fault-injection hook for f_lseek. Self-contained: this is NOT
 * Chan FatFs and must never be linked into a real build. Separate from the
 * opaque compile-check stub t/ff.h — use -I t/mem for behavioural tests only.
 */
#ifndef TINYCDBFF_MEM_FF_H
#define TINYCDBFF_MEM_FF_H

typedef unsigned int  UINT;
typedef unsigned char BYTE;
typedef unsigned long FSIZE_t;

typedef enum { FR_OK = 0, FR_DISK_ERR, FR_INT_ERR, FR_INVALID_OBJECT } FRESULT;

#define FA_READ           0x01
#define FA_WRITE          0x02
#define FA_CREATE_ALWAYS  0x08

typedef struct FIL {
  unsigned char *buf;   /* RAM image of the file */
  FSIZE_t size;         /* bytes in use */
  FSIZE_t cap;          /* allocated bytes */
  FSIZE_t pos;          /* file pointer */
} FIL;

FRESULT f_open (FIL *fp, const char *path, BYTE mode);  /* path ignored */
FRESULT f_close(FIL *fp);
FRESULT f_read (FIL *fp, void *buff, UINT btr, UINT *br);
FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw);
FRESULT f_lseek(FIL *fp, FSIZE_t ofs);

/* fault injection: the (n+1)-th next f_lseek fails with FR_DISK_ERR
 * (n == 0 fails the very next one). One-shot; mem_fault_clear() disarms. */
void mem_fault_lseek_after(unsigned n);
void mem_fault_clear(void);

#endif /* TINYCDBFF_MEM_FF_H */
