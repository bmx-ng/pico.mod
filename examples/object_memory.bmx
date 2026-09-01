SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Type TDefaults
	Field base:Int = 5
	Field factor:Int = 3

	Method Product:Int()
		Return base * factor
	End Method
End Type

Type TCounter
	Field value:Int
	Field updates:UInt

	Method New(initial:Int)
		value = initial
	End Method

	Method Add:Int(delta:Int)
		value :+ delta
		updates :+ 1
		Return value
	End Method

	Method UpdateCount:UInt()
		Return updates
	End Method

	Method Compare:Int(other:Object) Override
		Return value
	End Method

	Method HashCode:UInt() Override
		Return UInt(value * 31)
	End Method

	Method Equals:Int(other:Object) Override
		Return Self = other
	End Method
End Type

Type TNode
	Field value:Int
	Field nextNode:TNode

	Method New(value:Int, nextNode:TNode = Null)
		Self.value = value
		Self.nextNode = nextNode
	End Method
End Type

Function BuildCycle:TNode()
	Local tail:TNode = New TNode(3)
	Local middle:TNode = New TNode(2, tail)
	Local head:TNode = New TNode(1, middle)
	tail.nextNode = head
	Return head
End Function

Function BuildUnreachableCycle()
	Local orphan:TNode = New TNode(99)
	orphan.nextNode = orphan
End Function

Function BuildUnreachablePair:Int(seed:Int)
	Local first:TNode = New TNode(seed)
	Local second:TNode = New TNode(seed + 1)
	first.nextNode = second
	second.nextNode = first
	Return first.nextNode.value = seed + 1 And second.nextNode.value = seed
End Function

Function StressObjectHeap:Int(iterations:Int)
	For Local index:Int = 0 Until iterations
		If Not BuildUnreachablePair(index) Then Return False
	Next
	Return True
End Function

Function ValidateCycle:Int(head:TNode)
	Return head.value = 1 And head.nextNode.value = 2 And ..
		head.nextNode.nextNode.value = 3 And head.nextNode.nextNode.nextNode = head
End Function

Function DriveCounter:Int(counter:TCounter)
	Return counter.Add(4) + counter.Add(6)
End Function

StandardIOInit()

Local defaults:TDefaults = New TDefaults
Local counter:TCounter = New TCounter(10)
Local missing:TCounter
Local driven:Int = DriveCounter(counter)
Local counterOrdering:Int = counter.Compare(defaults)
Local counterHash:UInt = counter.HashCode()
Local counterEqual:Int = counter.Equals(counter) And Not counter.Equals(defaults)
Local counterAsObject:Object = counter
Local objectTypedValues:Int = counterAsObject.Compare(defaults) = 20 And ..
	counterAsObject.HashCode() = 620 And counterAsObject.Equals(counter)
Local defaultHash:UInt = defaults.HashCode()
Local defaultsValid:Int = defaults.Compare(defaults) = 0 And defaults.Equals(defaults) And ..
	Not defaults.Equals(counter) And defaultHash = defaults.HashCode()
Local head:TNode = BuildCycle()
Local cycleValid:Int = ValidateCycle(head)
BuildUnreachableCycle()
Local firstReachable:UInt = ReachabilityAudit()
Local firstReportedReachable:UInt = ReachableObjectCount()
Local firstUnreachable:UInt = UnreachableObjectCount()
head = Null
Local secondReachable:UInt = ReachabilityAudit()
Local usedBeforeCollection:UInt = ArenaUsed()
Local reclaimed:UInt = CollectObjects()
Local usedAfterCollection:UInt = ArenaUsed()
Local liveAfterCollection:UInt = ObjectLiveCount()
Local liveBytesAfterCollection:UInt = ObjectLiveBytes()
Local reusableAfterCollection:UInt = HeapReusableBytes()
Local largestFreeAfterCollection:UInt = HeapLargestFreeBlock()
Local reachableAfterCollection:UInt = ReachableObjectCount()
Local unreachableAfterCollection:UInt = UnreachableObjectCount()
Local collectionsAfterInitial:UInt = CollectionCount()
Local initiallyReclaimedObjects:UInt = LastReclaimedObjectCount()
Local initiallyReclaimedBytes:UInt = LastReclaimedBytes()
Local replacement:TNode = New TNode(7)
Local usedAfterReplacement:UInt = ArenaUsed()
Local reusableAfterReplacement:UInt = HeapReusableBytes()
Local liveAfterReplacement:UInt = ObjectLiveCount()
Local liveBytesAfterReplacement:UInt = ObjectLiveBytes()
Local automaticBeforeStress:UInt = AutomaticCollectionCount()
Local stressPassed:Int = StressObjectHeap(1200)
Local automaticAfterStress:UInt = AutomaticCollectionCount()
Local collectionsAfterStress:UInt = CollectionCount()
Local highWaterAfterStress:UInt = ArenaHighWater()
Local finalReclaimed:UInt = CollectObjects()
Local reusableAfterStress:UInt = HeapReusableBytes()

Local checksPassed:Int = defaults And counter And Not missing And ..
	defaults.Product() = 15 And driven = 34 And ..
	counterOrdering = 20 And counterHash = 620 And counterEqual And objectTypedValues And defaultsValid And ..
	counter.UpdateCount() = 2 And counter = counter And counter <> missing And ..
	cycleValid And ..
	firstReachable = 5 And firstReportedReachable = 5 And firstUnreachable = 1 And ..
	secondReachable = 2 And reachableAfterCollection = 2 And unreachableAfterCollection = 0 And ..
	reclaimed = 4 And collectionsAfterInitial = 1 And ..
	initiallyReclaimedObjects = 4 And initiallyReclaimedBytes = 48 And ..
	liveAfterCollection = 2 And liveBytesAfterCollection = 24 And ..
	reusableAfterCollection = largestFreeAfterCollection And largestFreeAfterCollection > 48 And ..
	replacement.value = 7 And liveAfterReplacement = 3 And liveBytesAfterReplacement = 36 And ..
	usedBeforeCollection = usedAfterCollection And usedAfterCollection = usedAfterReplacement And ..
	reusableAfterReplacement > 0 And reusableAfterReplacement < reusableAfterCollection And ..
	stressPassed And automaticBeforeStress = 0 And automaticAfterStress > 0 And ..
	collectionsAfterStress = automaticAfterStress + 1 And ..
	finalReclaimed > 0 And CollectionCount() = automaticAfterStress + 2 And ..
	LastReclaimedObjectCount() = finalReclaimed And LastReclaimedBytes() = finalReclaimed * 12 And ..
	highWaterAfterStress > usedAfterReplacement And highWaterAfterStress <= ArenaCapacity() And ..
	reusableAfterStress > 0 And ..
	FinalizerPendingCount() = 0 And ..
	RootFrameCount() = 1 And RootSlotCount() = 6 And ..
	ObjectRootCount() = 0 And InvalidReferenceCount() = 0 And ..
	ArenaAllocationCount() = 2407 And ArenaUsed() >= 256 And ..
	ArenaHighWater() = ArenaUsed() And ArenaFailureCount() = 0 And ..
	ObjectAllocationCount() = 2407 And ObjectAllocatedBytes() = 28884 And ..
	ObjectLiveCount() = 3 And ObjectLiveBytes() = 36 And ..
	ReachableObjectCount() = 3 And UnreachableObjectCount() = 0 And ..
	ObjectFailureCount() = 0

If checksPassed Then
	PutString("Scalar Object checks passed")
Else
	PutString("Scalar Object check failed")
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
