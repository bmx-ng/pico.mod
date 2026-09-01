SuperStrict

Module Pico.Tests.GenericObjectHooks

Public

Global baseCompareCalls:UInt
Global baseHashCodeCalls:UInt
Global baseEqualsCalls:UInt
Global derivedCompareCalls:UInt
Global derivedHashCodeCalls:UInt
Global derivedEqualsCalls:UInt

Function ResetGenericObjectHookCounts()
	baseCompareCalls = 0
	baseHashCodeCalls = 0
	baseEqualsCalls = 0
	derivedCompareCalls = 0
	derivedHashCodeCalls = 0
	derivedEqualsCalls = 0
End Function

Function BaseGenericObjectHookCalls:UInt()
	Return baseCompareCalls + baseHashCodeCalls + baseEqualsCalls
End Function

Function DerivedGenericObjectHookCalls:UInt()
	Return derivedCompareCalls + derivedHashCodeCalls + derivedEqualsCalls
End Function

Type TGenericHookBase<T>
	Field marker:T

	Method Compare:Int(other:Object) Override
		baseCompareCalls :+ 1
		Return -10
	End Method

	Method HashCode:UInt() Override
		baseHashCodeCalls :+ 1
		Return 100
	End Method

	Method Equals:Int(other:Object) Override
		baseEqualsCalls :+ 1
		Return True
	End Method
End Type

Type TGenericHookDerived<T> Extends TGenericHookBase<T>
	Method Compare:Int(other:Object) Override
		derivedCompareCalls :+ 1
		Return -20
	End Method

	Method HashCode:UInt() Override
		derivedHashCodeCalls :+ 1
		Return 200
	End Method

	Method Equals:Int(other:Object) Override
		derivedEqualsCalls :+ 1
		Return False
	End Method
End Type

Type TGenericHookInherited<T> Extends TGenericHookBase<T>
End Type
