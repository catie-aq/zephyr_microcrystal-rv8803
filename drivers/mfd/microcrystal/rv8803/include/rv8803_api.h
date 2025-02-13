/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_API_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_API_H_

#include <zephyr/kernel.h>

#define DT_DRV_COMPAT microcrystal_rv8803

/**
 * @brief RV8803 Calendar Registers
 *
 * These registers store the calendar date and time.
 *
 * @details
 *
 * | Register | Description |
 * |----------|-------------|
 * | 0x00     | Seconds     |
 * | 0x01     | Minutes     |
 * | 0x02     | Hours       |
 * | 0x03     | Weekday     |
 * | 0x04     | Date        |
 * | 0x05     | Month       |
 * | 0x06     | Year        |
 */
#define RV8803_REGISTER_SECONDS 0x00
#define RV8803_REGISTER_MINUTES 0x01
#define RV8803_REGISTER_HOURS   0x02
#define RV8803_REGISTER_WEEKDAY 0x03
#define RV8803_REGISTER_DATE    0x04
#define RV8803_REGISTER_MONTH   0x05
#define RV8803_REGISTER_YEAR    0x06

/**
 * @brief RV8803 Registers
 *
 * These registers are used to control the RTC and its features.
 */
#define RV8803_REGISTER_EXTENSION 0x0D
#define RV8803_REGISTER_FLAG      0x0E
#define RV8803_REGISTER_CONTROL   0x0F

/**
 * @brief Control Register Masks for RV8803
 *
 * These masks are used to configure the control register of the RV8803 device.
 * Each mask represents a specific feature or interrupt that can be enabled or
 * disabled in the control register.
 */
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
// clang-format on

/**
 * @brief Extension Register Masks for RV8803
 *
 * These masks are used to configure the extension register of the RV8803 device.
 * Each mask represents a specific feature that can be enabled or disabled.
 */
#define RV8803_EXTENSION_MASK_WADA    (0x01 << 6)
#define RV8803_EXTENSION_MASK_UPDATE  (0x01 << 5)
#define RV8803_EXTENSION_MASK_COUNTER (0x01 << 4)

/**
 * @brief Flag Register Masks for RV8803
 *
 * These masks are used to access the individual flags in the flag register of
 * the RV8803 device. Each mask represents a specific flag that can be accessed
 * in the flag register.
 */
#define RV8803_FLAG_MASK_ALARM   (0x01 << 3)
#define RV8803_FLAG_MASK_COUNTER (0x01 << 4)
#define RV8803_FLAG_MASK_UPDATE  (0x01 << 5)

// clang-format off
/**
 * @brief Generate a macro to count the number of children of a given instance
 * that have a given property.
 *
 * @details This macro takes an instance number and a property name and
 * generates a macro that takes an instance number and evaluates to the
 * number of children of the given instance that have the given property.
 *
 * @param idx The instance number.
 * @param prop The property name.
 *
 * @return The number of children of the given instance that have the given
 * property.
 */
#define RV8803_DT_CNT_BOOL(idx, prop) COND_CODE_1(DT_PROP(idx, prop), (1 +), (0 +))

/**
 * @brief Generate a macro to count the number of children of a given instance
 * that have a given property.
 *
 * @details This macro takes an instance number and a property name and
 * generates a macro that takes an instance number and evaluates to the
 * number of children of the given instance that have the given property.
 *
 * @param inst The instance number.
 * @param prop The property name.
 *
 * @return The number of children of the given instance that have the given
 * property.
 */
#define RV8803_DT_CHILD_CNT_BOOL_STATUS_OKAY(inst, prop) DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(inst, RV8803_DT_CNT_BOOL, prop) +

/**
 * @brief Generate a macro to count the number of children of any instance
 * that have a given property.
 *
 * @details This macro takes a property name and generates a macro that takes
 * no arguments and evaluates to the number of children of any instance that
 * have the given property.
 *
 * @param prop The property name.
 *
 * @return The number of children of any instance that have the given property.
 */
#define RV8803_DT_ANY_INST_CHILD_CNT_BOOL_STATUS_OKAY(prop) DT_INST_FOREACH_STATUS_OKAY_VARGS(RV8803_DT_CHILD_CNT_BOOL_STATUS_OKAY, prop) 0
// clang-format on

/**
 * @brief Check if any instance of the RV8803 MFD has the use-irq property
 *
 * @details This macro evaluates to 1 if any instance of the RV8803 MFD
 * has the use-irq property, and 0 otherwise.
 *
 * @return 1 if any instance has the use-irq property, 0 otherwise.
 */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(irq_gpios)
#if RV8803_DT_ANY_INST_CHILD_CNT_BOOL_STATUS_OKAY(use_irq)
#define RV8803_HAS_IRQ 1
#else
#define RV8803_HAS_IRQ 0
#endif
#else
#define RV8803_HAS_IRQ 0
#endif

/**
 * @brief Read a byte from the RV8803 register
 *
 * @param dev RV8803 device structure
 * @param reg_addr Register address
 * @param value Pointer to store the read value
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
int rv8803_bus_reg_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *value);

/**
 * @brief Write a byte to the RV8803 register
 *
 * @param dev RV8803 device structure
 * @param reg_addr Register address
 * @param value Value to write
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
int rv8803_bus_reg_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t value);

/**
 * @brief Update a byte in the RV8803 register
 *
 * @param dev RV8803 device structure
 * @param reg_addr Register address
 * @param mask Mask of bits to update
 * @param value Value to write
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
int rv8803_bus_reg_update_byte(const struct device *dev, uint8_t reg_addr, uint8_t mask,
			       uint8_t value);

/**
 * @brief Perform a burst read from the RV8803 registers
 *
 * @param dev RV8803 device structure
 * @param start_addr Starting register address for the burst read
 * @param buf Buffer to store the read data
 * @param num_bytes Number of bytes to read
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
int rv8803_bus_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *buf,
			  uint32_t num_bytes);

/**
 * @brief Perform a burst write to the RV8803 registers
 *
 * @param dev RV8803 device structure
 * @param start_addr Starting register address for the burst write
 * @param buf Buffer containing the data to write
 * @param num_bytes Number of bytes to write
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
int rv8803_bus_burst_write(const struct device *dev, uint8_t start_addr, const uint8_t *buf,
			   uint32_t num_bytes);

/**
 * @brief Check if the IRQ GPIO is available.
 *
 * @param dev Pointer to the RV8803 device structure.
 *
 * @return true if the IRQ GPIO is available, false otherwise.
 */
bool rv8803_irq_gpio_is_available(const struct device *dev);

/**
 * @brief Append an IRQ listener to the RV8803 device.
 *
 * Adds a new work item to the IRQ listener array for the RV8803 device.
 * Ensures that the listener array is not full before appending the new listener.
 * If the array is full, an error code is returned.
 *
 * @param dev Pointer to the RV8803 device structure.
 * @param worker Pointer to the work item to be appended as an IRQ listener.
 *
 * @retval 0 on success.
 * @retval -ENOSR if the listener array is full.
 */
int rv8803_append_irq_listener(const struct device *dev, struct k_work *worker);

#if CONFIG_MFD_RV8803_DETECT_BATTERY_STATE
/**
 * @brief RV8803 battery information structure.
 *
 * @details This structure contains the following:
 *
 * - @a power_on_reset: Power-on reset flag.
 * - @a low_battery: Low battery flag.
 */
struct rv8803_battery {
	bool power_on_reset;
	bool low_battery;
};
#endif /* CONFIG_MFD_RV8803_DETECT_BATTERY_STATE */

#undef DT_DRV_COMPAT

#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_API_H_ */
