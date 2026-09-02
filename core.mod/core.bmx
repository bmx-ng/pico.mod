' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Common definitions for Raspberry Pi Pico targets.
End Rem
Module Pico.Core
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Rem
bbdoc: Value returned by board-default pin queries when the selected board defines no default pin.
End Rem
Const PicoUnavailablePin:UInt = $ffffffff

Rem
bbdoc: Value returned by board-default controller queries when the selected board defines no default controller.
End Rem
Const PicoUnavailableController:Int = -1
?
