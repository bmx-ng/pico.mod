SuperStrict

Module Pico.Tests.ImportedTypes

Import "types/counter.bmx"

Private
Global StaticArray sharedMetadata:TCounterMetadata[2]

Public

Function CreateImportedCounter:TImportedCounter(initial:Int, label:String)
	Return New TImportedCounter(initial, label)
End Function

Function CreateCounterShape:TCounterShape(width:Int, height:Int)
	Return New TCounterShape(width, height)
End Function

Function ResizeCounterShape(shape:TCounterShape Var, width:Int, height:Int)
	shape.width = width
	shape.height = height
End Function

Function ConfigureSharedMetadata(owner:Object)
	sharedMetadata[0].title = "shared" + Chr(33)
	sharedMetadata[0].values = New Int[2]
	sharedMetadata[0].values[0] = 19
	sharedMetadata[0].values[1] = 23
	sharedMetadata[0].owner = owner
End Function

Function SharedMetadataTitle:String()
	Return sharedMetadata[0].title
End Function

Function SharedMetadataOwner:Object()
	Return sharedMetadata[0].owner
End Function

Function SharedMetadataTotal:Int()
	Return sharedMetadata[0].values[0] + sharedMetadata[0].values[1]
End Function
