/*
 * File:   epd.c
 * Author: Jared
 *
 * Created on 25 June 2013, 1:40 AM
 *
 * Ported from https://github.com/repaper/gratis/blob/master/Sketches/libraries/EPD/EPD.cpp
 * 
 *
 */

////////// INCLUDES ////////////////////////////////////////////////////////////

#include "common.h"
#include "epd.h"
#include "delay.h"

////////// DEFINES /////////////////////////////////////////////////////////////

#if DISPLAY_SIZE == EPD_2_7
    #define FILLER 1
    const uint8 channel_select[] = {0x00, 0x00, 0x00, 0x7f, 0xff, 0xfe, 0x00, 0x00};
    const uint8 gate_source[] = {0x00};

#elif DISPLAY_SIZE == EPD_2_0
    #define FILLER 1
    const uint8 channel_select[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0xe0, 0x00};
    const uint8 gate_source[] = {0x03};

#else
    #define FILLER 0
    const uint8 channel_select[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00};
    const uint8 gate_source[] = {0x03};
#endif

#define len(array) (sizeof(array)/sizeof(array[0]))

//static SpiChannel epd_spi_channel;

////////// PROTOTYPES //////////////////////////////////////////////////////////

void EpdIoInit();
void spi_putc(char c);
void spi_write(uint8* buf, unsigned int len);
void EpdPwmInit();
void EpdSetFactor(int temperature);
void EpdPwmOn();
void EpdPwmOff();

void EpdWaitBusy();
void EpdStrobe();
void EpdWriteBuffer(uint8 register_index, uint8* buffer, uint8 len);
void EpdWriteIndex(uint8 register_index);
void EpdWrite(uint8 register_index, uint8 value);

void EpdPowerOn();
void EpdPowerOff();

void EpdClear();

void EpdFrameFixed(uint8 fixed_value, EPD_stage stage);
void EpdFrameData(const uint8 *image, EPD_stage stage);
void EpdFrameFixedRepeat(uint8 fixed_value, EPD_stage stage);
void EpdFrameDataRepeat(const uint8 *image, EPD_stage stage);
void EpdLine(uint16 line, const uint8 *data, uint8 fixed_value, bool read_progmem, EPD_stage stage);

void EpdImage(uint8 *old, uint8 *new);
void EpdUpdateImage(uint8 *image);

////////// VARIABLES ///////////////////////////////////////////////////////////

long factored_stage_time = STAGE_TIME;

////////// CODE ////////////////////////////////////////////////////////////////

void EpdIoInit() {
    //epd_spi_channel = spi_channel;

    // Inputs
    EPD_TEMPERATURE_TRIS = INPUT;
    EPD_BUSY_TRIS = INPUT;
    SDI1_TRIS = INPUT;
    
    // Outputs
    EPD_PWM_TRIS = OUTPUT;
    EPD_RESET_TRIS = OUTPUT;
    EPD_PANEL_ON_TRIS = OUTPUT;
    EPD_DISCHARGE_TRIS = OUTPUT;
    EPD_BORDER_TRIS = OUTPUT;
    EPD_CS_TRIS = OUTPUT;
    //SDO1_TRIS = OUTPUT;
    //SCK1_TRIS = OUTPUT;

    EPD_RESET_PIN = 0;
    EPD_PANEL_ON_PIN = 0;
    EPD_DISCHARGE_PIN = 0;
    EPD_BORDER_PIN = 0;
    EPD_CS_PIN = 0;
    
    _TRISD1 = 0;

    EPD_CS_PIN = 1;
    FLASH_CS_PIN = 1;
}

void spi_putc(char c) {
    putcSPI1(c);

    // HNNNG. It took me a few days to work out that this is required.
    // If you do not put a delay between bytes, the display WILL NOT WORK!!!
    Delay2Us();
}
void spi_write(uint8* buf, unsigned int len) {
    unsigned int i;
    for (i=0; i<len; i++) {
        spi_putc(buf[i]);
    }
}

