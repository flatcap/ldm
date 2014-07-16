#ifndef _LDM_EXTRA_H_
#define _LDM_EXTRA_H_

/* Work around <linux/config.h> */

#define CONFIG_LDM_DEBUG 1

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,1)
void exit (int status);
#define BUG_ON(condition)							\
	do {									\
		if (condition) {						\
			printk (KERN_CRIT "BUG in %s(%d) %s [%s]\n",		\
				__FILE__, __LINE__, __FUNCTION__, #condition);	\
			exit (1);						\
		}								\
	} while(0)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,28)
#define MAX_PART 256
struct parsed_partitions {
	char name[40];
	struct {
		unsigned long from;
		unsigned long size;
		int flags;
	} parts[MAX_PART];
	int next;
	int limit;
};
void put_partition(struct parsed_partitions *p, int n, int from, int size);
#endif

#include <linux/slab.h>
#include <linux/pagemap.h>

#define KDEV_MINOR_BITS		8
#define __mkdev(major,minor)	(((major) << KDEV_MINOR_BITS) + (minor))
#define mk_kdev(major, minor)	((kdev_t) { __mkdev(major,minor) } )

void * __kmalloc (size_t size, int flags, char *fn);
void   __kfree   (const void *objp, char *fn);

#define kmalloc(X,Y)	__kmalloc(X,Y,__FUNCTION__)
#define kfree(X)	__kfree(X,__FUNCTION__)

#ifdef page_cache_release
#undef page_cache_release
#endif

#ifdef __free_page
#undef __free_page
#endif

void page_cache_release (struct page *page);
void __free_page (struct page *page);

#include <linux/list.h>

#undef list_for_each
#define list_for_each(pos, head) for (pos = (head)->next; pos != (head); pos = pos->next)

#endif /* _LDM_EXTRA_H_ */

