/**
 * ldminfo - Part of the Linux-NTFS project.
 *
 * Copyright (C) 2001 Richard Russon <ldm@flatcap.org>
 *
 * Documentation is available at http://linux-ntfs.sourceforge.net/ldm
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS source
 * in the file COPYING); if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ldminfo.h"
#include "check.h"

int device = 0;
int debug  = 0;

/* external dependencies */
void *	malloc	(size_t size);
void	free	(void *ptr);

/* general kernel functions */
void * __kmalloc (size_t size, int flags, char *fn);
void   __kfree   (const void *objp, char *fn);
int printk (const char *fmt, ...);
void __brelse (struct buffer_head * buf);

int ldm_mem_alloc = 0;	/* Number of allocations */
int ldm_mem_free  = 0;	/* Number of frees */
int ldm_mem_size  = 0;	/* Memory allocated */
int ldm_mem_maxa  = 0;	/* Max memory allocated */
int ldm_mem_count = 0;	/* Number of memory blocks */
int ldm_mem_maxc  = 0;	/* Max memory blocks */

void * __kmalloc (size_t size, int flags, char *fn)
{
	void *ptr = malloc (size + sizeof (int));
	//printf ("malloc %p %6zu in %s\n", ptr, size, fn);
	ldm_mem_alloc++;
	ldm_mem_size += size;
	ldm_mem_maxa = max (ldm_mem_maxa, ldm_mem_size);
	ldm_mem_count++;
	ldm_mem_maxc = max (ldm_mem_maxc, ldm_mem_count);
	*((int *)ptr) = size;
	return (ptr + sizeof (int));
}

void __kfree (const void *objp, char *fn)
{
	//printf ("free   %p        in %s\n", objp, fn);
	if (!objp)
		return;
	ldm_mem_free++;
	ldm_mem_count--;
	ldm_mem_size -= *((int *)(objp - sizeof (int)));
	free ((void *)(objp - sizeof (int)));
}

int printk (const char *fmt, ...)
{
	static int ignore;
	char buf[1024];
	va_list args;

	va_start (args, fmt);
	vsnprintf (buf, sizeof (buf), fmt, args);
	va_end (args);

	if ((!debug) && (!strcmp (buf, " [LDM]")))
		return 0;

	if ((!debug) && (buf[0] == ' ') && isdigit(buf[1]))
		return 0;

	if (ignore) {
		ignore = (strchr (buf, '\n') == NULL);
		return 0;
	}

	if ((buf[0] == '<') && (buf[2] == '>'))
		if (debug || buf[1] != '7')
			printf ("%s", buf+3);
		else
			ignore = (strchr (buf, '\n') == NULL);
	else
		printf ("%s", buf);

	return 0;
}

void __brelse (struct buffer_head * buf)
{
	if (buf) {
		kfree (buf->b_data);
		kfree (buf);
	}
}

#ifdef CONFIG_BLK_DEV_MD
void md_autodetect_dev(kdev_t dev)
{
}
#endif

struct buffer_head * ldm_bread (kdev_t dev, int block, int size)
{
	struct buffer_head *bh;
	long long offset;

	offset = (((long long) block) * size);

	bh = kmalloc (sizeof (*bh), 0);
	if (bh) {
		memset (bh, 0, sizeof (*bh));

		bh->b_data = kmalloc (size, 0);

		if (lseek64 (device, offset, SEEK_SET) < 0)
			printk (LDM_CRIT "lseek to %lld failed\n", offset);
		else if (read (device, bh->b_data, size) < size)
			printk (LDM_CRIT "read failed\n");
		else
			goto bread_end;

		kfree (bh);
		bh = NULL;
	}
bread_end:
	return bh;
}

unsigned char *read_dev_sector (struct block_device *bdev, unsigned long n, Sector *sect)
{
	struct page        *pg = NULL;
	struct buffer_head *bh = NULL;

	if (!bdev || !sect)
		return NULL;

	pg = (struct page *) kmalloc (sizeof (*pg), 0);
	if (!pg)
		return NULL;

	memset (pg, 0, sizeof (*pg));
	atomic_inc (&pg->count);

	bh = ldm_bread ((*((kdev_t*) (&bdev->bd_dev))), n, 512);
	if (!bh) {
		put_page (pg);
		return NULL;
	}
		
	sect->v = pg;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,8)
	pg->buffers = bh;
#else
	pg->private = (unsigned long) bh;
#endif
	return bh->b_data;
}

void __free_pages(struct page *page, unsigned int order)
{
	atomic_dec (&page->count);
	if (atomic_read (&page->count) < 1) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,8)
		if ((page->buffers) && (page->buffers->b_data))
			kfree (page->buffers->b_data);
		if (page->buffers)
			kfree (page->buffers);
#else
		if ((page->private) && (((struct buffer_head*)(page->private))->b_data))
			kfree (((struct buffer_head*)(page->private))->b_data);
		if (page->private);
			kfree ((void*)(page->private));
#endif
		kfree (page);
	}
}

void page_cache_release (struct page *page)
{
	__free_pages (page, 0);
}

void __free_page (struct page *page)
{
	__free_pages((page), 0);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,28)
void put_partition(struct parsed_partitions *p, int n, int from, int size)
{
	if (n < p->limit) {
		p->parts[n].from = from;
		p->parts[n].size = size;
	}
}
#endif