void EpdPwmInit() {
    // Timer2 for PWM Timebase
    T2CONbits.ON = 0;
    Nop();
    T2CONbits.TCKPS = 0b010;    //1:1 prescale
    T2CONbits.TCS = 0;          //Internal peripheral clock
    T2CONbits.T32 = 0;
    //PR2 = 0x00D0; // approx 300kHz (TCKPS = 0b000)
    PR2 = 0xF000;   // approx 300Hz (TCKPS = 0b010)

    // PWM Output
    OC2CONbits.ON = 0;
    OC2CONbits.OC32 = 0;        //16 bit
    OC2CONbits.OCTSEL = 0;      //Clock Source from Timer 2
    OC2CONbits.OCM = 0b110;     //PWM on OC1, Fault pin disabled
    OC2RS = PR2 / 2;
    OC2R = OC2RS;
}

// Turn the PWM output on
void EpdPwmOn() {
    T2CONbits.ON = 1;
    OC2CONbits.ON = 1;
}

// Turn the PWM output off
void EpdPwmOff() {
    OC2CONbits.ON = 0;
    T2CONbits.ON = 0;
    EPD_PWM_PIN = 0;
}

// Set stage_time temperature factor (in degrees)
// Default: 25
void EpdSetFactor(int temperature) {
    int factor = 7;
    if (temperature <= -10) {
        factor = 170;
    } else if (temperature <= -5) {
        factor = 120;
    } else if (temperature <= 5) {
        factor = 80;
    } else if (temperature <= 10) {
        factor = 40;
    } else if (temperature <= 15) {
        factor = 30;
    } else if (temperature <= 20) {
        factor = 20;
    } else if (temperature <= 40) {
        factor = 10;
    }

    factored_stage_time = STAGE_TIME * factor / 10;
}

// Wait for the EPD to become free
void EpdWaitBusy() {
    LED2 = 0;
    while (EPD_BUSY_PIN == 1)
        ;
    Delay10Us();
    LED2 = 1;
}

// Pulse the CS line
void EpdStrobe() {
    EPD_CS_PIN = 1;
    Delay10Us();
    EPD_CS_PIN = 0;
}

// Write an array of bytes to the EPD
void EpdWriteBuffer(uint8 register_index, uint8* buffer, uint8 len) {
    uint8 i;
    EpdStrobe();
    
    // Write Register Index
    spi_putc(0x70);
    spi_putc(register_index);

    EpdStrobe();

    // Write Data
    spi_putc(0x72);
    for (i=0; i<len; i++) {
        spi_putc(buffer[i]);
    }

    EPD_CS_PIN = 1;
}

// Write a single byte to the EPD
void EpdWrite(uint8 register_index, uint8 value) {
    EpdWriteBuffer(register_index, &value, 1);
}

// Write the index part of the packet, for when writing data bytes dynamically
void EpdWriteIndex(uint8 register_index) {
    EpdStrobe();

    // Write Register Index
    spi_putc(0x70);
    spi_putc(register_index);
    EPD_CS_PIN = 1;
}


// Turn on the EPD display
void EpdPowerOn() {

    ///// Power on COG driver /////

    // Power up sequence
    spi_putc(0x00);

    EPD_RESET_PIN = 0;
    EPD_PANEL_ON_PIN = 0;
    EPD_DISCHARGE_PIN = 0;
    EPD_BORDER_PIN = 0;
    EPD_CS_PIN = 0;

    // Start PWM
    EpdPwmOn();
    DelayMs(5);

    EPD_PANEL_ON_PIN = 1;
    DelayMs(10);

    EPD_RESET_PIN = 1;
    EPD_BORDER_PIN = 1;
    EPD_CS_PIN = 1;
    DelayMs(5);

    EPD_RESET_PIN = 0;
    DelayMs(5);

    EPD_RESET_PIN = 1;
    DelayMs(5);


    ///// Initialization Sequence /////

    // wait for COG to become ready
    EpdWaitBusy();
    //DelayMs(100);

    // Channel Select
    EpdWriteBuffer(0x01, (uint8*)channel_select, 8);//len(channel_select));

    // DC/DC frequency
    EpdWrite(0x06, 0xFF);

    // high power mode osc
    EpdWrite(0x07, 0x9D);

    // disable ADC
    EpdWrite(0x08, 0x00);

    // Vcom level
    uint8 vcom_level[] = {0xd0, 0x00};
    EpdWriteBuffer(0x09, (uint8*)vcom_level, 2);

    // gate and source voltage levels
    //EpdWriteBuffer(0x04, (uint8*)gate_source, len(gate_source));
    EpdWrite(0x04, 0x03); // Because it's 0x03 for all image sizes anyway.

    DelayMs(5);  //???

    // driver latch on
    EpdWrite(0x03, 0x01);

    // driver latch off
    EpdWrite(0x03, 0x00);

    //DelayMs(5);

    // charge pump positive voltage on
    EpdWrite(0x05, 0x01);

    // final delay before PWM off
    DelayMs(30);
    EpdPwmOff();

    // charge pump negative voltage on
    EpdWrite(0x05, 0x03);

    DelayMs(30);

    // Vcom driver on
    EpdWrite(0x05, 0x0F);

    DelayMs(30);

    // output enable to disable
    EpdWrite(0x02, 0x24);

}

