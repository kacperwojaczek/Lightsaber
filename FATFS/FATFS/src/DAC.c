/*
 * DAC.c
 *
 * Created: 2013-09-14 17:16:30
 *  Author: tmf
 */

#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "DAC.h"

void DAC_init()
{
	//DACB.CTRLC=DAC_REFSEL_INT1V_gc | DAC_LEFTADJ_bm;                //Wewn. napi�cie ref. 1 V
	DACB.CTRLC=DAC_REFSEL_AVCC_gc | DAC_LEFTADJ_bm;                //Wewn. napi�cie ref. 1 V
	DACB.CTRLB=DAC_CH0TRIG_bm;               //Konwersja wyzwalana zdarzeniem w kanale CH0
	DACB.EVCTRL=DAC_EVSEL_0_gc;              //EvCh 0
	EVSYS_CH0MUX=EVSYS_CHMUX_TCC0_OVF_gc; //taktowanie evch0 z przepe�nienia timera C0
	DACB.CTRLA=DAC_ENABLE_bm | DAC_CH0EN_bm; //W��cz DAC, kana� 0 routowany na PB2
}

ISR(DMA_CH0_vect)
{
	DMA_INTFLAGS=DMA_CH0TRNIF_bm; //Skasuj flag� przerwania
	if(((DMA_CH0_CTRLA & DMA_CH_ENABLE_bm) == 0) && ((DMA_CH1_CTRLA & DMA_CH_ENABLE_bm) == 0))
	{
		isPlaying=false;  //zako�czyli�my odtwarzanie d�wi�ku
		return;
	}
	if(DMA_CH0_REPCNT == 1) DMA_CH0_TRFCNT=samplesize % BUFFER_SIZE;
	ReadFile("hum.wav", srcaddr, &samplebuffer[0][0], BUFFER_SIZE);
	srcaddr+=BUFFER_SIZE;
}

ISR(DMA_CH1_vect)
{
	DMA_INTFLAGS=DMA_CH1TRNIF_bm; //Skasuj flag� przerwania
	if(((DMA_CH0_CTRLA & DMA_CH_ENABLE_bm) == 0) && ((DMA_CH1_CTRLA & DMA_CH_ENABLE_bm) == 0))
	{
		isPlaying=false;  //zako�czyli�my odtwarzanie d�wi�ku
		return;
	}
	if(DMA_CH0_REPCNT == 1) DMA_CH0_TRFCNT=samplesize % BUFFER_SIZE;
	ReadFile("hum.wav", srcaddr, &samplebuffer[1][0], BUFFER_SIZE);
	srcaddr+=BUFFER_SIZE;
}

void Timer_init(uint16_t samplerate)  //Generuje zdarzenia wywo�uj�ce konwersj� DAC i prze��czanie bufor�w
{
	//TCC0 - generuje zdarzenia wywo�uj�ce konwersj� (okre�la samplerate)
	TCC0.PER=F_CPU/(samplerate- 1); //Wygeneruj zdarzenia o odpowiedniej cz�stotliwo�ci
	TCC0.CTRLA=TC_CLKSEL_DIV1_gc;
}

void Amplifier_init()
{
	PORTB.OUTCLR=PIN2_bm;
	PORTB.DIRSET=PIN2_bm;  //Pin DACB0 jako wyj�cie
}

