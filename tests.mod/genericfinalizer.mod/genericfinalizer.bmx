SuperStrict

Module Pico.Tests.GenericFinalizer

Public

Global genericBaseFinalized:UInt
Global genericDerivedFinalized:UInt

Function ResetGenericFinalizerCounts()
	genericBaseFinalized = 0
	genericDerivedFinalized = 0
End Function

Function GenericBaseFinalizedCount:UInt()
	Return genericBaseFinalized
End Function

Function GenericDerivedFinalizedCount:UInt()
	Return genericDerivedFinalized
End Function

Type TGenericFinalizerBase<T>
	Field value:T

	Method New(value:T)
		Self.value = value
	End Method

	Method Delete()
		genericBaseFinalized :+ 1
	End Method
End Type

Type TGenericFinalizerDerived<T> Extends TGenericFinalizerBase<T>
	Method New(value:T)
		Super.New(value)
	End Method

	Method Delete()
		genericDerivedFinalized :+ 1
	End Method
End Type

Type TGenericInheritedFinalizer<T> Extends TGenericFinalizerBase<T>
End Type
