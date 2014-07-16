/**
 * ldmutil - Part of the Linux-NTFS project.
 *
 * Copyright (C) 2001 Jakob Kemi <jakob.kemi@telia.com>
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

#include <cstdio>
#include <cstring>

#include "types.h"
#include "endian.h"
#include "ldm_parse.h"

using namespace ldm;

static const u64 _SIGN_PRIVHEAD = 0x5052495648454144ULL;	// "PRIVHEAD"
static const u64 _SIGN_TOCBLOCK = 0x544F43424C4F434BULL;	// "TOCBLOCK"
static const u32 _SIGN_VMDB = 0x564D4442UL;			// "VMDB"
static const u32 _SIGN_VBLK = 0x56424C4BUL;			// "VBLK"

#pragma pack(1)

struct _raw_privhead_t {
	u64	signature;
	u32	unknown_1;
	u16	ver_major;
	u16	ver_minor;
	u64	timestamp;
	u64	unknown_2;				// number ?
	u64	unknown_3;				// size ?
	u64	unknown_4;				// size ?
	u8	disk_id[64];			// zero padded
	u8	host_id[64];			// zero padded
	u8	diskgroup_id[64];		// zero padded
	u8	diskgroup_name[32];		// zero padded
	u16	unknown_5;
	u8	unknown_6[9];			// zeros
	u64	logical_disk_start;
	u64	logical_disk_size;
	u64	db_start;
	u64	db_size;
	u64	num_tocs;
	u64	toc_size;
	u32	num_configs;
	u32	num_logs;
	u64	config_size;
	u64	log_size;
	u32	disk_signature;
	u8	disk_set_guid[16];
	u8	disk_set_guid2[16];		// duplicate ??
	u8	padding[512-391];		// Pad to 512 bytes (sector size)
};

struct _raw_tocblock_t {
	u64	signature;
	u32	sequence1;
	u8	unknown1[4];			// zeros
	u32	sequence2;
	u8	unknown2[16];			// zeros
	u8	bitmap1_name[10];
	u64	bitmap1_start;
	u64	bitmap1_size;
	u64	bitmap1_flags;			// ????
	u8	bitmap2_name[10];
	u64	bitmap2_start;
	u64	bitmap2_size;
	u64	bitmap2_flags;			// ????
	u8	padding[512-104];		// Pad to 512 bytes (sector size)
};

struct _raw_vmdb_t {
	u32	signature;
	u32	seq;
	u32	vblk_size;
	u32	vblk_offset;
	u16	unknown1;		// 0x01 ??
	u16	ver_major;
	u16	ver_minor;
	u8	dg_name[31];	// disk group name (0 padded)
	u8	dg_guid[64];	// disk group id guid (0 padded)
	u64	committed_seq;
	u64	pending_seq;
	u32	unknown2[14];
	u32	timestamp;
	u8	padding[512-193];
};

struct _raw_vblk_t {
	u32	signature;
	u32	vmdb_seq;
	u32	grpnum;
	u16	record;			// x of y
	u16	nrecords;
	u8	padding[128-16];
};

#pragma pack()


//***************************************************************************
static u64 _v_get_num(const void* ptr);
static int _v_get_str(const void* ptr, char* buf, const int bufsize);
static int _v_get_size(const void* ptr);
static bool _guid_to_string(char* dest, const u8* guid);


static u64 _v_get_num(const void* ptr)
{
	const u8* data = (u8*)ptr;
	u64 t = 0;
	int len = *data++;

	while (len--)
		t = (t << 8) | *data++;

	return t;
}

static int _v_get_str(const void* ptr, char* buf, const int bufsize)
{
	const u8* data = (u8*)ptr;
	int len;

	len = *data++;

	if (len >= bufsize)
		len = bufsize - 1;

	memcpy (buf, data, len);
	buf[len] = (u8)'\0';
	return len;
}

static int _v_get_size(const void* ptr)
{
	const u8* data = (u8*)ptr;
	return *data + 1;
}


static bool _guid_to_string(char* dest, const u8* guid)
{
	sprintf(dest, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
		guid[ 0], guid[ 1], guid[ 2], guid[ 3], guid[ 4], guid[ 5], guid[ 6], guid[ 7],
		guid[ 8], guid[ 9], guid[10], guid[11], guid[12], guid[13], guid[14], guid[15]
	);
	return true;
}

//***************************************************************************


bool ldm::raw_to_privhead(const void* raw, privhead_t* ph)
{
	_raw_privhead_t* rph = (_raw_privhead_t*)raw;
	if (__be64_to_cpu(rph->signature) != _SIGN_PRIVHEAD)
		return false;

	ph->v_major = __be16_to_cpu(rph->ver_major);
	ph->v_minor = __be16_to_cpu(rph->ver_minor);
	strncpy(ph->disk_id, (char*)rph->disk_id, 64);
	ph->disk_id[63] = '\0';

	strncpy(ph->dgrp_id, (char*)rph->diskgroup_id, 64);
	ph->dgrp_id[63] = '\0';

	ph->disk_start = __be64_to_cpu(rph->logical_disk_start);
	ph->disk_size = __be64_to_cpu(rph->logical_disk_size);

	ph->db_start = __be64_to_cpu(rph->db_start);
	ph->db_size = __be64_to_cpu(rph->db_size);

	ph->ntocs = __be64_to_cpu(rph->num_tocs);
	ph->toc_size = __be64_to_cpu(rph->toc_size);

	ph->nconfigs = __be32_to_cpu(rph->num_configs);
	ph->config_size = __be64_to_cpu(rph->config_size);

	return true;
}


bool ldm::raw_to_tocblock(const void* raw, tocblock_t* tb)
{
	_raw_tocblock_t* rtb = (_raw_tocblock_t*)raw;
	if (__be64_to_cpu(rtb->signature) != _SIGN_TOCBLOCK)
		return false;

	tb->bitmap1_start = __be64_to_cpu(rtb->bitmap1_start);

	return true;
}


bool ldm::raw_to_vmdb(const void* raw, vmdb_t* vmdb)
{
	_raw_vmdb_t* rvmdb = (_raw_vmdb_t*)raw;
	if (__be32_to_cpu(rvmdb->signature) != _SIGN_VMDB)
		return false;

	vmdb->seqlast = __be32_to_cpu(rvmdb->seq);
	vmdb->vblk_size = __be32_to_cpu(rvmdb->vblk_size);
	vmdb->v_major = __be16_to_cpu(rvmdb->ver_major);
	vmdb->v_minor = __be16_to_cpu(rvmdb->ver_minor);

	strncpy(vmdb->dg_guid, (char*)rvmdb->dg_guid, 64);

	return true;
}


bool ldm::raw_to_vblk(const void* raw, vblk_t* vblk)
{
	_raw_vblk_t* rvblk = (_raw_vblk_t*)raw;
	if (__be32_to_cpu(rvblk->signature) != _SIGN_VBLK )
		return false;

	vblk->vmdb_seq = __be32_to_cpu(rvblk->vmdb_seq);
	vblk->record = __be16_to_cpu(rvblk->record);
	vblk->nrecords = __be16_to_cpu(rvblk->nrecords);

	// FIXME, this is ugly
//	if (vblk->nrecords != 1)
//		return false;
	if (vblk->record != 0)
		return false;

	u8* ptr = (u8*)rvblk->padding;

	vblk->recordtype = __be32_to_cpup((u32*)ptr) & 0xff;
	ptr += 4;

	ptr += 4;

	vblk->objectid = _v_get_num(ptr);
	ptr += _v_get_size(ptr);

	_v_get_str(ptr, vblk->objname, LDM_VBLK_MAX_NAME);
	ptr += _v_get_size(ptr);

	switch (vblk->recordtype)
	{
	case LDM_VBLK_COMPONENT:
		ptr += _v_get_size(ptr);
		ptr += 23;
		vblk->component.parentid = _v_get_num(ptr);
		ptr += _v_get_size(ptr);
		break;

	case LDM_VBLK_PARTITION:
		ptr += 12;
		vblk->partition.start = __be64_to_cpup((u64*)ptr);
		ptr += 8;
		vblk->partition.offset = __be64_to_cpup((u64*)ptr);
		ptr += 8;
		vblk->partition.size = _v_get_num(ptr);
		ptr += _v_get_size(ptr);
		vblk->partition.parentid = _v_get_num(ptr);
		ptr += _v_get_size(ptr);
		vblk->partition.diskid = _v_get_num(ptr);
		ptr += _v_get_size(ptr);
		break;

	case LDM_VBLK_VOLUME:
		ptr += _v_get_size(ptr);
		ptr += 1;
		ptr += 14;
		ptr += 25;
		ptr += _v_get_size(ptr);
		ptr += 4;
		vblk->volume.type_at_offset = (int) (ptr - (u8*)rvblk);
		vblk->volume.type = *ptr;
		ptr += 1;
		ptr += 16;
		ptr += _v_get_size(ptr);
		break;

	case LDM_VBLK_DISK1:
	case LDM_VBLK_DISK2:
		return true;

	default:
		return false;
	}

	return true;
}
