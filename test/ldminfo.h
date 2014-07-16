#ifndef __LDMINFO_H_
#define __LDMINFO_H_

#include <linux/version.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/list.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,11)
#include <linux/buffer_head.h>
#endif
#include <features.h>

#define __ssize_t_defined
#include <stdio.h>
#include <unistd.h>

#include "../linux/fs/partitions/ldm.h"

#ifndef BOOL
typedef unsigned char	BOOL;
#endif

#define LDM_CRIT	KERN_CRIT
#define LDM_ERR		KERN_ERR
#define LDM_INFO	KERN_INFO
#define LDM_DEBUG	KERN_DEBUG

extern int device;
extern int debug;

extern int ldm_mem_alloc;
extern int ldm_mem_free;
extern int ldm_mem_size;
extern int ldm_mem_maxa;
extern int ldm_mem_count;
extern int ldm_mem_maxc;

void dump_database (char *name, struct ldmdb *ldb);
void copy_database (char *file, int fd, long long size);
void ldm_free_vblks (struct list_head *vb);

int		open64	(const char *file, int oflag, ...);
long long	lseek64 (int fd, long long offset, int whence);
int		stat64  (const char *file, struct stat64 *buf);
int		isdigit	(int c);
char *		basename(const char *filename);

#endif // __LDMINFO_H_

