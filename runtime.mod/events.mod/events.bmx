' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Pico-facing deferred native-event services.
about: The portable implementation lives in Embedded.Runtime.Events. These
names remain available to Pico-specific modules and applications.
End Rem
Module Pico.Runtime.Events
?pico

ModuleInfo "Version: 0.3"
ModuleInfo "License: zlib/libpng"

Import Embedded.Runtime.Events

Const PicoEventSourceCapacity:UInt = EmbeddedEventSourceCapacity

Rem
bbdoc: Registers a managed object as a deferred hardware-event source.
about: This is runtime infrastructure for Pico hardware modules. The returned
token is safe to store in native peripheral state. A persistent source remains
registered until explicitly released; a one-shot source is released after its
first event is emitted.
End Rem
Function RegisterPicoEventSource:UInt(source:Object, eventId:Int, persistent:Int = False)
	Return RegisterEmbeddedEventSource(source, eventId, persistent)
End Function

Rem
bbdoc: Releases a previously registered deferred hardware-event source.
End Rem
Function ReleasePicoEventSource:Int(token:UInt)
	Return ReleaseEmbeddedEventSource(token)
End Function

Rem
bbdoc: Returns the number of native deferred events waiting to be dispatched.
End Rem
Function PicoDeferredEventPending:UInt()
	Return EmbeddedDeferredEventPending()
End Function

Rem
bbdoc: Returns the number of events discarded because the native mailbox was full.
End Rem
Function PicoDeferredEventDropped:UInt()
	Return EmbeddedDeferredEventDropped()
End Function
?
