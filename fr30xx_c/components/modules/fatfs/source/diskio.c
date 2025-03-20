/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <string.h>

#include "fr30xx.h"

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

#include "IC_W25Qxx.h"

/* Definitions of physical drive number for each drive */
#define DEV_RAM		            0   /* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		            1   /* Example: Map MMC to physical drive 1 */
#define DEV_SPI_FLASH		    2   /* Example: Map SPI Flash to physical drive 2 */
#define DEV_NAND_FLASH		    3   /* Example: Map Nand Flash to physical drive 3 */
#define DEV_SD_CARD             4   /* Example: Map SD Card to physical drive 4 */

#define RAM_SECTOR_SIZE         512
#define RAM_SECTOR_COUNT        (16*48)
#define RAM_BLOCK_SIZE          4096
#define RAM_DISK_SIZE           (RAM_SECTOR_SIZE * RAM_SECTOR_COUNT)

#define SD_CARD_SECTOR_SIZE     512
#define SD_CARD_BLOCK_SIZE      4096

#define SPI_FLASH_SECTOR_SIZE   512
#define SPI_FLASH_BLOCK_SIZE    4096
#define SPI_FLASH_SECTOR_COUNT  (2*1024*4)

__ALIGNED(4) static uint8_t ram_disk_space[RAM_DISK_SIZE];
static uint8_t spi_flash_poll[4096];
extern SD_HandleTypeDef sdio_handle;

static void _SPI_Flash_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)
{
    uint32_t secpos;
    uint16_t secoff;
    uint16_t secremain;
    uint16_t i;

    secpos = WriteAddr/4096;    // 扇区地址 0~511 for w25x16
    secoff = WriteAddr%4096;    // 在扇区内的偏�?
    secremain = 4096 - secoff;  // 扇区剩余空间大小

    if (NumByteToWrite <= secremain) {
        secremain=NumByteToWrite;//不大�?4096个字�?
    }

    while(1)
    {
        IC_W25Qxx_Read_Data(spi_flash_poll, secpos*4096, 4096); // 读出整个扇区的内�?
        for(i=0; i<secremain; i++)  // 校验数据
        {
            if(spi_flash_poll[secoff+i] != 0xFF)
                break;      // 需要擦�?
        }
        if(i < secremain)   // 需要擦�?
        {
            IC_W25Qxx_EraseSector(secpos * 4096);
            for(i=0; i<secremain; i++)     // 复制
            {
                spi_flash_poll[i+secoff] = pBuffer[i];
            }
            for (uint8_t j=0; j<16; j++) {
                IC_W25Qxx_PageProgram(&spi_flash_poll[j * 256], secpos*4096 + j * 256, 256);   // 写入整个扇区
            }
        }
        else
        {
            IC_W25Qxx_PageProgram(pBuffer, WriteAddr, secremain);       // 写已经擦除了�?,直接写入扇区剩余区间.
        }

        if(NumByteToWrite==secremain)
        {
            break;  // 写入结束�?
        }
        else    // 写入未结�?
        {
            secpos++;   // 扇区地址�?1
            secoff=0;   // 偏移位置�?0

            pBuffer += secremain;           // 指针偏移
            WriteAddr += secremain;         // 写地址偏移
            NumByteToWrite -= secremain;    // 字节数递减
            if(NumByteToWrite > 4096)
            {
                secremain=4096;             // 下一个扇区还是写不完
            }
            else 
            {
                secremain=NumByteToWrite;   // 下一个扇区可以写完了
            }
        }
    }
}

static DSTATUS RAM_disk_status(void)
{
    return RES_OK;
}

static DSTATUS MMC_disk_status(void)
{
    return RES_OK;
}

static DSTATUS SPI_flash_status(void)
{
    return RES_OK;
}

static DSTATUS SD_Card_status(void)
{
    return RES_OK;
}

static DSTATUS RAM_disk_initialize(void)
{
//    memset(ram_disk_space, 0, RAM_DISK_SIZE);
    return RES_OK;
}

static DSTATUS MMC_disk_initialize(void)
{
    return RES_ERROR;
}

static DSTATUS SPI_flash_initialize(void)
{
    return RES_OK;
}

static DSTATUS SD_Card_initialize(void)
{
//    uint32_t EER;
//    
//    EER = SDCard_Init(&SDIO);
//    if (EER != INT_NO_ERR) {
//        return RES_ERROR;
//    }
//    EER = SDCard_BusWidth_Select(&SDIO, SDIO_BUS_WIDTH_4BIT);
//    if (EER != INT_NO_ERR) {
//        return RES_ERROR;
//    }

    return RES_OK;
}

static DSTATUS RAM_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    printf("RAM_disk_read: sector = %d, count = %d\r\n", sector, count);
    memcpy(buff, &ram_disk_space[sector*RAM_SECTOR_SIZE], count * RAM_SECTOR_SIZE);
    return RES_OK;
}

static DSTATUS SPI_flash_read(BYTE *buff, LBA_t sector, UINT count)
{
    printf("SPI_flash_read: sector = %d, count = %d\r\n", sector, count);
    IC_W25Qxx_Read_Data(buff, sector * SPI_FLASH_SECTOR_SIZE, count * SPI_FLASH_SECTOR_SIZE);
    return RES_OK;
}

static DSTATUS MMC_disk_read(BYTE *buff, LBA_t sector, UINT count)
{
    return RES_ERROR;
}

static DSTATUS SD_Card_read(BYTE *buff, LBA_t sector, UINT count)
{
    SDCard_ReadBolcks(&sdio_handle, (uint32_t *)buff, sector, count);

    return RES_OK;
}

