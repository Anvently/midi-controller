#include <matrix.h>

volatile uint32_t		color_data[NBR_ROWS][COLOR_RESOLUTION] = {0};

void	update_colors_data(void) {
	t_color		color;
	uint32_t	data;

	for (uint8_t row = 0; row < NBR_ROWS; row++) {
		for (uint8_t bam_bit = 0; bam_bit < COLOR_RESOLUTION; bam_bit++) {
			data = (1 << row);
			for (uint8_t col = 0; col < NBR_COLUMNS; col++) {
				color = colors[row][col];
				data |= (GET_BIT_VALUE(
					(((int)(color.r * RED_DAMPENING) & (1 << bam_bit)) != 0),
					(((int)(color.g * GREEN_DAMPENING) & (1 << bam_bit)) != 0),
					(((int)(color.b * BLUE_DAMPENING) & (1 << bam_bit)) != 0)) << COLUMN_SHIFT(col));
			}
			color_data[row][bam_bit] = data;
		}
	}
}

// void TIM2_IRQHandler(void) {
// 	static uint8_t		current_row = 0;
// 	static uint8_t		current_bam_bit = 0;
// 	uint32_t	data;
// 	uint8_t		button_reading;

// 	data = color_data[current_row][current_bam_bit];

// 	button_reading = spi_transmit_receive(data);
// 	if (current_bam_bit == COLOR_RESOLUTION - 1) // If next period is the longest one
// 		check_button_state(button_reading, current_row);

// 	// Setup next interrupt by changing autoreload value
// 	TIM2->ARR = BAM_PERIODS[current_bam_bit];
// 	TIM2->CNT = 0;

// 	current_bam_bit = (current_bam_bit + 1) % COLOR_RESOLUTION;
// 	if (current_bam_bit == 0) // If end of cycle, select the next column
// 		current_row = (current_row + 1) % NBR_ROWS;
// 	TIM2->SR &= (uint16_t)~TIM_IT_UPDATE;
// }

// void TIM2_IRQHandler(void) {
// 	static uint8_t		current_row = 0;
// 	static uint8_t		current_bam_bit = 0;
// 	uint32_t	data = (1 << current_row);
// 	t_color		color;
// 	uint8_t		button_reading;

// 	// Update data from colors
// 	for (uint8_t i = 0; i < NBR_COLUMNS; i++) {
// 		color = colors[current_row][i];
// 		data |= (GET_BIT_VALUE(
// 			(((int)(color.r * RED_DAMPENING) & (1 << current_bam_bit)) != 0),
// 			(((int)(color.g * GREEN_DAMPENING) & (1 << current_bam_bit)) != 0),
// 			(((int)(color.b * BLUE_DAMPENING) & (1 << current_bam_bit)) != 0)) << COLUMN_SHIFT(i));
// 	}

// 	button_reading = spi_transmit_receive(data);
// 	if (current_bam_bit == COLOR_RESOLUTION - 1) // If next period is the longest one
// 		check_button_state(button_reading, current_row);

// 	// Setup next interrupt by changing autoreload value
// 	TIM2->ARR = BAM_PERIODS[current_bam_bit];
// 	TIM2->CNT = 0;

// 	current_bam_bit = (current_bam_bit + 1) % COLOR_RESOLUTION;
// 	if (current_bam_bit == 0) // If end of cycle, select the next column
// 		current_row = (current_row + 1) % NBR_ROWS;
// 	TIM2->SR &= (uint16_t)~TIM_IT_UPDATE;
// }

static t_color wheel(uint8_t pos) {
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

void	show_intensity(uint8_t r, uint8_t g, uint8_t b) {
	const uint16_t	steps = NBR_COLUMNS * NBR_ROWS;
	const uint8_t	max = (0xFF >> (8 - COLOR_RESOLUTION));
	// const uint8_t	step_value = max / steps;
	uint16_t			value = 0;

	for (uint8_t i = 0; i < NBR_COLUMNS; i++) {
		for (uint8_t j = 0; j < NBR_ROWS; j++) {
			value = (((i * NBR_ROWS) + j) * max) / steps;
			colors[j][i] =  (t_color){(r ? value : 0), (g ? value : 0), (b ? value : 0)};
		}
	} 
}

void	trigger_color_wheel(void) {
	uint8_t		pos = 0; // 0 => 255
	uint16_t	index = 0; // 0 => 48
	uint16_t		color_per_cell = 255 / 4;
	while (1) {
		colors[index / (NBR_COLUMNS * color_per_cell)][(index % (NBR_COLUMNS * color_per_cell)) / color_per_cell] = wheel(pos);
		// HAL_Delay(1);
		// colors[index / (NBR_COLUMNS * color_per_cell)][(index % (NBR_COLUMNS * color_per_cell)) / color_per_cell] = (t_color){0};
		pos++;
		index = (index + 1) % (NBR_COLUMNS * NBR_ROWS * color_per_cell);
		HAL_Delay(1);
	}

}

void	set_row(uint8_t row, t_color color) {
	for (uint8_t col = 0; col < NBR_COLUMNS; col++) {
		colors[row][col] = color;
	}
}

void	set_col(uint8_t col, t_color color) {
	for (uint8_t row = 0; row < NBR_ROWS; row++) {
		colors[row][col] = color;
	}
}