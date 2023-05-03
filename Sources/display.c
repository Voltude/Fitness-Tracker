#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "driverlib/pin_map.h" //Needed for pin configure
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "../OrbitOLED/OrbitOLEDInterface.h"
#include "utils/ustdlib.h"
#include "display.h"

/*
 * display.c
 *
 *  Created on: 22/03/2022
 *      Author: vga29
 */

/*********************************************************
 * initDisplay
 *********************************************************/

void initDisplay (void);
void displayUpdate (char *str1, char *str2, int16_t num, uint8_t charLine);

void
initDisplay (void)
{
    // Initialise the Orbit OLED display
    OLEDInitialise ();
}

//*****************************************************************************
// Function to display a changing message on the display.
// The display has 4 rows of 16 characters, with 0, 0 at top left.
//*****************************************************************************

void
displayUpdate (char *str1, char *str2, int16_t num, uint8_t charLine)
{
    char text_buffer[17];           //Display fits 16 characters wide.

    // "Undraw" the previous contents of the line to be updated.
    OLEDStringDraw ("                ", 0, charLine);
    // Form a new string for the line.  The maximum width specified for the
    //  number field ensures it is displayed right justified.
    usnprintf(text_buffer, sizeof(text_buffer), "%s %s %3d", str1, str2, num);
    // Update line on display.
    OLEDStringDraw (text_buffer, 0, charLine);
}

void
displayUpdateWithDecimals (char *str1, char *str2, int16_t whole, int16_t decimals, int16_t zeros, uint8_t charLine)
{
    char text_buffer[17];           //Display fits 16 characters wide.

    // "Undraw" the previous contents of the line to be updated.
    OLEDStringDraw ("                ", 0, charLine);
    // Form a new string for the line.  The maximum width specified for the
    //  number field ensures it is displayed right justified.
    if (zeros == 3) {
        usnprintf(text_buffer, sizeof(text_buffer), "%s%s %4d.000%d", str1, str2, whole, decimals);
    } else if (zeros == 2) {
        usnprintf(text_buffer, sizeof(text_buffer), "%s%s %4d.00%2d", str1, str2, whole, decimals);
    } else if (zeros == 1) {
        usnprintf(text_buffer, sizeof(text_buffer), "%s%s %4d.0%3d", str1, str2, whole, decimals);
    } else {
        usnprintf(text_buffer, sizeof(text_buffer), "%s%s %4d.%4d", str1, str2, whole, decimals);
    }

    // Update line on display.
    OLEDStringDraw (text_buffer, 0, charLine);
}

void
displayUpdateWithDecimalsForPercentage (char *str1, char *str2, int16_t whole, int16_t decimals, int16_t zeros, uint8_t charLine)
{
    char text_buffer[17];           //Display fits 16 characters wide.

    // "Undraw" the previous contents of the line to be updated.
    OLEDStringDraw ("                ", 0, charLine);
    // Form a new string for the line.  The maximum width specified for the
    //  number field ensures it is displayed right justified.
    if (zeros == 1) {
        usnprintf(text_buffer, sizeof(text_buffer), "%s%s %3d.0%d", str1, str2, whole, decimals);
    } else {
        usnprintf(text_buffer, sizeof(text_buffer), "%s%s %3d.%2d", str1, str2, whole, decimals);
    }

    // Update line on display.
    OLEDStringDraw (text_buffer, 0, charLine);
}
