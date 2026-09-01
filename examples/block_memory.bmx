SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Type TManagedBuffer
	Field bytes:Byte[]
	Field pointer:Byte Ptr

	Method New(size:Int)
		bytes = New Byte[size]
		pointer = bytes
	End Method
End Type

StandardIOInit()

Local checksPassed:Int = True
Local source:Byte[] = New Byte[32]
Local destination:Byte[] = New Byte[32]
For Local index:Int = 0 Until source.Length
	source[index] = Byte(index * 3 + 1)
Next

' Managed Array payloads are ordinary contiguous storage at a Byte Ptr boundary.
MemCopy(destination, source, Size_T(source.Length))
For Local index:Int = 0 Until source.Length
	checksPassed :& destination[index] = source[index]
Next

' Static Arrays decay to their inline storage and can exchange data with managed Arrays.
Local StaticArray fixed:Byte[32]
MemCopy(fixed, destination, Size_T(destination.Length))
MemClear(destination, Size_T(destination.Length))
MemCopy(destination, fixed, Size_T(destination.Length))
For Local index:Int = 0 Until destination.Length
	checksPassed :& destination[index] = Byte(index * 3 + 1)
Next

' Manually owned memory supports clear, copy, overlapping move, and extension.
Local raw:Byte Ptr = MemAlloc(32)
checksPassed :& raw <> Null
If raw Then
	MemClear(raw, 32)
	For Local index:Int = 0 Until 32
		checksPassed :& raw[index] = 0
	Next
	MemCopy(raw, source, 32)
	MemMove(raw + 4, raw, 12)
	For Local index:Int = 0 Until 12
		checksPassed :& raw[index + 4] = source[index]
	Next
	raw = MemExtend(raw, 32, 64)
	checksPassed :& raw <> Null
	If raw Then
		For Local index:Int = 0 Until 12
			checksPassed :& raw[index + 4] = source[index]
		Next
		MemCopy(destination, raw + 4, 12)
		For Local index:Int = 0 Until 12
			checksPassed :& destination[index] = source[index]
		Next
		MemFree(raw)
	End If
End If

' Freed manual blocks return to the same reusable free list as managed allocations.
Local first:Byte Ptr = MemAlloc(96)
Local second:Byte Ptr = MemAlloc(96)
checksPassed :& first <> Null And second <> Null
MemFree(first)
Local reused:Byte Ptr = MemAlloc(64)
checksPassed :& reused = first
MemFree(reused)
MemFree(second)

' Keeping the owning Object alive keeps its Array payload pointer valid across collection.
Local retained:TManagedBuffer = New TManagedBuffer(64)
For Local index:Int = 0 Until retained.bytes.Length
	retained.pointer[index] = Byte(index + 7)
Next
For Local index:Int = 0 Until 160
	Local pressure:Int[] = New Int[16]
	pressure[0] = index
Next
CollectObjects()
For Local index:Int = 0 Until retained.bytes.Length
	checksPassed :& retained.pointer[index] = Byte(index + 7)
Next
checksPassed :& InvalidReferenceCount() = 0 And ArenaFailureCount() = 0

If checksPassed Then
	PutString("Block-memory checks passed")
Else
	PutString("Block-memory check failed")
End If
PutCharacter(10)

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then PutCharacter(46) Else PutCharacter(33)
	SleepMilliseconds(250)
Wend
