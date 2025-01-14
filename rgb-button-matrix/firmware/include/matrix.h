#include "stm32f1xx_hal.h"

#define NBR_COLUMNS 6
#define NBR_ROWS 8
#define COLOR_RESOLUTION 7
#define BAM_PRESCALER 1

#define RED_DAMPENING 1.f
#define GREEN_DAMPENING 0.5f
#define BLUE_DAMPENING 0.5f

#define LATCH_PIN								GPIO_PIN_4
#define LOAD_PIN								GPIO_PIN_3

#define SET_PIN(PORT, PINS)((PORT)->BSRR = (PINS))
#define RESET_PIN(PORT, PINS)((PORT)->BSRR = ((PINS) << 16))

#define GET_BIT_VALUE(r, g, b) (((r) << 0) | ((g) << 1) | ((b) << 2))
#define COLUMN_SHIFT(col) ((((col) / 2 ) * 8) + (((col) % 2) * 3) + 8)

typedef struct s_color {
	uint8_t	r;
	uint8_t	g;
	uint8_t	b;
}	t_color;

extern volatile t_color			colors[NBR_ROWS][NBR_COLUMNS];
extern volatile uint8_t			button_state[NBR_ROWS];
extern volatile uint32_t		color_data[NBR_ROWS][COLOR_RESOLUTION];

void	set_col(uint8_t col, t_color color);
void	set_row(uint8_t row, t_color color);
void	trigger_color_wheel(void);
void	show_intensity(uint8_t r, uint8_t g, uint8_t b);
void	check_button_state(uint8_t button_reading, uint8_t current_row);
uint8_t	spi_transmit_receive(uint32_t data);
void	button_press(uint8_t row, uint8_t col);
void	button_release(uint8_t row, uint8_t col);
void	update_colors_data(void);

void	Error_Handler(void);
