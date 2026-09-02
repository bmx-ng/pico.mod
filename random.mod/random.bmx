' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Raspberry Pi Pico SDK random-number generator
End Rem
Module Pico.Random
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"
ModuleInfo "Copyright: 2026 Bruce A Henderson and contributors"

Import Random.Core

Rem
bbdoc: A stateless random-number source backed by the Raspberry Pi Pico SDK.
about: On RP2350 the SDK uses the hardware TRNG. On RP2040 it combines the
available device entropy sources. Instances cannot be seeded or serialized.
End Rem
Type TPicoRandom Extends TRandom
	Private
	Const SIGNBIT_32:UInt = $80000000:UInt
	Const SIGNBIT_64:ULong = $8000000000000000:ULong

	Method RangeUInt:UInt(lo:UInt, hi:UInt)
		If lo > hi Then
			Local swap:UInt = lo
			lo = hi
			hi = swap
		End If

		Local span:UInt = hi - lo + 1:UInt
		If span = 0:UInt Then Return PicoRandomUInt()

		Local maximum:UInt = $FFFFFFFF:UInt
		Local limit:UInt = (maximum / span) * span - 1:UInt
		Local value:UInt
		Repeat
			value = PicoRandomUInt()
		Until value <= limit
		Return lo + value Mod span
	End Method

	Method RangeULong:ULong(lo:ULong, hi:ULong)
		If lo > hi Then
			Local swap:ULong = lo
			lo = hi
			hi = swap
		End If

		Local span:ULong = hi - lo + 1:ULong
		If span = 0:ULong Then Return PicoRandomULong()

		Local maximum:ULong = $FFFFFFFFFFFFFFFF:ULong
		Local limit:ULong = (maximum / span) * span - 1:ULong
		Local value:ULong
		Repeat
			value = PicoRandomULong()
		Until value <= limit
		Return lo + value Mod span
	End Method

	Public
	Method RndFloat:Float() Override
		Return Float(PicoRandomUInt() Shr 8) * (1.0:Float / 16777216.0:Float)
	End Method

	Method RndDouble:Double() Override
		Return Double(PicoRandomULong() Shr 11) * (1.0 / 9007199254740992.0)
	End Method

	Method Rnd:Double(minValue:Double = 1, maxValue:Double = 0) Override
		If maxValue > minValue Then Return RndDouble() * (maxValue - minValue) + minValue
		Return RndDouble() * (minValue - maxValue) + maxValue
	End Method

	Method Rand:Int(minValue:Int, maxValue:Int = 1) Override
		Return RandomInt(minValue, maxValue)
	End Method

	Method RandomByte:Byte(minValue:Byte, maxValue:Byte = 1) Override
		Return Byte(RangeUInt(UInt(minValue), UInt(maxValue)))
	End Method

	Method RandomShort:Short(minValue:Short, maxValue:Short = 1) Override
		Return Short(RangeUInt(UInt(minValue), UInt(maxValue)))
	End Method

	Method RandomUInt:UInt(minValue:UInt, maxValue:UInt = 1) Override
		Return RangeUInt(minValue, maxValue)
	End Method

	Method RandomULong:ULong(minValue:ULong, maxValue:ULong = 1) Override
		Return RangeULong(minValue, maxValue)
	End Method

	Method RandomULongInt:ULongInt(minValue:ULongInt, maxValue:ULongInt = 1) Override
		Return ULongInt(RangeULong(ULong(minValue), ULong(maxValue)))
	End Method

	Method RandomSizeT:Size_T(minValue:Size_T, maxValue:Size_T = 1) Override
		Return Size_T(RangeULong(ULong(minValue), ULong(maxValue)))
	End Method

	Method RandomLong:Long(minValue:Long, maxValue:Long = 1) Override
		Local lo:ULong = ULong(minValue) ~ SIGNBIT_64
		Local hi:ULong = ULong(maxValue) ~ SIGNBIT_64
		Return Long(RangeULong(lo, hi) ~ SIGNBIT_64)
	End Method

	Method RandomInt:Int(minValue:Int, maxValue:Int = 1) Override
		Local lo:UInt = UInt(minValue) ~ SIGNBIT_32
		Local hi:UInt = UInt(maxValue) ~ SIGNBIT_32
		Return Int(RangeUInt(lo, hi) ~ SIGNBIT_32)
	End Method

	Method RandomLongInt:LongInt(minValue:LongInt, maxValue:LongInt = 1) Override
		Return LongInt(RandomLong(Long(minValue), Long(maxValue)))
	End Method

	Method SeedRnd(seed:Int) Override
		' The SDK entropy source is intentionally not seedable.
	End Method

	Method RndSeed:Int() Override
		Return 0
	End Method

	Method GetName:String() Override
		Return "Pico"
	End Method

	Method SerializeState:String() Override
		Return Null
	End Method

	Method CanSaveState:Int() Override
		Return False
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

Extern "C"
	Function PicoRandomUInt:UInt() = "get_rand_32"
	Function PicoRandomULong:ULong() = "get_rand_64"
End Extern

New TPicoRandomFactory
?
