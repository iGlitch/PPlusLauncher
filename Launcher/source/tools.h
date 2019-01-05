#ifndef _TOOLS_H_
#define _TOOLS_H_


#define le16(i) ((((u16) ((i) & 0xFF)) << 8) | ((u16) (((i) & 0xFF00) >> 8)))
#define le32(i) ((((u32)le16((i) & 0xFFFF)) << 16) | ((u32)le16(((i) & 0xFFFF0000) >> 16)))
#define le64(i) ((((u64)le32((i) & 0xFFFFFFFFLL)) << 32) | ((u64)le32(((i) & 0xFFFFFFFF00000000LL) >> 32)))


#define MAX_PARTITIONS				32 /* Maximum number of partitions that can be found */
#define MAX_MOUNTS					10 /* Maximum number of mounts available at one time */
#define MAX_SYMLINK_DEPTH			10 /* Maximum search depth when resolving symbolic links */

#define MBR_SIGNATURE				0x55AA
#define EBR_SIGNATURE				MBR_SIGNATURE

#define PARTITION_BOOTABLE			0x80 /* Bootable (active) */
#define PARTITION_NONBOOTABLE		0x00 /* Non-bootable */
#define PARTITION_TYPE_GPT			0xEE /* Indicates that a GPT header is available */

#define GUID_SYSTEM_PARTITION		0x0000000000000001LL	/* System partition (disk partitioning utilities must reserve the partition as is) */
#define GUID_READ_ONLY_PARTITION	0x0800000000000000LL	/* Read-only partition */
#define GUID_HIDDEN_PARTITION		0x2000000000000000LL	/* Hidden partition */
#define GUID_NO_AUTOMOUNT_PARTITION	0x4000000000000000LL	/* Do not automount (e.g., do not assign drive letter) */

#define MAX_SECTOR_SIZE				4096

typedef struct _PARTITION_RECORD {
	u8 status;							  /* Partition status; see above */
	u8 chs_start[3];						/* Cylinder-head-sector address to first block of partition */
	u8 type;								/* Partition type; see above */
	u8 chs_end[3];						  /* Cylinder-head-sector address to last block of partition */
	u32 lba_start;						  /* Local block address to first sector of partition */
	u32 block_count;						/* Number of blocks in partition */
} __attribute__((__packed__)) PARTITION_RECORD;

typedef struct _MASTER_BOOT_RECORD {
	u8 code_area[446];					  /* Code area; normally empty */
	PARTITION_RECORD partitions[4];		 /* 4 primary partitions */
	u16 signature;						  /* MBR signature; 0xAA55 */
} __attribute__((__packed__)) MASTER_BOOT_RECORD;


bool checkSDLabel();
int mountSDCard();
#endif
