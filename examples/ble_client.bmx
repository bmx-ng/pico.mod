' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Network.BLE

If BLEInitialize("BlitzMax Pico client") <> 0 Then Throw "BLE initialization failed"
If Not BLEWaitReady(10000) Then Throw "BLE controller did not become ready"
If BLEStartScan(15000) <> 0 Then Throw "BLE scan failed"

Local connected:Int
Local finished:Int
Local deadline:UInt = MilliSecs() + 30000
While Not finished And MilliSecs() < deadline
	Select PollEvent()
		Case EVENT_BLESCANRESULT
			Local advertisement:TBLEAdvertisement = TBLEAdvertisement(EventExtra())
			If advertisement.IsConnectable() And Not connected
				Print "Connecting to " + advertisement.AddressString()
				BLEStopScan()
				If BLEConnectAdvertisement(advertisement) <> 0 Then Throw "BLE connect failed"
				connected = True
			End If
		Case EVENT_BLECONNECTED
			Local connection:TBLEConnectionEvent = TBLEConnectionEvent(EventExtra())
			If connection.status <> 0 Then Throw "BLE connection failed: " + connection.status
			Print "Connected; discovering services"
			If connection.DiscoverServices() <> 0 Then Throw "GATT discovery failed"
		Case EVENT_BLESERVICEDISCOVERED
			Local service:TBLEClientService = TBLEClientService(EventExtra())
			Print "Service " + service.UUID()
		Case EVENT_BLESERVICEDISCOVERYCOMPLETE
			Print "Service discovery complete"
			finished = True
	End Select
	Delay 1
Wend

For Local index:Int = 0 Until BLEConnectionCount()
	Local info:TBLEConnectionInfo = BLEConnectionAt(index)
	If info Then BLEDisconnect(info.connectionHandle)
Next
Local disconnectDeadline:UInt = MilliSecs() + 5000
While BLEConnectionCount() And MilliSecs() < disconnectDeadline
	PollSystem()
	Delay 1
Wend
BLEDeinitialize()
