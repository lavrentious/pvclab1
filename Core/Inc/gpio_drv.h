#ifndef GPIO_DRV_H
#define GPIO_DRV_H

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx.h"

#define GPIO_DRV_PIN(n) ((uint16_t)(1U << (n)))

/* Значения совпадают с кодировкой полей в регистрах GPIO */
typedef enum {
	GPIO_DRV_MODE_INPUT,
	GPIO_DRV_MODE_OUTPUT,
	GPIO_DRV_MODE_AF,
	GPIO_DRV_MODE_ANALOG,
} gpio_drv_mode_t;

typedef enum {
	GPIO_DRV_OTYPE_PUSH_PULL,
	GPIO_DRV_OTYPE_OPEN_DRAIN,
} gpio_drv_otype_t;

typedef enum {
	GPIO_DRV_SPEED_LOW,
	GPIO_DRV_SPEED_MEDIUM,
	GPIO_DRV_SPEED_HIGH,
	GPIO_DRV_SPEED_VERY_HIGH,
} gpio_drv_speed_t;

typedef enum {
	GPIO_DRV_PULL_NONE,
	GPIO_DRV_PULL_UP,
	GPIO_DRV_PULL_DOWN,
} gpio_drv_pull_t;

typedef enum {
	GPIO_DRV_LEVEL_KEEP,
	GPIO_DRV_LEVEL_LOW,
	GPIO_DRV_LEVEL_HIGH,
} gpio_drv_level_t;

typedef struct {
	gpio_drv_mode_t mode;
	gpio_drv_otype_t otype;
	gpio_drv_speed_t speed;
	gpio_drv_pull_t pull;
	uint8_t af;             /* только для MODE_AF */
	gpio_drv_level_t level; /* только для MODE_OUTPUT */
} gpio_drv_config_t;

/* Можно вызывать повторно: смена режима проходит без выбросов */
void gpio_drv_init(GPIO_TypeDef *port, uint16_t pins, const gpio_drv_config_t *cfg);
gpio_drv_config_t gpio_drv_get_config(GPIO_TypeDef *port, uint16_t pin);

bool gpio_drv_read(GPIO_TypeDef *port, uint16_t pin);
uint16_t gpio_drv_read_port(GPIO_TypeDef *port, uint16_t pins);
void gpio_drv_write(GPIO_TypeDef *port, uint16_t pins, bool high);
void gpio_drv_write_port(GPIO_TypeDef *port, uint16_t pins, uint16_t values);
void gpio_drv_toggle(GPIO_TypeDef *port, uint16_t pins);

#endif
