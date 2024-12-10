
#include "stm32f1xx_hal.h"
#include <memory.h>

#define LED_PIN                                GPIO_PIN_13
#define LED_GPIO_PORT                          GPIOC
#define LATCH_PIN							GPIO_PIN_4

#define NBR_COLUMNS 2
#define NBR_ROWS 8
#define COLOR_RESOLUTION 8
#define BAM_PRESCALER 0.5

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
	uint32_t	data = (1 << current_row);
	t_color		color;

	if (htim->Instance == TIM2) {
		// Update data from colors
		for (uint8_t i = 0; i < NBR_COLUMNS; i++) {
			color = colors[current_row][i];
			data |= (GET_BIT_VALUE(
				((color.r & (1 << current_bam_bit)) != 0),
				((color.g & (1 << current_bam_bit)) != 0),
				((color.b & (1 << current_bam_bit)) != 0)) << COLUMN_SHIFT(i));
		}

		// Send data to shift register
		SPI_Transmit(data); // Select COL1
	
		// Setup next interrupt by changing autoreload value
		htim->Instance->ARR = BAM_PERIODS[current_bam_bit];
		htim->Instance->CNT = 0;

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
	hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  // Vitesse
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

t_color wheel(uint8_t pos) {
	t_color result;

	pos = 255 - pos;
	if (pos < 85) {
		return ((t_color){255 - pos * 3, 0, pos * 3});
	} else if (pos < 170) {
		pos = pos - 85;
		return ((t_color){0, pos * 3, 255 - pos * 3});
	;
	} else {
		pos = pos - 170;
		return ((t_color){pos * 3, 255 - pos * 3, 0});
	}
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

	// colors[3][1] = (t_color){255, 141, 34};
	// colors[6][0] = (t_color){0, 255, 0};
	// colors[5][0] = (t_color){0, 129, 0};
	colors[0][0] = (t_color){0, 8 * 1, 0};
	colors[0][1] = (t_color){0, 8 * 3, 0};
	colors[1][0] = (t_color){0, 8 * 5, 0};
	colors[1][1] = (t_color){0, 8 * 7, 0};
	colors[2][0] = (t_color){0, 8 * 9, 0};
	colors[2][1] = (t_color){0, 8 * 11, 0};
	colors[3][0] = (t_color){0, 8 * 13, 0};
	colors[3][1] = (t_color){0, 8 * 15, 0};
	colors[4][0] = (t_color){0, 8 * 17, 0};
	colors[4][1] = (t_color){0, 8 * 19, 0};
	colors[5][0] = (t_color){0, 8 * 21, 0};
	colors[5][1] = (t_color){0, 8 * 23, 0};
	colors[6][0] = (t_color){0, 8 * 25, 0};
	colors[6][1] = (t_color){0, 8 * 27, 0};
	colors[7][0] = (t_color){0, 8 * 29, 0};
	colors[7][1] = (t_color){0, 8 * 31, 0};


	uint8_t	pos = 0; // 0 => 255
	uint16_t	index = 0; // 0 => 48
	uint8_t color_per_cell = 255;
	while (1) {
		colors[index / (NBR_COLUMNS * color_per_cell)][(index % (NBR_COLUMNS * color_per_cell)) / color_per_cell] = wheel(pos);
		// HAL_Delay(1);
		// colors[index / (NBR_COLUMNS * color_per_cell)][(index % (NBR_COLUMNS * color_per_cell)) / color_per_cell] = (t_color){0};
		pos++;
		index = ++index % (NBR_COLUMNS * NBR_ROWS * color_per_cell);
	}
	while (1);

}
