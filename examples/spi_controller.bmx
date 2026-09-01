SuperStrict

Import BRL.StandardIO
Import Pico.Hardware.GPIO
Import Pico.Hardware.SPI

Local controller:Int = SPIDefaultController()
Local rxPin:UInt = SPIDefaultRXPin()
Local txPin:UInt = SPIDefaultTXPin()
Local clockPin:UInt = SPIDefaultClockPin()
Local chipSelectPin:UInt = SPIDefaultChipSelectPin()
Local checksPassed:Int = controller = SPIController0
checksPassed :& rxPin = 16 And txPin = 19 And clockPin = 18 And chipSelectPin = 17
checksPassed :& SPIConfigurePins(controller, rxPin, txPin, clockPin)
checksPassed :& Not SPIConfigurePins(controller, 12, 15, 14)
checksPassed :& GPIOGetFunction(rxPin) = GPIOFunctionSPI
checksPassed :& GPIOGetFunction(txPin) = GPIOFunctionSPI
checksPassed :& GPIOGetFunction(clockPin) = GPIOFunctionSPI

GPIOInit(chipSelectPin)
GPIOSetOutput(chipSelectPin)
GPIOPut(chipSelectPin, True)

Local actualRate:UInt = SPIInit(controller, 1000000)
checksPassed :& actualRate >= 990000 And actualRate <= 1010000
checksPassed :& SPIGetBaudrate(controller) = actualRate
checksPassed :& SPISetFormat(controller, 8, SPIClockPolarity0, SPIClockPhase0, SPIBitOrderMSBFirst)
checksPassed :& Not SPISetFormat(controller, 8, SPIClockPolarity0, SPIClockPhase0, SPIBitOrderLSBFirst)
checksPassed :& SPIIsWritable(controller)

' SPI supplies its own clock, so these transfers complete even without a
' peripheral. RX data is intentionally not interpreted while MISO is floating.
Local outgoing:Byte[4]
Local incoming:Byte[4]
outgoing[0] = $9f
outgoing[1] = $aa
outgoing[2] = $55
outgoing[3] = $00
GPIOPut(chipSelectPin, False)
checksPassed :& SPIWriteReadBlocking(controller, outgoing, incoming, 4) = 4
checksPassed :& SPIWriteBlocking(controller, outgoing, 4) = 4
checksPassed :& SPIReadBlocking(controller, $ff, incoming, 4) = 4
GPIOPut(chipSelectPin, True)
checksPassed :& Not SPIIsBusy(controller)
checksPassed :& Not SPIIsReadable(controller)
checksPassed :& SPIWriteBlocking(2, outgoing, 1) = SPIErrorInvalidArgument

' The typed Short Ptr boundary drives the same controller in 16-bit mode.
Local outgoing16:Short[2]
Local incoming16:Short[2]
outgoing16[0] = $1234
outgoing16[1] = $55aa
checksPassed :& SPISetFormat(controller, 16, SPIClockPolarity1, SPIClockPhase1, SPIBitOrderMSBFirst)
GPIOPut(chipSelectPin, False)
checksPassed :& SPIWrite16Read16Blocking(controller, outgoing16, incoming16, 2) = 2
checksPassed :& SPIWrite16Blocking(controller, outgoing16, 2) = 2
checksPassed :& SPIRead16Blocking(controller, $ffff, incoming16, 2) = 2
GPIOPut(chipSelectPin, True)
checksPassed :& Not SPIIsBusy(controller)

SPIDeinit(controller)
GPIOSetFunction(rxPin, GPIOFunctionNull)
GPIOSetFunction(txPin, GPIOFunctionNull)
GPIOSetFunction(clockPin, GPIOFunctionNull)
GPIOSetInput(chipSelectPin)

' Exercise independent configuration of the second controller as well.
checksPassed :& SPIConfigurePins(SPIController1, 12, 15, 14)
actualRate = SPIInit(SPIController1, 2000000)
checksPassed :& actualRate >= 1950000 And actualRate <= 2050000
SPIDeinit(SPIController1)
GPIOSetFunction(12, GPIOFunctionNull)
GPIOSetFunction(15, GPIOFunctionNull)
GPIOSetFunction(14, GPIOFunctionNull)

While True
	If checksPassed Then
		Print "SPI configuration, clocks and buffer transfers passed"
	Else
		Print "SPI configuration, clocks or buffer transfer failed"
	End If
	Delay(1000)
Wend
