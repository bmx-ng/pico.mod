SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Type TNode
	Field value:Int
	Field nextNode:TNode

	Method New(value:Int)
		Self.value = value
	End Method
End Type

Type TBundle
	Field label:String
	Field values:Int[]
	Field names:String[]
	Field nodes:TNode[]

	Method New()
		label = "managed"
		values = New Int[3]
		values[0] = 4
		values[1] = 5
		values[2] = 6
		names = New String[2]
		names[0] = "left"
		names[1] = "right"
		nodes = New TNode[3]
		Local first:TNode = New TNode(1)
		Local second:TNode = New TNode(2)
		Local third:TNode = New TNode(3)
		first.nextNode = second
		second.nextNode = third
		third.nextNode = first
		nodes[0] = first
		nodes[1] = second
		nodes[2] = third
	End Method
End Type

Function SumValues:Int(values:Int[])
	Local total:Int
	For Local value:Int = EachIn values
		total :+ value
	Next
	Return total
End Function

Function BundleValid:Int(bundle:TBundle)
	Return bundle.label = "managed" And SumValues(bundle.values) = 15 And ..
		bundle.names[0] = "left" And bundle.names[1] = "right" And ..
		bundle.nodes[0].nextNode = bundle.nodes[1] And ..
		bundle.nodes[1].nextNode = bundle.nodes[2] And ..
		bundle.nodes[2].nextNode = bundle.nodes[0]
End Function

Function LinkCycle(nodes:TNode[])
	nodes[0].nextNode = nodes[1]
	nodes[1].nextNode = nodes[0]
End Function

StandardIOInit()

Local bundle:TBundle = New TBundle
Local bundleValid:Int = BundleValid(bundle)
Local bundleReachable:UInt = ReachabilityAudit()
Local bundleArrays:UInt = ReachableArrayCount()
Local retainedBundleObjects:UInt = CollectObjects()
bundle = Null
Local reclaimedBundleObjects:UInt = CollectObjects()

Local roots:TNode[] = New TNode[2]
Local defaultElements:Int = Not roots[0] And Not roots[1]
roots[0] = New TNode(10)
roots[1] = New TNode(11)
LinkCycle(roots)
Local arrayRootReachable:UInt = ReachabilityAudit()
Local arrayRoots:UInt = ReachableArrayCount()
Local retainedArrayObjects:UInt = CollectObjects()
roots = New TNode[0]
Local reclaimedArrayObjects:UInt = CollectObjects()

Local checksPassed:Int = bundleValid And defaultElements And ..
	bundleReachable = 4 And bundleArrays = 3 And retainedBundleObjects = 0 And ..
	reclaimedBundleObjects = 4 And ..
	arrayRootReachable = 2 And arrayRoots = 1 And retainedArrayObjects = 0 And ..
	reclaimedArrayObjects = 2 And ..
	CollectionCount() = 4 And AutomaticCollectionCount() = 0 And ..
	LastReclaimedObjectCount() = 2 And LastReclaimedBytes() = 24 And ..
	LastReclaimedArrayCount() = 1 And LastReclaimedArrayBytes() = 24 And ..
	ObjectAllocationCount() = 6 And ObjectAllocatedBytes() = 80 And ..
	ObjectLiveCount() = 0 And ObjectLiveBytes() = 0 And ..
	ReachableObjectCount() = 0 And UnreachableObjectCount() = 0 And ..
	ArrayAllocationCount() = 4 And ArrayAllocatedBytes() = 104 And ..
	ArrayLiveCount() = 0 And ArrayLiveBytes() = 0 And ..
	ReachableArrayCount() = 0 And UnreachableArrayCount() = 0 And ..
	RootFrameCount() = 1 And RootSlotCount() = 2 And ..
	ArenaAllocationCount() = 10 And HeapReusableBytes() > 0 And ..
	FinalizerPendingCount() = 0 And InvalidReferenceCount() = 0 And ..
	ArenaFailureCount() = 0 And ArrayFailureCount() = 0 And ObjectFailureCount() = 0

If checksPassed Then
	PutString("Managed container checks passed")
Else
	PutString("Managed container check failed")
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
