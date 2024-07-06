/*
 * main.c
 *
 *  Created on: Jun 30, 2024
 *      Author: daniel
 */

#include "main.h"

// Peripheral Init Functions
void B_USER_Init(void);
void UART6_Init(void);
void DHT11_DATA_Init(void);
void TIM6_Init(void);
void TIM7_Init(void);

// Peripheral Test Functions
void SystemClock_Config(void);
void B_USER_Test(void);
void UART6_Test(void);
void TIM_Test(TIM_HandleTypeDef tim, uint16_t preScaler, uint16_t arr, GPIO_TypeDef *gpio, uint16_t pinNo);
void DHT11_DATA_Test(void);

// Timer handling functions
void wait(TIM_HandleTypeDef tim);
void resetTimer(TIM_HandleTypeDef tim);
void startTimer(TIM_HandleTypeDef tim, uint16_t preScaler, uint16_t arr);

// Error Handling Functions
void Error_Handler(void);

// Define global InitTypeDef structures
RCC_OscInitTypeDef oscInit;
RCC_ClkInitTypeDef clkInit;
GPIO_InitTypeDef usrBtnInit;
GPIO_InitTypeDef dataInit;

// Define global HandleTypeDef structures
UART_HandleTypeDef huart6;
//volatile TIM_HandleTypeDef tim6;
TIM_HandleTypeDef tim6;
TIM_HandleTypeDef tim7;

char msg[100];

int main() {
	// Some user variables
	int segment = 1;
	int bitPosition = 7;
	uint8_t value = 0;
	uint8_t rh, rhDec, temp, tempDec;

	// Initialize HAL
	HAL_Init();

	// Initialize Peripherals
	B_USER_Init();
	UART6_Init();
	SystemClock_Config();
	TIM6_Init();
	TIM7_Init();
	DHT11_DATA_Init();

	// Wait two seconds for DHT11 to stabilize (only one is recommended)
	startTimer(tim6, 3999, 25000);
	wait(tim6);
	resetTimer(tim6);

	/*
	 * Initialization is done. Print some type of welcome message
	 */

	memset(msg, 0, strlen(msg));
	sprintf((char*)msg, "\nEnvironmental System Monitor\n================================\n");
	HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);


	// Send request to DHT11 to send back RH & T by holding DATA down for 18ms
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4, GPIO_PIN_RESET);
	startTimer(tim6, 3999, 300);
	wait(tim6);
	resetTimer(tim6);

	// Pull DATA line up,switch it to GPIO Input Mode, and wait for response from DHT11
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4,GPIO_PIN_SET);
	dataInit.Mode = GPIO_MODE_INPUT;
	dataInit.Pin = GPIO_PIN_4;
	dataInit.Pull = GPIO_PULLDOWN;
	dataInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOJ, &dataInit);

	// Wait for DATA line to go down
	while(HAL_GPIO_ReadPin(GPIOJ, GPIO_PIN_4) == GPIO_PIN_SET);
	// Wait for DATA line to go high
	while(HAL_GPIO_ReadPin(GPIOJ, GPIO_PIN_4) == GPIO_PIN_RESET);
	// Wait for DATA signal to go low
	while(HAL_GPIO_ReadPin(GPIOJ, GPIO_PIN_4) == GPIO_PIN_SET);

	// Start Capturing data packets from DHT11
	while(segment < 5) {
		while(bitPosition >= 0) {
			// Wait for DATA line to go high
			while(HAL_GPIO_ReadPin(GPIOJ, GPIO_PIN_4) == GPIO_PIN_RESET);

			// Start Timer
			tim7.Instance->CR1 |= (0x1 << 1);
			tim7.Instance->CR1 |= (0x1);
			tim7.Instance->CR1 &= ~(0x1 << 1);
			wait(tim7);
			resetTimer(tim7);

			// Classify as either a 1 or 0
			if(HAL_GPIO_ReadPin(GPIOJ, GPIO_PIN_4) == GPIO_PIN_SET) {
				value |= (0x1 << bitPosition);
				while(HAL_GPIO_ReadPin(GPIOJ, GPIO_PIN_4));// == GPIO_PIN_SET);
			}
			else {
				value &= ~(0x1 << bitPosition);
			}

			bitPosition = bitPosition - 1;
		}
		switch(segment) {
			case 1: rh = value;
			case 2: rhDec = value;
			case 3: temp = value;
			case 4: tempDec = value;
		}
		value = (uint8_t)0;
		segment = segment + 1;
		bitPosition = 7;
	}

	memset(msg, 0, strlen(msg));
	sprintf((char*)msg, "RH:%d\%%\n", rh);
	HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

	memset(msg, 0, strlen(msg));
	sprintf((char*)msg, "Temperature:%d C\n", temp);
	HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);


	return 0;
}

void SystemClock_Config(void) {
	oscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	oscInit.HSEState = RCC_HSE_BYPASS;
	oscInit.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	oscInit.PLL.PLLM = 19;
	oscInit.PLL.PLLN = 152;
	oscInit.PLL.PLLP = RCC_PLLP_DIV2;
	oscInit.PLL.PLLState = RCC_PLL_ON;
	if(HAL_RCC_OscConfig(&oscInit) != HAL_OK) {
		Error_Handler();
	}

	clkInit.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	clkInit.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	clkInit.AHBCLKDivider = RCC_SYSCLK_DIV2;
	clkInit.APB1CLKDivider = RCC_HCLK_DIV1;
	clkInit.APB2CLKDivider = RCC_HCLK_DIV1;
	if( HAL_RCC_ClockConfig(&clkInit, FLASH_ACR_LATENCY_1WS) != HAL_OK) {
		Error_Handler();
	}

	__HAL_RCC_HSI_DISABLE(); // Turn off the HSI to save power now

	// Reconfigure Systick now to work withe the new System Clock Frequency
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	// Renable the USART Peripheral to regenerate the baud rate
	UART6_Init();
}


