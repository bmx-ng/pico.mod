' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Optional Unicode-aware String case conversion and case folding for Pico targets.
End Rem
Module Pico.Text.Unicode
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Extern "C"
	Function EnableUnicodeStringCase() = "bmx_pico_unicode_enable"
End Extern

EnableUnicodeStringCase()
?
