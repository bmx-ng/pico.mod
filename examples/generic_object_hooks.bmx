SuperStrict

Framework BRL.StandardIO
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO
Import Pico.System.Time
Import Pico.Tests.GenericObjectHooks

ResetGenericObjectHookCounts()

Local baseValue:Object = New TGenericHookBase<Int>
Local derivedValue:Object = New TGenericHookDerived<Int>
Local inheritedValue:Object = New TGenericHookInherited<Int>

Local baseCompare:Int = baseValue.Compare(derivedValue)
Local baseHashCode:UInt = baseValue.HashCode()
Local baseEquals:Int = baseValue.Equals(derivedValue)
Local derivedCompare:Int = derivedValue.Compare(baseValue)
Local derivedHashCode:UInt = derivedValue.HashCode()
Local derivedEquals:Int = derivedValue.Equals(baseValue)
Local inheritedCompare:Int = inheritedValue.Compare(baseValue)
Local inheritedHashCode:UInt = inheritedValue.HashCode()
Local inheritedEquals:Int = inheritedValue.Equals(baseValue)

Local checksPassed:Int = baseCompare = -10 And baseHashCode = 100 And baseEquals And ..
	derivedCompare = -20 And derivedHashCode = 200 And Not derivedEquals And ..
	inheritedCompare = -10 And inheritedHashCode = 100 And inheritedEquals And ..
	BaseGenericObjectHookCalls() = 6 And DerivedGenericObjectHookCalls() = 3

If checksPassed Then
	Print "Generic Object hook checks passed"
Else
	Print "Generic Object hook check failed"
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
