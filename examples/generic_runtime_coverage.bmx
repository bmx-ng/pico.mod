SuperStrict

Framework BRL.StandardIO
Import BRL.Blitz
?pico
Import Pico.Runtime.Memory
?

Enum EGenericMode
	Disabled
	Enabled
End Enum

Type TGenericResource Implements ICloseable
	Global closeCount:Int

	Method Close()
		closeCount :+ 1
	End Method
End Type

Struct SGenericCell<T>
	Field value:T
End Struct

Type TGenericRuntimeCoverage<T>
	Field text:String
	Field mode:EGenericMode
	Field retainedCell:SGenericCell<T>
	Field retainedCells:SGenericCell<T>[]

	Method NumericAndArrays:Int()
		Local byteValue:Byte = Byte(255)
		Local shortValue:Short = Short(65535)
		Local left:Int[] = [1, 2]
		Local right:Int[] = [3]
		Local joined:Int[] = left + right
		text = String(Int(byteValue)) + ":" + String(Int(shortValue))
		mode = EGenericMode.Enabled
		Return joined.Length + joined[2] + Int(mode)
	End Method

	Method UsingReturn:Int()
		Using
			Local resource:TGenericResource = New TGenericResource
			Do
				Return 42
		End Using
	End Method

	Method FinallyReturn:Int()
		Try
			text = "protected"
			Return 42
		Finally
			text = "finalized"
		End Try
	End Method

	Method CatchString:Int()
		Try
			Throw "expected"
		Catch caught:String
			text = caught
			Return caught.Length
		End Try
	End Method

	Method RethrowString:Int()
		Try
			Try
				Throw "reraised"
			Catch ignored:Int[]
				Return 0
			End Try
		Catch caught:String
			text = caught
			Return caught.Length
		End Try
	End Method

	Method ManagedStructArrays:Int(value:T)
		Local cells:SGenericCell<T>[] = New SGenericCell<T>[2]
		cells[1].value = value
		Local expanded:SGenericCell<T>[] = cells[-1..3]
		retainedCell = cells[1]
		retainedCells = expanded
		Return expanded.Length
	End Method

	Method RetainedValue:T()
		Return retainedCell.value
	End Method

	Method RetainedArrayValue:T()
		Return retainedCells[2].value
	End Method
End Type

Local coverage:TGenericRuntimeCoverage<Int> = New TGenericRuntimeCoverage<Int>
Local checksPassed:Int = coverage.NumericAndArrays() = 7
checksPassed :& coverage.text = "255:65535"
checksPassed :& coverage.UsingReturn() = 42
checksPassed :& TGenericResource.closeCount = 1
checksPassed :& coverage.FinallyReturn() = 42
checksPassed :& coverage.text = "finalized"
checksPassed :& coverage.CatchString() = 8
checksPassed :& coverage.text = "expected"
checksPassed :& coverage.RethrowString() = 8
checksPassed :& coverage.text = "reraised"

Local structCoverage:TGenericRuntimeCoverage<String> = New TGenericRuntimeCoverage<String>
checksPassed :& structCoverage.ManagedStructArrays("retained") = 4
checksPassed :& structCoverage.retainedCells[0].value = ""
checksPassed :& structCoverage.retainedCells[3].value = ""
?pico
CollectObjects()
checksPassed :& InvalidReferenceCount() = 0
?
checksPassed :& structCoverage.RetainedValue() = "retained"
checksPassed :& structCoverage.RetainedArrayValue() = "retained"

If checksPassed Then
	Print "Generic Pico runtime coverage passed"
Else
	Print "Generic Pico runtime coverage failed"
End If

?pico
While True
	Delay 1000
	If checksPassed Then
		Print "Generic Pico runtime coverage passed"
	Else
		Print "Generic Pico runtime coverage failed"
	End If
Wend
?
