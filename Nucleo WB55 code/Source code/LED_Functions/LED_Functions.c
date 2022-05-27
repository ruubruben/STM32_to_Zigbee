/*

 ******************************************************************************
 * @file           : LED_Functions.c
 * @brief          : Adds some functions to work with the WS2812b LED's
 ******************************************************************************
 *  			Created on: Mar 16, 2022
 *      		Author: r_middelman
 */

#include "LED_Functions.h"

//import the needed handles from the main
extern TIM_HandleTypeDef htim1;

//2 2D arrays for the LED data and the LED data with brightness
uint8_t LED_Data[AMOUNT_OF_LEDS][4];
uint8_t LED_Mod[AMOUNT_OF_LEDS][4];

int data_Sent_Flag = 0;
//PWM array that is used to send all the data in the end
uint16_t pwmData[24 * AMOUNT_OF_LEDS + 50];
//effect step used by the rainbow effect
uint16_t effect_Step;

//function to set the colors of an LED
//sets the LED color by filling in the array
void Set_LED(int LED_Number, uint8_t red, uint8_t green, uint8_t blue) {
	LED_Data[LED_Number][0] = LED_Number;
	LED_Data[LED_Number][1] = green;
	LED_Data[LED_Number][2] = red;
	LED_Data[LED_Number][3] = blue;
}

//function to set the brightness
//brightness can be set from 0 to 45
//TODO: Set a function to scale to o to 255 so it will be easier to connect
void Set_Brightness(uint8_t brightness) {
	for (int i = 0; i < AMOUNT_OF_LEDS; i++)			//loop though all LED's
			{
		LED_Mod[i][0] = LED_Data[i][0];	//Set the first variable of both arrays as the same
		for (int j = 1; j < 4; j++)				//Loop from 1 to 4 for colors
				{
			float angle = 90 - brightness;	//Measure the angle of brightness
			angle = angle * PI / 180;			//Convert from degrees to radian
			LED_Mod[i][j] = (LED_Data[i][j]) / tan(angle);//Set the modified array the same as the data array but with a new RGB data
		}
	}
}

//Sends the Data for the LEDs to the LEDs
//does this by changeing the LED_Mod array to time frames and sending that to the PWM DMA
//also sets a flag to shut down and start things
void Send_To_LEDS(void) {
	uint32_t index = 0;
	uint32_t color;
	for (int i = 0; i < AMOUNT_OF_LEDS; i++)			//loop through all LED's
			{
		color =
				((LED_Mod[i][1] << 16) | (LED_Mod[i][2] << 8) | (LED_Mod[i][3]));//Set the MSB and LSB
		for (int i = 23; i >= 0; i--)	//loop through all bits to set the time
				{
			if (color & (1 << i)) {
				pwmData[index] = 60;		//set to 60 if code has to be a 1
			} else {
				pwmData[index] = 20;
			}
			index++;
		}
	}
	for (int i = 0; i < 50; i++)							//loop from 0 to 50
			{
		pwmData[index] = 0;					//set the rest of the numbers to 0
		index++;
	}
	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*) pwmData, index);//Start timer and set the data
	while (!data_Sent_Flag) {
	};											//check for flag and change flag
	data_Sent_Flag = 0;
}

//Rainbow effect
//taken from an effect generator,
//TODO write my own rainbow effect.
void Rainbow_Effect(uint8_t brightness, int rainbow_Time) {
	float factor1, factor2;
	uint16_t ind;
	for (uint16_t j = 0; j < 3; j++) {
		ind = effect_Step + j * 0.9;
		switch ((int) ((ind % 9) / 3)) {
		case 0:
			factor1 = 1.0 - ((float) (ind % 9 - 0 * 3) / 3);
			factor2 = (float) ((int) (ind - 0) % 9) / 3;
			Set_LED(j, 255 * factor1 + 0 * factor2, 0 * factor1 + 255 * factor2,
					0 * factor1 + 0 * factor2);
			Set_Brightness(brightness);
			Send_To_LEDS();
			HAL_Delay(rainbow_Time);
			break;
		case 1:
			factor1 = 1.0 - ((float) (ind % 9 - 1 * 3) / 3);
			factor2 = (float) ((int) (ind - 3) % 9) / 3;
			Set_LED(j, 0 * factor1 + 0 * factor2, 255 * factor1 + 0 * factor2,
					0 * factor1 + 255 * factor2);
			Set_Brightness(brightness);
			Send_To_LEDS();
			HAL_Delay(rainbow_Time);
			break;
		case 2:
			factor1 = 1.0 - ((float) (ind % 9 - 2 * 3) / 3);
			factor2 = (float) ((int) (ind - 6) % 9) / 3;
			Set_LED(j, 0 * factor1 + 255 * factor2, 0 * factor1 + 0 * factor2,
					255 * factor1 + 0 * factor2);
			Set_Brightness(brightness);
			Send_To_LEDS();
			HAL_Delay(rainbow_Time);
			break;
		}
	}
	if (effect_Step >= 30) {
		effect_Step = 0;
	} else
		effect_Step++;
}