// Turn the EPD display off
void EpdPowerOff() {

    EpdFrameFixed(0x55, EPD_normal); // dummy frame
    EpdLine(0x7FFFU, 0, 0x55, 0, EPD_normal); // dummy line

    DelayMs(25);

    EPD_BORDER_PIN = 0;
    DelayMs(200);

    EPD_BORDER_PIN = 1;

    // latch reset turn on
    EpdWrite(0x03, 0x01);

    // output enable off
    EpdWrite(0x02, 0x05);

    // Vcom power off
    EpdWrite(0x05, 0x0E);

    // power off negative charge pump
    EpdWrite(0x05, 0x02);

    // discharge
    EpdWrite(0x04, 0x0C);

    DelayMs(120);

    // all charge pumps off
    EpdWrite(0x05, 0x00);

    // turn of osc
    EpdWrite(0x07, 0x0D);

    // discharge internal - 1
    EpdWrite(0x04, 0x50);
    
    DelayMs(40);

    // discharge internal - 2
    EpdWrite(0x04, 0xA0);

    DelayMs(40);

    // discharge internal - 3
    EpdWrite(0x04, 0x00);

    // turn of power and all signals
    EPD_RESET_PIN = 0;
    EPD_PANEL_ON_PIN = 0;
    EPD_BORDER_PIN = 0;

    // Ensure SDO is low before disabling CS
    spi_putc(0x00);
    EPD_CS_PIN = 0;

    // discharge pulse
    EPD_DISCHARGE_PIN = 1;
    DelayMs(150);
    EPD_DISCHARGE_PIN = 0;
}

// Clear the EPD screen
void EpdClear() {
    EpdFrameFixedRepeat(0xFF, EPD_compensate);
    EpdFrameFixedRepeat(0xFF, EPD_white);
    EpdFrameFixedRepeat(0xAA, EPD_inverse);
    EpdFrameFixedRepeat(0xAA, EPD_normal);
}

// One frame of data is the number of lines * rows. For example:
// The 1.44? frame of data is 96 lines * 128 dots.
// The 2? frame of data is 96 lines * 200 dots.
// The 2.7? frame of data is 176 lines * 264 dots.

// the image is arranged by line which matches the display size
// so smallest would have 96 * 32 bytes

// Draw a single frame of a fixed value
void EpdFrameFixed(uint8 fixed_value, EPD_stage stage) {
    uint8 line;
    for (line = 0; line < LINES_PER_DISPLAY ; ++line) {
        EpdLine(line, 0, fixed_value, 0, stage);
    }
}

// Draw a single frame of image data
void EpdFrameData(const uint8 *image, EPD_stage stage){
    uint8 line;
    for (line = 0; line < LINES_PER_DISPLAY ; ++line) {
        EpdLine(line, &image[line * BYTES_PER_LINE], 0, 1, stage);
    }
}

// Draw a fixed frame repeatedly for STAGE_TIME, to ensure maximum contrast
void EpdFrameFixedRepeat(uint8 fixed_value, EPD_stage stage) {
    ticker = factored_stage_time;
    do {
        EpdFrameFixed(fixed_value, stage);
    } while (ticker);
}

