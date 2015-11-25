/*
 * DAC.h
 *
 * Created: 25.11.2015 19:59:14
 *  Author: Kacper
 */ 


#ifndef DAC_H_
#define DAC_H_

#include "ff.h"

void Snd_init(DWORD smpsize, uint16_t bitrate, _Bool HiRes);
void Amplifier_init();
void Timer_init(uint16_t samplerate);
void DAC_init();
void ReadFile(char* name, DWORD offset, void *dst, size_t len);

#define BUFFER_SIZE  512     //Wielko�� tymczasowego bufora na pr�bki (u�yta jest dwukrotnie wi�ksza)

uint8_t samplebuffer[2][BUFFER_SIZE];  //Bufor na odgrywane pr�bki
DWORD srcaddr;                      //Adres pr�bek d�wi�kowych
DWORD samplesize;                   //D�ugo�� odtwarzanej pr�bki w bajtach
volatile _Bool isPlaying;              //Czy aktualnie jest co� odtwarzane


#endif /* DAC_H_ */