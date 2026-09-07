' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Programmable I/O controllers for Raspberry Pi Pico targets.
End Rem
Module Pico.Hardware.PIO
?pico

ModuleInfo "Version: 0.5"
ModuleInfo "License: zlib/libpng"

Import BRL.Event
Import Pico.Runtime.Events

Const PIOController0:Int = 0
Const PIOController1:Int = 1
Const PIOController2:Int = 2
Const PIOProgramAutomaticOrigin:Int = -1
Const PIOErrorInvalidArgument:Int = -5

Const PIOIRQLine0:UInt = 0
Const PIOIRQLine1:UInt = 1

Const PIOFIFOJoinNone:UInt = 0
Const PIOFIFOJoinTX:UInt = 1
Const PIOFIFOJoinRX:UInt = 2
Const PIOFIFOJoinTXGet:UInt = 4
Const PIOFIFOJoinTXPut:UInt = 8
Const PIOFIFOJoinPutGet:UInt = 12

Const PIOMoveStatusTXLessThan:UInt = 0
Const PIOMoveStatusRXLessThan:UInt = 1
Const PIOMoveStatusIRQSet:UInt = 2

Const PIOConfigClockDivider:UInt = 1 Shl 0
Const PIOConfigOutPins:UInt = 1 Shl 1
Const PIOConfigSetPins:UInt = 1 Shl 2
Const PIOConfigInPins:UInt = 1 Shl 3
Const PIOConfigSideSetPins:UInt = 1 Shl 4
Const PIOConfigJumpPin:UInt = 1 Shl 5
Const PIOConfigInShift:UInt = 1 Shl 6
Const PIOConfigOutShift:UInt = 1 Shl 7
Const PIOConfigFIFOJoin:UInt = 1 Shl 8
Const PIOConfigSideSet:UInt = 1 Shl 9
Const PIOConfigOutSpecial:UInt = 1 Shl 10
Const PIOConfigMoveStatus:UInt = 1 Shl 11

Rem
bbdoc: Emitted after a registered PIO interrupt source becomes pending.
about: EventData contains the PIO source mask and EventMods contains the PIO
controller number. Use #PIOIRQTimeUS to recover the 64-bit timestamp captured
inside the native IRQ handler. Event delivery itself is deferred to PollSystem
or WaitSystem and can occur later than that timestamp.
End Rem
Global EVENT_PIOIRQ:Int = AllocUserEventId("PIOIRQ")

