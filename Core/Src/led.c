#include "led.h"
#include "gpio_drv.h"

/* Боковая панель SDK-1.1M: зелёный VD6 — PD13, двухцветный VD7 — PD14/PD15.
 * Все светодиоды горят при "1" на выходе. Красный и жёлтый — встречно-параллельные
 * кристаллы между PD14 и PD15: цвет горит, только если второй вывод в "0".
 * Если на стенде красный и жёлтый окажутся перепутаны — поменять местами строки ниже. */
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
		.otype = GPIO_DRV_OTYPE_PUSH_PULL,
		.speed = GPIO_DRV_SPEED_LOW,
		.pull  = GPIO_DRV_PULL_NONE,
		.level = GPIO_DRV_LEVEL_LOW, /* уровень задаётся до перевода в выход — все погашены */
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

void led_all_off(void)
{
	for (int i = 0; i < LED_COUNT; i++)
		led_set(i, false);
}