void Snd_init(DWORD smpsize, uint16_t bitrate, _Bool HiRes)
{
	void DMA_init()
	{
		void DMA_CH_Cfg(DMA_CH_t *CH, uint16_t srcaddr, uint8_t repcnt)
		{
			CH->TRIGSRC=DMA_CH_TRIGSRC_DACB_CH0_gc;          //Zdarzeniem wyzwalaj�cym jest pusty rejestr danych kana�u CH0 DACA
			CH->TRFCNT=BUFFER_SIZE;                          //Blok ma d�ugo�� r�wn� buforowi pr�bek
			CH->REPCNT=repcnt;
			CH->SRCADDR0=srcaddr & 0xFF;
			CH->SRCADDR1=srcaddr >> 8;
			CH->SRCADDR2=0;                                  //�r�d�em danych jest wskazany bufor
			CH->CTRLB=DMA_CH_TRNINTLVL_LO_gc;                //Generuj przerwaie ko�ca transakcji
			if(HiRes)
			{
				CH->DESTADDR0=(uint16_t)(&DACB.CH0DATAL) & 0xFF; //Dane wpisujemy do rejestru DATA uk�adu DAC
				CH->DESTADDR1=(uint16_t)(&DACB.CH0DATAL) >> 8;
				CH->DESTADDR2=0;
				CH->ADDRCTRL=DMA_CH_SRCRELOAD_BLOCK_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTDIR_INC_gc | DMA_CH_DESTRELOAD_BURST_gc; //Zwiekszamy adres �r�d�a i przeznaczenia, reload adresu co blok, adresu docelowego co transfer
				CH->CTRLA=DMA_CH_ENABLE_bm | DMA_CH_REPEAT_bm | DMA_CH_BURSTLEN_2BYTE_gc | DMA_CH_SINGLE_bm;  //Kana� 0 w trybie powtarzania, d�ugo�� transferu 2 bajty, single shot
			} else
			{
				CH->DESTADDR0=(uint16_t)(&DACB.CH0DATAH) & 0xFF; //Dane wpisujemy do rejestru DATA uk�adu DAC
				CH->DESTADDR1=(uint16_t)(&DACB.CH0DATAH) >> 8;
				CH->DESTADDR2=0;
				CH->ADDRCTRL=DMA_CH_SRCRELOAD_BLOCK_gc | DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTDIR_FIXED_gc | DMA_CH_DESTRELOAD_NONE_gc; //Zwiekszamy adres �r�d�a, reload adresu co blok, adres docelowy bez zmian
				CH->CTRLA=DMA_CH_ENABLE_bm | DMA_CH_REPEAT_bm | DMA_CH_BURSTLEN_1BYTE_gc | DMA_CH_SINGLE_bm;  //Kana� 0 w trybie powtarzania, d�ugo�� transferu 1 bajt, single shot
			}
		}

		uint16_t repcnt=(smpsize + BUFFER_SIZE) / BUFFER_SIZE;
		DMA_CH_Cfg(&DMA.CH0, (uint16_t)&samplebuffer[0][0], (repcnt & 1) ? (repcnt/2+1) : (repcnt/2));      //Adres pierwszego bufora
		DMA_CH_Cfg(&DMA.CH1, (uint16_t)&samplebuffer[1][0], repcnt/2);                                      //Adres drugiego bufora
		DMA.CTRL=DMA_ENABLE_bm | DMA_DBUFMODE_CH01_gc;  //Odblokuj kontroler DMA, round robin, podw�jne buforowanie kana�y 0 & 1
	}

	samplesize=smpsize;  //D�ugo�� odtwarzanej pr�bki w bajtach
	srcaddr=0;    //Adres pr�bki
	ReadFile("hum.wav", srcaddr, &samplebuffer, BUFFER_SIZE*2);//Zainicjuj bufor pr�bek w SRAM
	srcaddr+=BUFFER_SIZE*2;                            //Adres dalszej cz�ci pr�bek

	DMA_init();
	Timer_init(bitrate);  //Zainicjuj timer z odpowiednim samplerate
	DAC_init();           //Zainicjuj transfer DMA pr�bek
	isPlaying=true;
}

void ReadFile(char* name, DWORD offset, void *dst, size_t len)
{
		FIL fdst;

		if(f_open(&fdst, name, FA_READ) == FR_OK)  //Otw�rz plik
		{
			UINT zapisane;
			f_lseek (&fdst, offset);
			f_read(&fdst, dst, len, &zapisane);  //Odczytaj porcj� danych z pliku
			f_close(&fdst);
		}
}

    /*while(1)
    {
       if(((PORTD_IN & PIN0_bm) == 0) && (isPlaying == false)) Snd_init(GET_FAR_ADDRESS(_binary____intro8kHz8PWMHL_raw_start), GET_FAR_ADDRESS(_binary____intro8kHz8PWMHL_raw_size), 8000, false);  //Zagraj pr�bk� 8kHz/8 bit�w
	   if(((PORTD_IN & PIN1_bm) == 0) && (isPlaying == false)) Snd_init(GET_FAR_ADDRESS(_binary____intro8kHz16PWMu_raw_start), GET_FAR_ADDRESS(_binary____intro8kHz16PWMu_raw_size), 8000, true);  //Zagraj pr�bk� 16kHz/16 bit�w
    }*/