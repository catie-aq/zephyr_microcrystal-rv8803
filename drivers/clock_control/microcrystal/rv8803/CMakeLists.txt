# Copyright (c) 2024 CATIE
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(clock_control_rv8803.c)
#zephyr_library_add_dependencies(rv8803_base)
#zephyr_append_cmake_library(rv8803_base)
