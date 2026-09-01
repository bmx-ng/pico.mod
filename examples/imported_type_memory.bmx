SuperStrict

Import BRL.Blitz
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.IO.StandardIO
Import Pico.Runtime.Memory
Import Pico.System.Time
Import Pico.Tests.DerivedTypes
Import Pico.Tests.ImportedTypes

Global finalizedLeafCounters:Int

Type TLeafCounter Extends TModuleCounter
	Field leafBonus:Int

	Method New(initial:Int, label:String, bonus:Int, leafBonus:Int)
		Super.New(initial, label, bonus)
		Self.leafBonus = leafBonus
	End Method

	Method Add:Int(delta:Int) Override
		Return Super.Add(delta) + leafBonus
	End Method

	Method Delete()
		finalizedLeafCounters :+ 1
	End Method
End Type

StandardIOInit()

Local checksPassed:Int = True
Local shape:TCounterShape = New TCounterShape(6, 7)
Local shapeCopy:TCounterShape = shape
shapeCopy.width = 7
shapeCopy.height = 6
checksPassed :& shape.Area() = 42 And shapeCopy.Area() = 42
Local producedShape:TCounterShape = CreateCounterShape(5, 8)
ResizeCounterShape(producedShape, 6, 7)
checksPassed :& producedShape.Area() = 42
Local shapes:TCounterShape[] = New TCounterShape[2]
shapes[0] = New TCounterShape(6, 7)
shapes[1] = CreateCounterShape(7, 6)
checksPassed :& shapes[0].Area() = 42 And shapes[1].Area() = 42
Local direct:TImportedCounter = New TImportedCounter(40, "direct")
Local created:TImportedCounter = CreateImportedCounter(5, "factory")
ConfigureSharedMetadata(direct)
Local metadataValues:TCounterMetadata[] = New TCounterMetadata[2]
metadataValues[0].title = "array" + Chr(33)
metadataValues[0].values = New Int[2]
metadataValues[0].values[0] = 19
metadataValues[0].values[1] = 23
metadataValues[0].owner = direct
metadataValues[1] = direct.metadata
Local StaticArray fixedMetadata:TCounterMetadata[2]
fixedMetadata[0] = metadataValues[0]
fixedMetadata[1] = direct.metadata

checksPassed :& direct.value = 40 And direct.label = "direct"
checksPassed :& direct.shape.Area() = 40
checksPassed :& direct.metadata.title = "direct!" And direct.metadata.values[0] = 40
checksPassed :& direct.checkpoints[0].title = "direct:fixed" And direct.checkpoints[0].owner = direct
direct.value = 41
direct.label = "direct" + Chr(33)
direct.shape.width = 6
direct.shape.height = 7
created.label = "factory" + Chr(33)
direct.peer = created
direct.samples = New Int[2]
direct.samples[0] = 19
direct.samples[1] = 23
checksPassed :& direct.Add(1) = 42
checksPassed :& created.Current() = 5 And direct.peer = created
checksPassed :& direct.Label() = "direct!" And created.Label() = "factory!"
checksPassed :& direct.samples[0] + direct.samples[1] = 42
checksPassed :& direct.shape.Area() = 42
checksPassed :& ProducerLabel() = "producer!"
checksPassed :& SharedMetadataTitle() = "shared!" And SharedMetadataOwner() = direct And SharedMetadataTotal() = 42

Local moduleCounter:TModuleCounter = CreateModuleCounter(10, "module child", 2)
Local moduleAsBase:TImportedCounter = moduleCounter
Local moduleAsCounter:ICounter = moduleCounter
Local moduleAsLabelled:ILabelledCounter = moduleCounter
checksPassed :& moduleAsBase.Add(30) = 42
checksPassed :& moduleAsCounter.Add(0) = 42
checksPassed :& moduleAsLabelled.Label() = "module child:module!"
checksPassed :& moduleCounter.Snapshot() = 12
checksPassed :& moduleCounter.envelope.shape.Area() = 20
checksPassed :& moduleCounter.envelope.metadata.title = "module child:nested"
checksPassed :& moduleCounter.envelope.history[0].owner = moduleCounter
checksPassed :& moduleCounter.fixedMetadata[0].title = "module child:nested"

