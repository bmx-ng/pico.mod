SuperStrict

Framework BRL.StandardIO
Import Pico.System.Time
Import Pico.Runtime.Memory

Const IntegerIterations:Int = 1000000
Const CallIterations:Int = 500000
Const BufferSize:Int = 1024
Const BufferRounds:Int = 500
Const MethodIterations:Int = 300000
Const AllocationWidth:Int = 64
Const AllocationRounds:Int = 100
Const StringWidth:Int = 128
Const StringRounds:Int = 50

Type TBenchCounter
	Field value:UInt

	Method New(initial:UInt)
		value = initial
	End Method

	Method Step:UInt(input:UInt)
		value = value * 1664525:UInt + input + 1013904223:UInt
		Return value
	End Method
End Type

Type TBenchAllocation
	Field value:Int

	Method New(value:Int)
		Self.value = value
	End Method
End Type

Function MixStep:UInt(value:UInt)
	Return value * 1664525:UInt + 1013904223:UInt
End Function

Function Minimum:ULong(first:ULong, second:ULong, third:ULong)
	Local result:ULong = first
	If second < result Then result = second
	If third < result Then result = third
	Return result
End Function

Function RunIntegerMix:ULong(iterations:Int, checksum:UInt Var)
	Local value:UInt = $12345678:UInt
	Local started:ULong = MonotonicMicroseconds()
	For Local index:Int = 0 Until iterations
		value = value * 1664525:UInt + 1013904223:UInt
	Next
	Local elapsed:ULong = MonotonicMicroseconds() - started
	checksum = value
	Return elapsed
End Function

Function RunFunctionCalls:ULong(iterations:Int, checksum:UInt Var)
	Local value:UInt = $12345678:UInt
	Local started:ULong = MonotonicMicroseconds()
	For Local index:Int = 0 Until iterations
		value = MixStep(value)
	Next
	Local elapsed:ULong = MonotonicMicroseconds() - started
	checksum = value
	Return elapsed
End Function

Function RunBufferMix:ULong(buffer:Byte[], rounds:Int, checksum:UInt Var)
	Local total:UInt
	Local started:ULong = MonotonicMicroseconds()
	For Local round:Int = 0 Until rounds
		For Local index:Int = 0 Until buffer.length
			Local value:Byte = Byte(index * 13 + round)
			buffer[index] = value
			total :+ UInt(value)
		Next
	Next
	Local elapsed:ULong = MonotonicMicroseconds() - started
	checksum = total
	Return elapsed
End Function

Function RunMethodCalls:ULong(iterations:Int, checksum:UInt Var)
	Local counter:TBenchCounter = New TBenchCounter($12345678:UInt)
	Local value:UInt
	Local started:ULong = MonotonicMicroseconds()
	For Local index:Int = 0 Until iterations
		value = counter.Step(UInt(index))
	Next
	Local elapsed:ULong = MonotonicMicroseconds() - started
	checksum = value
	Return elapsed
End Function

Function RunAllocationGC:ULong(rounds:Int, checksum:UInt Var)
	Local objects:TBenchAllocation[] = New TBenchAllocation[AllocationWidth]
	Local total:UInt
	Local started:ULong = MonotonicMicroseconds()
	For Local round:Int = 0 Until rounds
		For Local index:Int = 0 Until objects.length
			objects[index] = New TBenchAllocation(round + index)
			total :+ UInt(objects[index].value)
		Next
		For Local index:Int = 0 Until objects.length
			objects[index] = Null
		Next
		CollectObjects()
	Next
	Local elapsed:ULong = MonotonicMicroseconds() - started
	checksum = total
	Return elapsed
End Function

Function RunStringBuild:ULong(rounds:Int, checksum:UInt Var)
	Local total:UInt
	Local started:ULong = MonotonicMicroseconds()
	For Local round:Int = 0 Until rounds
		Local text:String
		For Local index:Int = 0 Until StringWidth
			text :+ "x"
		Next
		total :+ UInt(text.length)
		text = ""
		CollectObjects()
	Next
	Local elapsed:ULong = MonotonicMicroseconds() - started
	checksum = total
	Return elapsed
End Function

Function Report(name:String, work:Int, elapsed:ULong, checksum:UInt)
	Print "BENCH,blitzmax," + SystemClockFrequency() + "," + name + "," + work + "," + elapsed + "," + checksum
End Function

Local checksum:UInt
Local buffer:Byte[] = New Byte[BufferSize]

' Allow a USB serial terminal to reconnect after reset; this is outside every
' timed region and mirrors the interactive MicroPython launch.
Delay 10000

' Warm up lazy runtime paths and exclude initial USB output setup.
RunIntegerMix(1000, checksum)
RunFunctionCalls(1000, checksum)
RunBufferMix(buffer, 1, checksum)
RunMethodCalls(1000, checksum)
CollectObjects()

Print "BENCH_HEADER,engine,cpu_hz,test,work_units,microseconds,checksum"

Local first:ULong = RunIntegerMix(IntegerIterations, checksum)
Local second:ULong = RunIntegerMix(IntegerIterations, checksum)
Local third:ULong = RunIntegerMix(IntegerIterations, checksum)
Report("integer_mix", IntegerIterations, Minimum(first, second, third), checksum)

first = RunFunctionCalls(CallIterations, checksum)
second = RunFunctionCalls(CallIterations, checksum)
third = RunFunctionCalls(CallIterations, checksum)
Report("function_calls", CallIterations, Minimum(first, second, third), checksum)

first = RunBufferMix(buffer, BufferRounds, checksum)
second = RunBufferMix(buffer, BufferRounds, checksum)
third = RunBufferMix(buffer, BufferRounds, checksum)
Report("byte_buffer", BufferSize * BufferRounds, Minimum(first, second, third), checksum)

first = RunMethodCalls(MethodIterations, checksum)
second = RunMethodCalls(MethodIterations, checksum)
third = RunMethodCalls(MethodIterations, checksum)
Report("method_calls", MethodIterations, Minimum(first, second, third), checksum)

first = RunAllocationGC(AllocationRounds, checksum)
second = RunAllocationGC(AllocationRounds, checksum)
third = RunAllocationGC(AllocationRounds, checksum)
Report("object_alloc_gc", AllocationWidth * AllocationRounds, Minimum(first, second, third), checksum)

first = RunStringBuild(StringRounds, checksum)
second = RunStringBuild(StringRounds, checksum)
third = RunStringBuild(StringRounds, checksum)
Report("string_build_gc", StringWidth * StringRounds, Minimum(first, second, third), checksum)

Print "BENCH_DONE"
