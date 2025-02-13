/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_API_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_API_H_

#include <zephyr/kernel.h>

#define DT_DRV_COMPAT microcrystal_rv8803

/* Calendar Registers */
#define RV8803_REGISTER_SECONDS 0x00
#define RV8803_REGISTER_MINUTES 0x01
#define RV8803_REGISTER_HOURS   0x02
#define RV8803_REGISTER_WEEKDAY 0x03
#define RV8803_REGISTER_DATE    0x04
#define RV8803_REGISTER_MONTH   0x05
#define RV8803_REGISTER_YEAR    0x06

/* Control Registers */
#define RV8803_REGISTER_EXTENSION 0x0D
#define RV8803_REGISTER_FLAG      0x0E
#define RV8803_REGISTER_CONTROL   0x0F

// clang-format off
#define RV8803_CONTROL_MASK_UPDATE   (0x01 << 5)
#define RV8803_CONTROL_MASK_COUNTER  (0x01 << 4)
#define RV8803_CONTROL_MASK_ALARM    (0x01 << 3)
#define RV8803_CONTROL_MASK_EXTERN   (0x01 << 2)
#define RV8803_CONTROL_MASK_INTERRUPT ( \
				RV8803_CONTROL_MASK_UPDATE | \
				RV8803_CONTROL_MASK_COUNTER | \
				RV8803_CONTROL_MASK_ALARM | \
				RV8803_CONTROL_MASK_EXTERN)
#define RV8803_DISABLE_INTERRUPT     (0x00)

#define RV8803_EXTENSION_MASK_WADA    (0x01 << 6)
#define RV8803_EXTENSION_MASK_UPDATE  (0x01 << 5)
#define RV8803_EXTENSION_MASK_COUNTER (0x01 << 4)

#define RV8803_FLAG_MASK_ALARM   (0x01 << 3)
#define RV8803_FLAG_MASK_COUNTER (0x01 << 4)
#define RV8803_FLAG_MASK_UPDATE  (0x01 << 5)

#define RV8803_DT_CNT_BOOL(idx, prop) COND_CODE_1(DT_PROP(idx, prop), (1 +), (0 +))
#define RV8803_DT_CHILD_CNT_BOOL_STATUS_OKAY(inst, prop) DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(inst, RV8803_DT_CNT_BOOL, prop) +
#define RV8803_DT_ANY_INST_CHILD_CNT_BOOL_STATUS_OKAY(prop) DT_INST_FOREACH_STATUS_OKAY_VARGS(RV8803_DT_CHILD_CNT_BOOL_STATUS_OKAY, prop) 0
// clang-format on

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(irq_gpios)
#if RV8803_DT_ANY_INST_CHILD_CNT_BOOL_STATUS_OKAY(use_irq)
#define RV8803_HAS_IRQ 1
#else
#define RV8803_HAS_IRQ 0
#endif
#else
#define RV8803_HAS_IRQ 0
#endif

int rv8803_bus_reg_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *value);
int rv8803_bus_reg_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t value);
int rv8803_bus_reg_update_byte(const struct device *dev, uint8_t reg_addr, uint8_t mask,
			       uint8_t value);
int rv8803_bus_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *buf,
			  uint32_t num_bytes);
int rv8803_bus_burst_write(const struct device *dev, uint8_t start_addr, const uint8_t *buf,
			   uint32_t num_bytes);

bool rv8803_irq_gpio_is_available(const struct device *dev);
int rv8803_append_irq_listener(const struct device *dev, struct k_work *worker);

#if CONFIG_MFD_RV8803_DETECT_BATTERY_STATE
struct rv8803_battery {
	bool power_on_reset;
	bool low_battery;
};
#endif /* CONFIG_MFD_RV8803_DETECT_BATTERY_STATE */

#undef DT_DRV_COMPAT

#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_API_H_ */
