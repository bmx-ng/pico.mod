' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Pico W and Pico 2 W Bluetooth Low Energy support.
about: Imports the portable Embedded.Network.BLE API. Controller callbacks
are copied into a native queue and delivered by PollSystem or WaitSystem.
End Rem
Module Pico.Network.BLE
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Import Embedded.Network.BLE

Const PicoBLEUnsupported:Int = -100
?
