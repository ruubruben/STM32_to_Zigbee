/*

 ******************************************************************************
 * @file           : LED_Functions.c
 * @brief          : Adds some functions to work with the WS2812b LED's
 ******************************************************************************
 *  			Created on: Mar 16, 2022
 *      		Author: r_middelman
 *      		Owend by Triple Audio,
 *   			Will be used for the M!ka Mult!color project
 */

#ifndef APPLICATION_USER_CORE_LED_FUNCTIONS_H_
#define APPLICATION_USER_CORE_LED_FUNCTIONS_H_

#include "main.h"
#include <math.h>

//here you can set the amount of LED's
#define AMOUNT_OF_LEDS 3
#define PI 3.14159265

//function prototypes
void Set_LED(int LED_Number, uint8_t red, uint8_t green, uint8_t blue);
void Set_Brightness(uint8_t brightness);
void Send_To_LEDS(void);
void Rainbow_Effect(uint8_t brightness, int rainbow_Time);
void Fade_In_Effect(uint8_t start_Brightness, uint8_t end_Brightness, int fade_Time, uint8_t fade_up_down, uint8_t r, uint8_t g, uint8_t b);
void Fade_In_Single_LED_Effect(uint8_t start_Brightness, uint8_t end_Brightness, int fade_Time, uint8_t fade_up_down, uint8_t random, uint8_t r, uint8_t g, uint8_t b);
void Blink_Effect(int blink_Time, uint8_t brightness, uint8_t r, uint8_t g, uint8_t b);
void Turn_Off_LEDS();





#endif /* APPLICATION_USER_CORE_LED_FUNCTIONS_H_ */
