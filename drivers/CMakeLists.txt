# Copyright (c) 2024 CATIE
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_MFD mfd)
add_subdirectory_ifdef(CONFIG_RTC rtc)
add_subdirectory_ifdef(CONFIG_COUNTER counter)
add_subdirectory_ifdef(CONFIG_CLOCK_CONTROL clock)

if (CONFIG_MFD)
zephyr_library_include_directories(common)
endif()
