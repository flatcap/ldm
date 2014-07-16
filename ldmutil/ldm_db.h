#ifndef __LDM_DB_H__
#define __LDM_DB_H__

#include <map>
#include <list>
#include <cstdio>

#include "types.h"
#include "diskio.h"
#include "ldm_parse.h"

namespace ldm {

class Volume {
public:
	u32 id;
	u32 vblk_sect;
	u8 vblk_subsect;
	u8 type;
	u8 toffset;
public:
	Volume(void) {id = 0; type = 0;}
};

class Partition {
public:
	u32 id;
	u32 p_id;
	Volume* vol;
	u32 start;
	u32 size;
public:
	bool operator < (const Partition& p) const {return (start < p.start);}
};

class Disk {
public:
	Disk(void) {name[0] = '\0';}
	u32 id;
	char name[LDM_VBLK_MAX_NAME];
	std::list<Partition> partlist;
};

class ldmdb_c {
private:
	std::map<u32, Volume> _volmap;
	std::map<u32, Disk> _diskmap;
public:
	void Read(diskio& dev);
	void Dump(std::ostream& s);
	void ChangeVolType(diskio& dev, u32 vblkid, u8 type);
};

}

#endif
