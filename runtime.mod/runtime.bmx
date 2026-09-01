SuperStrict

Rem
bbdoc: Embedded runtime services for Raspberry Pi Pico targets.
End Rem
Module Pico.Runtime

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

' This module will expose only runtime services that are safe on the selected
' execution context (main loop, interrupt handler, or second core).
