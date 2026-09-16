' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Framework BRL.EventQueue
Import BRL.StandardIO
Import Pico.Network.BLE

If BLEInitialize("BlitzMax Pico") <> 0 Then Throw "Unable to initialize BLE"
If Not BLEWaitReady(10000) Then Throw "BLE controller did not become ready"
If BLEStartScan(10000) <> 0 Then Throw "Unable to start BLE scan"

While True
	Select PollEvent()
		Case EVENT_BLESCANRESULT
			Local advertisement:TBLEAdvertisement = TBLEAdvertisement(EventExtra())
			Print advertisement.AddressString() + " " + advertisement.LocalName() + ..
				" RSSI " + advertisement.rssi
		Case EVENT_BLESCANCOMPLETE
			Print "BLE scan complete; dropped events: " + BLEDroppedEvents()
			Exit
	End Select
	Delay 1
Wend
BLEDeinitialize()
