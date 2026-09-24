#include "led.h"
#include "main.h"

/* Боковая панель SDK-1.1M: зелёный VD6 — PD13, двухцветный VD7 — PD14/PD15.
 * Все светодиоды горят при «1» на выходе. Красный и жёлтый — встречно-параллельные
 * кристаллы между PD14 и PD15: цвет горит, только если второй вывод в «0».
 * Если на стенде красный и жёлтый окажутся перепутаны — поменять местами строки ниже. */
static const struct {
	GPIO_TypeDef *port;
	uint16_t pin;
} leds[LED_COUNT] = {
	[LED_GREEN]  = { GPIOD, GPIO_PIN_13 },
	[LED_RED]    = { GPIOD, GPIO_PIN_15 },
	[LED_YELLOW] = { GPIOD, GPIO_PIN_14 },
};

void led_set(led_id_t led, bool on)
{
	HAL_GPIO_WritePin(leds[led].port, leds[led].pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void led_toggle(led_id_t led)
{
	HAL_GPIO_TogglePin(leds[led].port, leds[led].pin);
}

void led_all_off(void)
{
	for (int i = 0; i < LED_COUNT; i++)
		led_set(i, false);
}
