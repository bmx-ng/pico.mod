' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import BRL.Socket
Import Pico.Network.WiFi
Import Pico.Network.BLE

' Set these in a local copy to verify an active station link during BLE scan.
Const stationName:String = "<ssid>"
Const stationPassword:String = "<password>"

If WiFiInitialize(WiFiCountryUK) <> 0 Then Throw "Wi-Fi initialization failed"
If stationName <> "<ssid>"
	If WiFiConnectWait(stationName, stationPassword) <> 0 Then ..
		Throw "Station join failed"
Else
	If WiFiStartAccessPoint("BlitzMax-BLE-test", "", WiFiAuthenticationOpen, 6) <> 0 Then ..
		Throw "SoftAP start failed"
End If
If BLEInitialize("BlitzMax Pico") <> 0 Then Throw "BLE initialization failed"
If Not BLEWaitReady(10000) Then Throw "BLE controller did not become ready"
Local socket:TSocket = TSocket.CreateTCP()
If Not socket Then Throw "TCP socket creation failed alongside BLE"
If WiFiDeinitialize() = 0 Then Throw "Wi-Fi teardown accepted an active socket"
socket.Close()
If BLEStartScan(5000) <> 0 Then Throw "BLE scan failed"

Local deadline:UInt = MilliSecs() + 7000
While BLEScanActive() And MilliSecs() < deadline
	PollSystem()
	If stationName <> "<ssid>" And WiFiLinkStatus() <> WiFiLinkUp Then ..
		Throw "BLE scanning dropped the station link"
	If stationName = "<ssid>" And Not WiFiAccessPointActive() Then ..
		Throw "BLE scanning stopped SoftAP"
	Delay 1
Wend
If BLEScanActive() Then Throw "BLE scan did not complete"
If BLEDeinitialize() <> 0 Then Throw "BLE teardown failed"
If Not WiFiInitialized() Then Throw "BLE teardown released Wi-Fi"
If stationName <> "<ssid>" And WiFiLinkStatus() <> WiFiLinkUp Then ..
	Throw "BLE teardown dropped the station link"
If stationName <> "<ssid>" Then WiFiDisconnect()
If stationName = "<ssid>" Then
	If WiFiStopAccessPoint() <> 0 Then Throw "SoftAP stop failed"
End If
If WiFiDeinitialize() <> 0 Then Throw "Wi-Fi teardown failed"
Print "BLE/Wi-Fi coexistence passed"
