/*
 * @note
 * Copyright(C) NXP Semiconductors, 2012
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

/** @file
 *
 *  Functions to manage the physical Dataflash media, including reading and writing of
 *  blocks of data. These functions are called by the SCSI layer when data must be stored
 *  or retrieved to/from the physical storage media. If a different media is used (such
 *  as a SD card or EEPROM), functions similar to these will need to be generated.
 */

#include "DataRam.h"
#include "fatutil.h"
#include "iap_driver.h"

#include "xprintf.h"

/** @ingroup EXAMPLE_MassStorage
 * @{
 */

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* static */ int InitializedFlag = 0;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/


extern uint8_t __flash_disk[];
uint8_t* DiskImageF = &__flash_disk[0];

static uint8_t DiskImageA[UTILITY_AREA_RAM_SIZE] __BSS(USBRAM_SECTION); 		//<! disk utility area: boot, fat, directory

static uint8_t DiskImageB[DATA_RAM_USER_SIZE] __BSS(DISKIMAGE_SECTION); 		//<! main disk data area


/*
	DiskImagePtr->BootSector.BPB_FATSz16    = SectorsPerFAT;
	DiskImagePtr->BootSector.BPB_NumFATs    = NumFATs;
	DiskImagePtr->BootSector.BPB_RootEntCnt = RootEntries;
	DiskImagePtr->BootSector.BPB_BytsPerSec = BytesPerSector;
	DiskImagePtr->BootSector.BPB_SecPerClus = SECTORSPERCLUSTER;
	DiskImagePtr->BootSector.BPB_Media      = 0xf8;
	DiskImagePtr->BootSector.BPB_RsvdSecCnt = RESERVEDSECTORCNT;

	DiskImagePtr->BootSector.BPB_TotSec16   = VolumeSize / BytesPerSector;
*/
// typedef struct {
// 	uint8_t     BS_jmpBoot[3];	/* Jump instruction to the boot code */
// 	uint8_t     BS_OEMName[8];	/* Name of system that formatted the volume */
// 	uint16_t    BPB_BytsPerSec;	/* Bytes per sector (should be 512) */
// 	uint8_t     BPB_SecPerClus;	/* Sectors per cluster (FAT-12 = 1) */
// 	uint16_t    BPB_RsvdSecCnt;	/* Reserved sectors (FAT-12 = 1) */
// 	uint8_t     BPB_NumFATs;	/* FAT tables on the disk (should be 2) */
// 	uint16_t    BPB_RootEntCnt;	/* Max directory entries in root directory */
// 	uint16_t    BPB_TotSec16;	/* Total number of sectors on the disk (FAT-12 and FAT-16) */
// 	uint8_t     BPB_Media;		/* Media type {fixed, removable, etc.} */
// 	uint16_t    BPB_FATSz16;	/* Sector size of FAT table (FAT-12 = 9) */
// 	uint16_t    BPB_SecPerTrk;	/* # of sectors per cylindrical track */
// 	uint16_t    BPB_NumHeads;	/* # of heads per volume (1.4Mb 3.5" = 2) */
// 	uint32_t    BPB_HiddSec;	/* # of preceding hidden sectors (0) */
// 	uint32_t    BPB_TotSec32;	/* # of FAT-32 sectors (0 for FAT-12) */
// 	uint8_t     BS_DrvNum;		/* A drive number for the media (OS specific) */
// 	uint8_t     BS_Reserved1;	/* Reserved space for Windows NT (set to 0) */
// 	uint8_t     BS_BootSig;		/* (0x29) Indicates following: */
// 	uint32_t    BS_VolID;		/* Volume serial # (for tracking this disk) */
// 	uint8_t     BS_VolLab[11];	/* Volume label (RDL or "NO NAME    ") */
// 	uint8_t     BS_FilSysType[8];	/* Deceptive FAT type Label */
// } ATTR_PACKED BSStruct;


