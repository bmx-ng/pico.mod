SuperStrict

Import BRL.StandardIO
Import Pico.Hardware.GPIO
Import Pico.Hardware.UART

Local checksPassed:Int = UARTDefaultController() = UARTController0
checksPassed :& UARTDefaultTXPin() = 0 And UARTDefaultRXPin() = 1
checksPassed :& UARTDefaultBaudrate() = 115200
checksPassed :& UARTIsEnabled(UARTController0)

' UART1 stays independent of the UART0 console used by BRL.StandardIO.
Const txPin:UInt = 4
Const rxPin:UInt = 5
Const ctsPin:UInt = 6
Const rtsPin:UInt = 7
checksPassed :& UARTConfigurePins(UARTController1, txPin, rxPin)
checksPassed :& UARTConfigureFlowControlPins(UARTController1, ctsPin, rtsPin)
checksPassed :& Not UARTConfigurePins(UARTController0, txPin, rxPin)
checksPassed :& GPIOGetFunction(txPin) = GPIOFunctionUART
checksPassed :& GPIOGetFunction(rxPin) = GPIOFunctionUART
checksPassed :& GPIOGetFunction(ctsPin) = GPIOFunctionUART
checksPassed :& GPIOGetFunction(rtsPin) = GPIOFunctionUART

Local actualRate:UInt = UARTInit(UARTController1, 115200)
checksPassed :& actualRate >= 114000 And actualRate <= 116000
actualRate = UARTSetBaudrate(UARTController1, 230400)
checksPassed :& actualRate >= 228000 And actualRate <= 233000
checksPassed :& UARTSetFormat(UARTController1, 7, 2, UARTParityEven)
checksPassed :& UARTSetFormat(UARTController1, 8, 1, UARTParityNone)
checksPassed :& Not UARTSetFormat(UARTController1, 9, 1, UARTParityNone)
checksPassed :& UARTSetFlowControl(UARTController1, False, False)
checksPassed :& UARTSetFIFOEnabled(UARTController1, True)
checksPassed :& UARTIsEnabled(UARTController1)
checksPassed :& UARTIsWritable(UARTController1)

Local outgoing:Byte[5]
outgoing[0] = Asc("U")
outgoing[1] = Asc("A")
outgoing[2] = Asc("R")
outgoing[3] = Asc("T")
outgoing[4] = Asc("1")
checksPassed :& UARTWriteBlocking(UARTController1, outgoing, 5) = 5
checksPassed :& UARTPutByte(UARTController1, 10)
UARTTXWaitBlocking(UARTController1)

Local incoming:Byte[4]
checksPassed :& UARTReadTimeout(UARTController1, incoming, 4, 2000) = 0
checksPassed :& UARTReadAvailable(UARTController1, incoming, 4) = 0
checksPassed :& Not UARTIsReadable(UARTController1)
checksPassed :& Not UARTIsReadableWithin(UARTController1, 100)
checksPassed :& UARTGetErrors(UARTController1) = 0
UARTClearErrors(UARTController1)
checksPassed :& UARTSetBreak(UARTController1, True)
Delay(1)
checksPassed :& UARTSetBreak(UARTController1, False)
checksPassed :& UARTSetTranslateCRLF(UARTController1, False)
checksPassed :& UARTWriteBlocking(2, outgoing, 1) = UARTErrorInvalidArgument

UARTDeinit(UARTController1)
GPIOSetFunction(txPin, GPIOFunctionNull)
GPIOSetFunction(rxPin, GPIOFunctionNull)
GPIOSetFunction(ctsPin, GPIOFunctionNull)
GPIOSetFunction(rtsPin, GPIOFunctionNull)

' RP2350 also routes these signals through its UART_AUX pin function.
If UARTSupportsAuxiliaryPinMappings() Then
	checksPassed :& UARTConfigurePins(UARTController1, 6, 7)
	checksPassed :& GPIOGetFunction(6) = GPIOFunctionUARTAux
	checksPassed :& GPIOGetFunction(7) = GPIOFunctionUARTAux
	GPIOSetFunction(6, GPIOFunctionNull)
	GPIOSetFunction(7, GPIOFunctionNull)
	checksPassed :& UARTConfigureFlowControlPins(UARTController1, 4, 5)
	checksPassed :& GPIOGetFunction(4) = GPIOFunctionUARTAux
	checksPassed :& GPIOGetFunction(5) = GPIOFunctionUARTAux
	GPIOSetFunction(4, GPIOFunctionNull)
	GPIOSetFunction(5, GPIOFunctionNull)
End If

While True
	If checksPassed Then
		Print "UART configuration, transmission and timeout checks passed"
	Else
		Print "UART configuration, transmission or timeout check failed"
	End If
	Delay(1000)
Wend
