SuperStrict

Import BRL.Blitz
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Global closeCount:Int
Global closeOrder:Int

Type TTransient
	Field value:Int
End Type

Function AllocateClosePressure(count:Int)
	Local transient:TTransient
	For Local index:Int = 0 Until count
		transient = New TTransient
		transient.value = index
	Next
	CollectObjects()
End Function

Type TResource Implements ICloseable
	Field identifier:Int
	Field failOnClose:Int
	Field label:String
	Field values:Int[]

	Method New(identifier:Int, failOnClose:Int = False)
		Self.identifier = identifier
		Self.failOnClose = failOnClose
		label = "resource" + Chr(33)
		values = New Int[2]
		values[1] = identifier + 10
	End Method

	Method Close()
		AllocateClosePressure(120)
		closeCount :+ 1
		closeOrder = closeOrder * 10 + identifier
		If failOnClose Then Throw "close" + Chr(33)
	End Method
End Type

Function MakeResource:TResource(identifier:Int, fail:Int = False)
	If fail Then Throw "initializer" + Chr(33)
	Return New TResource(identifier)
End Function

Function ReturnThroughUsing:Int()
	Using
		Local resource:TResource = New TResource(6)
	Do
		Return 41
	End Using
End Function

StandardIOInit()

Local checksPassed:Int = True
Local rootFramesBefore:UInt = RootFrameCount()

' Normal cleanup is reverse declaration order.
closeOrder = 0
Using
	Local outer:TResource = New TResource(1)
	Local inner:TResource = New TResource(2)
Do
	checksPassed :& outer.values[1] = 11 And inner.label = "resource!"
End Using
checksPassed :& closeOrder = 21

' Close exceptions are suppressed and cannot replace the body exception.
closeOrder = 0
Try
	Using
		Local outer:TResource = New TResource(3)
		Local inner:TResource = New TResource(4, True)
	Do
		Throw "body" + Chr(33)
	End Using
Catch message:String
	checksPassed :& message = "body!"
End Try
checksPassed :& closeOrder = 43

' If a later initializer throws, only previously initialized resources close.
closeOrder = 0
Try
	Using
		Local initialized:TResource = MakeResource(5)
		Local missing:TResource = MakeResource(0, True)
	Do
		checksPassed = False
	End Using
Catch message:String
	checksPassed :& message = "initializer!"
End Try
checksPassed :& closeOrder = 5

' Non-local control flow executes the same cleanup edge.
closeOrder = 0
checksPassed :& ReturnThroughUsing() = 41 And closeOrder = 6

closeOrder = 0
#Outer
While True
	Using
		Local resource:TResource = New TResource(7)
	Do
		Exit Outer
	End Using
Wend
checksPassed :& closeOrder = 7

ReachabilityAudit()
checksPassed :& closeCount = 7 And ExceptionDepth() = 0 And ..
	ExceptionThrowCount() = 5 And ExceptionCatchCount() = 5 And ..
	ExceptionUnhandledCount() = 0 And RootFrameCount() = rootFramesBefore And ..
	InvalidReferenceCount() = 0 And ObjectFailureCount() = 0

If checksPassed Then
	PutString("Using checks passed")
Else
	PutString("Using check failed")
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