const uint8_t BootSector[512] = {
	0xEB, 0x3C, 0x90, 0x4D, 0x53, 0x44, 0x4F, 0x53, 0x35, 0x2E, 0x30, 
	BYTESPERSECTOR & 0xff, (BYTESPERSECTOR>>8) & 0xff, // BytsPerSec = 0x200
	SECTORSPERCLUSTER,   	// SecPerClus
	RESERVEDSECTORCNT, 		// RsvdSecCnt
	0x00,
	NUMFATS,				// NumFATs
	ROOTENTRIES & 0xff, (ROOTENTRIES>>8) & 0xff,   //0x10, 0x00, 			// BPB_RootEntCnt
	TOTALSECTORS & 0xff, (TOTALSECTORS>>8) & 0xff, //0x10, 0x00, 			// Total sectors
	0xF8, // Media
	SECTORSPERFAT & 0xff, (SECTORSPERFAT>>8) & 0xff, //0x01, 0x00, // FATSz16
	0x01, 0x00, 
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x74, 0x19, 0x02, 0x27, 0x4C, 0x50, 0x43, 0x31, 0x31,
	0x78, 0x78, 0x20, 0x55, 0x53, 0x42, 0x46, 0x41, 0x54, 0x31, 0x32, 0x20, 0x20, 0x20, 0x33, 0xC9,
	0x8E, 0xD1, 0xBC, 0xF0, 0x7B, 0x8E, 0xD9, 0xB8, 0x00, 0x20, 0x8E, 0xC0, 0xFC, 0xBD, 0x00, 0x7C,
	0x38, 0x4E, 0x24, 0x7D, 0x24, 0x8B, 0xC1, 0x99, 0xE8, 0x3C, 0x01, 0x72, 0x1C, 0x83, 0xEB, 0x3A,
	0x66, 0xA1, 0x1C, 0x7C, 0x26, 0x66, 0x3B, 0x07, 0x26, 0x8A, 0x57, 0xFC, 0x75, 0x06, 0x80, 0xCA,
	0x02, 0x88, 0x56, 0x02, 0x80, 0xC3, 0x10, 0x73, 0xEB, 0x33, 0xC9, 0x8A, 0x46, 0x10, 0x98, 0xF7,
	0x66, 0x16, 0x03, 0x46, 0x1C, 0x13, 0x56, 0x1E, 0x03, 0x46, 0x0E, 0x13, 0xD1, 0x8B, 0x76, 0x11,
	0x60, 0x89, 0x46, 0xFC, 0x89, 0x56, 0xFE, 0xB8, 0x20, 0x00, 0xF7, 0xE6, 0x8B, 0x5E, 0x0B, 0x03,
	0xC3, 0x48, 0xF7, 0xF3, 0x01, 0x46, 0xFC, 0x11, 0x4E, 0xFE, 0x61, 0xBF, 0x00, 0x00, 0xE8, 0xE6,
	0x00, 0x72, 0x39, 0x26, 0x38, 0x2D, 0x74, 0x17, 0x60, 0xB1, 0x0B, 0xBE, 0xA1, 0x7D, 0xF3, 0xA6,
	0x61, 0x74, 0x32, 0x4E, 0x74, 0x09, 0x83, 0xC7, 0x20, 0x3B, 0xFB, 0x72, 0xE6, 0xEB, 0xDC, 0xA0,
	0xFB, 0x7D, 0xB4, 0x7D, 0x8B, 0xF0, 0xAC, 0x98, 0x40, 0x74, 0x0C, 0x48, 0x74, 0x13, 0xB4, 0x0E,
	0xBB, 0x07, 0x00, 0xCD, 0x10, 0xEB, 0xEF, 0xA0, 0xFD, 0x7D, 0xEB, 0xE6, 0xA0, 0xFC, 0x7D, 0xEB,
	0xE1, 0xCD, 0x16, 0xCD, 0x19, 0x26, 0x8B, 0x55, 0x1A, 0x52, 0xB0, 0x01, 0xBB, 0x00, 0x00, 0xE8,
	0x3B, 0x00, 0x72, 0xE8, 0x5B, 0x8A, 0x56, 0x24, 0xBE, 0x0B, 0x7C, 0x8B, 0xFC, 0xC7, 0x46, 0xF0,
	0x3D, 0x7D, 0xC7, 0x46, 0xF4, 0x29, 0x7D, 0x8C, 0xD9, 0x89, 0x4E, 0xF2, 0x89, 0x4E, 0xF6, 0xC6,
	0x06, 0x96, 0x7D, 0xCB, 0xEA, 0x03, 0x00, 0x00, 0x20, 0x0F, 0xB6, 0xC8, 0x66, 0x8B, 0x46, 0xF8,
	0x66, 0x03, 0x46, 0x1C, 0x66, 0x8B, 0xD0, 0x66, 0xC1, 0xEA, 0x10, 0xEB, 0x5E, 0x0F, 0xB6, 0xC8,
	0x4A, 0x4A, 0x8A, 0x46, 0x0D, 0x32, 0xE4, 0xF7, 0xE2, 0x03, 0x46, 0xFC, 0x13, 0x56, 0xFE, 0xEB,
	0x4A, 0x52, 0x50, 0x06, 0x53, 0x6A, 0x01, 0x6A, 0x10, 0x91, 0x8B, 0x46, 0x18, 0x96, 0x92, 0x33,
	0xD2, 0xF7, 0xF6, 0x91, 0xF7, 0xF6, 0x42, 0x87, 0xCA, 0xF7, 0x76, 0x1A, 0x8A, 0xF2, 0x8A, 0xE8,
	0xC0, 0xCC, 0x02, 0x0A, 0xCC, 0xB8, 0x01, 0x02, 0x80, 0x7E, 0x02, 0x0E, 0x75, 0x04, 0xB4, 0x42,
	0x8B, 0xF4, 0x8A, 0x56, 0x24, 0xCD, 0x13, 0x61, 0x61, 0x72, 0x0B, 0x40, 0x75, 0x01, 0x42, 0x03,
	0x5E, 0x0B, 0x49, 0x75, 0x06, 0xF8, 0xC3, 0x41, 0xBB, 0x00, 0x00, 0x60, 0x66, 0x6A, 0x00, 0xEB,
	0xB0, 0x4E, 0x54, 0x4C, 0x44, 0x52, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0D, 0x0A, 0x52, 0x65,
	0x6D, 0x6F, 0x76, 0x65, 0x20, 0x64, 0x69, 0x73, 0x6B, 0x73, 0x20, 0x6F, 0x72, 0x20, 0x6F, 0x74,
	0x68, 0x65, 0x72, 0x20, 0x6D, 0x65, 0x64, 0x69, 0x61, 0x2E, 0xFF, 0x0D, 0x0A, 0x44, 0x69, 0x73,
	0x6B, 0x20, 0x65, 0x72, 0x72, 0x6F, 0x72, 0xFF, 0x0D, 0x0A, 0x50, 0x72, 0x65, 0x73, 0x73, 0x20,
	0x61, 0x6E, 0x79, 0x20, 0x6B, 0x65, 0x79, 0x20, 0x74, 0x6F, 0x20, 0x72, 0x65, 0x73, 0x74, 0x61,
	0x72, 0x74, 0x0D, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAC, 0xCB, 0xD8, 0x55, 0xAA,
};

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

