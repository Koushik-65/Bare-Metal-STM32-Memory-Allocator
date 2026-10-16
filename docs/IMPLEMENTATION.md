# Implementation

## Overview

The allocator manages a fixed 1024-byte buffer on the STM32F103C8T6. It uses block headers stored inside the buffer, a first-fit search for released blocks, and a bump offset (`used`) when no suitable free block is available. Allocation sizes, including header overhead, are rounded to multiples of eight bytes.

The implementation is in `Src/main.cpp`. The memory pool is placed in a custom linker section defined in `STM32F103C8TX_FLASH.ld`.

## Linker-controlled SRAM placement

The pool is declared as:

```cpp
alignas(8) __attribute__((section(".allocator_pool")))
static uint8_t memory_pool[POOL_SIZE];
```

The linker script includes:

```ld
.allocator_pool :
{
    . = ALIGN(8);
    *(.allocator_pool)
    . = ALIGN(8);
} >RAM
```

The `RAM` region begins at `0x20000000` and has a declared length of 20 KB. The `.allocator_pool` output section is placed in that RAM region, after the script's `._user_heap_stack` section. It is a separate *linker section*, not a separate physical RAM bank or a fixed-address memory partition. Its exact address depends on the linked image and can be checked in the linker map or debugger.

The linker determines where the 1024-byte pool is stored; the allocator determines how its contents are divided into blocks. The pool does not draw storage from the C library heap.

## Block layout

Each block begins with a `BlockHeader` containing:

- `block_size`: total block size, including the header, payload, and any padding.
- `is_free`: whether the block is available for reuse.

The allocator returns a pointer immediately after the header. The total size of a new block is calculated as:

```text
align_up(sizeof(BlockHeader) + requested_size, 8)
```

The `used` variable marks the end of the portion of the pool already organized into blocks. It is not a count of currently live payload bytes: freeing a block does not decrease `used`.

## Allocation: `my_malloc()`

1. Reject a zero-byte request or one larger than `POOL_SIZE - sizeof(BlockHeader)`.
2. Compute the header-inclusive block size, rounded up to an 8-byte multiple.
3. Search existing blocks from the beginning of the pool and select the first free block large enough.
4. Reuse that block. If the leftover space is at least `MIN_SPLIT_SIZE` (16 bytes), create a free-block header in the remainder. Otherwise, consume the entire existing block.
5. If no free block fits, append a new block at `memory_pool + used`, provided enough unallocated pool capacity remains.
6. Return the payload pointer, or `nullptr` if no allocation can be made.

The search walks blocks using the `block_size` values stored in their headers; it does not maintain a separate linked free list.

## Deallocation and coalescing

`my_free()` returns immediately for a null pointer. For a non-null pointer, it locates the header by subtracting `sizeof(BlockHeader)` from the payload pointer, marks the block free, and calls `coalesce()`.

`coalesce()` walks the organized portion of the pool and merges consecutive free blocks by adding their sizes. It checks the same block again after each merge, allowing runs of more than two adjacent free blocks to be combined.

The implementation assumes non-null pointers passed to `my_free()` refer to valid allocations from this pool. It does not check pointer ownership or guard against double-free operations.

## Debugger inspection

`update_memory_state()` scans blocks and populates `block_info_array`, with capacity for 64 entries. It records each block's header address, total size, and free state. It also computes:

- `total_free_blocks_memory`: sum of the sizes of reusable free blocks, including their headers.
- `largest_free_block`: size of the largest reusable free block.
- `fragmentation_percent`: `100 * (1 - largest_free_block / total_free_blocks_memory)` when reusable free blocks exist; otherwise zero.

These values describe the reusable free blocks in the organized portion of the pool, **not** the unallocated space beyond `used`. The current code refreshes these statistics after successful allocations, but not immediately after `my_free()`. To inspect current values after freeing, call `update_memory_state()` or perform a subsequent successful allocation.

## On-target exercise

The `main()` function performs allocations of 100, 25, and 25 bytes, frees the first block, then requests 8 and 25 bytes to exercise reuse. It subsequently frees another block and requests 35 bytes. The firmware then remains in an infinite loop so the memory pool and metadata can be inspected with STM32CubeIDE and ST-Link.

The exercise provides debugger-visible examples of allocator operations; it is not an exhaustive test suite for invalid pointers, fragmentation, or allocation failure paths.
