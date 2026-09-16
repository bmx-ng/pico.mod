' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Raspberry Pi Pico wireless LAN support.
about: Portable station, scanning and link APIs are provided by
Embedded.Network.WiFi. This module adds CYW43 SoftAP control and exposes the
wireless GPIO used for the onboard LED on official Pico W boards.
End Rem
Module Pico.Network.WiFi
?pico

ModuleInfo "Version: 0.5"
ModuleInfo "License: zlib/libpng"

Import Embedded.Network.WiFi

Extern "C"
	Function WiFiSetLED:Int(value:Int) = "bmx_pico_wifi_set_led"
	Function WiFiGetLED:Int() = "bmx_pico_wifi_get_led"
	Function _WiFiStartAccessPoint:Int(ssid:Byte Ptr, ssidLength:UInt, password:Byte Ptr, ..
		passwordLength:UInt, authentication:UInt, channel:UInt) = "bmx_pico_wifi_start_access_point"
	Function WiFiStopAccessPoint:Int() = "bmx_pico_wifi_stop_access_point"
	Function WiFiAccessPointActive:Int() = "bmx_pico_wifi_access_point_active"
	Function WiFiAccessPointClientCount:Int(count:UInt Var) = "bmx_pico_wifi_access_point_client_count"
	Function _WiFiAccessPointIPv4Address:UInt() = "bmx_pico_wifi_access_point_ipv4_address"
	Function _WiFiAccessPointIPv4Netmask:UInt() = "bmx_pico_wifi_access_point_ipv4_netmask"
	Function _WiFiAccessPointIPv4Gateway:UInt() = "bmx_pico_wifi_access_point_ipv4_gateway"
End Extern

Private
Function _PicoIPv4String:String(value:UInt)
	If value = 0 Then Return ""
	Return String(value & $ff) + "." + String((value Shr 8) & $ff) + "." + ..
		String((value Shr 16) & $ff) + "." + String((value Shr 24) & $ff)
End Function
Public

Function WiFiStartAccessPoint:Int(ssid:String, password:String = "", ..
		authentication:UInt = WiFiAuthenticationWPA2AESPSK, channel:UInt = 1)
	If Not ssid Then Return -5
	Local ssidLength:Size_T, passwordLength:Size_T
	Local ssidBytes:Byte Ptr = ssid.ToUTF8String(ssidLength)
	Local passwordBytes:Byte Ptr
	If password Then passwordBytes = password.ToUTF8String(passwordLength)
	Local result:Int = _WiFiStartAccessPoint(ssidBytes, UInt(ssidLength), ..
		passwordBytes, UInt(passwordLength), authentication, channel)
	MemFree(ssidBytes)
	If passwordBytes Then MemFree(passwordBytes)
	Return result
End Function

Function WiFiAccessPointIPv4Address:String()
	Return _PicoIPv4String(_WiFiAccessPointIPv4Address())
End Function

Function WiFiAccessPointIPv4Netmask:String()
	Return _PicoIPv4String(_WiFiAccessPointIPv4Netmask())
End Function

Function WiFiAccessPointIPv4Gateway:String()
	Return _PicoIPv4String(_WiFiAccessPointIPv4Gateway())
End Function
?
