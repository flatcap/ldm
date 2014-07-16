#ifndef __LDM_PARSE_H__
#define __LDM_PARSE_H__

#include "types.h"

namespace ldm {

#define LDM_VBLK_MAX_NAME		32

#define LDM_VBLK_COMPONENT		0x32
#define LDM_VBLK_PARTITION		0x33
#define LDM_VBLK_VOLUME			0x51
#define LDM_VBLK_DISK1			0x34
#define LDM_VBLK_DISK2			0x44

struct privhead_t {
	u16 v_major;
	u16 v_minor;
	char disk_id[64];
	char dgrp_id[64];
	u64 disk_start;
	u64 disk_size;
	u64 db_start;
	u64 db_size;
	u64 ntocs;
	u64 toc_size;
	u32 nconfigs;
	u64 config_size;
};

struct tocblock_t {
	u64 bitmap1_start;
};

struct vmdb_t {
	u32 seqlast;
	u32 vblk_size;
	u16 v_major;
	u16 v_minor;
	char dg_guid[64];	// disk group id guid
};

struct vblk_t {
	u32	sect;
	u8	sect_sub;

	u32	vmdb_seq;
	u16	record;			// x of y
	u16	nrecords;
	u8	recordtype;
	u64	objectid;
	char	objname[LDM_VBLK_MAX_NAME];
	union {
		struct {
			u64	parentid;
		} component;

		struct {
			u64	start;
			u64	offset;
			u64	size;
			u64	parentid;
			u64	diskid;
		} partition;

		struct {
			int		type_at_offset;
			u8	type;
		} volume;
	};
};

bool raw_to_privhead(const void* raw, privhead_t* ph);
bool raw_to_tocblock(const void* raw, tocblock_t* tb);
bool raw_to_vmdb(const void* raw, vmdb_t* vmdb);
bool raw_to_vblk(const void* raw, vblk_t* vblk);

}	// namespace ldm
#endif
