# RV-8803-C7 RTC example application
#
# Copyright (c) 2024 CATIE
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rv_8803)

FILE(GLOB app_sources src/main.c)
target_sources(app PRIVATE ${app_sources})