Rem
bbdoc: Optional semantic overrides for PIO state-machine initialization.
about: An empty value preserves every setting emitted by pioasm (or every SDK
default for a raw initial program counter). Calling a Set method overrides only
that group of settings. This avoids exposing the SDK's target-specific
pio_sm_config binary layout to BlitzMax code.
End Rem
Struct SPIOStateMachineConfig
	Private
	Field Overrides:UInt
	Field ClockDivider:Float
	Field OutPinBase:UInt
	Field OutPinCount:UInt
	Field SetPinBase:UInt
	Field SetPinCount:UInt
	Field InPinBase:UInt
	Field SideSetPinBase:UInt
	Field JumpPin:UInt
	Field InShiftRight:UInt
	Field AutoPush:UInt
	Field PushThreshold:UInt
	Field OutShiftRight:UInt
	Field AutoPull:UInt
	Field PullThreshold:UInt
	Field FIFOJoin:UInt
	Field SideSetBitCount:UInt
	Field SideSetOptional:UInt
	Field SideSetPinDirections:UInt
	Field OutSticky:UInt
	Field OutHasEnablePin:UInt
	Field OutEnableBitIndex:UInt
	Field MoveStatusType:UInt
	Field MoveStatusThreshold:UInt

	Public
	Method OverrideMask:UInt()
		Return Overrides
	End Method

	Method Reset()
		Overrides = 0
	End Method

	Method SetClockDivider(divider:Float)
		Overrides :| PIOConfigClockDivider
		ClockDivider = divider
	End Method

	Method SetOutPins(pinBase:UInt, pinCount:UInt)
		Overrides :| PIOConfigOutPins
		OutPinBase = pinBase
		OutPinCount = pinCount
	End Method

	Method SetSetPins(pinBase:UInt, pinCount:UInt)
		Overrides :| PIOConfigSetPins
		SetPinBase = pinBase
		SetPinCount = pinCount
	End Method

	Method SetInPins(pinBase:UInt)
		Overrides :| PIOConfigInPins
		InPinBase = pinBase
	End Method

	Method SetSideSetPins(pinBase:UInt)
		Overrides :| PIOConfigSideSetPins
		SideSetPinBase = pinBase
	End Method

	Method SetJumpPin(pin:UInt)
		Overrides :| PIOConfigJumpPin
		JumpPin = pin
	End Method

	Method SetInShift(shiftRight:Int, autoPush:Int, threshold:UInt)
		Overrides :| PIOConfigInShift
		InShiftRight = shiftRight <> 0
		Self.AutoPush = autoPush <> 0
		PushThreshold = threshold
	End Method

	Method SetOutShift(shiftRight:Int, autoPull:Int, threshold:UInt)
		Overrides :| PIOConfigOutShift
		OutShiftRight = shiftRight <> 0
		Self.AutoPull = autoPull <> 0
		PullThreshold = threshold
	End Method

	Method SetFIFOJoin(joinMode:UInt)
		Overrides :| PIOConfigFIFOJoin
		FIFOJoin = joinMode
	End Method

	Method SetSideSet(bitCount:UInt, optional:Int, pinDirections:Int)
		Overrides :| PIOConfigSideSet
		SideSetBitCount = bitCount
		SideSetOptional = optional <> 0
		SideSetPinDirections = pinDirections <> 0
	End Method

	Method SetOutSpecial(sticky:Int, hasEnablePin:Int, enableBitIndex:UInt)
		Overrides :| PIOConfigOutSpecial
		OutSticky = sticky <> 0
		OutHasEnablePin = hasEnablePin <> 0
		OutEnableBitIndex = enableBitIndex
	End Method

	Method SetMoveStatus(statusType:UInt, threshold:UInt)
		Overrides :| PIOConfigMoveStatus
		MoveStatusType = statusType
		MoveStatusThreshold = threshold
	End Method
End Struct

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

