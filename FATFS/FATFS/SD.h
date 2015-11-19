/*
 * SD.h
 *
 * Created: 2013-07-10 11:44:50
 *  Author: tmf
 */


#ifndef SD_H_
#define SD_H_

#include <avr/io.h>
#include <stdint.h>

#define SD_PORT PORTC        //Port przez który pod³¹czona jest karta SD
#define SD_USART USARTC1     //Initerfejs USART wykorzystywany do pod³¹czenia SD
#define SD_SPI  SPIC         //Alternatywnie port SPI do komunikacji

#define F_CS    PIN3_bm       //Sygna³ CS pamiêci FLASH
#define TP_CS   PIN2_bm       //Sygna³ CS kontrolera TP
#define SD_CS   PIN4_bm       //Sygna³ CS karty SD
#define SD_SCK  PIN7_bm       //Sygna³ SCK
#define SD_MISO PIN6_bm       //Sygna³ MISO
#define SD_MOSI PIN5_bm       //Sygna³ MOSI


/*
#define SD_USART USARTC0     //Initerfejs USART wykorzystywany do pod³¹czenia SD - w tym przypadku jest to USART 0
#define SD_CS   PIN0_bm       //Sygna³ CS karty SD
#define SD_SCK  PIN1_bm       //Sygna³ SCK
#define SD_MISO PIN2_bm       //Sygna³ MISO
#define SD_MOSI PIN3_bm       //Sygna³ MOSI
#define F_CS    PIN0_bm       //Sygna³ CS pamiêci FLASH
#define TP_CS   PIN0_bm       //Sygna³ CS kontrolera TP
*/

/* MMC/SD command (in SPI mode) */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

#define SD_R1_Idle				1   //Karta w stanie Idle
#define SD_R1_EraseReset		2
#define SD_R1_IllegaCommand		4	//Nierozpoznane polecenie
#define SD_R1_CommandCRCError	8	//B³¹d CRC polecenia
#define SD_R1_EraseSeqError		16
#define SD_R1_AddressError		32
#define SD_R1_ParameterError	64
#define SD_R1_Response			128	//Bit odpowiedzi - zawsze równy 0

#define SD_OCR_CCS              (1UL<<30) //Bit CCS rejestru OCR

typedef union
{
	uint8_t Reg[16];
	struct
	{
		uint8_t crc;
		uint8_t MDT_Month : 4; //Miesi¹c produkcji
		uint8_t MDT_Year2 : 4;
		uint8_t MDT_Year1 : 4; //Rok produkcji od 2000
		uint8_t reserved  : 4; //Same zera
		uint32_t PSN;	       //Numer seryjny
		uint8_t PRV;           //Numer wersji
		char PNM[5];           //Nazwa produktu
		char OID[2];           //Identyfikator producenta w ASCII
		uint8_t MID;           //Identyfikator producenta karty
	};
} SD_CIDReg;

typedef union
{
	uint8_t Reg[16];
	struct
	{
		uint8_t crc;
		uint8_t res1          : 2;
		uint8_t File_Format   : 2;
		_Bool Tmp_WR_Protect  : 1;
		_Bool Perm_WR_Protect : 1;
		_Bool Copy            : 1;
		_Bool File_Frm_Group  : 1;
		uint8_t res2          : 5;
		_Bool Write_BL_Partial : 1;
		uint8_t Write_BL_Len  : 4;
		uint8_t R2WFactor     : 3;
		uint8_t res3          : 2;
		_Bool WP_Grp_En       : 1;
		uint8_t WP_Grp_Size   : 7;
		uint8_t Sector_Size   : 7;
		_Bool Erase_Blk_En    : 1;
		uint8_t C_Size_mult   : 3;
		uint8_t VDD_W_Curr_max : 3;
		uint8_t VDD_W_Curr_min : 3;
		uint8_t VDD_R_Curr_max : 3;
		uint8_t VDD_R_Curr_min : 3;
		uint16_t C_Size       : 12;
		uint8_t res5          : 2;
		_Bool DSR_Imp         : 1;
		_Bool Read_Blk_Misalign  : 1;
		_Bool Write_Blk_Misalign : 1;
		_Bool Read_Bl_Partial : 1;
		uint8_t Read_Bl_Len   : 4;
		uint16_t CCC          : 12;
		uint8_t Tran_Speed;
		uint8_t NSAC;
		uint8_t TAAC;
		uint8_t res6          : 6;
		uint8_t CSD_Struct    : 2;  //Wersja struktury CSD 0 -1.0, 1 -2.0
	};

} SD_CSDRegv1;

typedef union
{
	uint8_t Reg[16];
	struct
	{
		uint8_t crc;
		uint8_t res1          : 2;
		uint8_t File_Format   : 2;
		_Bool Tmp_WR_Protect  : 1;
		_Bool Perm_WR_Protect : 1;
		_Bool Copy            : 1;
		_Bool File_Frm_Group  : 1;
		uint8_t res2          : 5;
		_Bool Write_BL_Partial : 1;
		uint8_t Write_BL_Len  : 4;
		uint8_t R2WFactor     : 3;
		uint8_t res3          : 2;
		_Bool WP_Grp_En       : 1;
		uint8_t WP_Grp_Size   : 7;
		uint8_t Sector_Size   : 7;
		_Bool Erase_Blk_En    : 1;
		uint8_t res4          : 1;
		__uint24 C_Size       : 22;
		uint8_t res5          : 6;
		_Bool DSR_Imp         : 1;
		_Bool Read_Blk_Misalign  : 1;
		_Bool Write_Blk_Misalign : 1;
		_Bool Read_Bl_Partial : 1;
		uint8_t Read_Bl_Len   : 4;
		uint16_t CCC          : 12;
		uint8_t Tran_Speed;
		uint8_t NSAC;
		uint8_t TAAC;
		uint8_t res6          : 6;
		uint8_t CSD_Struct    : 2;  //Wersja struktury CSD 0 -1.0, 1 -2.0
	};

} SD_CSDRegv2;

extern uint8_t SD_Status;                     //Co aktualnie jest podpiête
extern uint8_t SD_Type;                       //Typ karty - SD lub SDHC

void SPIInit();                               //Inicjalizacja interfejsu SPI
uint8_t SPI_RW_Byte(uint8_t byte);            //Funkcja odczytu/zapisu SPI
_Bool SD_CardInit();                          //Inicjalizacja karty SD/SDHC/SDXC
_Bool SD_WaitForReady();                      //Czeka na gotowoœæ karty, max 500 ms
uint8_t SD_SendCMD(uint8_t cmd, uint32_t arg); //Wysy³a polecenie do karty i zwraca kod R1
_Bool SD_GetCID(SD_CIDReg *cid);              //Zwróæ wartoœæ rejestru CID
_Bool SD_GetResponse(void *buf, uint8_t size); //Zapisz odpowied¿ w buforze
uint32_t SD_GetR7();                          //Pobierz 4 bajty odpowiedzi
void SD_CRC7(uint8_t *crc, uint8_t byte);     //Wylicza CRC7
uint8_t SD_disk_read (uint8_t *buff, uint32_t sector, uint8_t count); //Odczytaj count sektorów (1-255), pocz¹wszy od sector, wynik umieœæ w buff
uint8_t DS_disk_write(const uint8_t *buff, uint32_t sector, uint8_t count); //Zapisz count sektorów (1-255), pocz¹wszy od sector, dane z buff
_Bool SD_Rec_Datablock(uint8_t *buff, uint16_t size);   //Odczytaj blok danych z karty, false jeœli wyst¹pi³ b³¹d odczytu

#endif /* SD_H_ */