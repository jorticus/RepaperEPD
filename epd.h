/*
 * File:   epd.h
 * Author: Jared
 *
 * Created on 25 June 2013, 1:40 AM
 *
 * Ported from https://github.com/repaper/gratis/tree/master/Sketches/libraries/EPD/EPD.h
 * 
 * NOTE:
 * ADC reading of temperature sensor, and access to the board flash has not been implemented.
 * However the display is fully functional.
 */

#ifndef EPD_H
#define	EPD_H

////////// DEFINES /////////////////////////////////////////////////////////////

#define EPD_1_44 1
#define EPD_2_0 2
#define EPD_2_7 3

////////// CONFIGURATION ///////////////////////////////////////////////////////

#define DISPLAY_SIZE  EPD_1_44

//  Pin                 Colour      PIC32 (UBW32)   MEGA
//  1: VCC (3.3 - 10V)  Red
//  2: -                White
//  3: -                Gray
//  4: -                Purple
//  5: -                Blue
//  6: TEMPERATURE      Green       RB4             A0
//  7: SPI_CLK          Yellow      RD10            D52
//  8: BUSY             Orange      RD7             D7
//  9: PWM              Brown       RD1             D5
// 10: /RESET           Black       RD6             D6
// 11: PANEL_ON         Red         RD12            D2
// 12: DISCHARGE        White       RD4             D4
// 13: BORDER_CONTROL   Gray        RD13            D3
// 14: SPI_MISO         Purple      RC4 (optional)  D50
// 15: SPI_MOSI         Blue        RD0             D51
// 16: -                Green
// 17: -                Yellow
// 18: FLASH_CS         Orange      RD3             D9
// 19: /EPD_CS          Brown       RD2             D8
// 20: GND              Black

#define EPD_TEMPERATURE_PIN _RB4

#define EPD_BUSY_PIN        _RD7
#define EPD_RESET_PIN       _LATD6
#define EPD_PWM_PIN         _LATD1
#define EPD_DISCHARGE_PIN   _LATD4
#define EPD_BORDER_PIN      _LATD13
#define EPD_PANEL_ON_PIN    _LATD12

#define EPD_CS_PIN          _LATD2
#define FLASH_CS_PIN        _LATD3

#define EPD_TEMPERATURE_TRIS _TRISB4
#define EPD_BUSY_TRIS        _TRISD7
#define EPD_PANEL_ON_TRIS    _TRISD12
#define EPD_BORDER_TRIS      _TRISD13
#define EPD_DISCHARGE_TRIS   _TRISD4
#define EPD_PWM_TRIS         _TRISD1
#define EPD_RESET_TRIS       _TRISD6
#define EPD_CS_TRIS          _TRISD2
#define FLASH_CS_TRIS        _TRISD3
#define SDI1_TRIS _TRISC4
#define SDO1_TRIS _TRISD0
#define SCK1_TRIS _TRISD10


////////// TYPEDEFS ////////////////////////////////////////////////////////////

typedef enum {           // Image pixel -> Display pixel
	EPD_compensate,  // B -> W, W -> B (Current Image)
	EPD_white,       // B -> N, W -> W (Current Image)
	EPD_inverse,     // B -> N, W -> B (New Image)
	EPD_normal       // B -> B, W -> W (New Image)
} EPD_stage;

////////// CONSTANTS ///////////////////////////////////////////////////////////

// Device specific settings

#if DISPLAY_SIZE == EPD_2_7 // 2.7"
    #define STAGE_TIME 630 // milliseconds
    #define LINES_PER_DISPLAY 176
    #define DOTS_PER_LINE 264

#elif DISPLAY_SIZE == EPD_2_0 //2.0"
    #define STAGE_TIME 480 // milliseconds
    #define LINES_PER_DISPLAY 96
    #define DOTS_PER_LINE 200

#else // 1.44"
    #define STAGE_TIME 480 // milliseconds
    #define LINES_PER_DISPLAY 96
    #define DOTS_PER_LINE 128

#endif

#define BYTES_PER_LINE (DOTS_PER_LINE/8)
#define BYTES_PER_SCAN (LINES_PER_DISPLAY/4)


#define WIDTH   DOTS_PER_LINE
#define HEIGHT  LINES_PER_DISPLAY

////////// METHODS /////////////////////////////////////////////////////////////

extern void EpdIoInit();
extern void EpdPwmInit();
extern void EpdPowerOn();
extern void EpdSetFactor(int temperature);
extern void EpdPowerOff();

extern void EpdClear();
extern void EpdImage(uint8 *old, uint8 *new);
extern void EpdUpdateImage(uint8 *image);



#endif	/* EPD_H */

