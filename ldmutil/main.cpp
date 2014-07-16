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
#include "error.h"
#include "ldm_db.h"

using namespace std;
using namespace ldm;

static void _display_usage(int argc, char** argv)
{
	cerr << "\nldmutil v0.2, by Jakob Kemi (jakob.kemi@telia.com)\n\n";
	cerr << "usage:\n";
	cerr << "   " << argv[0] << " DEVICE l            -- list partitions to stdout\n";
	cerr << "   " << argv[0] << " DEVICE c DEVICE2    -- copy raw ldm database from DEVICE to DEVICE2\n";
	cerr << "   " << argv[0] << " DEVICE t VOLID TYPE -- set partition type for VOLID to TYPE\n";
	cerr << "   (see README for further information.)\n\n";
}

struct cmd_parse_t {
	char flag;
	void (*taskfunc)(ldm::diskio& dev, int argc, char** argv);
	bool readonly;
};

static void _task_dump(ldm::diskio& dev, int argc, char** argv)
{
	ldmdb_c ldm;

	ldm.Read(dev);
	ldm.Dump(cout);
}

static void _task_copy(ldm::diskio& dev, int argc, char** argv)
{
	ldm::diskio odev;
	unsigned char sect[512];
	bool newfile = false;
	int i;

	if (argc != 1)
		throw LDM_MKERROR("Bad argument count.");

	odev.Open(argv[0], false);

	if (odev.GetSize() == 0)
		newfile = true;

	// First 7 sectors contains partiontable & privhead #1
	for(i = 0; i < 7; i++) {
		dev.Read(sect);
		odev.Write(sect);
	}

	//ldm db
	dev.SetPos( dev.GetSize()-2048 );
	if (!newfile)
		odev.SetPos( odev.GetSize()-2048 );

	for(i = 0; i < 2048; i++) {
		dev.Read(sect);
		odev.Write(sect);
	}

	odev.Close();
}

static void _task_change(ldm::diskio& dev, int argc, char** argv)
{
	ldmdb_c ldm;
	unsigned long id;
	int type;

	if (
		argc != 2 || sscanf(argv[0], "%lu", &id) != 1 ||
		sscanf(argv[1], "%x", &type) != 1
	)
		throw LDM_MKERROR("Invalid parameters.");

	ldm.Read(dev);
	ldm.ChangeVolType(dev, id, type);
}

int main(int argc, char** argv)
{
	ldm::diskio dev;
	cmd_parse_t tasks[] = {
		{'l', &_task_dump, true},
		{'c', &_task_copy, true},
		{'t', &_task_change, false},
		{'\0', 0, 0}
	};


	if (argc < 3) {
		_display_usage(argc, argv);
		return 1;
	}

	for (cmd_parse_t* cp = tasks; cp->taskfunc != 0; cp++) {
		if (argv[2][0] != cp->flag)
			continue;

		try {
			dev.Open(argv[1], cp->readonly);
			cp->taskfunc(dev, argc - 3, argv + 3);
		}
		catch (ldm::Error& e) {
			_display_usage(argc, argv);
			std::cerr << e << '\n';
			cerr << "Task failed!\n";
			return 1;
		}
		catch(...) {
			_display_usage(argc, argv);
			cerr << "Unknown error!\n";
			return 1;
		}

		return 0;
	}

	_display_usage(argc, argv);
	return 1;
}
