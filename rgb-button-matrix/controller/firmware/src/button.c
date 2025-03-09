#include <matrix.h>

volatile uint8_t		button_state[NBR_ROWS] = {0};
volatile t_btn_queue	event_queue;

void	button_press(uint8_t row, uint8_t col) {
	colors[row][col] = (t_color){(colors[row][col].r == 255 ? 0 : 255), 0, 0};

	// update_colors_data();
}

void	button_release(uint8_t row, uint8_t col) {
	colors[row][col] = (t_color){0};
	// update_colors_data();
}

/// @brief Push event after current circular buffer tail.
/// @param event 
/// @return ```-1``` if buffer is full, event is not added.
/// ```idx``` in the buffer were the event was pushed
int	event_push(t_button_event event) {
	int next;
	
	next = (event_queue.tail + 1) % event_queue.max;
	if (next == event_queue.head)
		return (-1);
	event_queue.data[next] = event;
	event_queue.tail = next;
	return (next);
}

t_button_event	event_pop(void) {
	t_button_event event;

	if (event_queue.head == event_queue.tail)
		return (0);
	event = event_queue.data[event_queue.head];
	event_queue.head += 1;
	event_queue.head %= event_queue.max;
	return (event);
}

void handle_events(void) {
	t_button_event		event;
	volatile t_color*	color;
	bool				update = false;

	while ((event = event_pop())) {
		update = true;
		color = &colors[EVENT_ROW(event)][EVENT_COL(event)];
		*color = (t_color){
			EVENT_TYPE(event) == BTN_PRESS_EVENT ? 255 : 0,
			color->g,
			color->b
		};
	}
	if (update)
		update_colors_data();
}
