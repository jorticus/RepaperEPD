/*
 * File:   mainc..c
 * Author: Jared
 *
 * Created on 25 June 2013, 1:09 AM
 */

////////// INCLUDES ////////////////////////////////////////////////////////////

#include "common.h"
#include "delay.h"
#include "epd.h"

#include "cat_1_44.xbm"

////////// MAIN CODE ///////////////////////////////////////////////////////////

void init() {
    AD1PCFG = 0xFFFF;

    SYSTEMConfigPerformance(SYS_FREQ);

    mJTAGPortEnable(0);

    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISEbits.TRISE2 = 0;

    // Initialize ADC for temperature sensor
    // TODO

    // Initialize SPI (4MHz, data on rising edge, normal polarity)
    OpenSPI1(ENABLE_SDO_PIN | SPI_MODE8_ON | CLK_POL_ACTIVE_HIGH | SPI_CKE_ON | SPI_SMP_OFF | MASTER_ENABLE_ON | SEC_PRESCAL_4_1 | PRI_PRESCAL_4_1,
        SPI_ENABLE);

    
    EpdIoInit();
    EpdPwmInit();

    FLASH_CS_TRIS = OUTPUT;
    FLASH_CS_PIN = 1;

    DelayInit();

    // enable multi-vector interrupts
    INTEnableSystemMultiVectoredInt();
}

int main() {

    init();

    LED1 = 1;
    LED2 = 1;
    LED3 = 1;

    int temperature = 0; //Read ADC

    _TRISD1 = 0;

    while (1) {
        LED1 = 0;

        EpdPowerOn();
        EpdImage(NULL, cat_1_44_bits);
        EpdPowerOff();
        
        LED1 = 1;

        DelayMs(3000);
    }
}

////////////////////////////////////////////////////////////////////////////////