' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Internal deferred-event bridge for Raspberry Pi Pico targets.
about: Hardware interrupt handlers enqueue numeric records in fixed native
storage. PollSystem and WaitSystem drain those records and emit ordinary
BlitzMax events in normal application context.
End Rem
Module Pico.Runtime.Events
?pico

ModuleInfo "Version: 0.2"
ModuleInfo "License: zlib/libpng"

Import BRL.Event
Import BRL.System

Const PicoEventSourceCapacity:UInt = 64

Private

Const PicoEventSourceSlotBits:UInt = 6
Const PicoEventSourceSlotMask:UInt = PicoEventSourceCapacity - 1
Const PicoEventGenerationMask:UInt = $03ffffff

Global _picoEventSources:Object[PicoEventSourceCapacity]
Global _picoEventIds:Int[PicoEventSourceCapacity]
Global _picoEventPersistent:Int[PicoEventSourceCapacity]
Global _picoEventGenerations:UInt[PicoEventSourceCapacity]

Extern "C"
	Function _PicoTakeDeferredEvent:Int(token:UInt Var, eventData:UInt Var, eventMods:UInt Var, eventX:UInt Var, eventY:UInt Var) = "bmx_pico_event_take"
	Function _PicoDeferredEventPending:UInt() = "bmx_pico_event_pending"
	Function _PicoDeferredEventDropped:UInt() = "bmx_pico_event_dropped"
End Extern

Function _PicoEventSlotValid:Int(token:UInt, slot:UInt)
	Return slot < PicoEventSourceCapacity And ..
		_picoEventGenerations[slot] = (token Shr PicoEventSourceSlotBits)
End Function

Function _PicoPollDeferredEvents:Object(hookId:Int, data:Object, context:Object)
	Local token:UInt
	Local eventData:UInt
	Local eventMods:UInt
	Local eventX:UInt
	Local eventY:UInt
	While _PicoTakeDeferredEvent(token, eventData, eventMods, eventX, eventY)
		Local slot:UInt = token & PicoEventSourceSlotMask
		If _PicoEventSlotValid(token, slot)
			Local source:Object = _picoEventSources[slot]
			If source
				EmitEvent(CreateEvent(_picoEventIds[slot], source, Int(eventData), ..
					Int(eventMods), Int(eventX), Int(eventY)))
				If Not _picoEventPersistent[slot] Then ReleasePicoEventSource(token)
			End If
		End If
	Wend
	Return data
End Function

Public

Rem
bbdoc: Registers a managed object as a deferred hardware-event source.
about: This is runtime infrastructure for Pico hardware modules. The returned
token is safe to store in native peripheral state. A persistent source remains
registered until explicitly released; a one-shot source is released after its
first event is emitted.
End Rem
Function RegisterPicoEventSource:UInt(source:Object, eventId:Int, persistent:Int = False)
	If Not source Or eventId = 0 Then Return 0
	For Local slot:UInt = 0 Until PicoEventSourceCapacity
		If Not _picoEventSources[slot]
			Local generation:UInt = (_picoEventGenerations[slot] + 1) & PicoEventGenerationMask
			If generation = 0 Then generation = 1
			_picoEventGenerations[slot] = generation
			_picoEventIds[slot] = eventId
			_picoEventPersistent[slot] = persistent
			_picoEventSources[slot] = source
			Return (generation Shl PicoEventSourceSlotBits) | slot
		End If
	Next
	Return 0
End Function

Rem
bbdoc: Releases a previously registered deferred hardware-event source.
End Rem
Function ReleasePicoEventSource:Int(token:UInt)
	If token = 0 Then Return False
	Local slot:UInt = token & PicoEventSourceSlotMask
	If Not _PicoEventSlotValid(token, slot) Then Return False
	_picoEventSources[slot] = Null
	_picoEventIds[slot] = 0
	_picoEventPersistent[slot] = False
	Return True
End Function

Rem
bbdoc: Returns the number of native deferred events waiting to be dispatched.
End Rem
Function PicoDeferredEventPending:UInt()
	Return _PicoDeferredEventPending()
End Function

Rem
bbdoc: Returns the number of events discarded because the native mailbox was full.
End Rem
Function PicoDeferredEventDropped:UInt()
	Return _PicoDeferredEventDropped()
End Function

AddHook PollSystemHook, _PicoPollDeferredEvents
?
