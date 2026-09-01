SuperStrict

Framework BRL.StandardIO
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.Runtime.Memory
Import Pico.System.Time
Import Pico.Tests.GenericFinalizer

Function AllocateFinalizableGenericTypes()
	Local base:TGenericFinalizerBase<Int> = New TGenericFinalizerBase<Int>(10)
	Local derived:TGenericFinalizerDerived<Int> = New TGenericFinalizerDerived<Int>(20)
	Local inherited:TGenericInheritedFinalizer<Int> = New TGenericInheritedFinalizer<Int>(30)
End Function

ResetGenericFinalizerCounts()
Local baselineObjectAllocations:UInt = ObjectAllocationCount()
Local baselineLiveObjects:UInt = ObjectLiveCount()
AllocateFinalizableGenericTypes()

Local firstReclaimed:UInt = CollectObjects()
Local firstFinalized:UInt = LastFinalizedObjectCount()
Local baseAfterFinalizers:UInt = GenericBaseFinalizedCount()
Local derivedAfterFinalizers:UInt = GenericDerivedFinalizedCount()

Local secondReclaimed:UInt = CollectObjects()
Local checksPassed:Int = firstReclaimed = 0 And firstFinalized = 3 And ..
	baseAfterFinalizers = 3 And derivedAfterFinalizers = 1 And ..
	secondReclaimed = 3 And LastFinalizedObjectCount() = 0 And ..
	GenericBaseFinalizedCount() = 3 And GenericDerivedFinalizedCount() = 1 And ..
	FinalizerInvocationCount() = 3 And FinalizerPendingCount() = 0 And ..
	ObjectAllocationCount() = baselineObjectAllocations + 3 And ..
	ObjectLiveCount() = baselineLiveObjects And ..
	InvalidReferenceCount() = 0 And ObjectFailureCount() = 0

If checksPassed Then
	Print "Generic finalizer checks passed"
Else
	Print "Generic finalizer check failed"
End If

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	Delay 250
	GPIOPut(ledPin, False)
	Delay 250
Wend
