#include "button.h"
#include "gpio_drv.h"
#include "stm32f4xx_hal.h"

/* SB1: внешняя подтяжка к +3.3V, нажата = "0" */
#define BUTTON_PORT GPIOC
#define BUTTON_PIN  GPIO_DRV_PIN(15)

#define DEBOUNCE_MS   20
#define LONG_PRESS_MS 500

static bool stable_pressed;
static bool last_raw;
static uint32_t raw_changed_at;
static uint32_t pressed_at;

void button_init(void)
{
	static const gpio_drv_config_t cfg = { .mode = GPIO_DRV_MODE_INPUT };

	gpio_drv_init(BUTTON_PORT, BUTTON_PIN, &cfg);
}

button_event_t button_poll(void)
{
	uint32_t now = HAL_GetTick();
	bool raw = !gpio_drv_read(BUTTON_PORT, BUTTON_PIN);

	if (raw != last_raw) {
		last_raw = raw;
		raw_changed_at = now;
		return BUTTON_EVENT_NONE;
	}
	/* состояние принимается, только если продержалось DEBOUNCE_MS */
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
