/*
 * Copyright (c) 2026 Bruce A Henderson and contributors
 * SPDX-License-Identifier: Zlib
 */

#ifndef _BOARDS_BLITZMAX_TEST_RP2040_H
#define _BOARDS_BLITZMAX_TEST_RP2040_H

pico_board_cmake_set(PICO_PLATFORM, rp2040)
pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (4 * 1024 * 1024))

#define BLITZMAX_TEST_RP2040
#define PICO_DEFAULT_UART 0
#define PICO_DEFAULT_UART_TX_PIN 0
#define PICO_DEFAULT_UART_RX_PIN 1

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#endif

#endif