Rem
bbdoc: Owns a claimed PIO state machine and its private loaded program copy.
about: Create claims a state machine, loads the program, and initializes it
disabled. Configure GPIO functions and directions before enabling it. Close
stops and releases the state machine and removes its program instructions; a
Using block is recommended.
End Rem
Type TPIOStateMachine Implements ICloseable
	Private
	Field controller:Int = -1
	Field stateMachine:Int = -1
	Field offset:Int = PIOErrorInvalidArgument
	Field program:TPIOProgram

	Public
	Function Create:TPIOStateMachine(controller:Int, program:TPIOProgram)
		Local settings:SPIOStateMachineConfig
		Return Create(controller, program, settings)
	End Function

	Function Create:TPIOStateMachine(controller:Int, program:TPIOProgram, settings:SPIOStateMachineConfig)
		If Not program Then Return Null
		Local stateMachine:Int = PIOClaimUnusedStateMachine(controller)
		If stateMachine < 0 Then Return Null
		Local owner:TPIOStateMachine = New TPIOStateMachine
		owner.controller = controller
		owner.stateMachine = stateMachine
		owner.program = program
		owner.offset = PIOAddProgram(controller, program)
		If owner.offset < 0 Or PIOStateMachineInitProgram(controller, stateMachine, ..
				program, owner.offset, settings) < 0 Then
			owner.Close()
			Return Null
		End If
		Return owner
	End Function

	Method Controller:Int()
		Return controller
	End Method

	Method Index:Int()
		Return stateMachine
	End Method

	Method ProgramOffset:Int()
		Return offset
	End Method

	Method Program:TPIOProgram()
		Return program
	End Method

	Method IsOpen:Int()
		Return stateMachine >= 0
	End Method

	Method SetEnabled:Int(enabled:Int)
		If stateMachine < 0 Then Return False
		Return PIOStateMachineSetEnabled(controller, stateMachine, enabled)
	End Method

	Method Put:Int(value:UInt)
		If stateMachine < 0 Then Return False
		Return PIOStateMachinePut(controller, stateMachine, value)
	End Method

	Method PutBlocking:Int(value:UInt)
		If stateMachine < 0 Then Return False
		Return PIOStateMachinePutBlocking(controller, stateMachine, value)
	End Method

	Method Get:UInt()
		If stateMachine < 0 Then Return 0
		Return PIOStateMachineGet(controller, stateMachine)
	End Method

	Method GetBlocking:UInt()
		If stateMachine < 0 Then Return 0
		Return PIOStateMachineGetBlocking(controller, stateMachine)
	End Method

	Method TXDREQ:UInt()
		If stateMachine < 0 Then Return UInt(-1)
		Return PIOStateMachineTXDREQ(controller, stateMachine)
	End Method

	Method RXDREQ:UInt()
		If stateMachine < 0 Then Return UInt(-1)
		Return PIOStateMachineRXDREQ(controller, stateMachine)
	End Method

	Method Close()
		If stateMachine < 0 Then Return
		PIOStateMachineSetEnabled(controller, stateMachine, False)
		PIOStateMachineClearFIFOs(controller, stateMachine)
		PIOUnclaimStateMachine(controller, stateMachine)
		If program And offset >= 0 Then PIORemoveProgram(controller, program, offset)
		stateMachine = -1
		offset = PIOErrorInvalidArgument
		program = Null
		controller = -1
	End Method

	Method Delete()
		Close()
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
	Function PIOGPIOBase:UInt(controller:Int) = "bmx_pico_pio_gpio_base"
	Function PIOSetGPIOBase:Int(controller:Int, gpioBase:UInt) = "bmx_pico_pio_set_gpio_base"
	Function PIOStateMachineSetConsecutivePinDirections:Int(controller:Int, stateMachine:UInt, pinBase:UInt, pinCount:UInt, output:Int) = "bmx_pico_pio_sm_set_consecutive_pin_directions"

	Rem
	bbdoc: Resets a state machine to the SDK default configuration at an initial program counter.
	End Rem
	Function PIOStateMachineInit:Int(controller:Int, stateMachine:UInt, initialPC:UInt) = "bmx_pico_pio_sm_init"
	Function _PIOStateMachineInitConfigured:Int(controller:Int, stateMachine:UInt, initialPC:UInt, settings:Byte Ptr) = "bmx_pico_pio_sm_init_configured"
	Function PIOStateMachineSetWrap:Int(controller:Int, stateMachine:UInt, wrapTarget:UInt, wrap:UInt) = "bmx_pico_pio_sm_set_wrap"
	Function PIOStateMachineSetOutPins:Int(controller:Int, stateMachine:UInt, pinBase:UInt, pinCount:UInt) = "bmx_pico_pio_sm_set_out_pins"
	Function PIOStateMachineSetSetPins:Int(controller:Int, stateMachine:UInt, pinBase:UInt, pinCount:UInt) = "bmx_pico_pio_sm_set_set_pins"
	Function PIOStateMachineSetInPins:Int(controller:Int, stateMachine:UInt, pinBase:UInt) = "bmx_pico_pio_sm_set_in_pins"
	Function PIOStateMachineSetSideSetPins:Int(controller:Int, stateMachine:UInt, pinBase:UInt) = "bmx_pico_pio_sm_set_sideset_pins"
	Function PIOStateMachineSetJumpPin:Int(controller:Int, stateMachine:UInt, pin:UInt) = "bmx_pico_pio_sm_set_jump_pin"
	Function PIOStateMachineSetClockDivider:Int(controller:Int, stateMachine:UInt, divider:Float) = "bmx_pico_pio_sm_set_clock_divider"

	Function PIOStateMachineSetEnabled:Int(controller:Int, stateMachine:UInt, enabled:Int) = "bmx_pico_pio_sm_set_enabled"
	Function PIOStateMachinesSetEnabled:Int(controller:Int, stateMachineMask:UInt, enabled:Int) = "bmx_pico_pio_sm_mask_set_enabled"
	Function PIOStateMachinesRestart:Int(controller:Int, stateMachineMask:UInt) = "bmx_pico_pio_sm_mask_restart"
	Function PIOStateMachinesRestartClockDivider:Int(controller:Int, stateMachineMask:UInt) = "bmx_pico_pio_sm_mask_restart_clock_divider"
	Function PIOStateMachinesEnableSynchronized:Int(controller:Int, stateMachineMask:UInt) = "bmx_pico_pio_sm_mask_enable_synchronized"
	Function PIOStateMachineRestart:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_restart"
	Function PIOStateMachineRestartClockDivider:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_restart_clock_divider"
	Function PIOStateMachineClearFIFOs:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_clear_fifos"
	Function PIOStateMachineExecute:Int(controller:Int, stateMachine:UInt, instruction:UInt) = "bmx_pico_pio_sm_execute"
	Function PIOStateMachineExecuteBlocking:Int(controller:Int, stateMachine:UInt, instruction:UInt) = "bmx_pico_pio_sm_execute_blocking"
	Function PIOStateMachineExecuteStalled:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_execute_stalled"
	Function PIOStateMachineDrainTXFIFO:Int(controller:Int, stateMachine:UInt) = "bmx_pico_pio_sm_drain_tx_fifo"
	Function PIOStateMachineSetPinsMasked:Int(controller:Int, stateMachine:UInt, pinValues:ULong, pinMask:ULong) = "bmx_pico_pio_sm_set_pins_masked"
	Function PIOStateMachineSetPinDirectionsMasked:Int(controller:Int, stateMachine:UInt, pinDirections:ULong, pinMask:ULong) = "bmx_pico_pio_sm_set_pin_directions_masked"
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
	Function _PIOIRQSetEventToken:Int(controller:Int, irqLine:UInt, sourceMask:UInt, token:UInt) = "bmx_pico_pio_irq_set_event_token"
	Function PIOIRQEnabledSources:UInt(controller:Int, irqLine:UInt) = "bmx_pico_pio_irq_enabled_sources"
	Function PIOIRQArmedSources:UInt(controller:Int, irqLine:UInt) = "bmx_pico_pio_irq_armed_sources"
	Function PIOIRQPendingEvents:UInt(controller:Int, irqLine:UInt) = "bmx_pico_pio_irq_pending_events"
	Function PIOIRQTakeEvents:UInt(controller:Int, irqLine:UInt) = "bmx_pico_pio_irq_take_events"
	Function _PIOIRQTakeEventsMasked:UInt(controller:Int, irqLine:UInt, sourceMask:UInt) = "bmx_pico_pio_irq_take_events_masked"
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
	Function _PIOStateMachineInitImportedProgramConfigured:Int(controller:Int, stateMachine:UInt, program:Byte Ptr, offset:UInt, settings:Byte Ptr) = "bmx_pico_pio_sm_init_imported_program_configured"
