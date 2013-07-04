/*
 * File:   delay.h
 * Author: Jared Sanson
 *
 * Delay routines and system tick
 */

////////// METHODS /////////////////////////////////////////////////////////////

extern void DelayInit();
extern void DelayMs(unsigned int ms);
extern void Delay10Us();
extern void Delay2Us();

extern volatile uint32 ticker;


