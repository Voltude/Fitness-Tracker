/*
 * display.h
 *
 *  Created on: 22/03/2022
 *      Author: vga29
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

void
initDisplay (void);

void
displayUpdate (char *str1, char *str2, int16_t num, uint8_t charLine);

void
displayUpdateWithDecimals (char *str1, char *str2, int16_t whole, int16_t decimals, int16_t zeros, uint8_t charLine);

void
displayUpdateWithDecimalsForPercentage (char *str1, char *str2, int16_t whole, int16_t decimals, int16_t zeros, uint8_t charLine);

#endif /* DISPLAY_H_ */
