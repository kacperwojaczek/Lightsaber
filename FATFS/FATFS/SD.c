/*
 * SD.c
 *
 * Created: 2013-07-10 11:47:53
 *  Author: tmf
 */

#include "SD.h"
#include <diskio.h>
#include <avr/io.h>
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>

uint8_t SD_Status;               //Co aktualnie jest podpiête
uint8_t SD_Type;                 //Typ karty - SD lub SDHC

void SPI_SD_CS(_Bool SS_en)
{
	if(SS_en)
	{
		SD_PORT.OUTCLR=SD_CS;  //Aktywuj kartê SD
		SD_WaitForReady();       //Zaczekaj a¿ karta bêdzie gotowa
	}
	 else
	 {
		 SD_PORT.OUTSET=SD_CS;
		 SPI_RW_Byte(0xff);        //Karta ulega deaktywacji po otrzymaniu kolejnego zbocza sygna³u SCK
	 }
}

void SPI_SetBaudrate_400kHz()
{
#ifdef SPI_USART
	uint16_t baud=F_CPU/400000UL;
	SD_USART.BAUDCTRLA=baud & 0xff;
	SD_USART.BAUDCTRLB=(baud >> 8) & 0xff;
#else
	SD_SPI.CTRL=(SD_SPI.CTRL & (~SPI_PRESCALER_gm)) | SPI_PRESCALER_DIV64_gc;  //Preskaler nale¿y dobraæ zale¿nie od taktowania CPU
#endif
}

void SPI_SetMaxBaudrate()
{
#ifdef SPI_USART
	SD_USART.BAUDCTRLA=0;
	SD_USART.BAUDCTRLB=0;                     //Fclk=FPER/2 - maksymalne taktowanie SPI
#else
	SD_SPI.CTRL=(SD_SPI.CTRL & (~SPI_PRESCALER_gm)) | SPI_PRESCALER_DIV4_gc | SPI_CLK2X_bm;
#endif
}

uint8_t SPI_RW_Byte(uint8_t byte)
{
#ifdef SPI_USART
	SD_USART.DATA=byte;
	while(!(SD_USART.STATUS & USART_TXCIF_bm));
	SD_USART.STATUS=USART_TXCIF_bm;
	return SD_USART.DATA;
#else
	SD_SPI.DATA=byte;
	while(!(SD_SPI.STATUS & SPI_IF_bm));
	SD_SPI.STATUS=SPI_IF_bm;
	return SD_SPI.DATA;
#endif
}

void SPIInit()
{
	SD_PORT.DIRSET = SD_MOSI | SD_SCK;      //MOSI i SCK s¹ wyjœciami
#ifdef SPI_USART
	SPI_SetMaxBaudrate();
	SD_USART.CTRLC=USART_CMODE_MSPI_gc;       //Tryb SPI 0
	SD_USART.CTRLB=USART_TXEN_bm | USART_RXEN_bm;
#else
	SD_SPI.CTRL=SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_0_gc | SPI_PRESCALER_DIV64_gc;
#endif
	SD_Status=STA_NOINIT;  //Dysk niezainicjowany
}

void SD_CRC7(uint8_t *crc, uint8_t byte)  //Wylicza CRC7 dla karty, pocz¹tkowo crc=0
{
	for (uint8_t i=0; i < 8; i++)
	{
		*crc <<= 1;
		if ((((byte & 0x80) ^ (*crc & 0x80)) != 0))
		{
			*crc ^= 0x09;
		}
		byte <<= 1;
	}
}

