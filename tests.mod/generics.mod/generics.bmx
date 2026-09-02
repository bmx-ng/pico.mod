' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Module Pico.Tests.Generics
?pico

Public

Struct SPair<T>
	Field first:T
	Field second:T

	Method New(first:T, second:T)
		Self.first = first
		Self.second = second
	End Method

	Method First:T()
		Return first
	End Method

	Method Second:T()
		Return second
	End Method

	Method AppendFirst:T(suffix:T)
		Local retained:T = first
		Return retained + suffix
	End Method
End Struct

Function Identity<T>:T(value:T)
	Return value
End Function

Function SelectValue<T>:T(condition:Int, whenTrue:T, whenFalse:T)
	If condition Then Return whenTrue
	Return whenFalse
End Function
?
