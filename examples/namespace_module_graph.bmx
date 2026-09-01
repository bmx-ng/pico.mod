SuperStrict

Import BRL.StandardIO
Import Collections.ICollection
Import PicoGraphFixture.Utility

Local checksPassed:Int = PicoGraphFixtureValue(7) = 31

If Not checksPassed Then Throw "General Pico namespace module check failed"
Print "User-installed module imported through the general Pico module graph"
