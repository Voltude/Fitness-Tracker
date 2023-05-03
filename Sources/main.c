/*   Author: vga29, tlu31
   Created on: 22/03/2022

   main program for fitness monitor that measures steps taken */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include <stdlib.h>
#include "inc/hw_types.h"
#include "inc/hw_i2c.h"
#include "driverlib/adc.h"
#include "driverlib/pin_map.h" //Needed for pin configure
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/i2c.h"
#include "driverlib/pwm.h"
#include "driverlib/interrupt.h"
#include "driverlib/debug.h"
#include "../OrbitOLED/OrbitOLEDInterface.h"
#include "utils/ustdlib.h"
#include "i2c_driver.h"
#include "acceleration.h"
#include "display.h"
#include "led.h"
#include "circBufT.h"
#include "inc/tm4c123gh6pm.h"
#include "buttons.h"
#include "utils/ustdlib.h"
#include <math.h>
#include <string.h>
#include <stdio.h>


// Systick configuration
#define SYSTICK_RATE_HZ    10
#define BUF_SIZE 10
#define SAMPLE_RATE_HZ 100
#define _USE_MATH_DEFINES

// creating buffers for each axis
static circBuf_t x_buffer;
static circBuf_t y_buffer;
static circBuf_t z_buffer;
static circBuf_t pot_buffer;

uint64_t current_time = 0;  // measures the time



// system interrupt that performs a task every tick
void SysTickIntHandler(void)
{
    current_time ++;  // increases the time count by one

}

// initialises clock
void initClock (void)
{
    // Set the clock rate to 20 MHz
    SysCtlClockSet (SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);

    // Set up the period for the SysTick timer. The SysTick timer period is
    // set as a function of the system clock.
    SysTickPeriodSet(SysCtlClockGet() / SAMPLE_RATE_HZ);

    // Register the interrupt handler
    SysTickIntRegister(SysTickIntHandler);

    // Enable interrupt and device
    SysTickIntEnable();
    SysTickEnable();
}


void
ADCIntHandler(void)
{
    uint32_t ulValue;

    //
    // Get the single sample from ADC0.  ADC_BASE is defined in
    // inc/hw_memmap.h
    ADCSequenceDataGet(ADC0_BASE, 3, &ulValue);
    //
    // Place it in the circular buffer (advancing write index)
    writeCircBuf (&pot_buffer, ulValue);
    //
    // Clean up, clearing the interrupt
    ADCIntClear(ADC0_BASE, 3);
}

void
initADC (void)
{
    //
    // The ADC0 peripheral must be enabled for configuration and use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    // Enable sample sequence 3 with a processor signal trigger.  Sequence 3
    // will do a single sample when the processor sends a signal to start the
    // conversion.
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

    //
    // Configure step 0 on sequence 3.  Sample channel 0 (ADC_CTL_CH0) in
    // single-ended mode (default) and configure the interrupt flag
    // (ADC_CTL_IE) to be set when the sample is done.  Tell the ADC logic
    // that this is the last conversion on sequence 3 (ADC_CTL_END).  Sequence
    // 3 has only one programmable step.  Sequence 1 and 2 have 4 steps, and
    // sequence 0 has 8 programmable steps.  Since we are only doing a single
    // conversion using sequence 3 we will only configure step 0.  For more
    // on the ADC sequences and steps, refer to the LM3S1968 datasheet.
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE |
                             ADC_CTL_END);

    //
    // Since sample sequence 3 is now configured, it must be enabled.
    ADCSequenceEnable(ADC0_BASE, 3);

    //
    // Register the interrupt handler
    ADCIntRegister (ADC0_BASE, 3, ADCIntHandler);

    //
    // Enable interrupts for ADC0 sequence 3 (clears any outstanding interrupts)
    ADCIntEnable(ADC0_BASE, 3);
}


// Function for getting magnitude of all axises
int32_t accelVect (int32_t xAccel, int32_t yAccel, int32_t zAccel) {
    uint32_t temp = sqrt(xAccel*xAccel + yAccel*yAccel + zAccel*zAccel);
    return temp;
}

