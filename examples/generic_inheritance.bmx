SuperStrict

Framework BRL.StandardIO
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.Runtime.Memory
Import Pico.System.Time
Import Pico.Tests.GenericInheritance

Type TGenericDerived<T> Extends TGenericBase<T>
	Field derivedValue:T

	Method New(baseValue:T, derivedValue:T)
		Super.New(baseValue)
		Self.derivedValue = derivedValue
	End Method

	Method Value:T() Override
		Return derivedValue
	End Method

	Method OriginalValue:T()
		Return Super.Value()
	End Method
End Type

Local baseText:String = "base" + "-managed"
Local derivedText:String = "derived" + "-managed"
Local derived:TGenericDerived<String> = New TGenericDerived<String>(baseText, derivedText)
Local forwarded:TGenericDerived<String> = New TGenericDerived<String>("forwarded")
Local base:TGenericBase<String> = derived
Local recovered:TGenericDerived<String> = TGenericDerived<String>(base)
Local contract:IGenericValue<String> = derived

CollectObjects()
Local checksPassed:Int = derived.baseValue = "base-managed" And ..
	derived.derivedValue = "derived-managed" And ..
	forwarded.baseValue = "forwarded" And forwarded.derivedValue = "" And ..
	recovered = derived And ..
	base.Value() = "derived-managed" And ..
	derived.Dispatch() = "derived-managed" And ..
	derived.OriginalValue() = "base-managed" And ..
	contract.Value() = "derived-managed" And ..
	InvalidReferenceCount() = 0

If checksPassed Then
	Print "Generic inheritance and virtual dispatch checks passed"
Else
	Print "Generic inheritance or virtual dispatch check failed"
End If

Local ledPin:UInt = DefaultLEDPin()
GPIOInit(ledPin)
GPIOSetOutput(ledPin)

While True
	GPIOPut(ledPin, checksPassed)
	Delay 250
	GPIOPut(ledPin, False)
	Delay 250
Wend
