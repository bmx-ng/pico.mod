' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Programmable I/O controllers for Raspberry Pi Pico targets.
End Rem
Module Pico.Hardware.PIO
?pico

ModuleInfo "Version: 0.4"
ModuleInfo "License: zlib/libpng"

Const PIOController0:Int = 0
Const PIOController1:Int = 1
Const PIOController2:Int = 2
Const PIOProgramAutomaticOrigin:Int = -1
Const PIOErrorInvalidArgument:Int = -5

Const PIOIRQLine0:UInt = 0
Const PIOIRQLine1:UInt = 1

Type TPIOProgram
	Field handle:Byte Ptr

	Method IsValid:Int()
		Return handle <> Null
	End Method

	Method Instructions:Short Ptr()
		Return _PIOProgramInstructions(handle)
	End Method

	Method Length:UInt()
		Return _PIOProgramLength(handle)
	End Method

	Method Origin:Int()
		Return _PIOProgramOrigin(handle)
	End Method

	Method Version:UInt()
		Return _PIOProgramVersion(handle)
	End Method

	Method UsedGPIORanges:UInt()
		Return _PIOProgramUsedGPIORanges(handle)
	End Method

	Method WrapTarget:UInt()
		Return _PIOProgramWrapTarget(handle)
	End Method

	Method Wrap:UInt()
		Return _PIOProgramWrap(handle)
	End Method
End Type

