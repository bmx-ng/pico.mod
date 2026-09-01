SuperStrict

Import BRL.StandardIO
Import Pico.Board.Pico2
Import Pico.Hardware.GPIO

Local ledPin:UInt = DefaultLEDPin()
Local checksPassed:Int = True

GPIOInit(ledPin)
checksPassed :& GPIOGetFunction(ledPin) = GPIOFunctionSIO
GPIOSetDirection(ledPin, GPIOOutput)
checksPassed :& GPIOGetDirection(ledPin) = GPIOOutput

GPIOSetDriveStrength(ledPin, GPIODriveStrength4mA)
checksPassed :& GPIOGetDriveStrength(ledPin) = GPIODriveStrength4mA
GPIOSetSlewRate(ledPin, GPIOSlewRateSlow)
checksPassed :& GPIOGetSlewRate(ledPin) = GPIOSlewRateSlow

' The native ISR only latches these bits; no managed BlitzMax code runs in it.
checksPassed :& GPIOSetIRQEnabled(ledPin, GPIOIRQEdgeRise | GPIOIRQEdgeFall, True)
GPIOPut(ledPin, False)
Delay(2)
GPIOTakeIRQEvents(ledPin)
GPIOPut(ledPin, True)
Delay(2)
Local riseEvents:UInt = GPIOTakeIRQEvents(ledPin)
GPIOPut(ledPin, False)
Delay(2)
Local fallEvents:UInt = GPIOTakeIRQEvents(ledPin)
GPIOSetIRQEnabled(ledPin, GPIOIRQEdgeRise | GPIOIRQEdgeFall, False)

checksPassed :& (riseEvents & GPIOIRQEdgeRise) <> 0
checksPassed :& (fallEvents & GPIOIRQEdgeFall) <> 0
checksPassed :& GPIOGetOutput(ledPin) = False

' Exercise pull configuration after returning the pin to input mode.
GPIOSetInput(ledPin)
GPIOPullUp(ledPin)
checksPassed :& GPIOIsPulledUp(ledPin) And Not GPIOIsPulledDown(ledPin)
GPIODisablePulls(ledPin)
checksPassed :& Not GPIOIsPulledUp(ledPin) And Not GPIOIsPulledDown(ledPin)

GPIOSetOutput(ledPin)
GPIOPut(ledPin, checksPassed)

While True
	If checksPassed Then
		Print "GPIO input, configuration and IRQ latch checks passed"
	Else
		Print "GPIO input, configuration or IRQ latch check failed"
	End If
	Delay(1000)
Wend
