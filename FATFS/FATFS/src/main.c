/**
 * (c)2013 Tomasz Francuz
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <avr/interrupt.h>
#include <asf.h>
#include "conf_usb.h"
#include <conf_clock.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include "ff.h"
#include "SD.h"
#include "DAC.h"

//Makro umieszczaj�ce zadany �a�cuch w przestrzeni adresowej __flash
#define PGM_STR(X) ((const __flash char[]) { X })

typedef void (*CmdHandler)(char *param);

typedef struct
{
	const char __flash *cmd;                //Wska�nik do polecenia w formacie ASCIIZ
	const CmdHandler  Handler;              //Funkcja realizuj�ca polecenie
} Command;

void Cmd_Dir(char *param);     //Wy�wietla spis plik�w
void Cmd_Read(char *param);    //(nazwa, size) - odczytuje plik
void Cmd_MkDir(char *param);   //Utw�rz nowy katalog
void Cmd_cd(char *param);      //Wejd� do katalogu

//Lista rozpoznawanych polece�
const __flash Command __flash Polecenia[]={{PGM_STR("dir"), Cmd_Dir}, {PGM_STR("read"), Cmd_Read}, {PGM_STR("mkdir"), Cmd_MkDir}, {PGM_STR("cd"), Cmd_cd}};

FATFS fatfs;

int main(void)
{
	void InterpretCommand(char *cmdline)
	{
		uint8_t indeks;
		char *cmd=strtok(cmdline, " \r\n"); //Wydziel polecenie z przekazanej linii
		uint8_t max_indeks=sizeof(Polecenia)/sizeof(Polecenia[0]);
		for(indeks=0; indeks<max_indeks; indeks++)
		{
			if(strcmp_P(cmd, Polecenia[indeks].cmd)==0) //Przeszukaj list� polece�
			{
				Polecenia[indeks].Handler(cmdline);   //Wywo�aj funkcj� obs�ugi przes�anego polecenia
				break;
			}
		}
		if(indeks == max_indeks)  //Je�li polecenie nieznane...
		{
			udi_cdc_write_buf(cmd, strlen(cmd));
			udi_cdc_write_buf(" - nieznane polecenie\r\n", 22); //B��d
		}
	}
	
	Amplifier_init();
	isPlaying=false;
	PORTD_PIN0CTRL=PORT_OPC_PULLUP_gc; //W��czamy pullup na SW0
	PORTD_PIN1CTRL=PORT_OPC_PULLUP_gc; //W��czamy pullup na SW1

	SD_PORT.OUTSET=0xff;         //Ustaw ew. sygnay SS urz�dze� pod��czonych do SPI
	SD_PORT.DIRSET=SD_CS | F_CS | TP_CS; //Deaktywuj sygna�y CS karty SD, kontrolera TP i pami�ci FLASH (na module LCD)
	SPIInit();            //Zainicjuj interfejs SPI u�ywany przez pami��

	PMIC.CTRL = PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm; //W��czamy wszystkie poziomy przerwa� (potrzebujemy tylko najni�szy)
	sei();

	f_mount(0, &fatfs);  //Dost�p do systemu plik�w//
	sysclk_init();
	udc_start();

	while (1)
	{
		if (isPlaying == false) Snd_init(20000, false); //max = +4,294,967,295	
		char Bufor[256];   //Bufor na odebranie linii
		uint8_t indeks=0;

		while(1)
		 {
			if(udi_cdc_is_rx_ready())
			{
				Bufor[indeks] = udi_cdc_getc();
				if(Bufor[indeks] == '\n')
				{
					if((indeks >= 1) && (Bufor[indeks-1] == '\r')) --indeks;
					Bufor[indeks]=0; //Wpisz ko�cowy znak NUL
					break;           //Odebrano pe�n� lini�
				}
				++indeks;
			}
		 }
		InterpretCommand(Bufor);  //Zdekoduj przes�ane polecenie 
	}
}

void main_suspend_action(void)
{
//Interfejs wchodzi w tryb powerdown
}

void main_resume_action(void)
{
//Wybudzenie interfejsu
}

void main_sof_action(void)
{
//	Odebrano ramk�
}

bool main_cdc_enable(uint8_t port)
{
	// Otwarto port CDC
	return true;
}

void main_cdc_disable(uint8_t port)
{
	// Zamkni�to port CDC
}

void main_cdc_set_dtr(uint8_t port, bool b_enable)
{
	if (b_enable) {
		// Host otwar� port COM
	}else{
		// Host zamkn�� port COM
	}
}

void uart_rx_notify(uint8_t port)
{
	//Opcjonalnie powiadomienie o nadej�ciu znaku
}

void uart_config(uint8_t port, usb_cdc_line_coding_t * cfg)
{
	//Konfiguracja w odpowiedzi na otwarcie CDC
}

void udi_cdc_write_txt(char *txt)
{
	udi_cdc_write_buf(txt, strlen(txt));
}


void Cmd_Dir(char *param)
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    char *fn;
    #if _USE_LFN                    //Tylko je�li u�ywamy d�ugich nazw plik�w
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
    #endif
	char Bufor[_MAX_LFN + 1];
	f_getcwd(Bufor, sizeof(Bufor));

    res = f_opendir(&dir, Bufor);                       // Otw�rz kattalog
    if (res == FR_OK)
	{
	    for (;;)
		{
		    res = f_readdir(&dir, &fno);                   //Odczytaj pozycj� z katalogu
		    if (res != FR_OK || fno.fname[0] == 0) break;  //Je�li wyst�pi� b��d lub nie ma wi�cej pozycji to zako�cz
		    if (fno.fname[0] == '.') continue;             //Jedn� z pozycji jest '.' - zignoruj
		    #if _USE_LFN
		    fn = *fno.lfname ? fno.lfname : fno.fname;
		    #else
		    fn = fno.fname;
		    #endif
		    if (fno.fattrib & AM_DIR)
			    sprintf(Bufor, "%-12s <DIR>\r\n", fn);   //Odczytana pozycja jest katalogiem
			   else sprintf(Bufor, "%-12s %lu\r\n", fn, fno.fsize); //Odczytana pozycja jestplikiem
			udi_cdc_write_txt(Bufor);           //Wy�lij nazw� przez USB
	    }
    }
}

void Cmd_Read(char *param)
{
	char *name=strtok(NULL, " \r\n");          //Pobierz nazw� pliku
	FIL fdst;

	if(f_open(&fdst, name, FA_READ) == FR_OK)  //Otw�rz plik
	{
		uint8_t Bufor[64];
		UINT zapisane;

		do
		{
			f_read(&fdst, Bufor, 64, &zapisane);  //Odczytaj porcj� danych z pliku
			udi_cdc_write_buf(Bufor, zapisane);   //Wy�lij dane do PC
		} while(zapisane == 64);
		f_close(&fdst);
	} else udi_cdc_write_buf("Pliku nie znaleziono.\r\n", 23);
}

void Cmd_cd(char *param)
{
	char *name=strtok(NULL, " \r\n");  //Pobierz nazw� katalogu do utworzenia
	FRESULT res=f_chdir(name);
	if(res!=FR_OK) udi_cdc_write_txt("Bledna nazwa katalogu!");
}

void Cmd_MkDir(char *param)
{
#if !_FS_READONLY
	char *name=strtok(NULL, " \r\n");  //Pobierz nazw� katalogu do utworzenia
	FRESULT res=f_mkdir(name);
	if(res!=FR_OK)
		udi_cdc_write_txt("Blad, katalogu nie utworzono!");
#endif
}

