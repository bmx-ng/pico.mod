SuperStrict

Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time

Type TStringHolder
	Field value:String
End Type

Local message:String = "Hello from BlitzMax on Pico 2"
Local sameMessage:String = message.ToString()
Local messageHash:UInt = message.HashCode()
Local dynamicMessage:String = ("Hello" + " from ") + ("dynamic" + " Strings")
Local slice:String = dynamicMessage[11..18]
Local characters:String = Chr(80) + Chr(105)
Local values:String[] = New String[2]
values[0] = "array" + " root"
Local retainedArrayValue:String = values[0]

Local holder:TStringHolder = New TStringHolder
holder.value = "field" + " root"
Local retainedFieldValue:String = holder.value

StandardIOInit()
PutString(message)
PutCharacter(10)

Local firstAllocation:Byte Ptr = ArenaAllocate(24)
Local secondAllocation:Byte Ptr = ArenaAllocate(33)
Local valueChecksPassed:Int = message.Length = 29 And message = "Hello from BlitzMax on Pico 2" And ..
	message.Compare(sameMessage) = 0 And message.Compare("Hello") > 0 And ..
	message.Equals(sameMessage) And Not message.Equals("different") And ..
	messageHash = sameMessage.HashCode() And sameMessage = message And ..
	message[0] = 72 And Asc(message) = 72 And ..
	dynamicMessage = "Hello from dynamic Strings" And slice = "dynamic" And ..
	characters = "Pi" And "  Pico String  ".Trim() = "Pico String" And ..
	"one two one".FindLast("one") = 8 And "one two one".FindLast("one", 4) = 0 And ..
	"one two one".Replace("one", "1") = "1 two 1" And ..
	"Pico".ToLower() = "pico" And "Pico".ToUpper() = "PICO" And ..
	"Pico String".StartsWith("Pico") And "Pico String".EndsWith("String") And ..
	"Pico String".Contains("co St") And "ab".Replicate(3) = "ababab" And ..
	retainedArrayValue = "array root" And ..
	retainedFieldValue = "field root"
Local heapChecksPassed:Int = firstAllocation And secondAllocation And ..
	ArenaCapacity() = 16384 And ArenaUsed() >= 57 And ..
	ArenaRemaining() + ArenaUsed() = ArenaCapacity() And ..
	ArenaHighWater() = ArenaUsed() And ArenaAllocationCount() >= 11 And ..
	ArenaFailureCount() = 0 And StringFailureCount() = 0
Local checksPassed:Int = valueChecksPassed And heapChecksPassed

Local transient:String
For Local index:Int = 0 Until 600
	transient = "temporary" + " dynamic String"
Next
ReachabilityAudit()
Local allocationPressurePassed:Int = StringAllocationCount() >= 607
Local automaticCollectionPassed:Int = AutomaticCollectionCount() > 0
Local reachableStringsPassed:Int = ReachableStringCount() >= 7
Local containerTracingPassed:Int = values[0] = "array root" And holder.value = "field root"
Local stringBytesPassed:Int = StringLiveBytes() <= StringAllocatedBytes()
Local referenceAuditPassed:Int = InvalidReferenceCount() = 0
Local pressureChecksPassed:Int = allocationPressurePassed And automaticCollectionPassed And ..
	reachableStringsPassed And containerTracingPassed And stringBytesPassed And referenceAuditPassed
checksPassed :& pressureChecksPassed

If checksPassed Then
	PutString("String and arena checks passed")
Else
	PutString("String or arena check failed")
End If
PutCharacter(10)

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	If checksPassed Then
		PutCharacter(46)
	Else
		If Not valueChecksPassed Then PutCharacter(49)
		If Not heapChecksPassed Then PutCharacter(50)
		If Not allocationPressurePassed Then PutCharacter(51)
		If Not automaticCollectionPassed Then PutCharacter(52)
		If Not reachableStringsPassed Then PutCharacter(53)
		If Not containerTracingPassed Then PutCharacter(54)
		If Not stringBytesPassed Then PutCharacter(55)
		If Not referenceAuditPassed Then PutCharacter(56)
		PutCharacter(33)
	End If
	Delay(250)
	GPIOPut(ledPin, False)
	UDelay(250000)
Wend
