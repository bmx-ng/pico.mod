SuperStrict

Import BRL.StandardIO
Import Pico.Hardware.ADC

ADCInit()
ADCGPIOInit(26)
ADCSetTemperatureSensorEnabled(True)
ADCSelectInput(ADCTemperatureInput)
ADCSetRoundRobin(0)

Local checksPassed:Int = ADCGetSelectedInput() = ADCTemperatureInput
Local firstSample:UInt = ADCRead()
Local secondSample:UInt = ADCReadInput(ADCTemperatureInput)
checksPassed :& firstSample > 0 And firstSample <= ADCMaximumValue
checksPassed :& secondSample > 0 And secondSample <= ADCMaximumValue
checksPassed :& ADCInputForGPIO(26) = 0
checksPassed :& ADCInputForGPIO(29) = 3
checksPassed :& ADCInputForGPIO(25) = -1

' Exercise free-running sampling and the RP2350 FIFO at a modest rate.
ADCFIFODrain()
ADCFIFOSetup(True, False, 1, False, False)
ADCSetClockDivider(48000.0)
ADCRun(True)
Local blockingSample:UInt = ADCFIFOGetBlocking()
ADCRun(False)
checksPassed :& blockingSample <= ADCMaximumValue

' Let the FIFO fill so its RP2350 depth and non-blocking access are covered.
ADCRun(True)
Delay(5)
ADCRun(False)

Local fifoSamples:UInt = ADCFIFOLevel()
checksPassed :& fifoSamples > 0 And fifoSamples <= 8
While Not ADCFIFOIsEmpty()
	Local sample:UInt = ADCFIFOGet()
	checksPassed :& sample <= ADCMaximumValue
Wend
checksPassed :& ADCFIFOIsEmpty()
ADCFIFOSetup(False, False, 1, False, False)
ADCSetTemperatureSensorEnabled(False)

While True
	If checksPassed Then
		Print "ADC direct conversion and FIFO checks passed"
	Else
		Print "ADC direct conversion or FIFO check failed"
	End If
	Delay(1000)
Wend
