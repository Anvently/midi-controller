
#include "stm32f1xx_hal.h"
#include <memory.h>
#include <matrix.h>

#define LED_PIN                                GPIO_PIN_13
#define LED_GPIO_PORT                          GPIOC
#define LATCH_PIN								GPIO_PIN_4
#define LOAD_PIN								GPIO_PIN_3

#define OFFSET_PERIOD 46

volatile t_color			colors[NBR_ROWS][NBR_COLUMNS];
static	TIM_HandleTypeDef	htim2;
static	SPI_HandleTypeDef	hspi = {0};
static volatile uint8_t		current_row = 0;
static volatile uint8_t		current_bam_bit = 0;

static const uint32_t		BAM_PERIODS[COLOR_RESOLUTION] = {
	4UL * BAM_PRESCALER,    // Bit 0 : 2^0 * base_unit
    8UL * BAM_PRESCALER,    // Bit 1 : 2^1 * base_unit
    16UL * BAM_PRESCALER,   // Bit 2 : 2^2 * base_unit
    32UL * BAM_PRESCALER,   // Bit 3 : 2^3 * base_unit
    64UL * BAM_PRESCALER,   // Bit 4 : 2^4 * base_unit
    128UL * BAM_PRESCALER,  // Bit 5 : 2^5 * base_unit
    256UL * BAM_PRESCALER,  // Bit 6 : 2^6 * base_unit
    512UL * BAM_PRESCALER
};

#define GET_BIT_VALUE(r, g, b) (((r) << 0) | ((g) << 1) | ((b) << 2))
#define COLUMN_SHIFT(col) ((((col) / 2 ) * 8) + (((col) % 2) * 3) + 8)
// #define SET_COLOR(row, col, value) (rows[row] = ((value) & ~(0b111 << COLUMN_SHIFT(col))) | ((value) << COLUMN_SHIFT(col)))

void LED_Init();

static void SPI_Transmit(uint32_t data);
static uint8_t SPI_TransmitReceive(uint32_t data);

void	Error_Handler(void) {
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
	uint32_t	data = (1 << current_row);
	t_color		color;
	uint8_t		button_reading;

	// Update data from colors
	for (uint8_t i = 0; i < NBR_COLUMNS; i++) {
		color = colors[current_row][i];
		data |= (GET_BIT_VALUE(
			(((int)(color.r * RED_DAMPENING) & (1 << current_bam_bit)) != 0),
			(((int)(color.g * GREEN_DAMPENING) & (1 << current_bam_bit)) != 0),
			(((int)(color.b * BLUE_DAMPENING) & (1 << current_bam_bit)) != 0)) << COLUMN_SHIFT(i));
	}

	if (current_bam_bit == COLOR_RESOLUTION - 1) { // If next period is the longest one
		// Send data and read buttons
		button_reading = SPI_TransmitReceive(data);
		check_button_state(button_reading, current_row);
	} else {
		// Send data to shift register
		SPI_Transmit(data); // Select COL1
	}

	// Setup next interrupt by changing autoreload value
	TIM2->ARR = BAM_PERIODS[current_bam_bit];
	TIM2->CNT = 0;

	current_bam_bit = (current_bam_bit + 1) % COLOR_RESOLUTION;
	if (current_bam_bit == 0) // If end of cycle, select the next column
		current_row = (current_row + 1) % NBR_ROWS;
	TIM2->SR &= (uint16_t)~TIM_IT_UPDATE;
}

void Timer2_Init(void) {
	__HAL_RCC_TIM2_CLK_ENABLE();

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = (SystemCoreClock / 1000000) - 1; // Prescaler pour µs
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = BAM_PERIODS[COLOR_RESOLUTION - 1]; // Période pour 1 ms
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

	// SCK -> PA5, MISO -> PA6, MOSI -> PA7
	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;  // Mode alternatif push-pull
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LATCH_PIN | LOAD_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Disable both pin latch by default
	HAL_GPIO_WritePin(GPIOA, LATCH_PIN, 1);
	HAL_GPIO_WritePin(GPIOA, LOAD_PIN, 1);
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
	hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;  // Vitesse
	hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;  // MSB d'abord
	hspi->Init.TIMode = SPI_TIMODE_DISABLE;  // Pas de mode TI
	hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;  // Pas de CRC
	hspi->Init.CRCPolynomial = 7;

	if (HAL_SPI_Init(hspi) != HAL_OK) {
		// Gestion d'erreur
		Error_Handler();
	}
}

static uint8_t SPI_TransmitReceive(uint32_t data) {
	uint8_t	button_reading;

	//Load 74HC165 input into parallel shift register
	HAL_GPIO_WritePin(GPIOA, LOAD_PIN, 0);
	// __NOP();
	HAL_GPIO_WritePin(GPIOA, LOAD_PIN, 1);

	HAL_GPIO_WritePin(GPIOA, LATCH_PIN, 0);
	// if (HAL_SPI_Transmit(hspi, data, 2, 1) != HAL_OK) {
	// 	Error_Handler();
	// }
	if (HAL_SPI_TransmitReceive(&hspi, (uint8_t*)&data + 3, &button_reading, 1, 1) != HAL_OK) {
		Error_Handler();
	}
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

	return (button_reading);
}

static void SPI_Transmit(uint32_t data) {

	HAL_GPIO_WritePin(GPIOA, LATCH_PIN, 0);
	// if (HAL_SPI_Transmit(hspi, data, 2, 1) != HAL_OK) {
	// 	Error_Handler();
	// }
	if (HAL_SPI_Transmit(&hspi, (uint8_t*)&data + 3, 1, 1) != HAL_OK) {
		Error_Handler();
	}
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

void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	// 1. Configurer l'oscillateur principal
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE; // Utiliser HSE
	RCC_OscInitStruct.HSEState = RCC_HSE_ON; // Activer HSE
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1; // Pas de division sur HSE
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON; // Activer PLL
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE; // Source PLL = HSE
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9; // Multiplier par 9 (8 MHz * 9 = 72 MHz)
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		// Gestion d'erreur
		while (1);
	}

	// 2. Configurer les prescalers pour les bus AHB, APB1 et APB2
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
								RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; // Source SYSCLK = PLL
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; // AHB = /1
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2; // APB1 = /2 (max 36 MHz)
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; // APB2 = /1
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		// Gestion d'erreur
		while (1);
	}
}

int main(void) {
	HAL_Init();

	LED_Init();
	SPI_GPIO_Init();
	SPI1_Init(&hspi);

	SystemClock_Config();
	Timer2_Init();
	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

	// set_col(0, (t_color){255, 0, 0});
	// set_col(1, (t_color){0, 255, 0});
	// set_col(2, (t_color){255, 0, 0});
	// set_col(3, (t_color){255, 0, 0});
	// set_col(4, (t_color){0, 255, 0});
	// set_col(5, (t_color){0, 0, 255});
	
	// trigger_color_wheel();
	show_intensity(1, 0, 0);
	// set_row(0, (t_color){255, 0, 0});
	// set_row(2, (t_color){247, 0, 0});
	// set_row(3, (t_color){85, 0, 0});
	// set_row(5, (t_color){85, 0, 0});

	while (1);

}
