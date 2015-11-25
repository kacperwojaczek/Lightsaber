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

#define BUFFER_SIZE  512     //Wielkoœæ tymczasowego bufora na próbki (u¿yta jest dwukrotnie wiêksza)

uint8_t samplebuffer[2][BUFFER_SIZE];  //Bufor na odgrywane próbki
DWORD srcaddr;                      //Adres próbek dŸwiêkowych
DWORD samplesize;                   //D³ugoœæ odtwarzanej próbki w bajtach
volatile _Bool isPlaying;              //Czy aktualnie jest coœ odtwarzane


#endif /* DAC_H_ */