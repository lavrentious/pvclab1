#ifndef BUTTON_H
#define BUTTON_H

typedef enum {
	BUTTON_EVENT_NONE,
	BUTTON_EVENT_PRESS,
	BUTTON_EVENT_RELEASE_SHORT,
	BUTTON_EVENT_RELEASE_LONG,
} button_event_t;

void button_init(void);
button_event_t button_poll(void); /* не блокирует, вызывать из главного цикла */

#endif
