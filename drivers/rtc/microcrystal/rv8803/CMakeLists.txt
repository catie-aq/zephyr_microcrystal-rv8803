# Copyright (c) 2024 CATIE
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(rv8803.c)
zephyr_library_sources(rv8803_rtc.c)
zephyr_library_sources(rv8803_cnt.c)
zephyr_library_sources(rv8803_clk.c)
zephyr_include_directories_ifdef(CONFIG_RV8803_DETECT_BATTERY_STATE .)
