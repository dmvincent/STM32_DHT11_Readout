/*
 * it.c
 *
 *  Created on: Jun 30, 2024
 *      Author: daniel
 */

#include "main.h"

void SysTick_Handler(void) {
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
}

