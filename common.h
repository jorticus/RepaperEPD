/*
 * File:   common.h
 * Author: Jared
 *
 * Created on 25 June 2013, 1:09 AM
 */

#ifndef COMMON_H
#define	COMMON_H

#include <GenericTypeDefs.h>
#include <p32xxxx.h>
#include <plib.h>

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short int uint16;
typedef signed short int int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned char bool;

#define SYS_FREQ  80000000ULL



//Status LEDs (UBW32 board)
#define LED1 LATEbits.LATE2 //White
#define LED2 LATEbits.LATE1 //Red
#define LED3 LATEbits.LATE0 //Yellow


#define INPUT 1
#define OUTPUT 0

#endif	/* COMMON_H */

