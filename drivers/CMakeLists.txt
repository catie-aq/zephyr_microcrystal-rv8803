# Copyright (c) 2024 CATIE
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_RTC rtc)
add_subdirectory_ifdef(CONFIG_CLOCK_CONTROL clock_control)
