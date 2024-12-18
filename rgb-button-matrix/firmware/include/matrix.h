#include "stm32f1xx_hal.h"

#define NBR_COLUMNS 6
#define NBR_ROWS 8
#define COLOR_RESOLUTION 8
#define BAM_PRESCALER 1

#define RED_DAMPENING 1.f
#define GREEN_DAMPENING 0.5f
#define BLUE_DAMPENING 0.5f

typedef struct s_color {
	uint8_t	r;
	uint8_t	g;
	uint8_t	b;
}	t_color;

extern volatile t_color			colors[NBR_ROWS][NBR_COLUMNS];
extern volatile uint8_t			button_state[NBR_ROWS];

void	set_col(uint8_t col, t_color color);
void	set_row(uint8_t row, t_color color);
void	trigger_color_wheel(void);
void	show_intensity(uint8_t r, uint8_t g, uint8_t b);
void	check_button_state(uint8_t button_reading, uint8_t current_row);

void	Error_Handler(void);