//Fade in effect that adresses all the LED's at the same time
//does this by going up and down the brightness of the scale
//a lot of arguments for 1 function
void Fade_In_Effect(uint8_t start_Brightness, uint8_t end_Brightness,
		int fade_Time, uint8_t fade_up_down, uint8_t r, uint8_t g, uint8_t b) {
	uint8_t amount_of_Steps;
	amount_of_Steps = end_Brightness - start_Brightness;
	for (int i = 0; i < AMOUNT_OF_LEDS; i++) {
		LED_Data[i][0] = i;
		LED_Data[i][1] = g;
		LED_Data[i][2] = r;
		LED_Data[i][3] = b;
	}
	for (int i = 0; i < amount_of_Steps; i++) {
		Set_Brightness(start_Brightness + i);
		Send_To_LEDS();
		HAL_Delay(fade_Time);
	}
	if (fade_up_down) {
		for (int i = 0; i < amount_of_Steps; i++) {
			Set_Brightness(end_Brightness - i);
			Send_To_LEDS();
			HAL_Delay(fade_Time);
		}
	}
}

//Function that fades an LED
//can fade 1 2 or 3 LEDs at the same time.
//does this randomly or chronologicly
//works the same as the last function but now for 1 LED
void Fade_In_Single_LED_Effect(uint8_t start_Brightness, uint8_t end_Brightness, int fade_Time, uint8_t fade_up_down, uint8_t random, uint8_t r, uint8_t g, uint8_t b)
{
	uint8_t amount_of_Steps;
	amount_of_Steps = end_Brightness - start_Brightness;
	for(int i = 0; i< AMOUNT_OF_LEDS; i++)
	{
		for(int j = 0; j < AMOUNT_OF_LEDS; j++)
		{
			LED_Data[i][0] = i;
			LED_Data[i][1] = 0;
			LED_Data[i][2] = 0;
			LED_Data[i][3] = 0;
		}
		Set_Brightness(0);
		Send_To_LEDS();
		for(int j = 0; j < AMOUNT_OF_LEDS; j++)
		{
			LED_Data[j][0] = j;
			LED_Data[j][1] = 0;
			LED_Data[j][2] = 0;
			LED_Data[j][3] = 0;
		}
		LED_Data[i][0] = i;
		LED_Data[i][1] = g;
		LED_Data[i][2] = r;
		LED_Data[i][3] = b;
		for(int i = 0; i < amount_of_Steps; i++)
		{
			Set_Brightness(start_Brightness + i);
			Send_To_LEDS();
			HAL_Delay(fade_Time);
		}
		if(fade_up_down)
		{
			for(int i = 0; i< amount_of_Steps; i++)
			{
				Set_Brightness(end_Brightness - i);
				Send_To_LEDS();
				HAL_Delay(fade_Time);
			}
		}

	}
}

//Blink function.
//just turns the LED's on and of with a set color.
//Loops from 1 to 5 and if the modulo is 0 it turns the LED off
void Blink_Effect(int blink_Time, uint8_t brightness, uint8_t r, uint8_t g, uint8_t b)
{
	for (int i = 0; i < AMOUNT_OF_LEDS; i++) {
		LED_Data[i][0] = i;
		LED_Data[i][1] = g;
		LED_Data[i][2] = r;
		LED_Data[i][3] = b;
	}
	for (int i = 1; i < 5; i++) {
		if (i % 2 == 0) {
			Set_Brightness(0);
		} else
		{
			Set_Brightness(brightness);
		}
		Send_To_LEDS();
		HAL_Delay(blink_Time);
	}
}

void Turn_Off_LEDS()
{
	Set_LED(0, 0, 0, 0);
	Set_LED(1, 0, 0, 0);
	Set_LED(2, 0, 0, 0);
	Set_Brightness(0);
	Send_To_LEDS();
}

//function is called when the PWM message is finnished. sets the flag high and stops the timmer
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	data_Sent_Flag = 1;
}