Extern "C"
	Function PIOCount:UInt() = "bmx_pico_pio_count"
	Function PIOVersion:UInt() = "bmx_pico_pio_version"
	Function PIOStateMachineCount:UInt() = "bmx_pico_pio_state_machine_count"
	Function PIOInstructionCapacity:UInt() = "bmx_pico_pio_instruction_capacity"

	Rem
	bbdoc: Loads a PIO instruction program and returns its instruction-memory offset.
	about: Instructions must remain readable until this call returns. Version zero
	programs are portable between RP2040 and RP2350.
	End Rem
	Function PIOAddProgram:Int(controller:Int, instructions:Short Ptr, length:UInt, origin:Int, version:UInt, usedGPIORanges:UInt) = "bmx_pico_pio_add_program"
	Function PIOCanAddProgram:Int(controller:Int, instructions:Short Ptr, length:UInt, origin:Int, version:UInt, usedGPIORanges:UInt) = "bmx_pico_pio_can_add_program"
	Function PIORemoveProgram:Int(controller:Int, length:UInt, offset:UInt) = "bmx_pico_pio_remove_program"

	Function PIOClaimUnusedStateMachine:Int(controller:Int) = "bmx_pico_pio_claim_unused_state_machine"
	Function PIOUnclaimStateMachine:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_unclaim_state_machine"
	Function PIOStateMachineIsClaimed:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_state_machine_is_claimed"

	Function PIOGPIOInit:Int(controller:Int, pin:UInt) = "bmx_pico_pio_gpio_init"
	Function PIOStateMachineSetConsecutivePinDirections:Int(controller:Int, stateMachine:UInt, pinBase:UInt, pinCount:UInt, output:Int) = "bmx_pico_pio_sm_set_consecutive_pin_directions"

	Rem
	bbdoc: Resets a state machine to the SDK default configuration at an initial program counter.
	End Rem
	Function PIOStateMachineInit:Int(controller:Int, stateMachine:UInt, initialPC:UInt) = "bmx_pico_pio_sm_init"
	Function PIOStateMachineSetWrap:Int(controller:Int, stateMachine:UInt, wrapTarget:UInt, wrap:UInt) = "bmx_pico_pio_sm_set_wrap"
	Function PIOStateMachineSetOutPins:Int(controller:Int, stateMachine:UInt, pinBase:UInt, pinCount:UInt) = "bmx_pico_pio_sm_set_out_pins"
	Function PIOStateMachineSetSetPins:Int(controller:Int, stateMachine:UInt, pinBase:UInt, pinCount:UInt) = "bmx_pico_pio_sm_set_set_pins"
	Function PIOStateMachineSetInPins:Int(controller:Int, stateMachine:UInt, pinBase:UInt) = "bmx_pico_pio_sm_set_in_pins"
	Function PIOStateMachineSetSideSetPins:Int(controller:Int, stateMachine:UInt, pinBase:UInt) = "bmx_pico_pio_sm_set_sideset_pins"
	Function PIOStateMachineSetJumpPin:Int(controller:Int, stateMachine:UInt, pin:UInt) = "bmx_pico_pio_sm_set_jump_pin"
	Function PIOStateMachineSetClockDivider:Int(controller:Int, stateMachine:UInt, divider:Float) = "bmx_pico_pio_sm_set_clock_divider"

	Function PIOStateMachineSetEnabled:Int(controller:Int, stateMachine:UInt, enabled:Int) = "bmx_pico_pio_sm_set_enabled"
	Function PIOStateMachineRestart:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_restart"
	Function PIOStateMachineRestartClockDivider:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_restart_clock_divider"
	Function PIOStateMachineClearFIFOs:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_clear_fifos"
	Function PIOStateMachineExecute:Int(controller:Int, stateMachine:UInt, instruction:UInt) = "bmx_pico_pio_sm_execute"
	Function PIOStateMachineProgramCounter:UInt(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_program_counter"

	Function PIOStateMachineTXFull:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_tx_full"
	Function PIOStateMachineTXEmpty:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_tx_empty"
	Function PIOStateMachineRXFull:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_rx_full"
	Function PIOStateMachineRXEmpty:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_rx_empty"
	Function PIOStateMachineTXLevel:UInt(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_tx_level"
	Function PIOStateMachineRXLevel:UInt(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_rx_level"
	Function PIOStateMachinePut:Int(controller:Int, stateMachine:UInt, value:UInt) = "bmx_pico_pio_sm_put"
	Function PIOStateMachinePutBlocking:Int(controller:Int, stateMachine:UInt, value:UInt) = "bmx_pico_pio_sm_put_blocking"
	Function PIOStateMachineGet:UInt(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_get"
	Function PIOStateMachineGetBlocking:UInt(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_get_blocking"
	Function PIOStateMachineTXFIFOAddress:Byte Ptr(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_tx_fifo_address"
	Function PIOStateMachineRXFIFOAddress:Byte Ptr(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_rx_fifo_address"
	Function _PIOStateMachineDREQ:UInt(controller:Int, stateMachine:UInt, transmit:Int) = "bmx_pico_pio_sm_dreq"

	Function PIOInterruptCount:UInt() = "bmx_pico_pio_interrupt_count"
	Function PIOIRQSupportedSources:UInt() = "bmx_pico_pio_irq_supported_sources"

	Rem
	bbdoc: Enables or disables native one-shot event latching for PIO interrupt sources on core 0.
	about: The native IRQ handler never calls BlitzMax code. A triggered source is
	temporarily masked to prevent level-triggered FIFO sources from causing an
	interrupt storm. Take the event, service or clear its cause, then call
	#PIOIRQRearmSources to receive the next event.
	End Rem
	Function PIOIRQSetSourcesEnabled:Int(controller:Int, irqLine:UInt, sourceMask:UInt, enabled:Int) = "bmx_pico_pio_irq_set_sources_enabled"
	Function PIOIRQEnabledSources:UInt(controller:Int, irqLine:UInt) = "bmx_pico_pio_irq_enabled_sources"
	Function PIOIRQArmedSources:UInt(controller:Int, irqLine:UInt) = "bmx_pico_pio_irq_armed_sources"
	Function PIOIRQPendingEvents:UInt(controller:Int, irqLine:UInt) = "bmx_pico_pio_irq_pending_events"
	Function PIOIRQTakeEvents:UInt(controller:Int, irqLine:UInt) = "bmx_pico_pio_irq_take_events"
	Function PIOIRQRearmSources:Int(controller:Int, irqLine:UInt, sourceMask:UInt) = "bmx_pico_pio_irq_rearm_sources"

	Function PIOInterruptIsSet:Int(controller:Int, interruptNumber:UInt) = "bmx_pico_pio_interrupt_is_set"
	Function PIOInterruptClear:Int(controller:Int, interruptNumber:UInt) = "bmx_pico_pio_interrupt_clear"

	Function _PIOFindProgram:Byte Ptr(name:String) = "bmx_pico_pio_find_program"
	Function _PIOProgramInstructions:Short Ptr(program:Byte Ptr) = "bmx_pico_pio_program_instructions"
	Function _PIOProgramLength:UInt(program:Byte Ptr) = "bmx_pico_pio_program_length"
	Function _PIOProgramOrigin:Int(program:Byte Ptr) = "bmx_pico_pio_program_origin"
	Function _PIOProgramVersion:UInt(program:Byte Ptr) = "bmx_pico_pio_program_version"
	Function _PIOProgramUsedGPIORanges:UInt(program:Byte Ptr) = "bmx_pico_pio_program_used_gpio_ranges"
	Function _PIOProgramWrapTarget:UInt(program:Byte Ptr) = "bmx_pico_pio_program_wrap_target"
	Function _PIOProgramWrap:UInt(program:Byte Ptr) = "bmx_pico_pio_program_wrap"
	Function _PIOCanAddImportedProgram:Int(controller:Int, program:Byte Ptr) = "bmx_pico_pio_can_add_imported_program"
	Function _PIOAddImportedProgram:Int(controller:Int, program:Byte Ptr) = "bmx_pico_pio_add_imported_program"
	Function _PIOStateMachineInitImportedProgram:Int(controller:Int, stateMachine:UInt, program:Byte Ptr, offset:UInt) = "bmx_pico_pio_sm_init_imported_program"
End Extern

Function PIOStateMachineTXDREQ:UInt(controller:Int, stateMachine:UInt)
	Return _PIOStateMachineDREQ(controller, stateMachine, True)
End Function

Function PIOStateMachineRXDREQ:UInt(controller:Int, stateMachine:UInt)
	Return _PIOStateMachineDREQ(controller, stateMachine, False)
End Function

Rem
bbdoc: Returns the IRQ source mask for a state machine's RX-not-empty condition.
End Rem
Function PIOIRQSMRXNotEmptyMask:UInt(stateMachine:UInt)
	If stateMachine >= PIOStateMachineCount() Then Return 0
	Return 1 Shl stateMachine
End Function

Rem
bbdoc: Returns the IRQ source mask for a state machine's TX-not-full condition.
End Rem
Function PIOIRQSMTXNotFullMask:UInt(stateMachine:UInt)
	If stateMachine >= PIOStateMachineCount() Then Return 0
	Return 1 Shl (4 + stateMachine)
End Function

Rem
bbdoc: Returns the IRQ source mask for a PIO interrupt flag set by an IRQ instruction.
End Rem
Function PIOIRQInterruptMask:UInt(interruptNumber:UInt)
	If interruptNumber >= PIOInterruptCount() Then Return 0
	Return 1 Shl (8 + interruptNumber)
End Function

' Explicit short overloads survive module interface generation, unlike default
' argument expressions, and cover the common portable PIO-v0 case cleanly.
Function PIOAddProgram:Int(controller:Int, instructions:Short Ptr, length:UInt)
	Return PIOAddProgram(controller, instructions, length, PIOProgramAutomaticOrigin, 0, 0)
End Function

Function PIOCanAddProgram:Int(controller:Int, instructions:Short Ptr, length:UInt)
	Return PIOCanAddProgram(controller, instructions, length, PIOProgramAutomaticOrigin, 0, 0)
End Function

Rem
bbdoc: Finds a program declared by an imported `.pio` source.
about: The program name is the identifier following `.program` in the PIO
assembly source. Program names must be unique within one firmware image.
End Rem
Function PIOProgram:TPIOProgram(name:String)
	Local handle:Byte Ptr = _PIOFindProgram(name)
	If Not handle Then Return Null
	Local program:TPIOProgram = New TPIOProgram
	program.handle = handle
	Return program
End Function

Function PIOCanAddProgram:Int(controller:Int, program:TPIOProgram)
	If Not program Then Return False
	Return _PIOCanAddImportedProgram(controller, program.handle)
End Function

Function PIOAddProgram:Int(controller:Int, program:TPIOProgram)
	If Not program Then Return PIOErrorInvalidArgument
	Return _PIOAddImportedProgram(controller, program.handle)
End Function

Function PIORemoveProgram:Int(controller:Int, program:TPIOProgram, offset:UInt)
	If Not program Then Return False
	Return PIORemoveProgram(controller, program.Length(), offset)
End Function

Rem
bbdoc: Initializes a state machine with the complete pioasm-generated default configuration.
about: This applies wrap, shift, FIFO, sideset, status and other settings emitted
by pioasm, then leaves the state machine disabled at the loaded program offset.
End Rem
Function PIOStateMachineInitProgram:Int(controller:Int, stateMachine:UInt, program:TPIOProgram, offset:UInt)
	If Not program Then Return PIOErrorInvalidArgument
	Return _PIOStateMachineInitImportedProgram(controller, stateMachine, program.handle, offset)
End Function
?
