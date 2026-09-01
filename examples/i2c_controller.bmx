SuperStrict

Import BRL.StandardIO
Import Pico.Hardware.GPIO
Import Pico.Hardware.I2C

Local controller:Int = I2CDefaultController()
Local sdaPin:UInt = I2CDefaultSDAPin()
Local sclPin:UInt = I2CDefaultSCLPin()
Local checksPassed:Int = controller = I2CController0
checksPassed :& sdaPin = 4 And sclPin = 5
checksPassed :& I2CConfigurePins(controller, sdaPin, sclPin, True)
checksPassed :& GPIOGetFunction(sdaPin) = GPIOFunctionI2C
checksPassed :& GPIOGetFunction(sclPin) = GPIOFunctionI2C
checksPassed :& Not I2CConfigurePins(controller, 2, 3, True)

Local actualRate:UInt = I2CInit(controller, I2CSpeedStandard)
checksPassed :& actualRate >= 99000 And actualRate <= 101000
actualRate = I2CSetBaudrate(controller, I2CSpeedFast)
checksPassed :& actualRate >= 395000 And actualRate <= 405000
checksPassed :& I2CWriteAvailable(controller) = 16
checksPassed :& I2CReadAvailable(controller) = 0

' There is deliberately no peripheral at reserved address $7f. This exercises
' a bounded transfer and its Byte-array pointer without requiring extra wiring.
Local probe:Byte[1]
Local result:Int = I2CWriteTimeout(controller, $7f, probe, 1, False, 2000)
checksPassed :& result = I2CErrorGeneric Or result = I2CErrorTimeout
checksPassed :& I2CWriteTimeout(2, $40, probe, 1, False, 2000) = I2CErrorInvalidArgument

I2CDeinit(controller)
GPIODisablePulls(sdaPin)
GPIODisablePulls(sclPin)
GPIOSetFunction(sdaPin, GPIOFunctionNull)
GPIOSetFunction(sclPin, GPIOFunctionNull)

' The second controller is independent and uses its own valid pin pattern.
checksPassed :& I2CConfigurePins(I2CController1, 6, 7, False)
actualRate = I2CInit(I2CController1, I2CSpeedStandard)
checksPassed :& actualRate >= 99000 And actualRate <= 101000
checksPassed :& I2CWriteAvailable(I2CController1) = 16
I2CDeinit(I2CController1)
GPIOSetFunction(6, GPIOFunctionNull)
GPIOSetFunction(7, GPIOFunctionNull)

While True
	If checksPassed Then
		Print "I2C configuration, transfer timeout and buffer checks passed"
	Else
		Print "I2C configuration, transfer timeout or buffer check failed"
	End If
	Delay(1000)
Wend