static DSTATUS RAM_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    printf("RAM_disk_write: sector = %d, count = %d\r\n", sector, count);
    memcpy(&ram_disk_space[sector*RAM_SECTOR_SIZE], buff, count * RAM_SECTOR_SIZE);
    return RES_OK;
}

static DSTATUS MMC_disk_write(const BYTE *buff, LBA_t sector, UINT count)
{
    return RES_ERROR;
}

static DSTATUS SPI_flash_write(const BYTE *buff, LBA_t sector, UINT count)
{
    printf("SPI_flash_write: sector = %d, count = %d\r\n", sector, count);
    _SPI_Flash_Write((uint8_t *)buff, sector * SPI_FLASH_SECTOR_SIZE, count * SPI_FLASH_SECTOR_SIZE);
    return RES_OK;
}

static DSTATUS SD_Card_write(const BYTE *buff, LBA_t sector, UINT count)
{
    SDCard_WriteBolcks(&sdio_handle, (uint32_t *)buff, sector, count);

    return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
	case DEV_RAM :
		stat = RAM_disk_status();

		// translate the reslut code here

		return stat;

	case DEV_MMC :
		stat = MMC_disk_status();

		// translate the reslut code here

		return stat;
    
    case DEV_SPI_FLASH :
		stat = SPI_flash_status();

		// translate the reslut code here

		return stat;

    case DEV_SD_CARD :
		stat = SD_Card_status();

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
	case DEV_RAM :
		stat = RAM_disk_initialize();

		// translate the reslut code here

		return stat;

	case DEV_MMC :
		stat = MMC_disk_initialize();

		// translate the reslut code here

		return stat;
    
    case DEV_SPI_FLASH :
		stat = SPI_flash_initialize();

		// translate the reslut code here

		return stat;
    
    case DEV_SD_CARD :
		stat = SD_Card_initialize();

		// translate the reslut code here

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;

	switch (pdrv) {
	case DEV_RAM :
		// translate the arguments here

		res = RAM_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_MMC :
		// translate the arguments here

		res = MMC_disk_read(buff, sector, count);

		// translate the reslut code here

		return res;
    
    case DEV_SPI_FLASH :
		// translate the arguments here

		res = SPI_flash_read(buff, sector, count);

		// translate the reslut code here

		return res;
    
    case DEV_SD_CARD :
		// translate the arguments here

		res = SD_Card_read(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;

	switch (pdrv) {
	case DEV_RAM :
		// translate the arguments here

		res = RAM_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;

	case DEV_MMC :
		// translate the arguments here

		res = MMC_disk_write(buff, sector, count);

		// translate the reslut code here

		return res;
    
    case DEV_SPI_FLASH :
		// translate the arguments here

		res = SPI_flash_write(buff, sector, count);

		// translate the reslut code here

		return res;
    
    case DEV_SD_CARD :
		// translate the arguments here

		res = SD_Card_write(buff, sector, count);

		// translate the reslut code here

		return res;
	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_PARERR;

	switch (pdrv) {
	case DEV_RAM :

		// Process of the command for the RAM drive
            switch(cmd)
            {
                case CTRL_SYNC:
                    res = RES_OK;
                    break;
                case GET_SECTOR_SIZE:
                    *(WORD*)buff = RAM_SECTOR_SIZE;
                    res = RES_OK;
                    break;
                case GET_BLOCK_SIZE:
                    *(WORD*)buff = RAM_BLOCK_SIZE;
                    res = RES_OK;
                    break;
                case GET_SECTOR_COUNT:
                    *(DWORD*)buff = RAM_SECTOR_COUNT;
                    res = RES_OK;
                    break;
                case CTRL_TRIM:
                    res = RES_OK;
                    break;
                default:
                    res = RES_PARERR;
                    break;
            }

		return res;

	case DEV_MMC :

		// Process of the command for the MMC/SD card

		return res;
    
    case DEV_SPI_FLASH :

		// Process of the command for the MMC/SD card
            switch(cmd)
            {
                case CTRL_SYNC:
                    res = RES_OK;
                    break;
                case GET_SECTOR_SIZE:
                    *(WORD*)buff = SPI_FLASH_SECTOR_SIZE;
                    res = RES_OK;
                    break;
                case GET_BLOCK_SIZE:
                    *(WORD*)buff = SPI_FLASH_BLOCK_SIZE;
                    res = RES_OK;
                    break;
                case GET_SECTOR_COUNT:
                    *(DWORD*)buff = SPI_FLASH_SECTOR_COUNT;
                    res = RES_OK;
                    break;
                case CTRL_TRIM:
                    res = RES_OK;
                    break;
                default:
                    res = RES_PARERR;
                    break;
            }

		return res;
    
    case DEV_SD_CARD :

		// Process of the command for the RAM drive
            switch(cmd)
            {
                case CTRL_SYNC:
                    res = RES_OK;
                    break;
                case GET_SECTOR_SIZE:
                    *(WORD*)buff = SD_CARD_SECTOR_SIZE;
                    res = RES_OK;
                    break;
                case GET_BLOCK_SIZE:
                    *(WORD*)buff = SD_CARD_BLOCK_SIZE;
                    res = RES_OK;
                    break;
                case GET_SECTOR_COUNT:
                    *(DWORD*)buff = SDCard_Get_Block_count(&sdio_handle);
                    res = RES_OK;
                    break;
                case CTRL_TRIM:
                    res = RES_OK;
                    break;
                default:
                    res = RES_PARERR;
                    break;
            }

		return res;
	}

	return RES_PARERR;
}

