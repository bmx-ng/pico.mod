' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict
Framework BRL.EventQueue
Import BRL.StandardIO
Import BRL.Socket
Import Pico.Network.WiFi
Import Pico.Network.BLE

Const serviceUUID:String = "7c9a0001-8e5f-4b2a-9d74-7b7d25f1a100"
Const characteristicUUID:String = "7c9a0002-8e5f-4b2a-9d74-7b7d25f1a100"

For Local startup:Int = 0 Until 8
    Print "Radio suite startup " + startup
    Delay 500
Next

If BLEGATTReset() <> 0 Then Throw "GATT reset failed"
Local service:TBLEGATTService = CreateBLEGATTService(serviceUUID)
If Not service Then Throw "GATT service creation failed: " + BLEGATTLastError()
Local initialValue:Byte[] = [Byte(72), Byte(101), Byte(108), Byte(108), Byte(111)]
Local characteristic:TBLEGATTCharacteristic = service.AddCharacteristic(characteristicUUID, ..
    BLEGATTRead | BLEGATTWrite | BLEGATTNotify, initialValue, 64)
If Not characteristic Then Throw "GATT characteristic creation failed: " + BLEGATTLastError()
Print "GATT service ready"

If WiFiInitialize(WiFiCountryUK) <> 0 Then Throw "Wi-Fi initialization failed"
If WiFiStartAccessPoint("BlitzMax-BLE-test", "", WiFiAuthenticationOpen, 6) <> 0 Then ..
    Throw "SoftAP start failed"
If Not WiFiAccessPointActive() Then Throw "SoftAP inactive"
Print "SoftAP at " + WiFiAccessPointIPv4Address()

If BLEInitialize("BlitzMax GATT") <> 0 Then Throw "BLE initialization failed"
If Not BLEWaitReady(10000) Then Throw "BLE controller did not become ready"
Print "BLE controller ready"
If BLEStartScan(6000) <> 0 Then Throw "BLE scan failed"
Local sightings:Int = 0
Local scanDeadline:UInt = MilliSecs() + 8000
While BLEScanActive() And MilliSecs() < scanDeadline
    Select PollEvent()
        Case EVENT_BLESCANRESULT
            Local advertisement:TBLEAdvertisement = TBLEAdvertisement(EventExtra())
            sightings :+ 1
            If sightings <= 5 Then Print "Saw " + advertisement.AddressString() + " RSSI " + advertisement.rssi
    End Select
    If Not WiFiAccessPointActive() Then Throw "BLE scan stopped SoftAP"
    Delay 1
Wend
If BLEScanActive() Then Throw "BLE scan did not complete"
Print "BLE scan complete, sightings: " + sightings + ", dropped: " + BLEDroppedEvents()

If BLEStartAdvertising(service, 30000) <> 0 Then Throw "GATT advertising failed"
If Not BLEAdvertisingActive() Then Throw "GATT advertising inactive after start"
Print "GATT advertising with SoftAP for 30 seconds"
Local deadline:UInt = MilliSecs() + 32000
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
    If Not WiFiAccessPointActive() Then Throw "BLE advertising stopped SoftAP"
    Delay 1
Wend
If BLEAdvertisingActive() Then Throw "GATT advertising did not complete"
For Local index:Int = 0 Until BLEConnectionCount()
    Local info:TBLEConnectionInfo = BLEConnectionAt(index)
    If info Then BLEDisconnect(info.connectionHandle)
Next
Local disconnectDeadline:UInt = MilliSecs() + 5000
While BLEConnectionCount() And MilliSecs() < disconnectDeadline
    PollSystem()
    Delay 1
Wend
Print "Creating TCP socket"
Local socket:TSocket = TSocket.CreateTCP()
If Not socket Then Throw "TCP socket creation failed alongside BLE"
Print "TCP socket created"
Local apStopResult:Int = WiFiStopAccessPoint()
Print "SoftAP stop with socket returned: " + apStopResult
If apStopResult = 0 Then Throw "SoftAP stop accepted an active socket"
Local teardownResult:Int = WiFiDeinitialize()
Print "Wi-Fi teardown with socket returned: " + teardownResult
If teardownResult = 0 Then Throw "Wi-Fi teardown accepted an active socket"
socket.Close()
Print "Socket and radio lifetime checks passed"

If BLEDeinitialize() <> 0 Then Throw "BLE teardown failed"
If Not WiFiInitialized() Then Throw "BLE teardown released Wi-Fi"
If WiFiStopAccessPoint() <> 0 Then Throw "SoftAP stop failed"
If WiFiDeinitialize() <> 0 Then Throw "Wi-Fi teardown failed"
Print "Radio suite passed"
' Keep USB responsive so picotool can return the board to BOOTSEL mode.
While True
    PollSystem()
    Delay 100
Wend
