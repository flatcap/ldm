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

#include <iostream>
#include <iomanip>
#include <list>
#include <map>

#include "types.h"
#include "error.h"
#include "ldm_parse.h"
#include "ptypenames.h"

#include "ldm_db.h"

#define LDM_DB_SIZE		2048		// Size in sectors (= 1 mb).
#define LDM_SECT_SIZE		512
#define LDM_VBLK_SIZE		128

using namespace std;
using namespace ldm;

static const u32 _ldm_off_ph[3] = {6, 1856, 2047};   // 2 & 3 is relative db
static const u32 _ldm_off_tb[4] = {1, 2, 2045, 2046};// relative db

void ldmdb_c::Read(diskio& dev)
{
	u8 sect[LDM_SECT_SIZE];
	privhead_t ph;
	tocblock_t tb;
	vmdb_t vm;
	int i;

	if (dev.GetSectorSize() != LDM_SECT_SIZE)
		throw LDM_MKERROR("Illegal sector size.\n");

// read privheads
	dev.Read(sect, 1, _ldm_off_ph[0]);
	if (!raw_to_privhead(sect, &ph))
		throw LDM_MKERROR("Unable to parse privhead 1.\n");

	dev.Read(sect, 1, _ldm_off_ph[1] + ph.db_start);
	if (!raw_to_privhead(sect, &ph))
		throw LDM_MKERROR("Unable to parse privhead 2.\n");

	dev.Read(sect, 1, _ldm_off_ph[2] + ph.db_start);
	if (!raw_to_privhead(sect, &ph))
		throw LDM_MKERROR("Unable to parse privhead 3.\n");

	if (ph.v_major != 2 || ph.v_minor != 11)
		throw LDM_MKERROR("Bad privhead version.\n");

// read tocblock 1
	dev.Read(sect, 1, ph.db_start + _ldm_off_tb[0]);
	if (!raw_to_tocblock(sect, &tb))
		throw LDM_MKERROR("Unable to parse tocblock 1.\n");

// read vmdb
	dev.Read(sect, 1, ph.db_start + tb.bitmap1_start);
	if (!raw_to_vmdb(sect, &vm))
		throw LDM_MKERROR("Unable to parse vmdb.\n");

	if (vm.vblk_size != LDM_VBLK_SIZE)
		throw LDM_MKERROR("Illegal VBLK size.\n");

	std::map<u32, u32> compmap;
	u32 s = ph.db_start + tb.bitmap1_start;
// read vblks
	for (i = 0; i < vm.seqlast; i++)
	{
		vblk_t tvblk;
		if ((i & 0x3) == 0) {
			dev.Read(sect);
			s++;
		}

		if (!raw_to_vblk(sect + (i&0x03) * LDM_VBLK_SIZE, &tvblk))
			continue;

		// add vblks to correct container
		switch (tvblk.recordtype) {
			Partition tpart;

			case LDM_VBLK_COMPONENT:
				compmap[tvblk.objectid] = tvblk.component.parentid;
				break;
			case LDM_VBLK_DISK1:
			case LDM_VBLK_DISK2:
				_diskmap[ tvblk.objectid ].id = tvblk.objectid;
				strncpy(_diskmap[ tvblk.objectid ].name, tvblk.objname, LDM_VBLK_MAX_NAME);
				break;
			case LDM_VBLK_PARTITION:
				tpart.id = tvblk.objectid;
				tpart.p_id = tvblk.partition.parentid;
				tpart.start = ph.disk_start + tvblk.partition.start;
				tpart.size = tvblk.partition.size;
				tpart.vol = 0;
				_diskmap[ tvblk.partition.diskid ].partlist.push_back(tpart);
				break;
			case LDM_VBLK_VOLUME:
				{
					Volume tvol;
					tvol.id = tvblk.objectid;
					tvol.type = tvblk.volume.type;
					tvol.toffset = tvblk.volume.type_at_offset;
					tvol.vblk_sect = s;
					tvol.vblk_subsect = i & 0x3;
					_volmap[tvblk.objectid] = tvol;
				}
				break;
			default:
				break;
		}
	}

	// add partitions sorted to parent disks
	std::map<u32, Disk>::iterator mi;
	for (mi = _diskmap.begin(); mi != _diskmap.end(); mi++) {
		Disk& d = (*mi).second;
		std::list<Partition>::iterator li;
		for (li = d.partlist.begin(); li != d.partlist.end(); li++) {
			Partition& part = *li;
			part.vol = &_volmap[ compmap[part.p_id] ];
		}
		d.partlist.sort();
	}
}

void ldmdb_c::Dump(std::ostream& s)
{
	s << "+------+--------------+-----------+---------+------|---------------------------+\n";
	s << "|  id  | Start (sect) | Size (Mb) | Vol. id | Type |      Type description     |\n";
	s << "+------+--------------+-----------+---------+------|---------------------------+\n";

	std::map<u32, Disk>::iterator mi;
	for (mi = _diskmap.begin(); mi != _diskmap.end(); mi++) {
		Disk& d = (*mi).second;

		if (strlen(d.name) == 0)
			throw LDM_MKERROR("Bad disk entry found.");

		s << " Disk '" << d.name << "' (" << d.id << "):\n";

		std::list<Partition>::iterator i;
		for (i = d.partlist.begin(); i != d.partlist.end(); i++)
		{
			const Partition& part = *i;
			const int type = part.vol->type;

			s << setw(7) << part.id;
			s << setw(15) << part.start;
			s << setw(12) << part.size / 2048.0f;
			s << setw(8) << part.vol->id;
			s << hex << setw(9) << type << dec;
			s << "  " << setw(26) << PTYPE_NAMES[type];
			s << '\n';
		}
	}
}

void ldmdb_c::ChangeVolType(diskio& dev, u32 id, u8 type)
{
	u8 sect[LDM_SECT_SIZE];
	Volume& vol = _volmap[id];
	vblk_t vb;

	if (vol.id != id)
		throw LDM_MKERROR("Volume id not found.");

	dev.Read(sect, 1, vol.vblk_sect);

	sect[vol.vblk_subsect * LDM_VBLK_SIZE + vol.toffset] = type;

	dev.Write(sect, 1, vol.vblk_sect);
}