uint8_t SD_SendCMD(uint8_t cmd, uint32_t arg)
{
	uint8_t crc=0, res;

	if(cmd & 0x80)
	{
		cmd&=0x7F;            //Skasuj najstarszy bit polecenia
		res=SD_SendCMD(CMD55, 0);  //Kolejne polecenie nale¿y do grupy ACMD
		if(res > 0x01) return res; //Polecenie CMD55 zakoñczy³o siê b³êdem
	}
	SPI_SD_CS(false);         //Deaktywuj kartê
	SPI_SD_CS(true);          //Aktywuj kartê

	cmd|=0x40;                //najstarsze dwa bity zawsze s¹ równe 01
	SPI_RW_Byte(cmd);
	SD_CRC7(&crc, cmd);
	for(uint8_t i=0; i<4; i++)
	{
		SPI_RW_Byte((arg >> 24) & 0xff);
		SD_CRC7(&crc, (arg >> 24) & 0xff);
		arg<<=8;
	}
	crc=(crc << 1) | 1;        //CRC7 dla SD jest przesuniête o jeden bit w lewo i ma ustawiony najm³odszy bit
	SPI_RW_Byte(crc);          //Wyœlij CRC polecenia

	uint8_t i=10;         //OdpowiedŸ mo¿e nadejœæ od 1 do 10 bajtów po poleceniu
	do
	{
		res=SPI_RW_Byte(0xff);
	} while ((res & 0x80) && --i);
	return res;                //Po wys³aniu polecenia karta pozostaje wybrana w celu odczytu/zapisu kolejnych bajtów
}

uint32_t SD_GetR7()  //Pobierz odpowiedŸ typu R7
{
	uint32_t R7=0;
	for(uint8_t i=0; i<4; i++)
	{
		R7<<=8;
		R7|=SPI_RW_Byte(0xff);
	}
	return R7;
}

_Bool SD_CardInitV1()
{
	uint8_t i;
	for(i=0; i<255; i++)
	{
	  if(SD_SendCMD(ACMD41, 0) == 0) break;  //Karta w tryb aktywny
		else _delay_ms(10);
	}
	if(i == 255) return false;      //Karta nie akceptuje polecenia - uszkodzona?
	SD_GetR7();                     //Co prawda mamy odpowiedŸ R3, ale jej d³ugoœæ jest taka sama jak R7
	SD_SendCMD(CMD16, 512);         //D³ugoœæ bloku 512 bajtów
	SD_Status=0;
	SD_Type=CT_SD1;
	return true;
}

_Bool SD_CardInitV2()
{
	uint8_t i;

	if(SD_GetR7() != 0x1aa)
	 return false;  //B³edne napiêcie pracy karty

	for(i=0; i<255; i++)
	{
		if(SD_SendCMD(ACMD41, SD_OCR_CCS) == 0) break;  //Karta w tryb aktywny
		   else _delay_ms(40);

	}
	if(i == 255) return false;           //Karta nie akceptuje polecenia - uszkodzona?
	if(SD_SendCMD(CMD58, 0)==0)
	{
		SD_Status=0;
		if(SD_GetR7() & SD_OCR_CCS) SD_Type=CT_SD2;  //Pobierz OCR
		    else SD_Type=CT_SD1;
	}
	return true;
}

_Bool SD_CardInit()
{
	SPI_SetBaudrate_400kHz(&SD_USART);     //Kartê nale¿y zainicjowaæ przy SCK równym 400 kHz
	SPI_SD_CS(false);                      //Deaktywuj kartê

	for(uint8_t i=0; i<10; i++)
	{
		SPI_RW_Byte(0xff);                 //Wyœlij co najmniej 74 takty SCK dla inicjalizacji karty
	}

	uint8_t i=10, ret;
	do
	{
		ret=SD_SendCMD(CMD0, 0);

	} while ((ret != SD_R1_Idle) && --i);  //Zaczekaj na prawid³ow¹ odpowiedŸ ze strony karty

	if(!i)
	{
		SD_Status=STA_NODISK;              //Brak karty lub inny b³¹d
		return false;                      //Niedoczekaliœmy siê
	}

	i=10;
	do
	{
		ret=SD_SendCMD(CMD8, 0x1aa);
	} while ((ret & 0xc0) && --i);

	_Bool result;
	if(ret==0x05) result=SD_CardInitV1(); else result=SD_CardInitV2(); //Z jak¹ kart¹ mamy do czynienia
	if(result==false) SD_Status=STA_NODISK;
	SPI_SetMaxBaudrate(&USARTC1);  //Prze³¹cz interfejs na maksymaln¹ szybkoœæ pracy
	return result;
}

