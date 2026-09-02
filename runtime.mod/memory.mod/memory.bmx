' Copyright (c) 2026 Bruce A Henderson and contributors
' SPDX-License-Identifier: Zlib

SuperStrict

Rem
bbdoc: Prototype heap, precise Object collection, and allocation metrics for Pico targets.
End Rem
Module Pico.Runtime.Memory
?pico

ModuleInfo "Version: 0.1"
ModuleInfo "License: zlib/libpng"

Extern "C"
	Function ArenaAllocate:Byte Ptr(bytes:UInt) = "bmx_pico_arena_allocate"
	Function ArenaCapacity:UInt() = "bmx_pico_arena_capacity"
	Function ArenaUsed:UInt() = "bmx_pico_arena_used"
	Function ArenaRemaining:UInt() = "bmx_pico_arena_remaining"
	Function ArenaHighWater:UInt() = "bmx_pico_arena_high_water"
	Function ArenaAllocationCount:UInt() = "bmx_pico_arena_allocation_count"
	Function ArenaFailureCount:UInt() = "bmx_pico_arena_failure_count"
	Function ArrayFailureCount:UInt() = "bmx_pico_array_failure_count"
	Function ArrayAllocationCount:UInt() = "bmx_pico_array_allocation_count"
	Function ArrayAllocatedBytes:UInt() = "bmx_pico_array_allocated_bytes"
	Function ArrayLiveCount:UInt() = "bmx_pico_array_live_count"
	Function ArrayLiveBytes:UInt() = "bmx_pico_array_live_bytes"
	Function ReachableArrayCount:UInt() = "bmx_pico_reachable_array_count"
	Function UnreachableArrayCount:UInt() = "bmx_pico_unreachable_array_count"
	Function StringFailureCount:UInt() = "bmx_pico_string_failure_count"
	Function StringAllocationCount:UInt() = "bmx_pico_string_allocation_count"
	Function StringAllocatedBytes:UInt() = "bmx_pico_string_allocated_bytes"
	Function StringLiveCount:UInt() = "bmx_pico_string_live_count"
	Function StringLiveBytes:UInt() = "bmx_pico_string_live_bytes"
	Function ReachableStringCount:UInt() = "bmx_pico_reachable_string_count"
	Function UnreachableStringCount:UInt() = "bmx_pico_unreachable_string_count"
	Function EnumFailureCount:UInt() = "bmx_pico_enum_failure_count"
	Function ObjectFailureCount:UInt() = "bmx_pico_object_failure_count"
	Function ObjectAllocationCount:UInt() = "bmx_pico_object_allocation_count"
	Function ObjectAllocatedBytes:UInt() = "bmx_pico_object_allocated_bytes"
	Function ObjectLiveCount:UInt() = "bmx_pico_object_live_count"
	Function ObjectLiveBytes:UInt() = "bmx_pico_object_live_bytes"
	Function ObjectRootRetain:UInt(value:Object) = "bmx_pico_object_root_retain"
	Function ObjectRootRelease(token:UInt) = "bmx_pico_object_root_release"
	Function ObjectRootCount:UInt() = "bmx_pico_object_root_count"
	Function ReachabilityAudit:UInt() = "bmx_pico_reachability_audit"
	Function ReachableObjectCount:UInt() = "bmx_pico_reachable_object_count"
	Function UnreachableObjectCount:UInt() = "bmx_pico_unreachable_object_count"
	Function InvalidReferenceCount:UInt() = "bmx_pico_invalid_reference_count"
	Function CollectObjects:UInt() = "bmx_pico_collect_objects"
	Function CollectionCount:UInt() = "bmx_pico_collection_count"
	Function AutomaticCollectionCount:UInt() = "bmx_pico_automatic_collection_count"
	Function LastReclaimedObjectCount:UInt() = "bmx_pico_last_reclaimed_object_count"
	Function LastReclaimedBytes:UInt() = "bmx_pico_last_reclaimed_bytes"
	Function LastReclaimedArrayCount:UInt() = "bmx_pico_last_reclaimed_array_count"
	Function LastReclaimedArrayBytes:UInt() = "bmx_pico_last_reclaimed_array_bytes"
	Function LastReclaimedStringCount:UInt() = "bmx_pico_last_reclaimed_string_count"
	Function LastReclaimedStringBytes:UInt() = "bmx_pico_last_reclaimed_string_bytes"
	Function FinalizerPendingCount:UInt() = "bmx_pico_finalizer_pending_count"
	Function FinalizerInvocationCount:UInt() = "bmx_pico_finalizer_invocation_count"
	Function LastFinalizedObjectCount:UInt() = "bmx_pico_last_finalized_object_count"
	Function HeapReusableBytes:UInt() = "bmx_pico_heap_reusable_bytes"
	Function HeapLargestFreeBlock:UInt() = "bmx_pico_heap_largest_free_block"
	Function RootFrameCount:UInt() = "bmx_pico_root_frame_count"
	Function RootSlotCount:UInt() = "bmx_pico_root_slot_count"
	Function ExceptionDepth:UInt() = "bmx_pico_exception_depth"
	Function ExceptionThrowCount:UInt() = "bmx_pico_exception_throw_count"
	Function ExceptionCatchCount:UInt() = "bmx_pico_exception_catch_count"
	Function ExceptionMaxDepth:UInt() = "bmx_pico_exception_max_depth"
	Function ExceptionUnhandledCount:UInt() = "bmx_pico_exception_unhandled_count"
End Extern
?
