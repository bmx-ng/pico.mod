SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Interface IScore
	Method Score:Int(delta:Int)
End Interface

Interface IStatus Extends IScore
	Method Stable:Int()
End Interface

Interface ILabel
	Method NameText:String()
End Interface

Interface IMissing
	Method Missing:Int()
End Interface

Global baseDeletes:Int
Global leafDeletes:Int

Type TBase Implements IStatus
	Field value:Int
	Field label:String = "base"
	Field samples:Int[]

	Method New(value:Int)
		Self.value = value
		samples = New Int[2]
		samples[1] = value + 1
	End Method

	Method Score:Int(delta:Int)
		Return value + delta
	End Method

	Method Stable:Int()
		Return samples[1]
	End Method

	Method Delete()
		baseDeletes :+ 1
	End Method
End Type

Type TMiddle Extends TBase
	Field middle:Int

	Method New(value:Int)
		Super.New(value)
		middle = 3
	End Method

	Method Score:Int(delta:Int) Override
		Return Super.Score(delta) + middle
	End Method
End Type

Type TLeaf Extends TMiddle Implements ILabel
	Field leaf:Int

	Method New(value:Int)
		Super.New(value)
		leaf = 5
		label = "leaf" + Chr(33)
	End Method

	Method Score:Int(delta:Int) Override
		Return Super.Score(delta) + leaf
	End Method

	Method NameText:String()
		Return label
	End Method

	Method Delete()
		leafDeletes :+ 1
	End Method
End Type

StandardIOInit()

Local concrete:TLeaf = New TLeaf(10)
Local base:TBase = concrete
Local middle:TMiddle = concrete
Local score:IScore = concrete
Local status:IStatus = concrete
Local named:ILabel = concrete
Local objectValue:Object = concrete
Local recovered:IScore = IScore(objectValue)
Local recoveredLeaf:TLeaf = TLeaf(objectValue)
Local missing:IMissing = IMissing(objectValue)
Local contracts:IScore[] = New IScore[2]
contracts[1] = concrete
Local checksPassed:Int = base.Score(20) = 38 And middle.Score(20) = 38 And ..
	concrete.Score(20) = 38 And score.Score(20) = 38 And status.Score(20) = 38 And ..
	status.Stable() = 11 And named.NameText() = "leaf!" And recovered.Score(20) = 38 And ..
	recoveredLeaf.Score(20) = 38 And Not missing And contracts[1].Score(20) = 38 And ..
	base.Stable() = 11 And base.label = "leaf!"

Local transient:TBase
For Local index:Int = 0 Until 900
	transient = New TLeaf(index)
Next
CollectObjects()
CollectObjects()
ReachabilityAudit()
checksPassed :& AutomaticCollectionCount() > 0 And InvalidReferenceCount() = 0 And ..
	ObjectFailureCount() = 0 And ArrayFailureCount() = 0 And ..
	leafDeletes > 0 And baseDeletes = leafDeletes And ..
	base.Score(20) = 38 And base.Stable() = 11 And base.label = "leaf!" And ..
	score.Score(20) = 38 And named.NameText() = "leaf!" And contracts[1].Score(20) = 38 And ..
	transient.Score(1) = 908

If checksPassed Then
	PutString("Inheritance checks passed")
Else
	PutString("Inheritance check failed")
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
