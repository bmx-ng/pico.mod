' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Raspberry Pi Pico SDK random-number generator.
End Rem
Module Pico.Random
?pico

ModuleInfo "Version: 0.2"
ModuleInfo "License: zlib/libpng"

Import Embedded.Random

Rem
bbdoc: Compatibility name for the shared device-backed random generator.
about: On RP2350 the SDK uses the hardware TRNG. On RP2040 it combines the
available device entropy sources. Instances cannot be seeded or serialized.
End Rem
Type TPicoRandom Extends TEmbeddedRandom
	Method GetName:String() Override
		Return "Pico"
	End Method
End Type

Private
Type TPicoRandomFactory Extends TRandomFactory
	Method New()
		Super.New()
		Init()
	End Method

	Method GetName:String() Override
		Return "Pico"
	End Method

	Method Create:TRandom(seed:Int) Override
		Return New TPicoRandom
	End Method

	Method Create:TRandom() Override
		Return New TPicoRandom
	End Method

	Method DeserializeState:TRandom(data:String) Override
		Return Null
	End Method
End Type
Public

Function PicoRandomUInt:UInt()
	Return EmbeddedRandomUInt()
End Function

Function PicoRandomULong:ULong()
	Return EmbeddedRandomULong()
End Function

Function PicoFillRandom:Int(buffer:Byte Ptr, length:Int)
	Return EmbeddedFillRandom(buffer, length)
End Function

New TPicoRandomFactory
?
