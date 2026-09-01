SuperStrict

Public

Global producerLabel:String = "producer" + Chr(33)
Global finalizedCounters:Int

Function ProducerLabel:String()
	Return producerLabel
End Function

Function FinalizedCounterCount:Int()
	Return finalizedCounters
End Function

Function RecordFinalizedCounter()
	finalizedCounters :+ 1
End Function