// Draw a data frame repeatedly for STAGE_TIME, to ensure maximum contrast
void EpdFrameDataRepeat(const uint8 *image, EPD_stage stage) {
    ticker = factored_stage_time;
    do {
        EpdFrameData(image, stage);
    } while (ticker);
}

// Draw a single line of data
void EpdLine(uint16 line, const uint8 *data, uint8 fixed_value, bool read_progmem, EPD_stage stage) {
    // charge pump voltage levels
    EpdWriteBuffer(0x04, (uint8*)gate_source, len(gate_source));

    // Send Pixel Data
    EpdWriteIndex(0x0A);
    Delay10Us();
    EPD_CS_PIN = 0;
    spi_putc(0x72);


    // Even pixels
    uint16 b;
    for (b = BYTES_PER_LINE; b > 0; --b) {
        if (data != 0) {
            uint8 pixels = data[b - 1] & 0xAA;

            switch(stage) {
            case EPD_compensate:  // B -> W, W -> B (Current Image)
                pixels = 0xaa | ((pixels ^ 0xaa) >> 1);
                break;
            case EPD_white:       // B -> N, W -> W (Current Image)
                pixels = 0x55 + ((pixels ^ 0xaa) >> 1);
                break;
            case EPD_inverse:     // B -> N, W -> B (New Image)
                pixels = 0x55 | (pixels ^ 0xaa);
                break;
            case EPD_normal:       // B -> B, W -> W (New Image)
                pixels = 0xaa | (pixels >> 1);
                break;
            }

            spi_putc(pixels);
        } else {
            spi_putc(fixed_value);
        }

        EpdWaitBusy();
    }


    // Scan line
    for (b = 0; b < BYTES_PER_SCAN; ++b) {
        if (line / 4 == b) {
            spi_putc(0xc0 >> (2 * (line & 0x03)));
        } else {
            spi_putc(0x00);
        }

        EpdWaitBusy();
    }

    // Odd pixels
    for (b = 0; b < BYTES_PER_LINE; ++b) {
        if (0 != data) {
            uint8 pixels = data[b] & 0x55;

            switch(stage) {
            case EPD_compensate:  // B -> W, W -> B (Current Image)
                pixels = 0xaa | (pixels ^ 0x55);
                break;
            case EPD_white:       // B -> N, W -> W (Current Image)
                pixels = 0x55 + (pixels ^ 0x55);
                break;
            case EPD_inverse:     // B -> N, W -> B (New Image)
                pixels = 0x55 | ((pixels ^ 0x55) << 1);
                break;
            case EPD_normal:       // B -> B, W -> W (New Image)
                pixels = 0xaa | pixels;
                break;
            }

            uint8 p1 = (pixels >> 6) & 0x03;
            uint8 p2 = (pixels >> 4) & 0x03;
            uint8 p3 = (pixels >> 2) & 0x03;
            uint8 p4 = (pixels >> 0) & 0x03;
            pixels = (p1 << 0) | (p2 << 2) | (p3 << 4) | (p4 << 6);
            
            spi_putc(pixels);
        }  else {
            spi_putc(fixed_value);
        }
        
         EpdWaitBusy();
    }


#if FILLER
    spi_putc(0x00);
    EpdWaitBusy();
#endif

    EPD_CS_PIN = 1;

    // output data to panel
    EpdWrite(0x02, 0x2F);
}


// Draw an image (old image is optional)
void EpdImage(uint8 *old, uint8 *new) {
    // No old image specified, clear the screen first
    if (old == NULL) {
        EpdFrameFixedRepeat(0xAA, EPD_compensate);
        EpdFrameFixedRepeat(0xAA, EPD_white);

    // Old image specified, compensate using that first
    } else {
        EpdFrameDataRepeat(old, EPD_compensate);
        EpdFrameDataRepeat(old, EPD_white);
    }

    // Draw new image
    EpdFrameDataRepeat(new, EPD_inverse);
    EpdFrameDataRepeat(new, EPD_normal);
}

// EXPERIMENTAL: Only draw normal pixels over the existing screen
// NOTE: Does this cause image burn-in????
//       I've noticed some of my earlier experiments are still showing up when I do a black-white transition.
void EpdUpdateImage(uint8 *image) {
    EpdFrameDataRepeat(image, EPD_normal);
}