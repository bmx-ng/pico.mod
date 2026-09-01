SuperStrict

Import BRL.StandardIO
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Local intMinimum:Int = $80000000
Local intMaximum:Int = $7fffffff
Local uintMaximum:UInt = $ffffffff:UInt
Local longMinimum:Long = $8000000000000000:Long
Local longMaximum:Long = $7fffffffffffffff:Long
Local ulongMaximum:ULong = $ffffffffffffffff:ULong
Local sizeMaximum:Size_T = $ffffffff:Size_T
Local longIntMinimum:LongInt = $80000000:LongInt
Local ulongIntMaximum:ULongInt = $ffffffff:ULongInt

Local checksPassed:Int = True
Local integerFormatPassed:Int = True
integerFormatPassed :& String.FromInt(intMinimum) = "-2147483648"
integerFormatPassed :& String.FromInt(intMaximum) = "2147483647"
integerFormatPassed :& String.FromUInt(uintMaximum) = "4294967295"
integerFormatPassed :& String.FromLong(longMinimum) = "-9223372036854775808"
integerFormatPassed :& String.FromLong(longMaximum) = "9223372036854775807"
integerFormatPassed :& String.FromULong(ulongMaximum) = "18446744073709551615"
integerFormatPassed :& String.FromSizeT(sizeMaximum) = "4294967295"
integerFormatPassed :& String.FromLongInt(longIntMinimum) = "-2147483648"
integerFormatPassed :& String.FromULongInt(ulongIntMaximum) = "4294967295"

Local integerParsePassed:Int = True
integerParsePassed :& "  -123tail".ToInt() = -123
integerParsePassed :& "$ffffffff".ToUInt() = uintMaximum
integerParsePassed :& "-9223372036854775808".ToLong() = longMinimum
integerParsePassed :& "18446744073709551615".ToULong() = ulongMaximum
integerParsePassed :& "4294967295".ToSizet() = sizeMaximum
integerParsePassed :& "-2147483648".ToLongInt() = longIntMinimum
integerParsePassed :& "%11111111111111111111111111111111".ToULongInt() = ulongIntMaximum

' Exercise language conversions rather than only direct String methods.
Local implicitText:String = "value=" + 42
Local implicitInt:Int = Int("1234")
Local implicitLong:Long = Long("-5678")
Local implicitIntegerPassed:Int = implicitText = "value=42" And implicitInt = 1234 And implicitLong = -5678

Local floatFormatPassed:Int = True
floatFormatPassed :& String.FromFloat(0.0:Float) = "0.0"
floatFormatPassed :& String.FromFloat(-0.0:Float) = "-0.0"
floatFormatPassed :& String.FromFloat(3.1415927:Float, True) = "3.141592741"
floatFormatPassed :& String.FromDouble(42.0:Double) = "42.0"
floatFormatPassed :& String.FromDouble(3.141592653589793:Double, True) = "3.14159265358979312"
Local floatParsePassed:Int = "  +1.25tail".ToFloat() = 1.25:Float
floatParsePassed :& "-3.141592653589793".ToDouble() = -3.141592653589793:Double
Local implicitDouble:Double = Double("6.25")
Local implicitFloatText:String = "float=" + 1.5:Float
Local implicitFloatPassed:Int = implicitDouble = 6.25:Double And implicitFloatText = "float=1.5"
checksPassed = integerFormatPassed And integerParsePassed And implicitIntegerPassed And ..
	floatFormatPassed And floatParsePassed And implicitFloatPassed

' Converted Strings remain ordinary precisely traced values under pressure.
Local retainedConversion:String = String.FromLong(longMinimum)
Local transientConversion:String
For Local index:Int = 0 Until 800
	transientConversion = "sample=" + index
Next
checksPassed :& retainedConversion = "-9223372036854775808" And AutomaticCollectionCount() > 0

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		Print "String numeric conversion checks passed"
	Else
		ErrPrint "String numeric conversion check failed"
		If Not integerFormatPassed Then ErrPrint "integer format"
		If Not integerParsePassed Then ErrPrint "integer parse"
		If Not implicitIntegerPassed Then ErrPrint "implicit integer"
		If Not floatFormatPassed Then
			ErrPrint "float format"
			Print String.FromFloat(3.1415927:Float, True)
			Print String.FromDouble(3.141592653589793:Double, True)
		End If
		If Not floatParsePassed Then ErrPrint "float parse"
		If Not implicitFloatPassed Then ErrPrint "implicit float"
	End If
	SleepMilliseconds(1000)
Wend
