/**
 * sparse - Part of the Linux-NTFS project.
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main (int argc, char *argv[])
{
	char buf[512];
	char *part;
	char *data;
	char *out;
	int fpart;
	int fdata;
	int fout;
	__off64_t off;

	if (argc != 5) {
		printf ("\nUsage:\n\t%s part data sectors output\n\n", basename (argv[0]));
		return 1;
	}

	part = argv[1];
	data = argv[2];
	out  = argv[4];
	off  = atol (argv[3]);

	if (off == 0) {
		printf ("Sectors must be non-zero\n");
		return 1;
	}

	off -= 2048;
	off <<= 9;

	fpart = open (part, O_RDONLY);
	if (fpart < 0) {
		printf ("Cannot open part file '%s'\n", part);
		return 1;
	}

	fdata = open (data, O_RDONLY);
	if (fdata < 0) {
		printf ("Cannot open data file '%s'\n", data);
		return 1;
	}

	fout = open (out, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	if (fout < 0) {
		printf ("Cannot open output file '%s'\n", out);
		return 1;
	}

	while (read (fpart, buf, sizeof (buf)) == sizeof (buf))
	{
		if (write (fout, buf, sizeof (buf)) < sizeof (buf)) {
			printf ("Cannot write to '%s'\n", part);
			return 1;
		}
	}

	if (lseek64 (fout, off, SEEK_SET) < 0)
	{
		printf ("Seek failed (%s) in '%s', offset %llu\n",
			strerror (errno), out, off);
		printf ("Check this isn't limitation of your filesystem\n");
		printf ("e.g. By default ext2 has a limit of 16Gb\n");
		return 1;
	}

	while (read (fdata, buf, sizeof (buf)) == sizeof (buf))
	{
		if (write (fout, buf, sizeof (buf)) < sizeof (buf)) {
			printf ("Cannot write to '%s'\n", data);
			return 1;
		}
	}

	close (fpart);
	close (fdata);
	close (fout);
	
	printf ("Succeeded\n");
	return 0;
}

