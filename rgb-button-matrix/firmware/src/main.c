
#include "stm32f1xx_hal.h"

#define LED_PIN                                GPIO_PIN_13
#define LED_GPIO_PORT                          GPIOC
#define LATCH_PIN							GPIO_PIN_4


#define NBR_COLUMNS 4
#define NBR_ROWS 8

static	TIM_HandleTypeDef	htim2;
static	SPI_HandleTypeDef hspi = {0};
static volatile uint8_t		current_row = 0;
static uint32_t	rows[NBR_ROWS] = {0};

#define SET_COLOR(row, col, value) (rows[row] = (rows[row] & ~((uint32_t)0b111 << ((col) * 3))) | ((uint32_t)(value) << ((col) * 3)))
#define GET_COLOR_VALUE(i) (1 << i)

void LED_Init();

static void SPI_Transmit();

static void	Error_Handler(void) {
	HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET);
}

void LED_Init() {
	__HAL_RCC_GPIOC_CLK_ENABLE();
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = LED_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);
	HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
}

void TIM2_IRQHandler(void) {
	HAL_TIM_IRQHandler(&htim2); // Appelle la fonction de gestion HAL
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2) {
		current_row = ++current_row % 8;
		
		// data |= 0xFF;
		// data &= ~(1 << current_row);
		SPI_Transmit((uint32_t)(0xFF & ~(1 << current_row)) | ((rows[current_row]) << 8)); // Select COL1
	}
}

void Timer2_Init(void) {
	__HAL_RCC_TIM2_CLK_ENABLE();

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = (SystemCoreClock / 1000000) - 1; // Prescaler pour µs
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 999; // Période pour 1 ms
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
		Error_Handler();
	}

	HAL_TIM_Base_Start_IT(&htim2); // Activer le timer en mode interruption
}

void SPI_GPIO_Init(void) {
	__HAL_RCC_GPIOA_CLK_ENABLE();  // Activer l'horloge GPIO

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// SPI1 SCK -> PA5, MOSI -> PA7
	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;  // Mode alternatif push-pull
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LATCH_PIN ;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Disable both pin latch by default
	HAL_GPIO_WritePin(GPIOA, LATCH_PIN, 1);
}

void SPI1_Init(SPI_HandleTypeDef* hspi) {
	__HAL_RCC_SPI1_CLK_ENABLE();  // Activer l'horloge SPI1

	hspi->Instance = SPI1;
	hspi->Init.Mode = SPI_MODE_MASTER;  // Mode maître
	hspi->Init.Direction = SPI_DIRECTION_2LINES; 
	hspi->Init.DataSize = SPI_DATASIZE_8BIT;  // Transfert 8 bits
	hspi->Init.CLKPolarity = SPI_POLARITY_LOW;  // Polarité du signal d'horloge
	hspi->Init.CLKPhase = SPI_PHASE_1EDGE;  // Données capturées sur front montant
	hspi->Init.NSS = SPI_NSS_SOFT;  // Gestion manuelle de NSS
	hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;  // Vitesse
	hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;  // MSB d'abord
	hspi->Init.TIMode = SPI_TIMODE_DISABLE;  // Pas de mode TI
	hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Pas de CRC
	hspi->Init.CRCPolynomial = 7;

	if (HAL_SPI_Init(hspi) != HAL_OK) {
		// Gestion d'erreur
		Error_Handler();
	}
}

static void SPI_Transmit(uint32_t data) {
	HAL_GPIO_WritePin(GPIOA, LATCH_PIN, 0);
	// if (HAL_SPI_Transmit(hspi, data, 2, 1) != HAL_OK) {
	// 	Error_Handler();
	// }
	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data + 2, 1, 1) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data + 1, 1, 1) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data, 1, 1) != HAL_OK) {
		Error_Handler();
	}
	HAL_GPIO_WritePin(GPIOA, LATCH_PIN, 1); // Disable shift register selection
}

void SysTick_Handler(void) {
	HAL_IncTick();
}


int main(void) {
	HAL_Init();

	LED_Init();
	SPI_GPIO_Init();
	SPI1_Init(&hspi);

	// SystemClock_Config();
	Timer2_Init();
	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

	uint8_t	color = 0; // R = 0, G = 1, B = 2
	uint8_t	index = 0; // 0 => 7
	uint8_t	values[] = {0b1, 0b10, 0b100, 0b011, 0b101, 0b110, 0b111};
	// while (1) {
	// 	rows[index / NBR_COLUMNS] = 0;
	// 	SET_COLOR(index / NBR_COLUMNS, index % NBR_COLUMNS, (1 << (index % 3)));
	// 	HAL_Delay(200);
	// 	rows[index / NBR_COLUMNS] = 0;
	// 	color = ++color % 7;
	// 	index = ++index % (NBR_COLUMNS * NBR_ROWS * 3);
	// }

	// for (int i = 0; i < NBR_ROWS; i++) {
	// 	for (int j = 0; j < NBR_COLUMNS; j++) {
	// 		rows[i] = 0;
	// 		SET_COLOR(i, j, 0b1);
	// 		HAL_Delay(200);
	// 		rows[i] = 0;
	// 		SET_COLOR(i, j, 0b10);
	// 		HAL_Delay(200);
	// 		rows[i] = 0;
	// 		SET_COLOR(i, j, 0b100);
	// 		HAL_Delay(200);
	// 	}
	// }
	// SET_COLOR(0, 0, 0b100);
	// SET_COLOR(0, 1, 0b100);
	// SET_COLOR(0, 2, 0b100);
	rows[0] = 0b100100100;
	rows[0] = 0xFFFFFF;
	while (1);
	
	// for (uint8_t i = 0; ; i = ++i % 8) {
		
	// }
	// while (1)
	// {
	// 	SPI_Transmit(&hspi, 0xFF & ~(0b10)); // Select COL1
	// 	HAL_Delay(200);
	// }
}
