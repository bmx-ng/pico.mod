' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Raspberry Pi Pico wireless LAN support.
about: Portable station, scanning and link APIs are provided by
Embedded.Network.WiFi. This module also exposes the CYW43 GPIO used for the
onboard LED on official Pico W boards.
End Rem
Module Pico.Network.WiFi
?pico

ModuleInfo "Version: 0.4"
ModuleInfo "License: zlib/libpng"

Import Embedded.Network.WiFi

Extern "C"
	Function WiFiSetLED:Int(value:Int) = "bmx_pico_wifi_set_led"
	Function WiFiGetLED:Int() = "bmx_pico_wifi_get_led"
End Extern
?