void B_USER_Init(void) {
	// Start GPIOA Clock
	__HAL_RCC_GPIOA_CLK_ENABLE();
	// Inititialize GPIO Init Structure
	usrBtnInit.Mode = GPIO_MODE_INPUT;
	usrBtnInit.Pin = GPIO_PIN_0;
	usrBtnInit.Speed = GPIO_SPEED_FREQ_HIGH;
	usrBtnInit.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &usrBtnInit);
}

void B_USER_Test(void) {
	// Start GPIOJ Clock
	__HAL_RCC_GPIOJ_CLK_ENABLE();

	// Initialize Green LED
	GPIO_InitTypeDef greenLedInit;
	greenLedInit.Mode = GPIO_MODE_OUTPUT_PP;
	greenLedInit.Pin = GPIO_PIN_5;
	greenLedInit.Speed = GPIO_SPEED_LOW;
	greenLedInit.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOJ, &greenLedInit);

	// Wait until button is pressed
	while(!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0));
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_5,GPIO_PIN_SET);
	while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0));
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_5,GPIO_PIN_RESET);

	// De-initialize Green LED
	HAL_GPIO_DeInit(GPIOJ, GPIO_PIN_5);

	// Stop GPIOJ Clock
	__HAL_RCC_GPIOJ_CLK_DISABLE();
}

void UART6_Init(void) {
	// Start USART6 Clock
	__HAL_RCC_USART6_CLK_ENABLE();

	// Initialize USART6 Handle
	huart6.Instance = USART6;
	huart6.Init.BaudRate = 115200;
	huart6.Init.WordLength = UART_WORDLENGTH_8B;
	huart6.Init.StopBits = UART_STOPBITS_1;
	huart6.Init.Mode = UART_MODE_TX;
	huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	if(HAL_UART_Init(&huart6) != HAL_OK) {
		Error_Handler();
	}
}

void UART6_Test(void) {
	memset(msg, 0, strlen(msg));
	sprintf((char*)msg, "Hello from STM32");
	HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
}

void TIM6_Init(void) {
	// Start TIM6 Clock
	__HAL_RCC_TIM6_CLK_ENABLE();

	// Initialize TIM6 handler
	tim6.Instance = TIM6;
	tim6.Instance->CR1 |= (0x1 << 2);
	tim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if(HAL_TIM_Base_Init(&tim6) != HAL_OK) {
		Error_Handler();
	}
}

void TIM7_Init(void) {
	// Start TIM6 Clock
	__HAL_RCC_TIM7_CLK_ENABLE();

	// Initialize TIM6 handler
	tim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	tim7.Instance = TIM7;
	tim7.Init.Prescaler = 0;
	tim7.Init.Period = 2499;
	if(HAL_TIM_Base_Init(&tim7) != HAL_OK) {
		Error_Handler();
	}
	tim7.Instance->CR1 |= (0x1 << 2);

}

void TIM_Test(TIM_HandleTypeDef tim, uint16_t preScaler, uint16_t arr, GPIO_TypeDef *gpio, uint16_t pinNo) {
	// Initialize Output Pin for viewing on oscilloscope
	GPIO_InitTypeDef tim6OutputInit;
	tim6OutputInit.Mode = GPIO_MODE_OUTPUT_PP;
	tim6OutputInit.Pin = pinNo;
	tim6OutputInit.Pull = GPIO_NOPULL;
	tim6OutputInit.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(gpio, &tim6OutputInit);
	while(1) {
		startTimer(tim, preScaler, arr);
		wait(tim);
		HAL_GPIO_TogglePin(gpio, pinNo);
		resetTimer(tim);
	}

	// De-initialize Output Pin
	HAL_GPIO_DeInit(gpio, pinNo);
}

void DHT11_DATA_Init(void) {
	// Start GPIOJ Clock
	__HAL_RCC_GPIOJ_CLK_ENABLE();

	// Inititialize GPIO Init Structure
	dataInit.Mode = GPIO_MODE_OUTPUT_PP;
	dataInit.Pin = GPIO_PIN_4;
	dataInit.Speed = GPIO_SPEED_FREQ_HIGH;
	dataInit.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOJ, &dataInit);
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4,GPIO_PIN_SET);
}

void DHT11_DATA_Test(void) {
	// Send request to DHT11 to send back RH & T by holding DATA down for 18ms
	tim6.Instance->ARR = 304-1;
	tim6.Instance->PSC = 3999;

	//while(!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0));
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_1,GPIO_PIN_RESET);
	HAL_TIM_Base_Start(&tim6);
	while(!(tim6.Instance->SR & (0x0001)));
	tim6.Instance->SR = 0;
	HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_1,GPIO_PIN_SET);

	dataInit.Mode = GPIO_MODE_INPUT;
	dataInit.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOJ, &dataInit);

	while(1);
}

void startTimer(TIM_HandleTypeDef tim, uint16_t preScaler, uint16_t arr) {
	tim.Instance->ARR = arr;
	tim.Instance->PSC = preScaler;
	tim.Instance->EGR = 1;
	tim.Instance->SR = 0;
	HAL_TIM_Base_Start(&tim);
}

void wait(TIM_HandleTypeDef tim) {
	while(!(tim.Instance->SR & (0x0001)));
}

void resetTimer(TIM_HandleTypeDef tim) {
	tim.Instance->CR1 &= ~(0x1);
	tim.Instance->CNT &= ~(0xFFFF);
	tim.Instance->SR = 0;
}


void Error_Handler(void) {

}
