SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Global finalizedCount:UInt
Global finalizedValue:Int
Global resurrected:TResource

Type TResource
	Field identifier:Int
	Field payload:Int[]

	Method New(identifier:Int)
		Self.identifier = identifier
		payload = New Int[4]
		payload[0] = identifier * 10
	End Method

	Method Delete()
		finalizedCount :+ 1
		finalizedValue :+ identifier + payload[0]
		If identifier = 7 Then resurrected = Self
	End Method
End Type

Function AllocateTemporary(seed:Int)
	Local temporary:Int[] = New Int[32]
	temporary[0] = seed
End Function

StandardIOInit()

Local discarded:TResource = New TResource(3)
Local revived:TResource = New TResource(7)
discarded = Null
revived = Null

Local firstReclaimed:UInt = CollectObjects()
Local firstFinalized:UInt = LastFinalizedObjectCount()
Local liveAfterFinalizers:UInt = ObjectLiveCount()
Local arraysAfterFinalizers:UInt = ArrayLiveCount()

Local secondReclaimed:UInt = CollectObjects()
Local secondArrayReclaimed:UInt = LastReclaimedArrayCount()
Local resurrectionValid:Int = resurrected And resurrected.identifier = 7 And resurrected.payload[0] = 70

resurrected = Null
Local thirdReclaimed:UInt = CollectObjects()
Local thirdArrayReclaimed:UInt = LastReclaimedArrayCount()

Local pressure:TResource = New TResource(5)
pressure = Null
For Local index:Int = 0 Until 200
	AllocateTemporary(index)
Next
Local automaticAfterPressure:UInt = AutomaticCollectionCount()
Local pressureCleanupObjects:UInt = CollectObjects()

Local checksPassed:Int = firstReclaimed = 0 And firstFinalized = 2 And ..
	liveAfterFinalizers = 2 And arraysAfterFinalizers = 2 And ..
	finalizedCount = 3 And finalizedValue = 165 And ..
	secondReclaimed = 1 And secondArrayReclaimed = 1 And resurrectionValid And ..
	thirdReclaimed = 1 And thirdArrayReclaimed = 1 And ..
	automaticAfterPressure >= 2 And pressureCleanupObjects = 0 And ..
	FinalizerInvocationCount() = 3 And LastFinalizedObjectCount() = 0 And ..
	FinalizerPendingCount() = 0 And CollectionCount() = automaticAfterPressure + 4 And ..
	ObjectAllocationCount() = 3 And ObjectLiveCount() = 0 And ObjectLiveBytes() = 0 And ..
	ArrayAllocationCount() = 203 And ArrayLiveCount() = 0 And ArrayLiveBytes() = 0 And ..
	ReachableObjectCount() = 0 And UnreachableObjectCount() = 0 And ..
	ReachableArrayCount() = 0 And UnreachableArrayCount() = 0 And ..
	HeapReusableBytes() > 0 And RootFrameCount() = 1 And ..
	ArenaFailureCount() = 0 And ArrayFailureCount() = 0 And ObjectFailureCount() = 0

If checksPassed Then
	PutString("Finalizer checks passed")
Else
	PutString("Finalizer check failed")
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