#if 0
 /** @brief	Writes blocks (OS blocks, not Dataflash pages) to the storage medium, the board Dataflash IC(s), from
 *  the pre-selected data OUT endpoint. This routine reads in OS sized blocks from the endpoint and writes
 *  them to the Dataflash in Dataflash page sized blocks.
 *  @param	MSInterfaceInfo	: Pointer to a structure containing a Mass Storage Class configuration and state
 *  @param	BlockAddress	: Data block starting address for the write sequence
 *  @param	TotalBlocks		: Number of blocks of data to write
 */
void DataRam_WriteBlocks(USB_ClassInfo_MS_Device_t *const MSInterfaceInfo,
						 const uint32_t BlockAddress,
						 uint16_t TotalBlocks)
{
	uint32_t i;

	if (!InitializedFlag) {
		DataRam_Initialize();
	}

	for (i = 0; i < TotalBlocks; i++) {
		while (!Endpoint_IsReadWriteAllowed(MSInterfaceInfo->Config.PortNumber)) {}

		// Fake the read if the address is past the physical limit of the DiskImage buffer
		if (BlockAddress + i >= DATA_RAM_PHYSICAL_SIZE / VIRTUAL_MEMORY_BLOCK_SIZE) {
			Endpoint_Null_Stream(MSInterfaceInfo->Config.PortNumber, VIRTUAL_MEMORY_BLOCK_SIZE, NULL);
			Endpoint_ClearOUT(MSInterfaceInfo->Config.PortNumber);
			continue;
		}

		Endpoint_Read_Stream_LE(MSInterfaceInfo->Config.PortNumber,
								&DiskImage[(BlockAddress + i) * VIRTUAL_MEMORY_BLOCK_SIZE],
								VIRTUAL_MEMORY_BLOCK_SIZE,
								NULL);
		Endpoint_ClearOUT(MSInterfaceInfo->Config.PortNumber);
	}
}

/** @brief	Reads blocks (OS blocks, not Dataflash pages) from the storage medium, the board Dataflash IC(s), into
 *  the pre-selected data IN endpoint. This routine reads in Dataflash page sized blocks from the Dataflash
 *  and writes them in OS sized blocks to the endpoint.
 *
 *  @param	MSInterfaceInfo	: Pointer to a structure containing a Mass Storage Class configuration and state
 *  @param	BlockAddress	: Data block starting address for the read sequence
 *  @param	TotalBlocks		: Number of blocks of data to read
 */
