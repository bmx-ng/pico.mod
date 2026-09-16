' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Network.BLE

Const serviceUUID:String = "7c9a0001-8e5f-4b2a-9d74-7b7d25f1a100"
Const characteristicUUID:String = "7c9a0002-8e5f-4b2a-9d74-7b7d25f1a100"

If BLEGATTReset() <> 0 Then Throw "GATT reset failed"
Local service:TBLEGATTService = CreateBLEGATTService(serviceUUID)
If Not service Then Throw "GATT service creation failed: " + BLEGATTLastError()
Local initialValue:Byte[] = [Byte(72), Byte(101), Byte(108), Byte(108), Byte(111)]
Local characteristic:TBLEGATTCharacteristic = service.AddCharacteristic(characteristicUUID, ..
	BLEGATTRead | BLEGATTWrite | BLEGATTNotify, initialValue, 64)
If Not characteristic Then Throw "GATT characteristic creation failed: " + BLEGATTLastError()
If BLEInitialize("BlitzMax GATT") <> 0 Then Throw "BLE initialization failed"
If Not BLEWaitReady(10000) Then Throw "BLE controller did not become ready"
If BLEStartAdvertising(service, 30000) <> 0 Then Throw "BLE advertising failed"
Print "GATT peripheral advertising for 30 seconds"

Local deadline:UInt = MilliSecs() + 35000
While MilliSecs() < deadline
	Select PollEvent()
		Case EVENT_BLECONNECTED
			Local connection:TBLEConnectionEvent = TBLEConnectionEvent(EventExtra())
			Print "Connected: " + connection.AddressString()
		Case EVENT_BLEGATTWRITE
			Local writeEvent:TBLEGATTWriteEvent = TBLEGATTWriteEvent(EventExtra())
			Print "Received " + writeEvent.value.length + " bytes"
			If writeEvent.characteristic Then writeEvent.characteristic.SetValue(writeEvent.value, True)
		Case EVENT_BLEGATTSUBSCRIBE
			Local subscription:TBLEGATTSubscriptionEvent = TBLEGATTSubscriptionEvent(EventExtra())
			Print "Notifications enabled: " + subscription.notifications
		Case EVENT_BLEADVERTISINGCOMPLETE
			Exit
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
If BLEDeinitialize() <> 0 Then Throw "BLE teardown failed"
Print "GATT peripheral stopped"
