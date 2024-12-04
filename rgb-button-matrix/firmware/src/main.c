
#include "stm32f1xx_hal.h"
#include <memory.h>

#define LED_PIN                                GPIO_PIN_13
#define LED_GPIO_PORT                          GPIOC
#define LATCH_PIN							GPIO_PIN_4

#define NBR_COLUMNS 3
#define NBR_ROWS 8
#define COLOR_RESOLUTION 8

typedef struct s_color {
	uint8_t	r;
	uint8_t	g;
	uint8_t	b;
}	t_color;

static t_color				colors[NBR_ROWS][NBR_COLUMNS];
static	TIM_HandleTypeDef	htim2;
static	SPI_HandleTypeDef	hspi = {0};
static volatile uint8_t		current_row = 0;
static volatile uint8_t		current_bam_bit = 0;

static const uint16_t		BAM_PERIODS[COLOR_RESOLUTION] = {
	4,    // Bit 0 : 2^0 * base_unit
    8,    // Bit 1 : 2^1 * base_unit
    16,   // Bit 2 : 2^2 * base_unit
    32,   // Bit 3 : 2^3 * base_unit
    64,   // Bit 4 : 2^4 * base_unit
    128,  // Bit 5 : 2^5 * base_unit
    256,  // Bit 6 : 2^6 * base_unit
    512
};

#define GET_BIT_VALUE(r, g, b) (((r) << 0) | ((g) << 1) | ((b) << 2))
#define COLUMN_SHIFT(col) ((((col) / 2 ) * 8) + (((col) % 2) * 3) + 8)
// #define SET_COLOR(row, col, value) (rows[row] = ((value) & ~(0b111 << COLUMN_SHIFT(col))) | ((value) << COLUMN_SHIFT(col)))

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
	uint32_t	data = (0xFF & ~(1 << current_row));
	t_color		color;

	if (htim->Instance == TIM2) {
		// Update data from colors
		for (uint8_t i = 0; i < NBR_COLUMNS; i++) {
			color = colors[current_row][i];
			data |= GET_BIT_VALUE(color.r, color.g, color.b) << COLUMN_SHIFT(i);
		}

		// Send data to shift register
		SPI_Transmit(data); // Select COL1
	
		// Setup next interrupt by changing autoreload value
		htim->Instance->ARR = BAM_PERIODS[current_bam_bit];

		current_bam_bit = ++current_bam_bit % COLOR_RESOLUTION;
		if (current_bam_bit == 0) // If end of cycle, select the next column
			current_row = ++current_row % NBR_ROWS;
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


int main(void) {
	HAL_Init();

	LED_Init();
	SPI_GPIO_Init();
	SPI1_Init(&hspi);

	// SystemClock_Config();
	Timer2_Init();
	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

	uint32_t i = 0;
	while (1) {
		colors[i / NBR_COLUMNS][i % NBR_COLUMNS] = (t_color){((i * 1000) % 255), ((i * 100) % 255), ((i * 10000) % 255)};
		i = ++i % (NBR_COLUMNS * NBR_ROWS);
		HAL_Delay(10);
	}

	// uint8_t	color = 0; // R = 0, G = 1, B = 2
	// uint8_t	index = 0; // 0 => 7
	// uint8_t	values[] = {0b1, 0b10, 0b100, 0b011, 0b101, 0b110, 0b111};
	// while (1) {
	// 	rows[index / NBR_COLUMNS] = 0;
	// 	SET_COLOR(index / NBR_COLUMNS, index % NBR_COLUMNS, values[color]);
	// 	color = ++color % 7;
	// 	index = ++index % (NBR_COLUMNS * NBR_ROWS);
	// }

	// for (uint8_t i = 0; ; i = ++i % 8) {
		
	// }
	// while (1)
	// {
	// 	SPI_Transmit(&hspi, 0xFF & ~(0b10)); // Select COL1
	// 	HAL_Delay(200);
	// }
}
