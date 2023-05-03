// Authors: Vedang Gaikwad, Tony Luo
// Date: 19/03/2022

// Source file for led behavior

#include "driverlib/gpio.h"
#include "led.h"
#include "inc/hw_memmap.h"
#include <stdbool.h>
#include "driverlib/sysctl.h"

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3


// Initializes led
void initLED (void) {
    // Enable GPIO Port F
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Set up the specific port pin as medium strength current & pull-down config.
    // Refer to TivaWare peripheral lib user manual for set up for configuration options

    GPIOPadConfigSet(GPIO_PORTF_BASE, RED_LED, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPD);


    // Set data direction register as output

    GPIODirModeSet(GPIO_PORTF_BASE, RED_LED, GPIO_DIR_MODE_OUT);


    // Set up the specific port pin as medium strength current & pull-up config.
    // Refer to TivaWare peripheral lib user manual for set up for configuration options
//    GPIOPadConfigSet(GPIO_PORTF_BASE, SW1, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);
//
//    // Set data direction register as input
//    GPIODirModeSet(GPIO_PORTF_BASE, SW1, GPIO_DIR_MODE_IN);

    // Write a zero to the output pin 3 on port F

    GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, 0x00);
}

// Turns on LED
void ledOn (void) {
    // sets the pin that controls the led on
    GPIOPinWrite(GPIO_PORTF_BASE,  RED_LED, RED_LED);
}


// Turns off LED
void ledOff (void) {
    // sets the pin that controls the led off
    GPIOPinWrite(GPIO_PORTF_BASE,  RED_LED, 0x00);
}