int main (void)
{
    vector3_t acceleration_raw;

    // initialise functions for components (e.g LED, buffer, buttons ...)
    initClock ();
    initAccl ();
    initDisplay ();
    initLED();
    initButtons ();
    initADC();
    initCircBuf (&x_buffer, BUF_SIZE);
    initCircBuf (&y_buffer, BUF_SIZE);
    initCircBuf (&z_buffer, BUF_SIZE);
    initCircBuf (&pot_buffer, BUF_SIZE);
    initSwitches();

    /* initial variables for axis measurement */

    // sum of each sample from axis
    int32_t x_sum;
    int32_t y_sum;
    int32_t z_sum;
    int32_t pot_sum;

    // mean of the axis
    int32_t x_mean = 0;
    int32_t y_mean = 0;
    int32_t z_mean = 0;
    int32_t pot_mean = 0;

    // for loop counter
    uint32_t i;  // x axis
    uint32_t j;  // y axis
    uint32_t k;  // z axis
    uint32_t l;  // potentiometer

    uint8_t state = 4;  // dictates which display is shown (initial state displays raw acceleration data)

    uint8_t steps_unit = 0;   // 0 means raw and 1 means percentage
    uint8_t distance_unit = 0; // 0 means km and 1 means miles
    uint32_t current_goal = 1000;  // current step goal


    // time constants
    uint32_t acc_sample_period = 5;      // period where acceleration data is sampled
    uint32_t acc_read_period = 10;       // period where acceleration data is read
    uint32_t acc_write_last_time = 0;    // last time acceleration was written into buffer
    uint32_t acc_read_last_time = 0;     // last time acceleration was read from buffer
    uint32_t pot_read_period = 50;       // period where potentiometer data is sampled
    uint32_t pot_read_last_time = 0;     // last time potentiometer data was read from buffer
    uint32_t adc_trigger_last_time = 0;  // last time where the adc is triggered
    uint32_t adc_trigger_period = 10;    // period where the adc is triggered
    uint32_t long_push_period = 100;     // amount of time considered to be a long push
    uint32_t display_last_time = 0;      // Last time display was trigger
    uint32_t display_period = 50;        // Period for displaying
    uint32_t last_step_time = 0;         // Last time step was measured
    uint32_t step_period = 1;           // period for measuring step
    uint32_t last_update_time = 0;       // last time values to be displayed were updated
    uint32_t update_period = 30;         // updating values period

    // Variable to determine how many zeros
    // to display on OLED screen
    int16_t zeros_km = 0;
    int16_t zeros_miles = 0;
    int16_t zeros_percentage = 0;


    // Information variables
    int32_t steps_counted = 0;           // amount of steps counted
    float steps_percentage = 0;          // percentage of steps towards goal
    int16_t decimal_percentage = 0;      // whole percentage number for right of decimal point
    int16_t whole_percentage = 0;        // whole percentage number for left of decimal point
    float distance_km = 0;               // distance traveled in km
    float distance_miles = 0;            // distance traveled in miles
    int16_t whole_distance_km = 0;       // whole distance (km) number for left of decimal point
    int16_t decimal_distance_km = 0;     // whole distance (km) number for right of decimal point
    int16_t whole_distance_miles = 0;    // whole distance (miles) number for left of decimal point
    int16_t decimal_distance_miles = 0;  // whole distance (miles) number for right of decimal point
    uint32_t goal_reading;               // value to be set as new goal


    bool threshold = false;              // boolean for determining if acceleration threshold has been reached to register a step


    while (1)
    {
        // sets/resets the sum of each axis
        x_sum = 0;
        y_sum = 0;
        z_sum = 0;
        pot_sum = 0;


        updateButtons();

        // booleans that measure whether a SW1,SW2 is on or off
        bool sw1_state = readSW1();
        bool sw2_state = readSW2();


        // Updates steps and distance at 3 Hz using systick interrupt time
        if ((current_time - last_step_time) >= step_period) {
            if (sw1_state == false) {
                int32_t accel_Mag = accelVect(x_mean, y_mean, z_mean);
                if (accel_Mag > 263) {
                    threshold = true;
                } else {
                    if (threshold) {
                        steps_counted++;
                        distance_km = distance_km + 0.0009;

                    }
                    threshold = false;
                }
            }
            last_step_time = current_time;
        }


        // Updates values to be displayed at 3 Hz
        if ((current_time - last_update_time) >= update_period) {
            steps_percentage = ((float)steps_counted / (float)current_goal) * 100;
            whole_percentage = floor(steps_percentage);
            decimal_percentage = (int16_t)((steps_percentage - whole_percentage) * 100);
            if ((decimal_percentage / 10) == 0) {
                zeros_percentage = 1;
            } else {
                zeros_percentage = 0;
            }

            whole_distance_km = floor(distance_km);
            decimal_distance_km = (int16_t)((distance_km - whole_distance_km) * 10000);
            if ((decimal_distance_km / 10) == 0) {
                zeros_km = 3;
            } else if (((decimal_distance_km / 10) > 0) && ((decimal_distance_km / 10) <= 9)) {
                zeros_km = 2;
            } else if (((decimal_distance_km / 10) > 9) && ((decimal_distance_km / 10) <= 90)) {
                zeros_km = 1;
            } else {
                zeros_km = 0;
            }

            distance_miles = distance_km * 0.62;
            whole_distance_miles = floor(distance_miles);
            decimal_distance_miles = (int16_t)((distance_miles - whole_distance_miles) * 10000);
            if ((decimal_distance_miles / 10) == 0) {
                zeros_miles = 3;
            } else if (((decimal_distance_miles / 10) > 0) && ((decimal_distance_miles / 10) <= 9)) {
                zeros_miles = 2;
            } else if (((decimal_distance_miles / 10) > 9) && ((decimal_distance_miles / 10) <= 90)) {
                zeros_miles = 1;
            } else {
                zeros_miles = 0;
            }
        }


        // Checks if step goal has been reached to display message
        if (steps_counted >= current_goal) {
           state = 9;
        }

        // when up button is pushed switch displays (states 1-3)
        if (checkButton (UP) == PUSHED) {
            if (sw1_state) {
                if (state == 7) {
                    steps_counted = steps_counted + 100;
                } else if (state == 8) {
                    distance_km = distance_km + 0.09;
                }
            } else {
                if (state < 3) {
                    state += 1;
                } else if (state == 3) {
                    state = 1;
                } else if (state == 4) {
                    if (steps_unit) {
                        steps_unit = 0;
                    } else {
                        steps_unit = 1;
                    }
                } else if (state == 5) {
                    if (distance_unit) {
                        distance_unit = 0;
                    } else {
                        distance_unit = 1;
                    }
                }
            }
        }


        // when down button is pushed, set new goal step goal when in set goal state.
        // When other state, long push resets the number of steps and distance traveled to 0.
        // In test mode, decrements the step count by 500 and the distance by 0.45 km.
        if (checkButton (DOWN) ==  PUSHED) {
            if (sw1_state) {  // when in test mode
                if (state == 7) {  // If in test steps state
                    steps_counted = steps_counted - 500;  // decrements the step count by 500
                    if (steps_counted < 0) {
                        steps_counted = 0;
                    }
                } else if (state == 8) {  // If in test distance state
                    distance_km = distance_km -  0.45;  // decrements the distance by 0.45 km
                    if (distance_km < 0) {
                        distance_km = 0;
                    }
                }
            } else {
                bool short_push = true;
                uint32_t start_time = current_time;
                while (checkButton(DOWN) == NO_CHANGE) {
                    updateButtons();
                    if ((current_time - start_time) > long_push_period) { // checks if long push
                        if (state == 6) {
                            OLEDStringDraw ("SET", 0, 3);
                        } else {
                            OLEDStringDraw ("RESET", 0, 3);
                        }
                        short_push = false;
                    }
                }
                OLEDStringDraw ("                ", 0, 3);
                if (short_push) {  // If short push
                    if (state ==  6) { // If in set goal state set new step goal
                        current_goal = goal_reading;
                    }
                } else {
                    if (state == 6) {
                        current_goal = goal_reading;
                    } else if ((state == 4)) {
                        steps_counted = 0;
                    } else if (state == 5) {
                        distance_km = 0;
                        distance_miles = 0;
                    }
                }
            }
        }



        // when left button is pushed switch to reference orientation display state (4-6)
        if (checkButton (LEFT) == PUSHED) {
            if (sw1_state) {
                if (state == 7) {
                    state = 6;
                } else if (state == 6) {
                    state = 8;
                } else if (state == 8) {
                    state = 7;
                }
            } else {
                if (4 <= state && state <= 6) {
                    if (state < 6) {
                        state += 1;
                    } else {
                        state = 4;
                    }

                } else {
                    state = 4;
                }
            }
        }

        // when left button is pushed switch to reference orientation display state(6-4)
        // Same as left button but in opposite direction  (UNTESTED)
        if (checkButton (RIGHT) == PUSHED) {
            if (sw1_state) {
                if (state == 7) {
                    state = 8;
                } else if (state == 8) {
                    state = 6;
                } else if (state == 6) {
                    state = 7;
                } else if (state == 9) {
                    steps_counted = 0;
                    distance_km = 0;
                    state = 4;
                }
            } else {
                if (state == 9) {
                    steps_counted = 0;
                    distance_km = 0;
                    state = 4;
                } else if (4 <= state && state <= 6) {
                    if (state > 4) {
                        state -= 1;
                    } else {
                        state = 6;
                    }
                } else {
                    state = 6;
                }
            }
        }


        // when switch 1 is on
        if (sw1_state) {
            if ((state == 1) || (state == 2) || (state == 3) || (state == 4) || (state == 5)) {
                state = 7;
            }
        } else {
            if ((state == 7) || (state == 8)) {
                state = 4;
            }
        }



        // Using systick interrupt time to write acceleration values into buffer at 20 Hz
        if ((current_time - acc_write_last_time) >= acc_sample_period) {

            acceleration_raw = getAcclData (); // gets acceleration data from accelerometer

            // Places each axis in respective circular buffer (advancing write index)
            writeCircBuf (&x_buffer, acceleration_raw.x);
            writeCircBuf (&y_buffer, acceleration_raw.y);
            writeCircBuf (&z_buffer, acceleration_raw.z);

            acc_write_last_time = current_time;

        }

        // Using systick interrupt time to read accelerometer values from buffer at 10 Hz
        // and calculating the mean value in the buffer
        if ((current_time - acc_read_last_time) >= acc_read_period) {

            for (i = 0; i < BUF_SIZE; i++) {
                x_sum = x_sum + readCircBuf (&x_buffer);
            }
            x_mean = (2 * x_sum + BUF_SIZE) / 2 / BUF_SIZE;

            for (j = 0; j < BUF_SIZE; j++) {
                y_sum = y_sum + readCircBuf (&y_buffer);
            }
            y_mean = (2 * y_sum + BUF_SIZE) / 2 / BUF_SIZE;

            for (k = 0; k < BUF_SIZE; k++) {
                z_sum = z_sum + readCircBuf (&z_buffer);
            }
            z_mean = (2 * z_sum + BUF_SIZE) / 2 / BUF_SIZE;

            acc_read_last_time = current_time;
        }


        // Using systick interrupt time to trigger ADC and write into
        // potentiometer buffer
        if ((current_time - adc_trigger_last_time) >= adc_trigger_period) {
            ADCProcessorTrigger(ADC0_BASE, 3);

            adc_trigger_last_time = current_time;
        }


        // Using systick interrupt time to read potentiometer buffer
        if ((current_time - pot_read_last_time) >= pot_read_period) {
            for (l = 0; l < BUF_SIZE; l++) {
                pot_sum = pot_sum + readCircBuf (&pot_buffer);
            }
            pot_mean = (2 * pot_sum + BUF_SIZE) / 2 / BUF_SIZE;
            goal_reading = (pot_mean / 40) * 100;
            if (goal_reading > 10000) {
                goal_reading = 10000;
            }

            pot_read_last_time = current_time;
        }


        // Display data on screen. Which data is displayed
        // depends on state.

        if ((current_time - display_last_time) > display_period) {
            if (state == 4) {
                // Display number of steps taken in raw number or percentage of step goal
                OLEDStringDraw ("Steps taken      ", 0, 0);
                if (steps_unit) {
                    displayUpdateWithDecimalsForPercentage ("% Goal", "", whole_percentage, decimal_percentage, zeros_percentage, 1);
                    displayUpdate ("                ", "", 0, 2);
                    displayUpdate ("                ", "", 0, 3);
                } else {
                    displayUpdate ("Raw    ", "", steps_counted, 1);
                    displayUpdate ("                ", "", 0, 2);
                    displayUpdate ("                ", "", 0, 3);
                }

            } else if (state == 5) {
                // Display total distance traveled in kilometers or miles
                OLEDStringDraw ("Total Distance    ", 0, 0);
                if (distance_unit) {
                    displayUpdateWithDecimals ("Miles", "", whole_distance_miles, decimal_distance_miles, zeros_miles, 1);
                    displayUpdate ("                ", "", 0, 2);
                    displayUpdate ("                ", "", 0, 3);
                } else {
                    displayUpdateWithDecimals ("Km   ", "", whole_distance_km, decimal_distance_km, zeros_km, 1);
                    displayUpdate ("                ", "", 0, 2);
                    displayUpdate ("                ", "", 0, 3);
                }

            } else if (state == 6) {
                // Display set goal state
                OLEDStringDraw ("Setting Goal          ", 0, 0);
                displayUpdate ("The Goal:", "", current_goal, 1);
                displayUpdate ("Set Goal:", "", goal_reading, 2);
                displayUpdate ("                ", "", 0, 3);
                // Display test steps state
            } else if (state == 7) {
                OLEDStringDraw ("Test Steps      ", 0, 0);
                displayUpdate ("Raw    ", "", steps_counted, 1);
                displayUpdate ("                ", "", 0, 2);
                displayUpdate ("                ", "", 0, 3);
                // Display test steps state
            } else if (state == 8) {
                OLEDStringDraw ("Test Distance    ", 0, 0);
                displayUpdateWithDecimals ("Km   ", "", whole_distance_km, decimal_distance_km, zeros_km, 1);
                displayUpdate ("                ", "", 0, 2);
                displayUpdate ("                ", "", 0, 3);
            } else if (state == 9) {
                OLEDStringDraw ("GOAL REACHED!", 0, 0);
                OLEDStringDraw ("                ", 0, 1);
                OLEDStringDraw ("                ", 0, 2);
                OLEDStringDraw ("                ", 0, 3);
            }

        }
    }
}
