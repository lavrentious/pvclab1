#ifndef GPIO_DRV_H
#define GPIO_DRV_H

/* Собственный драйвер GPIO для STM32F4 (ЛР1, часть 2, способ 1).
 * Работает с регистрами порта напрямую, из CMSIS берутся только GPIO_TypeDef,
 * адреса портов и RCC. HAL и LL для GPIO не используются. */

#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx.h"

/* Маска вывода с номером n (0..15). Маски можно объединять через "|". */
#define GPIO_DRV_PIN(n) ((uint16_t)(1U << (n)))

/* Значения совпадают с кодировкой полей в регистрах (RM0090, раздел GPIO),
 * поэтому пишутся в регистры без преобразования. */
typedef enum {
	GPIO_DRV_MODE_INPUT  = 0, /* MODER = 00 */
	GPIO_DRV_MODE_OUTPUT = 1, /* MODER = 01 */
	GPIO_DRV_MODE_AF     = 2, /* MODER = 10, альтернативная функция */
	GPIO_DRV_MODE_ANALOG = 3, /* MODER = 11 */
} gpio_drv_mode_t;

typedef enum {
	GPIO_DRV_OTYPE_PUSH_PULL  = 0, /* OTYPER = 0 */
	GPIO_DRV_OTYPE_OPEN_DRAIN = 1, /* OTYPER = 1 */
} gpio_drv_otype_t;

typedef enum {
	GPIO_DRV_SPEED_LOW       = 0, /* OSPEEDR: крутизна фронтов выхода */
	GPIO_DRV_SPEED_MEDIUM    = 1,
	GPIO_DRV_SPEED_HIGH      = 2,
	GPIO_DRV_SPEED_VERY_HIGH = 3,
} gpio_drv_speed_t;

typedef enum {
	GPIO_DRV_PULL_NONE = 0, /* PUPDR = 00 */
	GPIO_DRV_PULL_UP   = 1, /* PUPDR = 01 */
	GPIO_DRV_PULL_DOWN = 2, /* PUPDR = 10 */
} gpio_drv_pull_t;

/* Уровень, который получит выход. Применяется только в режиме OUTPUT. */
typedef enum {
	GPIO_DRV_LEVEL_KEEP = 0, /* не менять ODR */
	GPIO_DRV_LEVEL_LOW,
	GPIO_DRV_LEVEL_HIGH,
} gpio_drv_level_t;

/* Нулевая структура = вход без подтяжки. */
typedef struct {
	gpio_drv_mode_t mode;
	gpio_drv_otype_t otype;
	gpio_drv_speed_t speed;
	gpio_drv_pull_t pull;
	uint8_t af;             /* 0..15, только для GPIO_DRV_MODE_AF */
	gpio_drv_level_t level;
} gpio_drv_config_t;

/* Настраивает выводы pins порта port одинаково. Включает тактирование порта.
 * Можно вызывать повторно для смены режима: регистры пишутся в таком порядке,
 * что выводы не проходят через промежуточные некорректные состояния. */
void gpio_drv_init(GPIO_TypeDef *port, uint16_t pins, const gpio_drv_config_t *cfg);

/* Текущие настройки одного вывода (pin - маска ровно одного вывода).
 * В level возвращается значение выходного регистра ODR. */
gpio_drv_config_t gpio_drv_get_config(GPIO_TypeDef *port, uint16_t pin);

/* Уровень на одном выводе: true - высокий. */
bool gpio_drv_read(GPIO_TypeDef *port, uint16_t pin);
/* Уровни нескольких выводов одновременно: биты IDR, выбранные маской. */
uint16_t gpio_drv_read_port(GPIO_TypeDef *port, uint16_t pins);

/* Один уровень на все выводы маски. */
void gpio_drv_write(GPIO_TypeDef *port, uint16_t pins, bool high);
/* Каждому выводу маски - свой уровень из соответствующего бита values,
 * одной атомарной записью. */
void gpio_drv_write_port(GPIO_TypeDef *port, uint16_t pins, uint16_t values);
void gpio_drv_toggle(GPIO_TypeDef *port, uint16_t pins);

#endif
