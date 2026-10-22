#include "gpio_drv.h"

/* Бит n -> бит n*width; умножение результата на v кладёт v в каждое выбранное поле */
static uint32_t spread(uint16_t pins, unsigned width)
{
	uint32_t res = 0;

	for (unsigned n = 0; n * width < 32; n++)
		if (pins & (1U << n))
			res |= 1U << (n * width);
	return res;
}

/* Одной записью, без промежуточного состояния с обнулённым полем */
static void write_fields(__IO uint32_t *reg, uint32_t mask, uint32_t value)
{
	*reg = (*reg & ~mask) | (value & mask);
}

static void clock_enable(GPIO_TypeDef *port)
{
	RCC->AHB1ENR |= 1U << (((uintptr_t)port - GPIOA_BASE) / 0x400U);
	(void)RCC->AHB1ENR; /* пауза после включения такта */
}

static void set_level(GPIO_TypeDef *port, uint16_t pins, gpio_drv_level_t level)
{
	if (level != GPIO_DRV_LEVEL_KEEP)
		gpio_drv_write(port, pins, level == GPIO_DRV_LEVEL_HIGH);
}

void gpio_drv_init(GPIO_TypeDef *port, uint16_t pins, const gpio_drv_config_t *cfg)
{
	uint32_t f2 = spread(pins, 2);
	bool output = cfg->mode == GPIO_DRV_MODE_OUTPUT;
	bool od = cfg->otype == GPIO_DRV_OTYPE_OPEN_DRAIN;

	clock_enable(port);

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	/* Против выбросов: уровень до перевода в выход (для open-drain - после типа),
	 * MODER после остальных полей, подтяжка - пока вывод управляется выходом */
	if (output || cfg->mode == GPIO_DRV_MODE_AF) {
		if (output && !od)
			set_level(port, pins, cfg->level);
		write_fields(&port->OTYPER, pins, od ? pins : 0);
		if (output && od)
			set_level(port, pins, cfg->level);
		write_fields(&port->OSPEEDR, f2 * 3U, f2 * cfg->speed);
		if (!output) {
			uint32_t lo = spread(pins & 0xFF, 4), hi = spread(pins >> 8, 4);
			write_fields(&port->AFR[0], lo * 0xFU, lo * cfg->af);
			write_fields(&port->AFR[1], hi * 0xFU, hi * cfg->af);
		}
		write_fields(&port->MODER, f2 * 3U, f2 * cfg->mode);
		write_fields(&port->PUPDR, f2 * 3U, f2 * cfg->pull);
	} else {
		write_fields(&port->PUPDR, f2 * 3U, f2 * cfg->pull);
		write_fields(&port->MODER, f2 * 3U, f2 * cfg->mode);
	}

	__set_PRIMASK(primask);
}

gpio_drv_config_t gpio_drv_get_config(GPIO_TypeDef *port, uint16_t pin)
{
	unsigned n = __builtin_ctz(pin);

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
	port->BSRR = high ? pins : (uint32_t)pins << 16;
}

void gpio_drv_write_port(GPIO_TypeDef *port, uint16_t pins, uint16_t values)
{
	port->BSRR = ((uint32_t)(pins & ~values) << 16) | (pins & values);
}

void gpio_drv_toggle(GPIO_TypeDef *port, uint16_t pins)
{
	uint32_t odr = port->ODR;

	port->BSRR = ((odr & pins) << 16) | (~odr & pins);
}
