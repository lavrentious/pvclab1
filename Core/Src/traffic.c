#include "traffic.h"
#include "led.h"
#include "button.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* Фазы в порядке следования: после последней снова первая. */
typedef enum {
	PHASE_RED,
	PHASE_GREEN,
	PHASE_GREEN_BLINK,
	PHASE_YELLOW,
	PHASE_COUNT
} phase_t;

#define RED_MS        12000
#define RED_SHORT_MS  (RED_MS / 4)
#define BLINK_HALF_MS 250 /* полпериода мигания зелёного */

static const uint32_t phase_ms[PHASE_COUNT] = {
	[PHASE_RED]         = RED_MS,
	[PHASE_GREEN]       = RED_MS / 4, /* по заданию: в 4 раза меньше красного */
	[PHASE_GREEN_BLINK] = 2000,
	[PHASE_YELLOW]      = 1500,
};

static phase_t phase;
static uint32_t phase_start;
static uint32_t phase_len;   /* длительность текущей фазы, мс */
static bool green_requested; /* кнопку нажали до красного — ближайший красный короткий */

static void enter_phase(phase_t next, uint32_t now)
{
	phase = next;
	phase_start = now;
	phase_len = phase_ms[next];
	if (next == PHASE_RED && green_requested) {
		phase_len = RED_SHORT_MS;
		green_requested = false;
	}
}

static void on_button_press(uint32_t now)
{
	if (phase == PHASE_RED) {
		/* Осталось больше 1/4 периода — сокращаем остаток до 1/4, иначе ничего. */
		uint32_t elapsed = now - phase_start;
		if (elapsed + RED_SHORT_MS < phase_len)
			phase_len = elapsed + RED_SHORT_MS;
	} else if (phase != PHASE_GREEN) {
		green_requested = true;
	}
}

static void show(uint32_t elapsed)
{
	bool blink_on = (elapsed / BLINK_HALF_MS) % 2 == 0;

	led_set(LED_RED, phase == PHASE_RED);
	led_set(LED_YELLOW, phase == PHASE_YELLOW);
	led_set(LED_GREEN, phase == PHASE_GREEN || (phase == PHASE_GREEN_BLINK && blink_on));
}

void traffic_init(void)
{
	green_requested = false;
	enter_phase(PHASE_RED, HAL_GetTick());
}

void traffic_update(void)
{
	uint32_t now = HAL_GetTick();

	if (button_poll() == BUTTON_EVENT_PRESS)
		on_button_press(now);

	if (now - phase_start >= phase_len)
		enter_phase((phase + 1) % PHASE_COUNT, now);

	show(now - phase_start);
}
