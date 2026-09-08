SuperStrict

Framework BRL.StandardIO
Import Pico.System.Time
Import "float_abi_work.c"

Const Iterations:Int = 1000000

Extern "C"
	Function FloatABIName:Byte Ptr() = "bmx_float_abi_name"
	Function IntegerCall4:UInt(first:UInt, second:UInt, third:UInt, fourth:UInt) = "bmx_float_abi_integer_call4"
	Function FloatCall4:Float(first:Float, second:Float, third:Float, fourth:Float) = "bmx_float_abi_float_call4"
	Function DoubleCall4:Double(first:Double, second:Double, third:Double, fourth:Double) = "bmx_float_abi_double_call4"
End Extern

Function Minimum:ULong(first:ULong, second:ULong, third:ULong)
	Local result:ULong = first
	If second < result Then result = second
	If third < result Then result = third
	Return result
End Function

Function RunIntegerCalls:ULong(checksum:UInt Var)
	Local value:UInt = 17
	Local started:ULong = MonotonicMicroseconds()
	For Local index:Int = 0 Until Iterations
		value = IntegerCall4(value, UInt(index & 255), 7, 11)
	Next
	Local elapsed:ULong = MonotonicMicroseconds() - started
	checksum = value
	Return elapsed
End Function

Function RunFloatCalls:ULong(checksum:Float Var)
	Local value:Float = 17.0
	Local started:ULong = MonotonicMicroseconds()
	For Local index:Int = 0 Until Iterations
		value = FloatCall4(value, Float(index & 255), 7.0, 11.0)
	Next
	Local elapsed:ULong = MonotonicMicroseconds() - started
	checksum = value
	Return elapsed
End Function

Function RunDoubleCalls:ULong(checksum:Double Var)
	Local value:Double = 17.0
	Local started:ULong = MonotonicMicroseconds()
	For Local index:Int = 0 Until Iterations
		value = DoubleCall4(value, Double(index & 255), 7.0, 11.0)
	Next
	Local elapsed:ULong = MonotonicMicroseconds() - started
	checksum = value
	Return elapsed
End Function

Function Report(name:String, elapsed:ULong, checksum:String)
	Print "FLOAT_ABI," + String.FromCString(FloatABIName()) + "," + SystemClockFrequency() + "," + name + "," + Iterations + "," + elapsed + "," + checksum
End Function

Delay 5000
Print "FLOAT_ABI_HEADER,abi,cpu_hz,test,calls,microseconds,checksum"

Local integerChecksum:UInt
Local floatChecksum:Float
Local doubleChecksum:Double

' Warm every path before taking the minimum of three complete runs.
RunIntegerCalls(integerChecksum)
RunFloatCalls(floatChecksum)
RunDoubleCalls(doubleChecksum)

Local first:ULong = RunIntegerCalls(integerChecksum)
Local second:ULong = RunIntegerCalls(integerChecksum)
Local third:ULong = RunIntegerCalls(integerChecksum)
Report("integer_call4", Minimum(first, second, third), String(integerChecksum))

first = RunFloatCalls(floatChecksum)
second = RunFloatCalls(floatChecksum)
third = RunFloatCalls(floatChecksum)
Report("float_call4", Minimum(first, second, third), String.FromFloat(floatChecksum))

first = RunDoubleCalls(doubleChecksum)
second = RunDoubleCalls(doubleChecksum)
third = RunDoubleCalls(doubleChecksum)
Report("double_call4", Minimum(first, second, third), String.FromDouble(doubleChecksum))

Print "FLOAT_ABI_DONE"

' Keep USB servicing alive so picotool can automatically replace this firmware.
While True
	Delay 1000
Wend