void DataRam_ReadBlocks(USB_ClassInfo_MS_Device_t *const MSInterfaceInfo,
						const uint32_t BlockAddress,
						uint16_t TotalBlocks)
{
	uint32_t i;

	if (!InitializedFlag) {
		DataRam_Initialize();
	}

	for (i = 0; i < TotalBlocks; i++) {
		// Fake the write if the address is past the physical limit of the DiskImage buffer
		if (BlockAddress + i >= DATA_RAM_PHYSICAL_SIZE / VIRTUAL_MEMORY_BLOCK_SIZE) {
			Endpoint_Null_Stream(MSInterfaceInfo->Config.PortNumber, VIRTUAL_MEMORY_BLOCK_SIZE, NULL);
			Endpoint_ClearIN(MSInterfaceInfo->Config.PortNumber);
			continue;
		}

		Endpoint_Write_Stream_LE(MSInterfaceInfo->Config.PortNumber,
								 &DiskImage[(BlockAddress + i) * VIRTUAL_MEMORY_BLOCK_SIZE],
								 VIRTUAL_MEMORY_BLOCK_SIZE,
								 NULL);
		Endpoint_ClearIN(MSInterfaceInfo->Config.PortNumber);
	}
}
#endif

uint32_t MassStorage_GetAddressInImage(uint32_t startblock, uint16_t requestblocks, uint16_t *availableblocks)
{
	if (!InitializedFlag) {
		DataRam_Initialize();
	}

	// remap the areas
	uint8_t* ramptr;
	if (startblock < UTILITY_AREA_SECTORS) {
		ramptr = DiskImageA;
	} else {
		ramptr = DiskImageB;
		startblock -= UTILITY_AREA_SECTORS;
	}

	if ((startblock + requestblocks) <= DATA_RAM_USER_SIZE / DATA_RAM_BLOCK_SIZE) {
		*availableblocks = requestblocks;
	}
	else {
		if (startblock >= DATA_RAM_USER_SIZE / DATA_RAM_BLOCK_SIZE) {
			*availableblocks = 0;
		}
		else {
			*availableblocks = DATA_RAM_USER_SIZE / DATA_RAM_BLOCK_SIZE - startblock; 
		}
	}

	if (*availableblocks != 0) {
		return (uint32_t) &ramptr[startblock * DATA_RAM_BLOCK_SIZE];
	}
	else {
		return (uint32_t) &ramptr[(DATA_RAM_USER_SIZE / DATA_RAM_BLOCK_SIZE - 1) * DATA_RAM_BLOCK_SIZE];
	}
}

uint32_t MassStorage_GetAddressInFlash(uint32_t startblock, uint16_t requestblocks, uint16_t *availableblocks)
{
	if (startblock == 0) {
		*availableblocks = 1;
		return (uint32_t) &BootSector[0];
	}

	startblock--;
	if ((startblock + requestblocks) <= FLASH_DISK_SIZE / DATA_RAM_BLOCK_SIZE) {
		*availableblocks = requestblocks;
	}
	else {
		if (startblock >= FLASH_DISK_SIZE / DATA_RAM_BLOCK_SIZE) {
			*availableblocks = 0;
		}
		else {
			*availableblocks = FLASH_DISK_SIZE / DATA_RAM_BLOCK_SIZE - startblock; 
		}
	}

	if (*availableblocks != 0) {
		return (uint32_t) &DiskImageF[startblock * DATA_RAM_BLOCK_SIZE];
	}
	else {
		return (uint32_t) &DiskImageF[(FLASH_DISK_SIZE / DATA_RAM_BLOCK_SIZE - 1) * DATA_RAM_BLOCK_SIZE];
	}
}

uint32_t MassStorage_WriteBlocks(uint32_t startblock, uint8_t* buffer, uint8_t block_count)
{
	// find flash sector 
	//iap_prepare_sector(startblock*);
	return 0;
}


void DataRam_Initialize(void)
{
	if (!InitializedFlag) {
		InitializedFlag = 1;
#ifndef PROGMEMDISK		
		memset(DiskImageA, 0, sizeof(DiskImageA));
		memset(DiskImageB, 0, sizeof(DiskImageB));
		memcpy(DiskImageA, BootSector, sizeof(BootSector));
		CreateDiskImage((DISKIMAGE *) DiskImageA);
#endif
	}
}

uint8_t* DataRam_GetDataPtr(void) 
{
	return DiskImageB;
}

/**
 * @}
 */