SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Enum EMode:Byte
	Idle = 2
	Active = 7
End Enum

Enum EAccess:UInt Flags
	None = 0
	Read
	Write
	Execute
End Enum

Struct SPoint
	Field x:Int = 3
	Field y:Int = 4

	Method New(x:Int, y:Int)
		Self.x = x
		Self.y = y
	End Method

	Method Translate(dx:Int, dy:Int)
		x :+ dx
		y :+ dy
	End Method

	Method Sum:Int()
		Return x + y
	End Method
End Struct

Struct SBounds
	Field first:SPoint
	Field last:SPoint

	Method Total:Int()
		Return first.Sum() + last.Sum()
	End Method
End Struct

Type TTag
	Field code:Int
End Type

Struct SLabel
	Field text:String = "seed"
	Field values:Int[]
	Field tag:TTag
	Field aliases:String[2]
End Struct

Struct SEnvelope
	Field label:SLabel
End Struct

Type TGeometry
	Field origin:SPoint
	Field points:SPoint[]
	Field envelope:SEnvelope
End Type

StandardIOInit()

Local mode:EMode = EMode.Active
Local ordinal:Byte = mode.Ordinal()
Local modeText:String = mode.ToString()
Local enumValues:EMode[] = EMode.Values()
Local converted:EMode
Local convertedOk:Int = EMode.TryConvert(7, converted)
Local parsed:EMode = EMode.FromString("active")
Local access:EAccess = EAccess.Read | EAccess.Write
Local accessText:String = access.ToString()
Local parsedAccess:EAccess = EAccess.FromString("read|WRITE")
Local modes:EMode[] = New EMode[2]
modes[0] = EMode.Idle
modes[1] = mode

Local point:SPoint = New SPoint(19, 23)
point.Translate(1, -1)
Local copy:SPoint = point
Local bounds:SBounds
bounds.first = point
bounds.last = New SPoint(10, 11)

Local geometry:TGeometry = New TGeometry
geometry.points = New SPoint[3]
geometry.points[1] = copy
geometry.points[2] = bounds.last
Local tag:TTag = New TTag
tag.code = 91
Local label:SLabel
label.text = "kept" + Chr(33)
label.values = New Int[2]
label.values[1] = 77
label.tag = tag
label.aliases[1] = "alias" + Chr(63)
Local labels:SLabel[] = New SLabel[2]
labels[1] = label
geometry.envelope.label = labels[1]

Local enumTotal:Int
For Local item:EMode = EachIn modes
	enumTotal :+ Int(item)
Next

Local pointTotal:Int
For Local item:SPoint = EachIn geometry.points
	pointTotal :+ item.Sum()
Next

Local checksPassed:Int = ordinal = 7 And enumTotal = 9 And modeText = "Active" And ..
	enumValues.Length = 2 And enumValues[0] = EMode.Idle And enumValues[1] = EMode.Active And ..
	convertedOk And converted = EMode.Active And parsed = EMode.Active And ..
	accessText = "Read|Write" And parsedAccess = access And ..
	(access & EAccess.Read) = EAccess.Read And ..
	(access & EAccess.Execute) = EAccess.None And ..
	point.Sum() = 42 And copy.Sum() = 42 And bounds.Total() = 63 And ..
	geometry.origin.Sum() = 7 And geometry.points[0].Sum() = 7 And ..
	geometry.points[1].Sum() = 42 And geometry.points[2].Sum() = 21 And ..
	pointTotal = 70 And labels[0].text = "seed" And ..
	geometry.envelope.label.text = "kept!" And geometry.envelope.label.values[1] = 77 And ..
	geometry.envelope.label.tag.code = 91 And geometry.envelope.label.aliases[1] = "alias?"

Local transient:SPoint[]
For Local index:Int = 0 Until 400
	transient = New SPoint[4]
	transient[3] = New SPoint(index, 1)
Next
ReachabilityAudit()
checksPassed :& AutomaticCollectionCount() > 0 And ArrayFailureCount() = 0 And ..
	ObjectFailureCount() = 0 And EnumFailureCount() = 0 And InvalidReferenceCount() = 0 And ..
	ReachableArrayCount() >= 4 And geometry.points[1].Sum() = 42 And ..
	modes[1] = EMode.Active And transient[3].Sum() = 400 And ..
	labels[1].text = "kept!" And labels[1].values[1] = 77 And labels[1].tag.code = 91 And ..
	labels[1].aliases[1] = "alias?" And geometry.envelope.label.text = "kept!"

If checksPassed Then
	PutString("Struct and Enum checks passed")
Else
	PutString("Struct or Enum check failed")
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
