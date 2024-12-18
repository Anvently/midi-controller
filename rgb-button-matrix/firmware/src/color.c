#include <matrix.h>

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

	for (uint8_t i = i; i < NBR_COLUMNS; i++) {
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