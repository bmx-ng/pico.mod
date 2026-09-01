SuperStrict

Import "../support/state.bmx"

Public

Struct TCounterShape
	Field width:Int
	Field height:Int

	Method New(width:Int, height:Int)
		Self.width = width
		Self.height = height
	End Method

	Method Area:Int()
		Return width * height
	End Method
End Struct

Struct TCounterMetadata
	Field title:String
	Field values:Int[]
	Field owner:Object
End Struct

Struct TCounterEnvelope
	Field shape:TCounterShape
	Field metadata:TCounterMetadata
	Field StaticArray history:TCounterMetadata[2]
End Struct

Interface ICounter
	Method Add:Int(delta:Int)
End Interface

Interface ILabelledCounter
	Method Label:String()
End Interface

Type TImportedCounter Implements ICounter, ILabelledCounter
	Field value:Int
	Field label:String
	Field shape:TCounterShape
	Field metadata:TCounterMetadata
	Field StaticArray checkpoints:TCounterMetadata[2]
	Field peer:TImportedCounter
	Field samples:Int[]

	Method New(initial:Int, label:String)
		value = initial
		Self.label = label
		shape = New TCounterShape(initial, 1)
		metadata.title = label + Chr(33)
		metadata.values = New Int[2]
		metadata.values[0] = initial
		metadata.values[1] = 2
		metadata.owner = Self
		checkpoints[0].title = label + ":fixed"
		checkpoints[0].values = metadata.values
		checkpoints[0].owner = Self
	End Method

	Method Add:Int(delta:Int)
		value :+ delta
		Return value
	End Method

	Method Current:Int()
		Return value
	End Method

	Method Label:String()
		Return label
	End Method

	Method Delete()
		RecordFinalizedCounter()
	End Method
End Type
