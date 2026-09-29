/* Public domain. Part of the tinycdbff test tree (in-memory FatFs shim). */
/* t/mem/ff.c — in-memory FatFs shim, see ff.h. */
#include <stdlib.h>
#include <string.h>
#include "ff.h"

static int fault_armed;
static unsigned fault_count;

void mem_fault_lseek_after(unsigned n) { fault_armed = 1; fault_count = n; }
void mem_fault_clear(void) { fault_armed = 0; fault_count = 0; }

FRESULT f_open(FIL *fp, const char *path, BYTE mode)
{
  (void)path; (void)mode;
  memset(fp, 0, sizeof(*fp));
  return FR_OK;
}

FRESULT f_close(FIL *fp)
{
  free(fp->buf);
  memset(fp, 0, sizeof(*fp));
  return FR_OK;
}

FRESULT f_read(FIL *fp, void *buff, UINT btr, UINT *br)
{
  FSIZE_t n = 0;
  if (fp->pos < fp->size) {
    n = fp->size - fp->pos;
    if (n > btr) n = btr;
    memcpy(buff, fp->buf + fp->pos, n);
    fp->pos += n;
  }
  *br = (UINT)n;
  return FR_OK;
}

static int grow(FIL *fp, FSIZE_t need)
{
  unsigned char *p;
  FSIZE_t cap;
  if (need <= fp->cap) return 0;
  cap = fp->cap ? fp->cap : 4096;
  while (cap < need) cap *= 2;
  p = (unsigned char *)realloc(fp->buf, cap);
  if (!p) return -1;
  memset(p + fp->cap, 0, cap - fp->cap);
  fp->buf = p;
  fp->cap = cap;
  return 0;
}

FRESULT f_write(FIL *fp, const void *buff, UINT btw, UINT *bw)
{
  *bw = 0;
  if (grow(fp, fp->pos + btw) < 0) return FR_DISK_ERR;
  memcpy(fp->buf + fp->pos, buff, btw);
  fp->pos += btw;
  if (fp->pos > fp->size) fp->size = fp->pos;
  *bw = btw;
  return FR_OK;
}

FRESULT f_lseek(FIL *fp, FSIZE_t ofs)
{
  if (fault_armed) {
    if (fault_count == 0) { fault_armed = 0; return FR_DISK_ERR; }
    --fault_count;
  }
  /* like FatFs in write mode: seeking past EOF extends the file with zeros */
  if (ofs > fp->size) {
    if (grow(fp, ofs) < 0) return FR_DISK_ERR;
    fp->size = ofs;
  }
  fp->pos = ofs;
  return FR_OK;
}