Local leaf:TLeafCounter = New TLeafCounter(10, "leaf", 2, 3)
Local leafAsModule:TModuleCounter = leaf
Local leafAsBase:TImportedCounter = leaf
Local leafAsCounter:ICounter = leaf
Local leafAsLabelled:ILabelledCounter = leaf
checksPassed :& leafAsBase.Add(27) = 42
checksPassed :& leafAsCounter.Add(0) = 42
checksPassed :& leafAsLabelled.Label() = "leaf:module!"
checksPassed :& leafAsModule.Snapshot() = 12
checksPassed :& leaf.envelope.metadata.values[0] + leaf.envelope.metadata.values[1] = 12
checksPassed :& TLeafCounter(leafAsBase) = leaf
checksPassed :& TModuleCounter(leafAsBase) = leafAsModule
Local derivedBoxed:Object = leaf
checksPassed :& TImportedCounter(derivedBoxed) = leafAsBase
Try
	Throw leaf
Catch caughtModule:TModuleCounter
	checksPassed :& caughtModule.Add(0) = 42
End Try
leaf.samples = New Int[2]
leaf.samples[0] = 20
leaf.samples[1] = 22

Local boxed:Object = direct
Local recovered:TImportedCounter = TImportedCounter(boxed)
checksPassed :& recovered.Current() = 42

Try
	Throw created
Catch caught:TImportedCounter
	checksPassed :& caught.Add(36) = 41
End Try

Local transient:TImportedCounter
For Local index:Int = 0 Until 240
	transient = CreateImportedCounter(index, "temporary")
Next
transient = Null
Local moduleTransient:TModuleCounter
For Local index:Int = 0 Until 80
	moduleTransient = New TModuleCounter(index, "module temporary", 1)
Next
moduleTransient = Null
Local leafTransient:TLeafCounter
For Local index:Int = 0 Until 40
	leafTransient = New TLeafCounter(index, "leaf temporary", 1, 1)
Next
leafTransient = Null
CollectObjects()
checksPassed :& FinalizedCounterCount() = 360
checksPassed :& FinalizedModuleCounterCount() = 120
checksPassed :& finalizedLeafCounters = 40
checksPassed :& direct.Current() = 42 And created.Current() = 41
checksPassed :& direct.label = "direct!" And direct.peer.Current() = 41
checksPassed :& direct.samples[0] + direct.samples[1] = 42
checksPassed :& direct.shape.Area() = 42
checksPassed :& direct.metadata.title = "direct!" And direct.metadata.values[0] = 40
checksPassed :& shapes[0].Area() = 42 And shapes[1].Area() = 42
checksPassed :& metadataValues[0].title = "array!" And metadataValues[0].values[0] + metadataValues[0].values[1] = 42
checksPassed :& TImportedCounter(metadataValues[0].owner) = direct
checksPassed :& metadataValues[1].title = "direct!" And metadataValues[1].owner = direct
checksPassed :& fixedMetadata[0].title = "array!" And fixedMetadata[0].owner = direct
checksPassed :& fixedMetadata[1].title = "direct!" And fixedMetadata[1].values[0] = 40
checksPassed :& moduleAsBase.Current() = 40 And moduleCounter.note = "module!"
checksPassed :& moduleCounter.envelope.metadata.title = "module child:nested"
checksPassed :& moduleCounter.envelope.history[0].title = "module child:nested"
checksPassed :& moduleCounter.fixedMetadata[0].owner = moduleCounter
checksPassed :& leafAsBase.Current() = 37 And leaf.note = "module!"
checksPassed :& leaf.envelope.metadata.owner = leaf
checksPassed :& leaf.samples[0] + leaf.samples[1] = 42
checksPassed :& ProducerLabel() = "producer!"
checksPassed :& SharedMetadataTitle() = "shared!" And SharedMetadataOwner() = direct And SharedMetadataTotal() = 42
checksPassed :& InvalidReferenceCount() = 0 And ObjectFailureCount() = 0

If checksPassed Then
	PutString("Imported Type module checks passed")
Else
	PutString("Imported Type module check failed")
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
