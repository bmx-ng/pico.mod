SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Function SumValues:Int(values:Int[])
	Local total:Int
	For Local value:Int = EachIn values
		total :+ value
	Next
	Return total
End Function

Function AllocateTemporary(seed:Int)
	Local temporary:Int[] = New Int[32]
	temporary[0] = seed
End Function

StandardIOInit()

Local values:Int[] = New Int[6]
Local empty:Int[]
Local initiallyZero:Int = values[0] = 0 And values[5] = 0
values[0] = 3
values[2] = 7
values[5] = 13

For Local index:Int = 0 Until 200
	AllocateTemporary(index)
Next
Local reclaimedObjects:UInt = CollectObjects()

Local checksPassed:Int = values And Not empty And values = values And ..
	values.Length = 6 And empty.Length = 0 And initiallyZero And ..
	values[0] = 3 And values[2] = 7 And values[5] = 13 And ..
	SumValues(values) = 23 And reclaimedObjects = 0 And ..
	ArrayAllocationCount() = 201 And ArrayAllocatedBytes() = 28840 And ..
	ArrayLiveCount() = 1 And ArrayLiveBytes() = 40 And ..
	ReachableArrayCount() = 1 And UnreachableArrayCount() = 0 And ..
	AutomaticCollectionCount() > 0 And CollectionCount() = AutomaticCollectionCount() + 1 And ..
	LastReclaimedArrayCount() > 0 And LastReclaimedArrayBytes() > 0 And ..
	ArenaAllocationCount() = 201 And HeapReusableBytes() > 0 And ..
	RootFrameCount() = 1 And RootSlotCount() >= 2 And ..
	ArenaUsed() > 0 And ArenaHighWater() = ArenaUsed() And ..
	ArenaFailureCount() = 0 And ArrayFailureCount() = 0

If checksPassed Then
	PutString("Scalar Array checks passed")
Else
	PutString("Scalar Array check failed")
End If
PutCharacter(10)

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then PutCharacter(46) Else PutCharacter(33)
	SleepMilliseconds(250)
	GPIOPut(ledPin, False)
	SleepMilliseconds(250)
Wend
