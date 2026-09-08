' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Wireless LAN discovery and radio control for CYW43-equipped Pico boards.
about: The driver is serviced in the background by the Pico SDK. Scan callbacks
copy their data into fixed native storage and are converted into ordinary
BlitzMax events by PollSystem or WaitSystem. Managed code is never called from
the SDK callback.
End Rem
Module Pico.Network.WiFi
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Import BRL.Event
Import BRL.System

Const WiFiCountryWorldwide:UInt = $00005858
Const WiFiCountryUK:UInt = $00004247
Const WiFiCountryUSA:UInt = $00005355

Const WiFiLinkDown:Int = 0
Const WiFiLinkJoined:Int = 1
Const WiFiLinkNoIP:Int = 2
Const WiFiLinkUp:Int = 3
Const WiFiLinkFailed:Int = -1
Const WiFiLinkNoNetwork:Int = -2
Const WiFiLinkBadAuthentication:Int = -3

' These are the compact security flags returned by CYW43 scan results. They
' are deliberately distinct from the 32-bit authentication values used when
' joining a network.
Const WiFiScanSecurityOpen:Int = 0
Const WiFiScanSecurityWEP:Int = $01
Const WiFiScanSecurityWPA:Int = $02
Const WiFiScanSecurityWPA2:Int = $04

Rem
bbdoc: Emitted for every wireless network found by an active scan.
about: EventExtra contains a #TWiFiNetwork, EventData contains its security
flags, EventX contains its channel and EventY contains its RSSI in dBm.
End Rem
Global EVENT_WIFISCANRESULT:Int = AllocUserEventId("WiFiScanResult")

Rem
bbdoc: Emitted once after the current wireless scan has finished.
End Rem
Global EVENT_WIFISCANCOMPLETE:Int = AllocUserEventId("WiFiScanComplete")

Rem
bbdoc: The source object used by wireless events.
End Rem
Global WiFiEventSource:Object = New TWiFiEventSource

Rem
bbdoc: A wireless access point reported by a scan.
End Rem
Type TWiFiNetwork
	Field ssid:String
	Field bssid:Byte[]
	Field channel:Int
	Field rssi:Int
	Field security:Int

	Rem
	bbdoc: Returns True when the scan reported an open network.
	End Rem
	Method IsOpen:Int()
		Return security = WiFiScanSecurityOpen
	End Method
End Type

Private

Const WiFiNativeEventScanResult:Int = 1
Const WiFiNativeEventScanComplete:Int = 2

Type TWiFiEventSource
End Type

Extern "C"
	Function _WiFiInitialize:Int(country:UInt) = "bmx_pico_wifi_initialize"
	Function _WiFiDeinitialize:Int() = "bmx_pico_wifi_deinitialize"
	Function _WiFiInitialized:Int() = "bmx_pico_wifi_initialized"
	Function _WiFiSetLED:Int(value:Int) = "bmx_pico_wifi_set_led"
	Function _WiFiGetLED:Int() = "bmx_pico_wifi_get_led"
	Function _WiFiStartScan:Int() = "bmx_pico_wifi_start_scan"
	Function _WiFiScanActive:Int() = "bmx_pico_wifi_scan_active"
	Function _WiFiLinkStatus:Int() = "bmx_pico_wifi_link_status"
	Function _WiFiService() = "bmx_pico_wifi_service"
	Function _WiFiTakeEvent:Int(kind:Int Var, ssid:Byte Ptr, ssidLength:Int Var, ..
		bssid:Byte Ptr, channel:Int Var, rssi:Int Var, security:Int Var) = "bmx_pico_wifi_take_event"
	Function _WiFiDroppedScanResults:UInt() = "bmx_pico_wifi_dropped_scan_results"
End Extern

