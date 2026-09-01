SuperStrict

Module Pico.Tests.GenericInheritance

Public

Interface IGenericValue<T>
	Method Value:T()
End Interface

Type TGenericBase<T> Implements IGenericValue<T>
	Field baseValue:T

	Method New(value:T)
		baseValue = value
	End Method

	Method Value:T()
		Return baseValue
	End Method

	Method Dispatch:T()
		Return Value()
	End Method
End Type
