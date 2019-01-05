/****************************************************************************
 * Copyright (C) 2009-2011 Dimok
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#ifndef ARCHIVESTRUCT_H_
#define ARCHIVESTRUCT_H_

#include <gctypes.h>

typedef struct
{
	char * filename; // full filename
	size_t length; // uncompressed file length in 64 bytes for sizes higher than 4GB
	size_t comp_length; // compressed file length in 64 bytes for sizes higher than 4GB
	bool isdir; // 0 - file, 1 - directory
	u32 fileindex; // fileindex number
	u64 ModTime; // modification time
	u8 archiveType; // modification time
} ArchiveFileStruct;

enum
{
	UNKNOWN = 1,
	ZIP,
	SZIP,
	RAR,
	U8Arch,
	ArcArch
};

#endif
