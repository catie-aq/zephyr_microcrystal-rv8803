/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(rv_8803_c70));
	//struct rtc_time *sample_time;

	if (!device_is_ready(dev)) {
		printk("Device is not ready\r\n");
		return 1;
	}

	while (1) {
		//sensor_sample_fetch(dev);
        // Example for ambiant temperature sensor
		//sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &value);
		printk("Test\r\n");
		k_sleep(K_MSEC(1000));
	}
}
