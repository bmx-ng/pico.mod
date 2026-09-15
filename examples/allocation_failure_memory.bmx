SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time
Import "allocation_failure_memory.c"

Extern "C"
	Function LanguageOOMBegin:Int() = "bmx_pico_language_oom_begin"
	Function LanguageOOMEnd() = "bmx_pico_language_oom_end"
End Extern

Type TAllocationProbe
	Field identifier:Int
	Field label:String
	Field values:Int[]
End Type

Global finalizerStarted:Int
Global finalizerCompleted:Int
Global finalizerFirst:TAllocationFinalizer
Global finalizerSecond:TAllocationFinalizer
Global localCatchFinalizer:TLocalCatchFinalizer
Global localCatchCompleted:Int

Type TAllocationFinalizer
	Field identifier:Int

	Method Delete()
		finalizerStarted :+ 1
		Local values:Int[] = New Int[64]
		values[0] = identifier
		finalizerCompleted :+ 1
	End Method
End Type

Type TLocalCatchFinalizer
	Method Delete()
		Try
			Throw "finalizer-local-catch"
		Catch message:String
			If message = "finalizer-local-catch" Then localCatchCompleted :+ 1
		End Try
	End Method
End Type

Function CatchObjectAllocationFailure:Int()
	If Not LanguageOOMBegin() Then Return False
	Local caught:String
	Try
		Local value:TAllocationProbe = New TAllocationProbe
	Catch message:String
		caught = message
	End Try
	LanguageOOMEnd()
	Return caught = "BlitzMax Object allocation failed"
End Function

Function CatchArrayAllocationFailure:Int()
	If Not LanguageOOMBegin() Then Return False
	Local caught:String
	Try
		Local values:Int[] = New Int[64]
	Catch message:String
		caught = message
	End Try
	LanguageOOMEnd()
	Return caught = "BlitzMax Array allocation failed"
End Function

Function CatchStringAllocationFailure:Int()
	If Not LanguageOOMBegin() Then Return False
	Local caught:String
	Local prefix:String = "managed"
	Try
		Local value:String = prefix + "-allocation"
	Catch message:String
		caught = message
	End Try
	LanguageOOMEnd()
	Return caught = "BlitzMax String allocation failed"
End Function

Function CatchFinalizerAllocationFailure:Int()
	finalizerStarted = 0
	finalizerCompleted = 0
	finalizerFirst = New TAllocationFinalizer
	finalizerFirst.identifier = 11
	finalizerSecond = New TAllocationFinalizer
	finalizerSecond.identifier = 22
	Local invocationsBefore:UInt = FinalizerInvocationCount()

	If Not LanguageOOMBegin() Then
		finalizerFirst = Null
		finalizerSecond = Null
		Return False
	End If
	finalizerFirst = Null
	finalizerSecond = Null

	Local caught:String
	Try
		CollectObjects()
	Catch message:String
		caught = message
	End Try
	Local pendingAfterThrow:UInt = FinalizerPendingCount()
	LanguageOOMEnd()

	Local resumedCollection:UInt = CollectObjects()
	Local resumedFinalizers:UInt = LastFinalizedObjectCount()
	Local reclaimed:UInt = CollectObjects()
	Return caught = "BlitzMax Array allocation failed" And pendingAfterThrow = 0 And ..
		resumedCollection = 0 And resumedFinalizers = 1 And reclaimed = 2 And ..
		finalizerStarted = 2 And finalizerCompleted = 1 And ..
		FinalizerInvocationCount() = invocationsBefore + 2 And ..
		FinalizerPendingCount() = 0 And HeapIntegrityValid()
End Function

Function CheckLocalFinalizerCatch:Int()
	localCatchCompleted = 0
	localCatchFinalizer = New TLocalCatchFinalizer
	Local invocationsBefore:UInt = FinalizerInvocationCount()
	localCatchFinalizer = Null
	Local finalized:UInt = CollectObjects()
	Local finalizedThisCycle:UInt = LastFinalizedObjectCount()
	Local reclaimed:UInt = CollectObjects()
	Return finalized = 0 And finalizedThisCycle = 1 And reclaimed = 1 And ..
		localCatchCompleted = 1 And FinalizerInvocationCount() = invocationsBefore + 1 And ..
		FinalizerPendingCount() = 0 And ExceptionDepth() = 0 And HeapIntegrityValid()
End Function

StandardIOInit()

Local throwsBefore:UInt = ExceptionThrowCount()
Local catchesBefore:UInt = ExceptionCatchCount()
Local checksPassed:Int = CatchObjectAllocationFailure() And ..
	CatchArrayAllocationFailure() And CatchStringAllocationFailure() And ..
	CatchFinalizerAllocationFailure() And CheckLocalFinalizerCatch()

Local recovered:TAllocationProbe = New TAllocationProbe
recovered.identifier = 42
recovered.label = "allocation-recovered"
recovered.values = New Int[4]
recovered.values[3] = recovered.identifier

checksPassed :& ExceptionThrowCount() = throwsBefore + 5 And ..
	ExceptionCatchCount() = catchesBefore + 5 And ExceptionDepth() = 0 And ..
	recovered.identifier = 42 And recovered.label = "allocation-recovered" And ..
	recovered.values[3] = 42 And HeapIntegrityValid() And InvalidReferenceCount() = 0

If checksPassed Then
	PutString("Allocation failure checks passed")
Else
	PutString("Allocation failure check failed")
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
