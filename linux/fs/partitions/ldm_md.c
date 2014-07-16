/**
 * ldm_md - Support for Windows' LDM RAID
 *
 * Copyright (C) 2001 Jakob Kemi <jakob.kemi@telia.com>
 *
 * Documentation is available at http://linux-ntfs.sf.net/ldm
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
/*
 * If partition detection is to be done in parallell we'd need spinlocks in
 * ldm_md_addpart()
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/list.h>
#include <linux/slab.h>
#include "ldm.h"			// For COMP_* defines

/****************
 *  Additional external declarations
 ****************/

// From md.c
extern int md_hook_add(
	int minor, int level, int chunk_size, int parts, kdev_t* devs);


/****************
 *  Private implementation structures and macros
 ****************/

//#define LDM_MD_DEBUG
#define LDM_MD_MSG_NOMEM	"Out of memory.\n"
#define LDM_MD_MSG_CORRUPT	"Volume data seems inconsistent, bailing out.\n"

// Debug messages
#ifdef LDM_MD_DEBUG
#    define ldm_debug(f, a...)					\
	do {							\
		printk ("%s(ldm_md.c, %d) (DEBUG) %s(): ",	\
			KERN_DEBUG, __LINE__, __func__);	\
		printk (f, ##a);				\
	} while (0)
#else
#   define ldm_debug(...)	do {} while (0)
#endif

// Warning messages
#define ldm_warn(f, a...)					\
	do {							\
		printk ("%s(ldm_md.c, %d) (WARNING): %s(): ",	\
			KERN_WARNING, __LINE__, __func__);	\
		printk (f, ##a);				\
	} while (0)

// Error messages
#define ldm_err(f, a...)					\
	do {							\
		printk ("%s(ldm_md.c, %d) (ERROR): %s(): ",	\
			KERN_ERR, __LINE__, __func__);		\
		printk (f, ##a);				\
	} while(0)

// Entry in volume list
typedef struct {
	struct list_head list;
	u8		dg_guid[16];		// diskgroup GUID
	u32		objid;			// volume id
	u8		level;			// RAID-level
	u8		parts;			// number of parts in set
	u8		chunk_s;		// size of chunks
	u8		added;			// parts added
	kdev_t*		kdevs;			// array of devs for the whole set
} _ldm_md_t;


/****************
 *  Private implementation global data
 ****************/
static LIST_HEAD(_vols);


/****************
 *  Private implementation functions
 ****************/

/**
 * _ldm_md_create - Create a new entry in the volume list
 * @dg_guid: Diskgroup GUID (Used togheter with @objid to identify volume)
 * @objid:   Object id to VBLK-Component
 * @level:   RAID-level (as in md.c)
 * @parts:   Number of partitions in set
 * @chunk_s: Chunksize (in 4k units)
 *
 * This function allocates a new ldm_md_t struct from memory and fill in
 * the initial values. The struct is also added to the volumelist (_vols)
 *
 * Return: ptr	Success
 *	   0	Error
 */
static _ldm_md_t* _ldm_md_create(const u8* dg_guid, u32 objid,
			u8 level, u8 parts, u8 chunk_s)
{
	_ldm_md_t* vol;
	int i;

	u8* ptr = (u8*)kmalloc (
		sizeof(_ldm_md_t) + sizeof(kdev_t) * parts, GFP_KERNEL);

	if (!ptr) {
		ldm_err(LDM_MD_MSG_NOMEM);
		return 0;
	}

	// Assign pointers and set initial data
	vol = (_ldm_md_t*)ptr;
	vol->kdevs = (kdev_t*)(ptr + sizeof(_ldm_md_t));

	memcpy(vol->dg_guid, dg_guid, 16);
	vol->objid   = objid;
	vol->level   = level;
	vol->parts   = parts;
	vol->chunk_s = chunk_s;
	vol->added   = 0;

	// Mark all devs as unused
	for(i = 0; i < parts; i++)
		vol->kdevs[i] = NODEV;

	// Add to list
	list_add_tail(&vol->list, &_vols);

	return vol;
}


/**
 * _ldm_md_get - Get existing entry from volume list or create a new entry
 * @dg_guid: Diskgroup GUID (Used togheter with @objid to identify volume)
 * @objid:   Object id to VBLK-Component
 * @level:   RAID-level (as in md.c)
 * @parts:   Number of partitions in set
 * @chunk_s: Chunksize (in 4k units)
 *
 * This functions try to find the requested entry in the volume list.
 * Otherwise the entry is created.
 *
 * Return: ptr	Success
 *         0	Error
 */
static _ldm_md_t* _ldm_md_get(const u8* dg_guid, u32 objid,
			u8 level, u8 parts, u8 chunk_s)
{
	struct list_head *item;
	_ldm_md_t* vol;

	// Search the list for a match
	list_for_each(item, &_vols) {
		_ldm_md_t* t = list_entry(item, _ldm_md_t, list);
		if (memcmp(t->dg_guid, dg_guid, 16) == 0 && t->objid == objid)
		{
			if (t->level != level ||
			    t->parts != parts ||
			    t->chunk_s != chunk_s) {
				ldm_warn(LDM_MD_MSG_CORRUPT);
				return 0;
			}

			ldm_debug("id %u t %u n %u !FOUND!\n", objid,
			    level, parts);

			return t;
		}
	}

	// No match was found, create a new entry in list
	if ((vol = _ldm_md_create(dg_guid, objid, level, parts, chunk_s)))
		ldm_debug("id %u t %u n %u !ADDED!\n",	objid, level, parts);

	return vol;
}


/****************
 * Public interface funtions
 ****************/

/**
 * ldm_md_destroy - Free up all used resources
 *
 * This function tries to free up all allocated resources
 *
 */
void ldm_md_destroy(void)
{
	struct list_head *item, *tmp;

	list_for_each_safe (item, tmp, &_vols)
		kfree (list_entry (item, _ldm_md_t, list));

	INIT_LIST_HEAD(&_vols);
}


/**
 * ldm_md_addpart - Used to register a new partition
 * @dg_guid:  Diskgroup GUID (Used togheter with @objid to identify volume)
 * @objid:    Object id to VBLK-Component
 * @comptype: Type of component (directly from VBLK struct)
 * @parts:    Number of partitions in set
 * @chunk_s:  Chunksize (in 4k units)
 *
 * This functions gets called from ldm.c when a partition gets registered
 * If the type is supported it might eventually get registered with MD.
 *
 */
void ldm_md_addpart(const u8* dg_guid, u32 objid, u8 comptype,
			u8 n, u8 parts, u8 chunk_s, kdev_t dev)
{
	_ldm_md_t* vol;
	u8 level;

	switch (comptype)
	{
	case COMP_STRIPE:
		level = 0;
		break;
	case COMP_RAID:
	case COMP_BASIC:
	default:
		ldm_debug("Component type %d is unsupported.\n", comptype);
		return;
	}

	if (parts < 2) {
		ldm_debug("Only %d parts.\n", parts);
		return;
	}

	if (!(vol = _ldm_md_get(dg_guid, objid, level, parts, chunk_s)))
		return; // Already logged

	if (n >= vol->parts || vol->kdevs[n] != NODEV) {
		ldm_warn(LDM_MD_MSG_CORRUPT);
		return;
	}

	vol->kdevs[n] = dev;
	vol->added++;
}


/**
 * ldm_md_flush - Register complete volumes and free up all used resources
 *
 * This function register volumes with the MD system.
 *
 */
void ldm_md_flush(void)
{
	struct list_head *item, *tmp;
	static int minor = CONFIG_LDM_MD_MINOR;

	// Scan for complete volumes
	list_for_each_safe(item, tmp, &_vols) {
		_ldm_md_t* v = list_entry(item, _ldm_md_t, list);

		if (v->added == v->parts) {
			ldm_debug("Registering vol %u (parts: %d, type %d).\n",
			    v->objid, v->parts, v->level);

			// Register to MD driver
			md_hook_add(minor++, v->level, v->chunk_s,
			    v->parts, v->kdevs);

			// Free up memory and remove from list
			list_del(&v->list);
			kfree(v);
		}
	}

	// Discard incomplete entries
	ldm_md_destroy();
}
