#include <matrix.h>

volatile uint8_t	button_state[NBR_ROWS] = {0};

void	button_press(uint8_t row, uint8_t col) {
	colors[row][col] = (t_color){(colors[row][col].r == 255 ? 0 : 255), 0, 0};

	// update_colors_data();
}

void	button_release(uint8_t row, uint8_t col) {
	colors[row][col] = (t_color){0};
	// update_colors_data();
}

