#include "gpio_drv.h"

#define GPIO_PORT_STEP 0x400U /* расстояние между блоками регистров соседних портов */

/* Разносит биты маски выводов по полям шириной width: бит n -> бит n*width.
 * Умножение результата на значение v < 2^width кладёт v в каждое выбранное
 * поле: поля не пересекаются, переносов в соседние нет. */
static uint32_t spread(uint16_t pins, unsigned width)
{
	uint32_t res = 0;

	for (unsigned n = 0; n < 16 && n * width < 32; n++)
		if (pins & (1U << n))
			res |= 1U << (n * width);
	return res;
}

/* Меняет поля регистра одной записью. Вариант reg &= ~mask; reg |= value
 * даёт две записи, и между ними поле на мгновение равно нулю - например,
 * MODER = 00 (вход), что на выходе выглядит как выброс. */
static void write_fields(__IO uint32_t *reg, uint32_t mask, uint32_t value)
{
	*reg = (*reg & ~mask) | (value & mask);
}

static void clock_enable(GPIO_TypeDef *port)
{
	/* Порты идут подряд с шагом 0x400 начиная с GPIOA, биты GPIOxEN в AHB1ENR -
	 * в том же порядке начиная с бита 0. */
	uint32_t index = ((uintptr_t)port - GPIOA_BASE) / GPIO_PORT_STEP;

	RCC->AHB1ENR |= 1U << index;
	(void)RCC->AHB1ENR; /* после включения такта нужна пауза перед доступом к порту */
}

static void set_level(GPIO_TypeDef *port, uint16_t pins, gpio_drv_level_t level)
{
	if (level == GPIO_DRV_LEVEL_HIGH)
		port->BSRR = pins;
	else if (level == GPIO_DRV_LEVEL_LOW)
		port->BSRR = (uint32_t)pins << 16;
}

void gpio_drv_init(GPIO_TypeDef *port, uint16_t pins, const gpio_drv_config_t *cfg)
{
	uint32_t f2 = spread(pins, 2);  /* поля по 2 бита: MODER, OSPEEDR, PUPDR */
	uint32_t af_lo = spread(pins & 0xFF, 4); /* поля по 4 бита: AFR[0] - выводы 0..7 */
	uint32_t af_hi = spread(pins >> 8, 4);   /* AFR[1] - выводы 8..15 */
	uint32_t otype = cfg->otype == GPIO_DRV_OTYPE_OPEN_DRAIN ? pins : 0;

	clock_enable(port);

	/* Чтение-изменение-запись регистров не атомарна: на время настройки
	 * запрещаем прерывания, сохраняя прежнее состояние флага. */
	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	if (cfg->mode == GPIO_DRV_MODE_OUTPUT || cfg->mode == GPIO_DRV_MODE_AF) {
		/* Вывод будет управляемым. Порядок записей:
		 * 1) тип выхода и уровень: для push-pull сначала уровень, для open-drain
		 *    сначала тип - так уже работающий выход не выдаст активную "1",
		 *    которой нет ни в старой, ни в новой настройке;
		 * 2) скорость и номер альтернативной функции - пока вывод в старом режиме;
		 * 3) MODER - вывод начинает выдавать уже готовое значение;
		 * 4) подтяжка - уже после того, как вывод управляется, иначе вход
		 *    успел бы переключиться на новую подтяжку. */
		if (cfg->otype == GPIO_DRV_OTYPE_OPEN_DRAIN) {
			write_fields(&port->OTYPER, pins, otype);
			if (cfg->mode == GPIO_DRV_MODE_OUTPUT)
				set_level(port, pins, cfg->level);
		} else {
			if (cfg->mode == GPIO_DRV_MODE_OUTPUT)
				set_level(port, pins, cfg->level);
			write_fields(&port->OTYPER, pins, otype);
		}
		write_fields(&port->OSPEEDR, f2 * 3U, f2 * cfg->speed);
		if (cfg->mode == GPIO_DRV_MODE_AF) {
			write_fields(&port->AFR[0], af_lo * 0xFU, af_lo * cfg->af);
			write_fields(&port->AFR[1], af_hi * 0xFU, af_hi * cfg->af);
		}
		write_fields(&port->MODER, f2 * 3U, f2 * cfg->mode);
		write_fields(&port->PUPDR, f2 * 3U, f2 * cfg->pull);
	} else {
		/* Вывод перестаёт управляться: новая подтяжка ставится, пока он ещё
		 * выход (на выходе её не видно), затем MODER его отпускает. */
		write_fields(&port->PUPDR, f2 * 3U, f2 * cfg->pull);
		write_fields(&port->MODER, f2 * 3U, f2 * cfg->mode);
	}

	__set_PRIMASK(primask);
}

gpio_drv_config_t gpio_drv_get_config(GPIO_TypeDef *port, uint16_t pin)
{
	unsigned n = __builtin_ctz(pin); /* номер вывода по маске */

	return (gpio_drv_config_t) {
		.mode  = (port->MODER >> (2 * n)) & 3U,
		.otype = (port->OTYPER >> n) & 1U,
		.speed = (port->OSPEEDR >> (2 * n)) & 3U,
		.pull  = (port->PUPDR >> (2 * n)) & 3U,
		.af    = (port->AFR[n / 8] >> (4 * (n % 8))) & 0xFU,
		.level = (port->ODR & pin) ? GPIO_DRV_LEVEL_HIGH : GPIO_DRV_LEVEL_LOW,
	};
}

bool gpio_drv_read(GPIO_TypeDef *port, uint16_t pin)
{
	return (port->IDR & pin) != 0;
}

uint16_t gpio_drv_read_port(GPIO_TypeDef *port, uint16_t pins)
{
	return port->IDR & pins;
}

void gpio_drv_write(GPIO_TypeDef *port, uint16_t pins, bool high)
{
	set_level(port, pins, high ? GPIO_DRV_LEVEL_HIGH : GPIO_DRV_LEVEL_LOW);
}

void gpio_drv_write_port(GPIO_TypeDef *port, uint16_t pins, uint16_t values)
{
	/* Младшая половина BSRR устанавливает выводы в "1", старшая - сбрасывает в "0". */
	port->BSRR = ((uint32_t)(pins & ~values) << 16) | (pins & values);
}

void gpio_drv_toggle(GPIO_TypeDef *port, uint16_t pins)
{
	uint32_t odr = port->ODR;

	port->BSRR = ((odr & pins) << 16) | (~odr & pins);
}
