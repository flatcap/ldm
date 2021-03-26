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

/*
#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET=64
*/

#include <climits>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "types.h"
#include "error.h"
#include "diskio.h"

#ifndef __CYGWIN__
#define open	open64
#define lseek	lseek64
#define off_t	off64_t
#endif

#define __SECTORSIZE		512

using namespace ldm;
using namespace std;


diskio::diskio(void)
{
	_open = false;
}

diskio::~diskio(void)
{
	Close();
}

void diskio::Open(const char* filename, bool readonly)
{
	Close();

	if (!readonly) {
#ifdef O_LARGEFILE
		_fd = open(filename, O_RDWR | O_CREAT | O_LARGEFILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // typo: O_LARGEFILE, S_IRUSR ?
#else
		_fd = open(filename, O_RDWR | O_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
		_readonly = false;
	} else {
#ifdef O_LARGEFILE
		_fd = open(filename, O_RDONLY | O_LARGEFILE);
#else
		_fd = open(filename, O_RDONLY );
#endif
		_readonly = true;
	}

	if (_fd == -1)
		throw LDM_MKERROR( strerror(errno) );

	off_t off = lseek(_fd, 0, SEEK_END);
	if (off == -1 || lseek(_fd, 0, SEEK_SET) == -1) {
		close(_fd);
		throw LDM_MKERROR( strerror(errno) );
	}

	_pos = 0;
	_open = true;
	_size = off;
}

void diskio::Close(void)
{
	if (!_open)
		return;

	close(_fd);
	_open = false;
}

u64 diskio::GetSectorSize(void)
{
	return __SECTORSIZE;
}

void diskio::SetPos(u64 sector)
{
	off_t offset = (u64) ( sector * __SECTORSIZE);
	if (_readonly && offset == _pos)
		return;

//	cerr << "sector = " << sector << " __SECTORSIZE = " << __SECTORSIZE <<"\n";
//	cerr << "sizeof(sector) = " << sizeof(sector) << "\n";

	_pos = lseek(_fd, offset, SEEK_SET);
	if (_pos != offset) {
		cerr << _pos << " =  lseek(" << _fd << "," << offset << ",SEEK_SET)\n";
		throw LDM_MKERROR( strerror(errno) );
	}
}

u64 diskio::GetPos(void)
{
	return _pos / __SECTORSIZE;
}

void diskio::Write(const void* src, int nsect, u64 pos)
{
	if (pos != INT_MIN)
		SetPos(pos);

	size_t left = nsect * __SECTORSIZE;
	const unsigned char* p = (const unsigned char*)src;

	while(left > 0) {
		ssize_t ret = write(_fd, p, left);
		if (ret == -1)
			throw LDM_MKERROR( strerror(errno) );

		p += ret;
		left -= ret;
	}
}

void diskio::Read(void* dest, int nsect, u64 pos)
{
	if (pos != INT_MIN)
		SetPos(pos);

	size_t left = nsect * __SECTORSIZE;
	unsigned char* p = (unsigned char*)dest;

	while(left > 0) {
		ssize_t ret = read(_fd, p, left);
		if (ret == -1)
			throw LDM_MKERROR( strerror(errno) );

		p += ret;
		left -= ret;
	}
}

u64 diskio::GetSize(void)
{
	return _size / __SECTORSIZE;
}
