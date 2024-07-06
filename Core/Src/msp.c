/*
 * msp.c
 *
 *  Created on: Jun 30, 2024
 *      Author: daniel
 */

#include "main.h"

void HAL_MspInit(void)
{
	// Perform the low level processor specific inits here using processor specific API's provided by the Cube HAL layer in Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c
	// 1. Set up the priority grouping of the arm cortex mx processor
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	// 2. Enable the required system exceptions of the arm cortex mx processor
	SCB->SHCSR |= (0x7 << 16);

	// 3. Configure the prority for the system
	HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
	HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
	HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	// Start GPIOC Clock
	__HAL_RCC_GPIOC_CLK_ENABLE();

	// Initialize GPIO Init Structure
	GPIO_InitTypeDef gpioCInit;
	gpioCInit.Pin = GPIO_PIN_6;
	gpioCInit.Mode = GPIO_MODE_AF_PP;
	gpioCInit.Pull = GPIO_NOPULL;
	gpioCInit.Speed = GPIO_SPEED_FREQ_HIGH;
	gpioCInit.Alternate = GPIO_AF8_USART6;
	HAL_GPIO_Init(GPIOC, &gpioCInit);
}














