SuperStrict

Framework BRL.StandardIO
Import Pico.Storage.Flash

Print "physical=" + PhysicalFlashSize()
Print "offset=" + FlashStorageOffset()
Print "storage=" + FlashStorageSize()
Print "tail=" + FlashStorageTailReservedSize()
Print "read=" + FlashStorageReadSize()
Print "program=" + FlashStorageProgramSize()
Print "erase=" + FlashStorageEraseSize()
