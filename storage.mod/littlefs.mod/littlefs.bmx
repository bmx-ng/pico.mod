' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Power-safe persistent filesystem for reserved Pico flash.
about: Importing this module installs LittleFS as the default BRL.FileSystem
backend. Ordinary paths and the file:: and littlefs:: protocols address the
filesystem. A blank reserved region is formatted automatically; existing
unrecognised data is never formatted automatically.
End Rem
Module Pico.Storage.LittleFS
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"
ModuleInfo "CC_OPTS: -DLFS_NO_MALLOC"

Import BRL.FileSystem
Import Pico.Storage.Flash

Import "littlefs/lfs.c"
Import "littlefs/lfs_util.c"
Import "glue.c"

Extern "C"
	Function _LittleFSMount:Int(formatBlank:Int) = "bmx_pico_littlefs_mount"
	Function _LittleFSUnmount:Int() = "bmx_pico_littlefs_unmount"
	Function _LittleFSFormat:Int() = "bmx_pico_littlefs_format"
	Function LittleFSIsMounted:Int() = "bmx_pico_littlefs_is_mounted"
	Function LittleFSLastError:Int() = "bmx_pico_littlefs_last_error"
	Function LittleFSCapacity:UInt() = "bmx_pico_littlefs_capacity"
	Function LittleFSUsed:Long() = "bmx_pico_littlefs_used"

	Function _LittleFSOpen:Byte Ptr(path:String, readable:Int, writeMode:Int) = "bmx_pico_littlefs_open"
	Function _LittleFSClose:Int(handle:Byte Ptr) = "bmx_pico_littlefs_close"
	Function _LittleFSRead:Long(handle:Byte Ptr, buffer:Byte Ptr, count:Long) = "bmx_pico_littlefs_read"
	Function _LittleFSWrite:Long(handle:Byte Ptr, buffer:Byte Ptr, count:Long) = "bmx_pico_littlefs_write"
	Function _LittleFSPosition:Long(handle:Byte Ptr) = "bmx_pico_littlefs_position"
	Function _LittleFSSize:Long(handle:Byte Ptr) = "bmx_pico_littlefs_size"
	Function _LittleFSSeek:Long(handle:Byte Ptr, offset:Long, whence:Int) = "bmx_pico_littlefs_seek"
	Function _LittleFSResize:Int(handle:Byte Ptr, size:Long) = "bmx_pico_littlefs_resize"
	Function _LittleFSFlush:Int(handle:Byte Ptr) = "bmx_pico_littlefs_flush"

	Function _LittleFSStat:Int(path:String, fileType:Int Var, size:Long Var, modified:Long Var, created:Long Var, accessed:Long Var) = "bmx_pico_littlefs_stat"
	Function _LittleFSSetTime:Int(path:String, time:Long, timeType:Int) = "bmx_pico_littlefs_set_time"
	Function _LittleFSMkdir:Int(path:String) = "bmx_pico_littlefs_mkdir"
	Function _LittleFSRemove:Int(path:String) = "bmx_pico_littlefs_remove"
	Function _LittleFSRename:Int(oldPath:String, newPath:String) = "bmx_pico_littlefs_rename"
	Function _LittleFSDirectoryOpen:Byte Ptr(path:String) = "bmx_pico_littlefs_directory_open"
	Function _LittleFSDirectoryNext:String(handle:Byte Ptr) = "bmx_pico_littlefs_directory_next"
	Function _LittleFSDirectoryClose:Int(handle:Byte Ptr) = "bmx_pico_littlefs_directory_close"
End Extern

Rem
bbdoc: Mounts the reserved flash filesystem.
about: When @formatBlank is true, a completely erased region is formatted if
it cannot be mounted. Nonblank unrecognised data is left untouched.
End Rem
Function MountLittleFS:Int(formatBlank:Int = True)
	If _littleFSBackend = Null Then _littleFSBackend = New TLittleFSBackend
	Return _LittleFSMount(formatBlank) = 0
End Function

Rem
bbdoc: Flushes and unmounts the reserved flash filesystem.
End Rem
Function UnmountLittleFS:Int()
	Return _LittleFSUnmount() = 0
End Function

Rem
bbdoc: Erases and formats the reserved region, then mounts the new filesystem.
about: This destroys every file in the reserved region.
End Rem
Function FormatLittleFS:Int()
	Return _LittleFSFormat() = 0
End Function