_Bool SD_WaitForReady()
{
	uint16_t counter=5000;              //Czekamy maksymalnie ok. 500 ms
	SPI_RW_Byte(0xff);
	do									//Zaczekaj na gotowoœæ karty - sygna³ CS musi byæ wczeœniej aktywowany
	{
		if(SPI_RW_Byte(0xff) == 0xff) break;
		_delay_us(100);
	} while (--counter);
	return counter ? true : false;      //Czy wyst¹pi³ timeout?
}

_Bool SD_Rec_Datablock(uint8_t *buff, uint16_t size)
{
	uint8_t rec, timeout;

	timeout=200;
	do
	{
		_delay_us(500);                  //Zanim blok danych bêdzie gotowy potrzebujemy chwilkê
		rec=SPI_RW_Byte(0xff);
	} while((rec == 0xff) && timeout--); //Czekamy na otrzymanie 0xfe, ale max 100 ms

	if(rec != 0xfe) return false;		 //B³¹d - nie ma pocz¹tku danych

	do {						         //Odbierz blok danych
		*buff=SPI_RW_Byte(0xff);
		buff++;
	} while(--size);
	SPI_RW_Byte(0xff);                   //Odbierz CRC bloku danych - obowi¹zkowe
	SPI_RW_Byte(0xff);
	return true;
}

_Bool SD_Sendblock(const uint8_t *buff, uint8_t token)  //Wyœlij 512-bajtowy blok danych i zakoñcz go tokenem
{
	uint8_t resp;
	uint16_t counter;

	if(SD_WaitForReady() == false) return false; //Zaczekaj na gotowoœæ karty

	SPI_RW_Byte(token);					//Wyœlij token pocz¹tku/koñca danych
	if(token != 0xfd)                   //0xfd to token koñca danych - po nim nic nie wysy³amy
	{
		counter=512;
		do
		{
			SPI_RW_Byte(*buff++);       //Wyœlij dane
		} while(--counter);
		SPI_RW_Byte(0xff);				//Musimy wys³aæ CRC16, nawet jeœli karta go nie sprawdza
		SPI_RW_Byte(0xff);
		resp=SPI_RW_Byte(0xff);			//Odczytaj odpoweidŸ karty
		if((resp & 0x1F) != 0x05) return false;	//SprawdŸ czy karta zaakceptowa³a dane
	}
	return true;
}

uint8_t SD_disk_read(uint8_t *buff, uint32_t sector, uint8_t count)
{
	if(SD_Status != 0) return RES_NOTRDY;
	if(SD_Type == CT_SD1) sector*=512;	//SDSC adresowana jest bajtami

	if(count == 1)
	{	//Odczytujemy pojedynczy sektor
		if((SD_SendCMD(CMD17, sector) == 0) && SD_Rec_Datablock(buff, 512))
		count=0;
	} else
	{				//Odczytujemy kilka sektorów na raz
		if(SD_SendCMD(CMD18, sector) == 0)
		{
			do
			{
				if(!SD_Rec_Datablock(buff, 512)) break;
				buff+=512;
			} while(--count);
			SD_SendCMD(CMD12, 0);				//Koniec transmisji
		}
	}

	SPI_SD_CS(false);

	return count ? RES_ERROR : RES_OK;
}

