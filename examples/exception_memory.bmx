SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Interface ITaggedProblem
	Method Tag:Int()
End Interface

Global problemDeletes:Int
Global finallyCount:Int

Type TProblem
	Field code:Int
	Field message:String
	Field payload:Int[]

	Method New(code:Int)
		Self.code = code
		message = "problem" + Chr(33)
		payload = New Int[2]
		payload[1] = code + 1
	End Method

	Method Delete()
		problemDeletes :+ 1
	End Method
End Type

Type TSpecialProblem Extends TProblem Implements ITaggedProblem
	Method New(code:Int)
		Super.New(code)
	End Method

	Method Tag:Int()
		Return code + 100
	End Method
End Type

Function RaiseSpecial(code:Int)
	Local problem:TSpecialProblem = New TSpecialProblem(code)
	Throw problem
End Function

Function RaiseDynamicString()
	Throw "dynamic" + Chr(33)
End Function

Function RaiseArray()
	Local values:Int[] = New Int[2]
	values[0] = 21
	values[1] = 22
	Throw values
End Function

Function ReturnThroughFinally:Int()
	Try
		Return 41
	Finally
		finallyCount :+ 1
	End Try
End Function

Function AllocatePressure(count:Int)
	Local transient:TProblem
	For Local index:Int = 0 Until count
		transient = New TProblem(index)
	Next
	CollectObjects()
End Function

StandardIOInit()

Local checksPassed:Int = True
Local rootFramesBefore:UInt = RootFrameCount()
Local rootSlotsBefore:UInt = RootSlotCount()

' A Throw crosses another function and is selected by its most specific class.
Try
	RaiseSpecial(7)
Catch special:TSpecialProblem
	AllocatePressure(700)
	checksPassed :& special.code = 7 And special.payload[1] = 8 And special.message = "problem!"
Catch problem:TProblem
	checksPassed = False
End Try

' Interface matching uses the same generated type descriptors as ordinary casts.
Try
	RaiseSpecial(9)
Catch tagged:ITaggedProblem
	checksPassed :& tagged.Tag() = 109
End Try

' The inner handler is already removed when a catch rethrows to its outer handler.
Try
	Try
		RaiseSpecial(11)
	Catch special:TSpecialProblem
		Throw special
	End Try
Catch problem:TProblem
	checksPassed :& problem.code = 11
End Try

' A pending exception is rooted while Finally allocates and collects.
Try
	Try
		RaiseSpecial(13)
	Finally
		finallyCount :+ 1
		AllocatePressure(700)
	End Try
Catch problem:TProblem
	checksPassed :& problem.code = 13 And problem.payload[1] = 14
End Try

checksPassed :& ReturnThroughFinally() = 41 And finallyCount = 2

' The tagged pending carrier traces a dynamic String throughout Finally.
Try
	Try
		RaiseDynamicString()
	Finally
		finallyCount :+ 1
		AllocatePressure(700)
	End Try
Catch message:String
	checksPassed :& message = "dynamic!"
End Try

' Array catches retain the existing runtime's category-based Array matching.
Try
	RaiseArray()
Catch values:Int[]
	AllocatePressure(700)
	checksPassed :& values.length = 2 And values[0] + values[1] = 43
End Try

checksPassed :& finallyCount = 3
CollectObjects()
CollectObjects()
ReachabilityAudit()

checksPassed :& ExceptionDepth() = 0 And ExceptionThrowCount() = 9 And ..
	ExceptionCatchCount() = 9 And ExceptionMaxDepth() >= 2 And ..
	ExceptionUnhandledCount() = 0 And RootFrameCount() = rootFramesBefore And ..
	RootSlotCount() = rootSlotsBefore And InvalidReferenceCount() = 0 And ..
	ObjectFailureCount() = 0 And problemDeletes > 0

If checksPassed Then
	PutString("Exception checks passed")
Else
	PutString("Exception check failed")
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
