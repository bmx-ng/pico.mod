' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Hardware universal asynchronous receivers/transmitters for Raspberry Pi Pico targets.
about: Board-default pin queries return PicoUnavailablePin when the selected
board does not define that pin.
End Rem
Module Pico.Hardware.UART
?pico

ModuleInfo "Version: 0.2"
ModuleInfo "License: zlib/libpng"

Import Pico.Core

Const UARTController0:Int = 0
Const UARTController1:Int = 1

Const UARTParityNone:UInt = 0
Const UARTParityEven:UInt = 1
Const UARTParityOdd:UInt = 2

Const UARTErrorFraming:UInt = $1
Const UARTErrorParity:UInt = $2
Const UARTErrorBreak:UInt = $4
Const UARTErrorOverrun:UInt = $8
Const UARTErrorInvalidArgument:Int = -5

Extern "C"
	Rem
	bbdoc: Returns the board's default UART controller, or PicoUnavailableController if none is defined.
	End Rem
	Function UARTDefaultController:Int() = "bmx_pico_uart_default_controller"
	Function UARTDefaultTXPin:UInt() = "bmx_pico_uart_default_tx_pin"
	Function UARTDefaultRXPin:UInt() = "bmx_pico_uart_default_rx_pin"
	Function UARTDefaultBaudrate:UInt() = "bmx_pico_uart_default_baudrate"
	Function UARTSupportsAuxiliaryPinMappings:Int() = "bmx_pico_uart_supports_auxiliary_pin_mappings"
	Function UARTConfigurePins:Int(controller:Int, txPin:UInt, rxPin:UInt) = "bmx_pico_uart_configure_pins"
	Function UARTConfigureFlowControlPins:Int(controller:Int, ctsPin:UInt, rtsPin:UInt) = "bmx_pico_uart_configure_flow_pins"
	Function UARTInit:UInt(controller:Int, baudrate:UInt) = "bmx_pico_uart_init"
	Function UARTDeinit(controller:Int) = "bmx_pico_uart_deinit"
	Function UARTSetBaudrate:UInt(controller:Int, baudrate:UInt) = "bmx_pico_uart_set_baudrate"
	Function UARTSetFormat:Int(controller:Int, dataBits:UInt, stopBits:UInt, parity:UInt) = "bmx_pico_uart_set_format"
	Function UARTSetFlowControl:Int(controller:Int, ctsEnabled:Int, rtsEnabled:Int) = "bmx_pico_uart_set_flow_control"
	Function UARTSetFIFOEnabled:Int(controller:Int, enabled:Int) = "bmx_pico_uart_set_fifo_enabled"
	Function UARTIsEnabled:Int(controller:Int) = "bmx_pico_uart_is_enabled"
	Function UARTIsWritable:Int(controller:Int) = "bmx_pico_uart_is_writable"
	Function UARTIsReadable:Int(controller:Int) = "bmx_pico_uart_is_readable"
	Function UARTIsReadableWithin:Int(controller:Int, timeoutMicroseconds:UInt) = "bmx_pico_uart_is_readable_within_us"

	Function UARTWriteBlocking:Int(controller:Int, source:Byte Ptr, length:Int) = "bmx_pico_uart_write_blocking"
	Function UARTReadBlocking:Int(controller:Int, destination:Byte Ptr, length:Int) = "bmx_pico_uart_read_blocking"

	Rem
	bbdoc: Reads up to length bytes until a single overall microsecond deadline expires.
	returns: The number of bytes read, which may be less than length.
	End Rem
	Function UARTReadTimeout:Int(controller:Int, destination:Byte Ptr, length:Int, timeoutMicroseconds:UInt) = "bmx_pico_uart_read_timeout_us"
	Function UARTReadAvailable:Int(controller:Int, destination:Byte Ptr, capacity:Int) = "bmx_pico_uart_read_available"
	Function UARTPutByte:Int(controller:Int, value:UInt) = "bmx_pico_uart_put_byte"
	Function UARTTXWaitBlocking(controller:Int) = "bmx_pico_uart_tx_wait_blocking"
	Function UARTSetBreak:Int(controller:Int, enabled:Int) = "bmx_pico_uart_set_break"
	Function UARTSetTranslateCRLF:Int(controller:Int, enabled:Int) = "bmx_pico_uart_set_translate_crlf"
	Function UARTGetErrors:UInt(controller:Int) = "bmx_pico_uart_get_errors"
	Function UARTClearErrors(controller:Int) = "bmx_pico_uart_clear_errors"
End Extern
?