uint8_t DS_disk_write(const uint8_t *buff, uint32_t sector, uint8_t count)
{
	if(SD_Status != 0) return RES_NOTRDY;
	if(SD_Type == CT_SD1) sector*=512;	//SDSC adresowana jest bajtami

	if(count == 1)
	{	//Zapisujemy pojedynczy sektor
		if((SD_SendCMD(CMD24, sector) == 0) && SD_Sendblock(buff, 0xfe)) count=0;
	} else
	{				//Zapisujemy kilka sektorów na raz
		SD_SendCMD(ACMD23, count);
		if(SD_SendCMD(CMD25, sector) == 0)
		 {
			do
			{
				if (!SD_Sendblock(buff, 0xfc)) break;
				buff += 512;
			} while (--count);
			if (!SD_Sendblock(0, 0xfd)) count=1;	//Token koñca wielobajtowego bloku danych
		}
	}
	SPI_SD_CS(false);

	return count ? RES_ERROR : RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
BYTE drv)		/* Physical drive nmuber (0) */
{
	 if (drv) return STA_NOINIT;			/* Supports only single drive */
	 SD_CardInit();
	 return SD_Status;
}

/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
BYTE drv)		/* Physical drive nmuber (0) */
{
	if (drv) return STA_NOINIT;		/* Supports only single drive */
	return SD_Status;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
BYTE drv,			/* Physical drive nmuber (0) */
BYTE *buff,			/* Pointer to the data buffer to store read data */
DWORD sector,		/* Start sector number (LBA) */
BYTE count)			/* Sector count (1..255) */
{
	if(drv) return RES_ERROR;
	return SD_disk_read(buff, sector, count);
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
BYTE drv,			/* Physical drive nmuber (0) */
const BYTE *buff,	/* Pointer to the data to be written */
DWORD sector,		/* Start sector number (LBA) */
BYTE count)			/* Sector count (1..255) */
{
	if(drv) return RES_ERROR;
	return DS_disk_write(buff, sector, count);
}

#if _USE_IOCTL

_Bool SD_GetResponse(void *buf, uint8_t size)
{
	uint8_t i=10;
	while((SPI_RW_Byte(0xff)!=0xfe) && (--i));
	if(!i) return false;
	while(size)
	{
		((uint8_t*)buf)[size-1]=SPI_RW_Byte(0xff);
		--size;
	}
	return true;
}

DRESULT disk_ioctl (
BYTE pdrv,		/* Physical drive nmuber (0..) */
BYTE cmd,		/* Control code */
void *buff)		/* Buffer to send/receive control data */
{
	if(SD_Status != 0) return RES_NOTRDY;    //Pamiêæ niezainicjowana

	DRESULT res=RES_OK;

	switch(cmd)
	{
		case GET_SECTOR_SIZE:   *((DWORD*)buff)=512;         //D³ugoœæ sektora 512 bajtów
								break;
		case GET_SECTOR_COUNT:
		{
								char buf[16];
								SD_CSDRegv1 *csd=(SD_CSDRegv1*)buf;
								uint8_t r1=SD_SendCMD(CMD9, 0);  //Odczytaj CSD
								if(r1==0)
								{
									SD_GetResponse(csd->Reg, 16);
									uint64_t size;  //Rozmiar karty
									if(csd->CSD_Struct == 0)
									{
										size=(csd->C_Size + 1) * (1ULL << csd->Read_Bl_Len) * (1UL << (csd->C_Size_mult + 2));
									} else
									{
										size=(((SD_CSDRegv2*)csd)->C_Size + 1) * 524288ULL;  //C_Size zawiera liczbê bloków po 512 kB
									}
									*((DWORD*)buff)=size/512;      //Zwracami liczbê 512-bajtowych sektorów
								} else res=RES_NOTRDY;
		}
								break;
		case GET_BLOCK_SIZE:    *((DWORD*)buff)=1;           //Ile mo¿na na raz skasowaæ sektorów
								break;
		case CTRL_SYNC:         break;
		case CTRL_ERASE_SECTOR: break;
		default:                res=RES_PARERR;              //Nieznane polecenie
	}

	return res;
}
#endif


#if _USE_WRITE

DWORD get_fattime (void)
{
	return 0;  //B³êdnie, nale¿y tu zwróciæ poprawn¹ datê
}

#endif