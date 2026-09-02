' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Serial peripheral interface controllers for Raspberry Pi Pico targets.
about: Board-default pin queries return PicoUnavailablePin when the selected
board does not define that pin.
End Rem
Module Pico.Hardware.SPI
?pico

ModuleInfo "Version: 0.2"
ModuleInfo "License: zlib/libpng"

Import Pico.Core

Const SPIController0:Int = 0
Const SPIController1:Int = 1

Const SPIClockPolarity0:UInt = 0
Const SPIClockPolarity1:UInt = 1
Const SPIClockPhase0:UInt = 0
Const SPIClockPhase1:UInt = 1
Const SPIBitOrderLSBFirst:UInt = 0
Const SPIBitOrderMSBFirst:UInt = 1
Const SPIErrorInvalidArgument:Int = -5

Extern "C"
	Rem
	bbdoc: Returns the board's default SPI controller, or PicoUnavailableController if none is defined.
	End Rem
	Function SPIDefaultController:Int() = "bmx_pico_spi_default_controller"
	Function SPIDefaultRXPin:UInt() = "bmx_pico_spi_default_rx_pin"
	Function SPIDefaultTXPin:UInt() = "bmx_pico_spi_default_tx_pin"
	Function SPIDefaultClockPin:UInt() = "bmx_pico_spi_default_sck_pin"
	Function SPIDefaultChipSelectPin:UInt() = "bmx_pico_spi_default_csn_pin"

	Rem
	bbdoc: Selects the SPI function for a valid RX, TX and clock pin set.
	about: Chip-select pins are deliberately controlled separately with Pico.Hardware.GPIO.
	End Rem
	Function SPIConfigurePins:Int(controller:Int, rxPin:UInt, txPin:UInt, clockPin:UInt) = "bmx_pico_spi_configure_pins"
	Function SPIInit:UInt(controller:Int, baudrate:UInt) = "bmx_pico_spi_init"
	Function SPIDeinit(controller:Int) = "bmx_pico_spi_deinit"
	Function SPISetBaudrate:UInt(controller:Int, baudrate:UInt) = "bmx_pico_spi_set_baudrate"
	Function SPIGetBaudrate:UInt(controller:Int) = "bmx_pico_spi_get_baudrate"

	Rem
	bbdoc: Configures frame width, clock polarity, clock phase, and bit order.
	about: RP2350 hardware supports 4-16 bit frames but only MSB-first order.
	End Rem
	Function SPISetFormat:Int(controller:Int, dataBits:UInt, polarity:UInt, phase:UInt, bitOrder:UInt) = "bmx_pico_spi_set_format"
	Function SPISetPeripheralMode:Int(controller:Int, enabled:Int) = "bmx_pico_spi_set_peripheral_mode"
	Function SPIIsWritable:Int(controller:Int) = "bmx_pico_spi_is_writable"
	Function SPIIsReadable:Int(controller:Int) = "bmx_pico_spi_is_readable"
	Function SPIIsBusy:Int(controller:Int) = "bmx_pico_spi_is_busy"

	Function SPIWriteReadBlocking:Int(controller:Int, source:Byte Ptr, destination:Byte Ptr, length:Int) = "bmx_pico_spi_write_read_blocking"
	Function SPIWriteBlocking:Int(controller:Int, source:Byte Ptr, length:Int) = "bmx_pico_spi_write_blocking"
	Function SPIReadBlocking:Int(controller:Int, repeatedData:UInt, destination:Byte Ptr, length:Int) = "bmx_pico_spi_read_blocking"
	Function SPIWrite16Read16Blocking:Int(controller:Int, source:Short Ptr, destination:Short Ptr, length:Int) = "bmx_pico_spi_write16_read16_blocking"
	Function SPIWrite16Blocking:Int(controller:Int, source:Short Ptr, length:Int) = "bmx_pico_spi_write16_blocking"
	Function SPIRead16Blocking:Int(controller:Int, repeatedData:UInt, destination:Short Ptr, length:Int) = "bmx_pico_spi_read16_blocking"

	Rem
	bbdoc: Returns the SPI data-register address used by transmit and receive DMA.
	about: Use 8-bit DMA transfers for an 8-bit SPI format and 16-bit DMA
	transfers for formats wider than eight bits. Managed buffers must remain
	strongly reachable until both sides of a full-duplex transfer complete or
	are aborted.
	End Rem
	Function SPIDataRegisterAddress:Byte Ptr(controller:Int) = "bmx_pico_spi_data_register_address"
	Function SPITXDREQ:UInt(controller:Int) = "bmx_pico_spi_tx_dreq"
	Function SPIRXDREQ:UInt(controller:Int) = "bmx_pico_spi_rx_dreq"
End Extern
?
