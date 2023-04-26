
/*
 Copyright (C)
    2011            J. Coliz <maniacbug@ymail.com>
    2015-2019       TMRh20
    2015            spaniakos <spaniakos@gmail.com>
    2015            nerdralph
    2015            zador-blood-stained
    2016            akatran
    2017-2019       Avamander <avamander@gmail.com>
    2019            IkpeohaGodson
    2021            2bndy5

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

#ifndef __RF24_CONFIG_H__
#define __RF24_CONFIG_H__

/*** USER DEFINES:    ***/
#define FAILURE_HANDLING
//#define SERIAL_DEBUG
//#define MINIMAL
//#define SPI_UART    // Requires library from https://github.com/TMRh20/Sketches/tree/master/SPI_UART
//#define SOFTSPI     // Requires library from https://github.com/greiman/DigitalIO

/**
 * User access to internally used delay time (in miliseconds) during RF24::powerUp()
 * @warning This default value compensates for all supported hardware. Only adjust this if you
 * know your radio's hardware is, in fact, genuine and reliable.
 */
#if !defined(RF24_POWERUP_DELAY)
    #define RF24_POWERUP_DELAY 5
#endif

/**********************/
#define rf24_max(a, b) (a > b ? a : b)
#define rf24_min(a, b) (a < b ? a : b)

/** @brief The default SPI speed (in Hz) */
#ifndef RF24_SPI_SPEED
    #define RF24_SPI_SPEED 10000000
#endif

#define RF24_PIN_INVALID 0xFF
#define PROGMEM
#define pgm_read_byte(addr) (*(const unsigned char*)(addr))

#endif // __RF24_CONFIG_H__