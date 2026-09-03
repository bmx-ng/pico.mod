' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Bounded access to persistent flash reserved by a Pico application build.
about: Configure the region with bmk's -storage option or the pico.storage
custom.bmk option. All offsets are relative to the reserved region, so this API
cannot erase or program the application image.
End Rem
Module Pico.Storage.Flash
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Rem
bbdoc: Status values returned by persistent-flash operations.
End Rem
Enum EFlashStorageResult:Int
	Success = 0
	Error = -1
	Timeout = -2
	NotPermitted = -4
	InvalidArgument = -5
	IOError = -6
	InsufficientResources = -9
	InvalidAddress = -10
	BadAlignment = -11
	InvalidState = -12
	UnsupportedModification = -18
	ResourceInUse = -21
End Enum

Extern "C"
	Rem
	bbdoc: Returns the physical flash capacity selected by the board definition.
	End Rem
	Function PhysicalFlashSize:UInt() = "bmx_pico_flash_storage_physical_size"

	Rem
	bbdoc: Returns the byte offset of the reserved storage region within flash.
	End Rem
	Function FlashStorageOffset:UInt() = "bmx_pico_flash_storage_offset"

	Rem
	bbdoc: Returns the size of the reserved storage region, or zero when storage is disabled.
	End Rem
	Function FlashStorageSize:UInt() = "bmx_pico_flash_storage_size"

	Rem
	bbdoc: Returns the board-dependent protected tail following the storage region.
	End Rem
	Function FlashStorageTailReservedSize:UInt() = "bmx_pico_flash_storage_tail_reserved_size"

	Function FlashStorageReadSize:UInt() = "bmx_pico_flash_storage_read_size"
	Function FlashStorageProgramSize:UInt() = "bmx_pico_flash_storage_program_size"
	Function FlashStorageEraseSize:UInt() = "bmx_pico_flash_storage_erase_size"

	Rem
	bbdoc: Reads bytes from the reserved region.
	End Rem
	Function FlashStorageRead:EFlashStorageResult(offset:UInt, destination:Byte Ptr, count:UInt) = "bmx_pico_flash_storage_read"

	Rem
	bbdoc: Returns one if the range is erased, zero if programmed, or a negative EFlashStorageResult on error.
	End Rem
	Function FlashStorageIsErased:Int(offset:UInt, count:UInt) = "bmx_pico_flash_storage_is_erased"

	Rem
	bbdoc: Programs one or more aligned 256-byte pages in the reserved region.
	about: The range must already be erased wherever programming would change a zero
	bit back to one. Source data is copied to RAM before flash access is suspended.
	End Rem
	Function FlashStorageProgram:EFlashStorageResult(offset:UInt, source:Byte Ptr, count:UInt, timeoutMilliseconds:UInt = 1000) = "bmx_pico_flash_storage_program"

	Rem
	bbdoc: Erases one or more aligned 4096-byte sectors in the reserved region.
	End Rem
	Function FlashStorageErase:EFlashStorageResult(offset:UInt, count:UInt, timeoutMilliseconds:UInt = 1000) = "bmx_pico_flash_storage_erase"
End Extern
?
