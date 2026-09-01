SuperStrict

Rem
bbdoc: Inter-integrated circuit controllers for Raspberry Pi Pico targets.
End Rem
Module Pico.Hardware.I2C

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Const I2CController0:Int = 0
Const I2CController1:Int = 1

Const I2CSpeedStandard:UInt = 100000
Const I2CSpeedFast:UInt = 400000
Const I2CSpeedFastPlus:UInt = 1000000

Const I2CErrorGeneric:Int = -1
Const I2CErrorTimeout:Int = -2
Const I2CErrorInvalidArgument:Int = -5

Extern "C"
	Function I2CDefaultController:Int() = "bmx_pico_i2c_default_controller"
	Function I2CDefaultSDAPin:UInt() = "bmx_pico_i2c_default_sda_pin"
	Function I2CDefaultSCLPin:UInt() = "bmx_pico_i2c_default_scl_pin"

	Rem
	bbdoc: Selects the I2C function for a valid SDA/SCL pair and optionally enables internal pull-ups.
	about: External pull-up resistors appropriate to the bus voltage and speed are
	normally preferable for a real I2C bus.
	End Rem
	Function I2CConfigurePins:Int(controller:Int, sdaPin:UInt, sclPin:UInt, pullUps:Int) = "bmx_pico_i2c_configure_pins"

	Function I2CInit:UInt(controller:Int, baudrate:UInt) = "bmx_pico_i2c_init"
	Function I2CDeinit(controller:Int) = "bmx_pico_i2c_deinit"
	Function I2CSetBaudrate:UInt(controller:Int, baudrate:UInt) = "bmx_pico_i2c_set_baudrate"
	Function I2CSetPeripheralMode:Int(controller:Int, enabled:Int, address:UInt) = "bmx_pico_i2c_set_slave_mode"

	Rem
	bbdoc: Writes bytes to a 7-bit I2C address.
	about: Set noStop to retain the bus so the following transfer begins with a repeated Start.
	End Rem
	Function I2CWriteBlocking:Int(controller:Int, address:UInt, data:Byte Ptr, length:Int, noStop:Int) = "bmx_pico_i2c_write_blocking"
	Function I2CReadBlocking:Int(controller:Int, address:UInt, data:Byte Ptr, length:Int, noStop:Int) = "bmx_pico_i2c_read_blocking"
	Function I2CWriteTimeout:Int(controller:Int, address:UInt, data:Byte Ptr, length:Int, noStop:Int, timeoutMicroseconds:UInt) = "bmx_pico_i2c_write_timeout_us"
	Function I2CReadTimeout:Int(controller:Int, address:UInt, data:Byte Ptr, length:Int, noStop:Int, timeoutMicroseconds:UInt) = "bmx_pico_i2c_read_timeout_us"

	Function I2CWriteAvailable:UInt(controller:Int) = "bmx_pico_i2c_write_available"
	Function I2CReadAvailable:UInt(controller:Int) = "bmx_pico_i2c_read_available"
	Function I2CWriteRawBlocking:Int(controller:Int, data:Byte Ptr, length:Int) = "bmx_pico_i2c_write_raw_blocking"
	Function I2CReadRawBlocking:Int(controller:Int, data:Byte Ptr, length:Int) = "bmx_pico_i2c_read_raw_blocking"
End Extern
