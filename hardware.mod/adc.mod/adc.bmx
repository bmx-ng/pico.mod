SuperStrict

Rem
bbdoc: Analogue-to-digital conversion for Raspberry Pi Pico targets.
End Rem
Module Pico.Hardware.ADC

ModuleInfo "Version: 0.2"
ModuleInfo "License: zlib/libpng"

' Pico 2 uses the RP2350A package: GPIO 26-29 are inputs 0-3 and the internal
' temperature sensor is input 4. ADCInputForGPIO avoids assuming that mapping
' in application code and can support other RP2350 packages later.
Const ADCInput0:UInt = 0
Const ADCInput1:UInt = 1
Const ADCInput2:UInt = 2
Const ADCInput3:UInt = 3
Const ADCTemperatureInput:UInt = 4
Const ADCResolutionBits:UInt = 12
Const ADCMaximumValue:UInt = $fff
Const ADCFIFOErrorBit:UInt = $8000

Extern "C"
	Function ADCInit() = "bmx_pico_adc_init"
	Function ADCGPIOInit(pin:UInt) = "bmx_pico_adc_gpio_init"
	Function ADCInputForGPIO:Int(pin:UInt) = "bmx_pico_adc_input_for_gpio"
	Function ADCSelectInput(input:UInt) = "bmx_pico_adc_select_input"
	Function ADCGetSelectedInput:UInt() = "bmx_pico_adc_get_selected_input"
	Function ADCSetRoundRobin(inputMask:UInt) = "bmx_pico_adc_set_round_robin"
	Function ADCSetTemperatureSensorEnabled(enabled:Int) = "bmx_pico_adc_set_temperature_sensor_enabled"
	Function ADCRead:UInt() = "bmx_pico_adc_read"
	Function ADCReadInput:UInt(input:UInt) = "bmx_pico_adc_read_input"
	Function ADCRun(enabled:Int) = "bmx_pico_adc_run"
	Function ADCSetClockDivider(divider:Float) = "bmx_pico_adc_set_clock_divider"

	Rem
	bbdoc: Configures the ADC conversion FIFO and its optional DMA request.
	about: FIFO depth is target-specific. DMA buffer ownership and transfer
	control are outside this module; dmaRequestEnabled should remain false until
	a DMA channel has been configured or is ready to start.
	End Rem
	Function ADCFIFOSetup(enabled:Int, dmaRequestEnabled:Int, dmaRequestThreshold:UInt, errorInFIFO:Int, byteShift:Int) = "bmx_pico_adc_fifo_setup"
	Function ADCFIFOIsEmpty:Int() = "bmx_pico_adc_fifo_is_empty"
	Function ADCFIFOLevel:UInt() = "bmx_pico_adc_fifo_level"
	Function ADCFIFOGet:UInt() = "bmx_pico_adc_fifo_get"
	Function ADCFIFOGetBlocking:UInt() = "bmx_pico_adc_fifo_get_blocking"
	Function ADCFIFODrain() = "bmx_pico_adc_fifo_drain"

	Rem
	bbdoc: Returns the address of the ADC FIFO register for peripheral-to-memory DMA.
	about: Configure a 16-bit DMA transfer with read increment disabled. The
	destination Array or manually allocated block must remain alive until the DMA
	channel completes or is aborted.
	End Rem
	Function ADCFIFOAddress:Byte Ptr() = "bmx_pico_adc_fifo_address"

	Rem
	bbdoc: Returns the DMA request number used to pace transfers from the ADC FIFO.
	about: This value is supplied by the selected Pico SDK target and differs
	between RP2040 and RP2350.
	End Rem
	Function ADCDREQ:UInt() = "bmx_pico_adc_dreq"
End Extern
