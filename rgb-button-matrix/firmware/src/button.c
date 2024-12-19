#include <matrix.h>

volatile uint8_t	button_state[NBR_ROWS] = {0};

void	button_press(uint8_t row, uint8_t col) {
	colors[row][col] = (t_color){255, 0, 0};
}

void	button_release(uint8_t row, uint8_t col) {
	colors[row][col] = (t_color){0};
}

// void	check_button_state(uint8_t button_reading, uint8_t current_row) {
// 	uint8_t	input = (button_state[current_row] ^ button_reading) & 0b00111111;

// 	for (uint8_t i = 0; input != 0; input >>= 1, i++) {
// 		if (input & 1) {
// 			if ((button_reading & (1 << i)))
// 				button_press(current_row, i);
// 			else
// 				button_release(current_row, i);
// 		}
// 	}
// 	button_state[current_row] = button_reading;
// }

