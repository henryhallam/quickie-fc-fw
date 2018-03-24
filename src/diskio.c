/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */

/* Definitions of physical drive number for each drive */
#define DEV_MICROSD 	0

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE pdrv) { /* Physical drive number to identify the drive */
  DSTATUS stat;
  int result;
  if (pdrv != DEV_MICROSD)
    return STA_NOINIT;
  //result = MMC_disk_status();
  (void)result;
  stat = STA_NOINIT;
  return stat;
}



/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive number to identify the drive */
)
{
  DSTATUS stat;
  int result;
  if (pdrv != DEV_MICROSD)
    return STA_NOINIT;
  //result = MMC_disk_init();
  (void)result;
  stat = STA_NOINIT;
  return stat;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive number to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
  int result;
  if (pdrv != DEV_MICROSD)
    return RES_PARERR;
  (void)buff;
  (void)sector;
  (void)count;
  //result = MMC_disk_read(buff, sector, count);
  result = RES_NOTRDY;
  return result;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive number to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
  int result;
  (void)buff;
  (void)sector;
  (void)count;
  if (pdrv != DEV_MICROSD)
    return RES_PARERR;
  //result = MMC_disk_write(buff, sector, count);
  result = RES_NOTRDY;
  return result;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive number (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  DRESULT res;
  (void)cmd;
  (void)buff;
  if (pdrv != DEV_MICROSD)
    return RES_PARERR;
  res = RES_NOTRDY;
  return res;
}

