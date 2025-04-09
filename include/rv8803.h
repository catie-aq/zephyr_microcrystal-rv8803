/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_H_

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

#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_H_ */
