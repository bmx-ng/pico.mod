' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Module Pico.Tests.DerivedTypes
?pico

Import Pico.Tests.ImportedTypes

Public

Global finalizedModuleCounters:Int

Type TModuleCounter Extends TImportedCounter
	Field bonus:Int
	Field note:String
	Field history:Int[]
	Field envelope:TCounterEnvelope
	Field StaticArray fixedMetadata:TCounterMetadata[2]

	Method New(initial:Int, label:String, bonus:Int)
		Super.New(initial, label)
		Self.bonus = bonus
		note = "module" + Chr(33)
		history = New Int[2]
		history[0] = initial
		history[1] = bonus
		envelope.shape = New TCounterShape(initial, bonus)
		envelope.metadata.title = label + ":nested"
		envelope.metadata.values = history
		envelope.metadata.owner = Self
		envelope.history[0] = envelope.metadata
		fixedMetadata[0] = envelope.metadata
	End Method

	Method Add:Int(delta:Int) Override
		Return Super.Add(delta) + bonus
	End Method

	Method Label:String() Override
		Return Super.Label() + ":" + note
	End Method

	Method Snapshot:Int()
		Return history[0] + history[1]
	End Method

	Method Delete()
		finalizedModuleCounters :+ 1
	End Method
End Type

Function CreateModuleCounter:TModuleCounter(initial:Int, label:String, bonus:Int)
	Return New TModuleCounter(initial, label, bonus)
End Function

Function FinalizedModuleCounterCount:Int()
	Return finalizedModuleCounters
End Function
?