Rem
bbdoc: Seekable stream backed by a LittleFS file.
End Rem
Type TLittleFSStream Extends TStream
	Private
	Field _handle:Byte Ptr
	Field _readable:Int
	Field _writeMode:Int

	Public
	Method Pos:Long() Override
		If Not _handle Then Return -1
		Return _LittleFSPosition(_handle)
	End Method

	Method Size:Long() Override
		If Not _handle Then Return 0
		Return _LittleFSSize(_handle)
	End Method

	Method Seek:Long(position:Long, whence:Int = SEEK_SET_) Override
		If Not _handle Then Return -1
		Return _LittleFSSeek(_handle, position, whence)
	End Method

	Method Flush() Override
		If _handle And _LittleFSFlush(_handle) < 0 Then
			Throw "Unable to flush LittleFS stream (error " + LittleFSLastError() + ")"
		End If
	End Method

	Method Close() Override
		If Not _handle Then Return
		Local handle:Byte Ptr = _handle
		_handle = Null
		If _LittleFSClose(handle) < 0 Then
			Throw "Unable to close LittleFS stream (error " + LittleFSLastError() + ")"
		End If
	End Method

	Method Read:Long(buffer:Byte Ptr, count:Long) Override
		If Not _handle Or Not _readable Then Throw New TStreamReadException
		Local result:Long = _LittleFSRead(_handle, buffer, count)
		If result < 0 Then Throw New TStreamReadException
		Return result
	End Method

	Method Write:Long(buffer:Byte Ptr, count:Long) Override
		If Not _handle Or Not _writeMode Then Throw New TStreamWriteException
		Local result:Long = _LittleFSWrite(_handle, buffer, count)
		If result < 0 Then Throw New TStreamWriteException
		Return result
	End Method

	Method SetSize:Int(size:Long) Override
		If Not _handle Or Not _writeMode Then Return False
		Return _LittleFSResize(_handle, size) = 0
	End Method

	Function Open:TLittleFSStream(path:String, readable:Int, writeMode:Int)
		Local handle:Byte Ptr = _LittleFSOpen(path, readable, writeMode)
		If Not handle Then Return Null
		Local stream:TLittleFSStream = New TLittleFSStream
		stream._handle = handle
		stream._readable = readable
		stream._writeMode = writeMode
		Return stream
	End Function
End Type

Private

Function _LittleFSRealPath:String(path:String)
	Local result:String = RealPath(path)
	If Not result.length Then Return "/"
	Return result
End Function

Type TLittleFSBackend Extends TFileSystemBackend
	Field _currentDirectory:String = "/"

	Method New()
		SetDefaultFileSystemBackend(Self)
	End Method

	Method HandlesProtocol:Int(protocol:String) Override
		protocol = protocol.ToLower()
		Return protocol = "littlefs" Or protocol = "file"
	End Method

	Method OpenPath:TStream(path:String, readable:Int, writeMode:Int) Override
		Return TLittleFSStream.Open(_LittleFSRealPath(path), readable, writeMode)
	End Method

	Method CurrentDirectory:String() Override
		Return _currentDirectory
	End Method

	Method ChangeDirectory:Int(path:String) Override
		Local realPath:String = _LittleFSRealPath(path)
		Local info:SFileStat
		If Not Stat(realPath, info) Or info.fileType <> FILETYPE_DIR Then Return False
		_currentDirectory = realPath
		Return True
	End Method

	Method Stat:Int(path:String, info:SFileStat Var) Override
		Local fileType:Int
		Local size:Long
		Local modified:Long
		Local created:Long
		Local accessed:Long
		If _LittleFSStat(_LittleFSRealPath(path), fileType, size, modified, created, accessed) < 0 Then Return False
		info.fileType = fileType
		info.size = size
		info.modifiedTime = modified
		info.creationTime = created
		info.accessTime = accessed
		info.isReadOnly = False
		Return True
	End Method

	Method SetTime(path:String, time:Long, timeType:Int) Override
		_LittleFSSetTime(_LittleFSRealPath(path), time, timeType)
	End Method

	Method FileMode:Int(path:String) Override
		Local info:SFileStat
		If Not Stat(path, info) Then Return -1
		If info.fileType = FILETYPE_DIR Then Return $1FF
		Return $1B6
	End Method

	Method CreateFile:Int(path:String) Override
		Local stream:TLittleFSStream = TLittleFSStream.Open(_LittleFSRealPath(path), False, WRITE_MODE_OVERWRITE)
		If Not stream Then Return False
		stream.Close()
		Return True
	End Method

	Method CreateDirectory:Int(path:String) Override
		Return _LittleFSMkdir(_LittleFSRealPath(path)) = 0
	End Method

	Method DeleteFile:Int(path:String) Override
		Local info:SFileStat
		If Not Stat(path, info) Or info.fileType <> FILETYPE_FILE Then Return False
		Return _LittleFSRemove(_LittleFSRealPath(path)) = 0
	End Method

	Method DeleteDirectory:Int(path:String) Override
		Local info:SFileStat
		If Not Stat(path, info) Or info.fileType <> FILETYPE_DIR Then Return False
		Return _LittleFSRemove(_LittleFSRealPath(path)) = 0
	End Method

	Method Rename:Int(oldPath:String, newPath:String) Override
		Return _LittleFSRename(_LittleFSRealPath(oldPath), _LittleFSRealPath(newPath)) = 0
	End Method

	Method OpenDirectory:Byte Ptr(path:String) Override
		Return _LittleFSDirectoryOpen(_LittleFSRealPath(path))
	End Method

	Method NextDirectoryEntry:String(handle:Byte Ptr) Override
		Return _LittleFSDirectoryNext(handle)
	End Method

	Method CloseDirectory(handle:Byte Ptr) Override
		_LittleFSDirectoryClose(handle)
	End Method
End Type

Global _littleFSBackend:TLittleFSBackend = New TLittleFSBackend
Global _littleFSMounted:Int = MountLittleFS(True)
?
