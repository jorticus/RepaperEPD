/*
 * File:   delay.c
 * Author: Jared
 *
 * Provides delay routines for the PIC32,
 * and runs a system tick timer
 * (one tick per milli second)
 *
 * If running this project on a different architecture that
 * implements things like DelayMs and DelayUs, you can
 * replace the code in the following functions with that.
 *
 * The system tick is required in EPD.c for stage timing.
 */

////////// INCLUDES ////////////////////////////////////////////////////////////

#include "common.h"
#include <peripheral/timer.h>
#include "delay.h"

////////// DEFINES /////////////////////////////////////////////////////////////

// at 80MHz, CoreTimer is 25ns per tick
// for 100ms, need 4,000,000 ticks

#define CORE_TICK_FREQUENCY 1000    //us
#define CORE_TICK_RATE SYS_FREQ/2/CORE_TICK_FREQUENCY

////////// GLOBALS /////////////////////////////////////////////////////////////

volatile uint32 ticker;

////////// CODE ////////////////////////////////////////////////////////////////

void DelayInit() {
    //mEnableIntCoreTimer();
    OpenCoreTimer(CORE_TICK_RATE);

    // set up the core timer interrupt with a prioirty of 2 and zero sub-priority
    mConfigIntCoreTimer((CT_INT_ON | CT_INT_PRIOR_2 | CT_INT_SUB_PRIOR_0));
}

// Precise ms delay using system tick
void DelayMs(unsigned int ms) {
    ticker = ms;
    while (ticker);
}

// Approximately 10us delay
void Delay10Us() {
    unsigned int i = 100;//(SYS_FREQ/4) / (10 * 1000);
    while (i--);
}

// Approximately 2us delay
void Delay2Us() {
    unsigned int i = 20;//(SYS_FREQ/4) / (10 * 1000);
    while (i--);
}

////////// INTERRUPTS //////////////////////////////////////////////////////////

void __ISR(_CORE_TIMER_VECTOR, ipl2) CoreTimerHandler(void)
{
    // clear the interrupt flag
    mCTClearIntFlag();

    LED3 = ~LED3;

    if (ticker) {
        ticker--;
    }

    // update the period
    UpdateCoreTimer(CORE_TICK_RATE);
}


