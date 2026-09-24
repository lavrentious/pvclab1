#include "button.h"
#include "main.h"
#include <stdbool.h>

/* Кнопка SB1 на PC15: внешняя подтяжка R11 к +3.3V, нажатие замыкает на землю,
 * т.е. нажата = «0». */
#define BUTTON_PORT GPIOC
#define BUTTON_PIN  GPIO_PIN_15

#define DEBOUNCE_MS   20  /* сколько сырой сигнал должен быть стабилен */
#define LONG_PRESS_MS 500 /* граница между коротким и длинным нажатием */

static bool stable_pressed;  /* состояние после антидребезга */
static bool last_raw;
static uint32_t raw_changed_at;
static uint32_t pressed_at;

static bool read_raw(void)
{
	return HAL_GPIO_ReadPin(BUTTON_PORT, BUTTON_PIN) == GPIO_PIN_RESET;
}

button_event_t button_poll(void)
{
	uint32_t now = HAL_GetTick();
	bool raw = read_raw();

	if (raw != last_raw) {
		last_raw = raw;
		raw_changed_at = now;
		return BUTTON_EVENT_NONE;
	}
	if (raw == stable_pressed || now - raw_changed_at < DEBOUNCE_MS)
		return BUTTON_EVENT_NONE;

	stable_pressed = raw;
	if (stable_pressed) {
		pressed_at = now;
		return BUTTON_EVENT_PRESS;
	}
	return now - pressed_at >= LONG_PRESS_MS ? BUTTON_EVENT_RELEASE_LONG
	                                         : BUTTON_EVENT_RELEASE_SHORT;
}
