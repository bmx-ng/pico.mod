SuperStrict

Extern "C"
	Function PicoStandardIOInit:Int() = "bmx_pico_stdio_init_all"
	Function PicoDefaultLEDPin:UInt() = "bmx_pico_default_led_pin"
	Function PicoGPIOInit(gpio:UInt) = "bmx_pico_gpio_init"
	Function PicoGPIOSetOutput(gpio:UInt) = "bmx_pico_gpio_set_output"
	Function PicoGPIOPut(gpio:UInt, value:Int) = "bmx_pico_gpio_put"
	Function PicoPutCharacter:Int(character:Int) = "bmx_pico_putchar_raw"
	Function PicoSleep(milliseconds:UInt) = "bmx_pico_sleep_ms"
End Extern

Global Tick:UInt

Function EmitTick()
	PicoPutCharacter(48 + Int(Tick Mod 10))
	PicoPutCharacter(10)
End Function

Local ledPin:UInt = PicoDefaultLEDPin()
PicoStandardIOInit()
PicoGPIOInit(ledPin)
PicoGPIOSetOutput(ledPin)

While True
	PicoGPIOPut(ledPin, Int(Tick & 1))
	EmitTick()
	Tick :+ 1
	PicoSleep(500)
Wend
