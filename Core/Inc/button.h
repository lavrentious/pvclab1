#ifndef BUTTON_H
#define BUTTON_H

typedef enum {
	BUTTON_EVENT_NONE,
	BUTTON_EVENT_PRESS,         /* кнопку только что нажали */
	BUTTON_EVENT_RELEASE_SHORT, /* отпустили, нажатие было коротким */
	BUTTON_EVENT_RELEASE_LONG,  /* отпустили, нажатие было длинным */
} button_event_t;

/* Вызывать как можно чаще из главного цикла. Не блокирует.
 * Возвращает не более одного события за вызов. */
button_event_t button_poll(void);

#endif