Function _PollWiFi:Object(hookId:Int, data:Object, context:Object)
	_WiFiService()
	Local kind:Int
	Local ssidLength:Int
	Local channel:Int
	Local rssi:Int
	Local security:Int
	Local ssidBytes:Byte[32]
	Local bssidBytes:Byte[6]
	While _WiFiTakeEvent(kind, ssidBytes, ssidLength, bssidBytes, channel, rssi, security)
		Select kind
			Case WiFiNativeEventScanResult
				Local network:TWiFiNetwork = New TWiFiNetwork
				network.ssid = String.FromUTF8Bytes(ssidBytes, ssidLength)
				network.bssid = New Byte[6]
				MemCopy(network.bssid, bssidBytes, 6)
				network.channel = channel
				network.rssi = rssi
				network.security = security
				EmitEvent(CreateEvent(EVENT_WIFISCANRESULT, WiFiEventSource, security, ..
					0, channel, rssi, network))
			Case WiFiNativeEventScanComplete
				EmitEvent(CreateEvent(EVENT_WIFISCANCOMPLETE, WiFiEventSource))
		End Select
	Wend
	Return data
End Function

Public

Rem
bbdoc: Builds a Pico SDK country code from a two-letter ISO country code.
about: Use the country in which the radio operates so the correct wireless
channels and power limits are selected. The default revision is suitable for
the country definitions shipped by the Pico SDK.
End Rem
Function WiFiCountryCode:UInt(code:String, revision:UInt = 0)
	If code.length <> 2 Or revision > $ffff Then Return 0
	Local upper:String = code.ToUpper()
	Local first:Int = upper[0]
	Local second:Int = upper[1]
	If first < Asc("A") Or first > Asc("Z") Or second < Asc("A") Or second > Asc("Z") Then Return 0
	Return UInt(first) | (UInt(second) Shl 8) | (revision Shl 16)
End Function

Rem
bbdoc: Initializes the CYW43 radio in station mode.
returns: Zero on success or a Pico SDK error code.
about: Initialize and deinitialize must run on core 0. Calling this function
again after successful initialization is harmless.
End Rem
Function WiFiInitialize:Int(country:UInt = WiFiCountryWorldwide)
	Return _WiFiInitialize(country)
End Function

Rem
bbdoc: Deinitializes the CYW43 radio.
returns: Zero on success or a Pico SDK error code.
about: An active scan must finish before the radio can be deinitialized.
End Rem
Function WiFiDeinitialize:Int()
	Return _WiFiDeinitialize()
End Function

Rem
bbdoc: Returns True after the radio has been initialized successfully.
End Rem
Function WiFiInitialized:Int()
	Return _WiFiInitialized()
End Function

Rem
bbdoc: Sets the wireless chip's GPIO 0 output, which controls the onboard LED on official Pico W boards.
End Rem
Function WiFiSetLED:Int(value:Int)
	Return _WiFiSetLED(value)
End Function

Rem
bbdoc: Returns the wireless chip's GPIO 0 output state, or -1 when unavailable.
End Rem
Function WiFiGetLED:Int()
	Return _WiFiGetLED()
End Function

Rem
bbdoc: Starts an asynchronous scan for nearby wireless networks.
returns: Zero when the scan started, or a Pico SDK error code.
about: Results are emitted as #EVENT_WIFISCANRESULT events followed by one
#EVENT_WIFISCANCOMPLETE event. Call PollSystem or WaitSystem to dispatch them.
End Rem
Function WiFiStartScan:Int()
	Return _WiFiStartScan()
End Function

Rem
bbdoc: Returns True while a wireless scan is active.
End Rem
Function WiFiScanActive:Int()
	Return _WiFiScanActive()
End Function

Rem
bbdoc: Returns the CYW43 station interface link status.
about: This initial radio-only implementation does not configure an IP stack,
so WiFiLinkUp is not expected until later network support is enabled.
End Rem
Function WiFiLinkStatus:Int()
	Return _WiFiLinkStatus()
End Function

Rem
bbdoc: Returns the number of scan records discarded because the fixed native queue was full.
End Rem
Function WiFiDroppedScanResults:UInt()
	Return _WiFiDroppedScanResults()
End Function

AddHook PollSystemHook, _PollWiFi
?
