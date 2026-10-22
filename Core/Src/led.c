#include "led.h"
#include "gpio_drv.h"

/* Красный и жёлтый - встречно-параллельные кристаллы VD7 между PD14 и PD15 */
static const struct {
	GPIO_TypeDef *port;
	uint16_t pin;
} leds[LED_COUNT] = {
	[LED_GREEN]  = { GPIOD, GPIO_DRV_PIN(13) },
	[LED_RED]    = { GPIOD, GPIO_DRV_PIN(15) },
	[LED_YELLOW] = { GPIOD, GPIO_DRV_PIN(14) },
};

void led_init(void)
{
	static const gpio_drv_config_t cfg = {
		.mode  = GPIO_DRV_MODE_OUTPUT,
		.level = GPIO_DRV_LEVEL_LOW,
	};

	for (int i = 0; i < LED_COUNT; i++)
		gpio_drv_init(leds[i].port, leds[i].pin, &cfg);
}

void led_set(led_id_t led, bool on)
{
	gpio_drv_write(leds[led].port, leds[led].pin, on);
}

void led_toggle(led_id_t led)
{
	gpio_drv_toggle(leds[led].port, leds[led].pin);
}
