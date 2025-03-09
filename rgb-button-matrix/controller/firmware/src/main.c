
#include "stm32f0xx_hal.h"
#include <memory.h>
#include <matrix.h>

#define OFFSET_PERIOD 5 // Measured duration of interrupt procedure and spi transmission
#define EXTRA_PERIOD 0 // Measured duration of button matrix routine + potentiometer reading during the last BAM period

volatile t_color			colors[NBR_ROWS][NBR_COLUMNS];
TIM_HandleTypeDef	hTIM14;

const uint32_t		BAM_PERIODS[COLOR_RESOLUTION] = {
	// 4UL * BAM_PRESCALER - OFFSET_PERIOD,    // Bit 0 : 2^0 * base_unit
    // 8UL * BAM_PRESCALER - OFFSET_PERIOD,    // Bit 1 : 2^1 * base_unit
    16UL * BAM_PRESCALER - OFFSET_PERIOD,   // Bit 2 : 2^2 * base_unit
    32UL * BAM_PRESCALER - OFFSET_PERIOD,   // Bit 3 : 2^3 * base_unit
    64UL * BAM_PRESCALER - OFFSET_PERIOD,   // Bit 4 : 2^4 * base_unit
    128UL * BAM_PRESCALER - OFFSET_PERIOD,  // Bit 5 : 2^5 * base_unit
    256UL * BAM_PRESCALER - OFFSET_PERIOD,  // Bit 6 : 2^6 * base_unit
    512UL * BAM_PRESCALER - OFFSET_PERIOD - EXTRA_PERIOD // 
};


// #define SET_COLOR(row, col, value) (rows[row] = ((value) & ~(0b111 << COLUMN_SHIFT(col))) | ((value) << COLUMN_SHIFT(col)))

void LED_Init();
void Timer2_Init(void);
void SystemClock_Config(void);
void SPI1_Init(void);
void SPI2_Init(void);
void SPI_GPIO_Init(void);

void	Error_Handler(void) {
	HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET);
}

void TIM14_IRQHandler(void) {
	static uint8_t		current_row = 0;
	static uint8_t		current_bam_bit = 0;
	uint32_t	data;
	uint8_t		button_reading;

	data = color_data[current_row][current_bam_bit];

	//Load 74HC165 input into parallel shift register
	RESET_PIN(GPIOA, LOAD_PIN);
	SET_PIN(GPIOA, LOAD_PIN);

	RESET_PIN(GPIOA, LATCH_PIN);
	SPI1->CR1 |= (SPI_CR1_MSTR | SPI_CR1_SPE); //Enable master mode and SPI
	SPI1->DR = (data >> 16); //Write 16 first MSB
	while ((SPI1->SR & SPI_SR_TXE) == 0); //While tx_buffer is not empty
	while ((SPI1->SR & SPI_SR_RXNE) == 0); // While rx_buffer is not ready
	button_reading = (uint8_t)(SPI1->DR >> 8); //Read received data
	SPI1->DR = (data & 0x0000FFFF); //Send 16 LSB
	while ((SPI1->SR & SPI_SR_TXE) == 0); //While tx_buffer is not empty
	while ((SPI1->SR & SPI_SR_RXNE) == 0); // While rx_buffer is not ready
	(void)SPI1->DR;
	SET_PIN(GPIOA, LATCH_PIN);

	if (current_bam_bit == COLOR_RESOLUTION - 1) { // If next period is the longest one => 512us
		uint8_t	input = (button_state[current_row] ^ button_reading) & 0b00111111;

		for (uint8_t i = 0; input != 0; input >>= 1, i++) {
			if (input & 1) {
					event_push(NEW_EVENT(current_row, //row
						(i % 2 ? i - 1 : i +1), //col (inverted)
						button_reading & (1 << i))); // 1=> press / 0=> release
			}
		}
		button_state[current_row] = button_reading;
		handle_events();
		read_adc();
	}

	if (current_bam_bit == COLOR_RESOLUTION - 2) { //If next period is 2nd longest one => 256us
		update_colors_data(); //Update colors data
		// Init DMA transfer for buttons and pot readings DMA1_Channel2
	}

	// Setup next interrupt by changing autoreload value
	TIM14->ARR = BAM_PERIODS[current_bam_bit];
	TIM14->CNT = 0;

	current_bam_bit = (current_bam_bit + 1) % COLOR_RESOLUTION;
	if (current_bam_bit == 0) // If end of cycle, select the next column
		current_row = (current_row + 1) % NBR_ROWS;
	TIM14->SR &= (uint16_t)~TIM_IT_UPDATE;
}

void SysTick_Handler(void) {
	HAL_IncTick();
}

int main(void) {
	HAL_Init();

	LED_Init();
	SPI_GPIO_Init();
	SPI1_Init();
	SPI2_Init();

	SystemClock_Config();
	Timer2_Init();
	HAL_NVIC_SetPriority(TIM14_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM14_IRQn);

	// set_col(0, (t_color){255, 0, 0});
	// set_col(1, (t_color){0, 255, 0});
	// set_col(2, (t_color){255, 0, 0});
	// set_col(3, (t_color){255, 0, 0});
	// set_col(4, (t_color){0, 255, 0});
	// set_col(5, (t_color){0, 0, 255});
	
	trigger_color_wheel();
	// show_intensity(1, 1, 1);
	// set_col(0, (t_color){255, 255, 0});
	// set_row(0, (t_color){255, 0, 0});
	// set_row(2, (t_color){247, 0, 0});
	// set_row(3, (t_color){85, 0, 0});
	// set_row(5, (t_color){85, 0, 0});

	while (1);

}
