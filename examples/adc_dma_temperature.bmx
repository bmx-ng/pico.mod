SuperStrict

Import BRL.StandardIO
Import Pico.Hardware.ADC
Import Pico.Hardware.DMA

Const sampleCount:Int = 64

Local samples:Short[] = New Short[sampleCount]
Local checksPassed:Int = True

ADCInit()
ADCSetTemperatureSensorEnabled(True)
ADCSelectInput(ADCTemperatureInput)
ADCSetRoundRobin(0)
ADCSetClockDivider(48000.0)
ADCFIFODrain()
ADCFIFOSetup(True, True, 1, False, False)

checksPassed :& ADCFIFOAddress() <> Null And ADCDREQ() <> DMAForceDREQ()

While True
	Local transferPassed:Int = checksPassed
	Local transfer:TDMATransfer
	If transferPassed Then
		transfer = DMAReadInto(ADCFIFOAddress(), TDMABuffer.FromArray(samples), ADCDREQ())
		transferPassed :& transfer <> Null
		If transferPassed Then transferPassed = transfer.Start()
		If transferPassed Then ADCRun(True)
		If transferPassed Then transferPassed = transfer.Wait(2000)
		ADCRun(False)
		If transfer Then
			transferPassed :& transfer.TakeCompletionEvents() = 1
			transferPassed :& transfer.IsComplete() And Not transfer.IsBusy()
			transferPassed :& transfer.Remaining() = 0
			transfer.Close()
		End If
	End If

	Local minimum:UInt = ADCMaximumValue
	Local maximum:UInt
	Local total:UInt
	For Local index:Int = 0 Until sampleCount
		Local sample:UInt = UInt(samples[index])
		transferPassed :& sample <= ADCMaximumValue
		minimum = Min(minimum, sample)
		maximum = Max(maximum, sample)
		total :+ sample
	Next

	ADCFIFODrain()
	If transferPassed Then
		Print "ADC DMA passed: " + sampleCount + " samples, min=" + minimum + ..
			", max=" + maximum + ", average=" + (total / sampleCount)
	Else
		Print "ADC DMA sampling failed"
	End If
	Delay(1000)
Wend