End Extern

Rem
bbdoc: Initializes a state machine with SDK defaults plus selected semantic overrides.
End Rem
Function PIOStateMachineInit:Int(controller:Int, stateMachine:UInt, initialPC:UInt, settings:SPIOStateMachineConfig)
	Return _PIOStateMachineInitConfigured(controller, stateMachine, initialPC, Byte Ptr(Varptr(settings.Overrides)))
End Function

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

Rem
bbdoc: A managed, timestamped PIO interrupt event source.
about: Create enables the selected native sources on one controller IRQ line on
core 0. The native handler records and masks pending sources, captures
time_us_64, then queues a numeric record without calling managed code. Service
or clear each cause and call #RearmSources before expecting another event.
Only one managed event source may own a controller IRQ line at a time. Close is
required because an open source is deliberately retained by the event bridge.
End Rem
Type TPIOIRQSource Implements ICloseable
	Private
	Field controller:Int = -1
	Field irqLine:UInt
	Field sourceMask:UInt
	Field eventToken:UInt
	Field tokenInstalled:Int
	Field irqEnabled:Int

	Public
	Function Create:TPIOIRQSource(controller:Int, irqLine:UInt, sourceMask:UInt)
		If sourceMask = 0 Or (sourceMask & ~PIOIRQSupportedSources()) <> 0 Then Return Null
		Local source:TPIOIRQSource = New TPIOIRQSource
		source.controller = controller
		source.irqLine = irqLine
		source.sourceMask = sourceMask
		source.eventToken = RegisterPicoEventSource(source, EVENT_PIOIRQ, True)
		If source.eventToken = 0 Then
			source.Close()
			Return Null
		End If
		If Not _PIOIRQSetEventToken(controller, irqLine, sourceMask, source.eventToken) Then
			source.Close()
			Return Null
		End If
		source.tokenInstalled = True
		If Not PIOIRQSetSourcesEnabled(controller, irqLine, sourceMask, True) Then
			source.Close()
			Return Null
		End If
		source.irqEnabled = True
		Return source
	End Function

	Method Controller:Int()
		Return controller
	End Method

	Method IRQLine:UInt()
		Return irqLine
	End Method

	Method SourceMask:UInt()
		Return sourceMask
	End Method

	Method IsOpen:Int()
		Return controller >= 0
	End Method

	Method PendingEvents:UInt()
		If controller < 0 Then Return 0
		Return PIOIRQPendingEvents(controller, irqLine) & sourceMask
	End Method

	Method TakeEvents:UInt()
		If controller < 0 Then Return 0
		Return _PIOIRQTakeEventsMasked(controller, irqLine, sourceMask)
	End Method

	Method RearmSources:Int(mask:UInt = 0)
		If controller < 0 Then Return False
		If mask = 0 Then mask = sourceMask
		If mask & ~sourceMask Then Return False
		Return PIOIRQRearmSources(controller, irqLine, mask)
	End Method

	Method Close()
		If controller < 0 Then Return
		If irqEnabled Then PIOIRQSetSourcesEnabled(controller, irqLine, sourceMask, False)
		If tokenInstalled Then _PIOIRQSetEventToken(controller, irqLine, sourceMask, 0)
		If eventToken Then ReleasePicoEventSource(eventToken)
		irqEnabled = False
		tokenInstalled = False
		eventToken = 0
		controller = -1
	End Method

	Method Delete()
		Close()
	End Method
End Type

Rem
bbdoc: Returns the native microsecond timestamp carried by a PIO IRQ event.
about: This is the instant at which the native interrupt handler ran, not the
later time at which the BlitzMax event queue delivered the event.
End Rem
Function PIOIRQTimeUS:ULong(event:TEvent)
	If Not event Or event.id <> EVENT_PIOIRQ Then Return 0
	Return ULong(UInt(event.x)) | (ULong(UInt(event.y)) Shl 32)
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

Rem
bbdoc: Initializes a state machine with its pioasm defaults plus selected semantic overrides.
End Rem
Function PIOStateMachineInitProgram:Int(controller:Int, stateMachine:UInt, program:TPIOProgram, offset:UInt, settings:SPIOStateMachineConfig)
	If Not program Then Return PIOErrorInvalidArgument
	Return _PIOStateMachineInitImportedProgramConfigured(controller, stateMachine, ..
		program.handle, offset, Byte Ptr(Varptr(settings.Overrides)))
End Function
?
