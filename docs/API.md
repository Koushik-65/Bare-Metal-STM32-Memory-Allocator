# Allocator API

The allocator exposes two functions in `Src/main.cpp`: `my_malloc()` and `my_free()`. Both operate on the project's fixed 1024-byte memory pool. The implementation does not call the standard library's `malloc()` or `free()`.

## `my_malloc()`

```cpp
void* my_malloc(size_t requested_size);
```

Allocates space for `requested_size` bytes from the pool.

**Returns**

- A pointer to the allocation's payload on success.
- `nullptr` for a zero-byte request, a request larger than the pool can accommodate after accounting for a block header, or when no suitable space is available.

**Behavior**

- Accounts for a block header and rounds the total block size up to a multiple of 8 bytes.
- Reuses the first sufficiently large free block when one is available. A larger block may be split to preserve the remainder for reuse.
- Otherwise allocates from the unused end of the pool, provided enough capacity remains.
- Updates the debugger-facing block information and fragmentation statistics after a successful allocation.

The returned memory is **not initialized** by this function. The usable allocation is at least the requested size; metadata and padding also consume pool capacity.

## `my_free()`

```cpp
void my_free(void* user_ptr);
```

Releases a block previously returned by `my_malloc()` so that its storage can be reused.

**Returns**

- Nothing (`void`).

**Behavior**

- Passing `nullptr` has no effect.
- Marks the referenced block free and merges adjacent free blocks.
- Does **not** immediately refresh the debugger-facing block information or fragmentation statistics. Those are refreshed by a subsequent successful `my_malloc()` call or an explicit internal call to `update_memory_state()`.

**Precondition:** `user_ptr` must be a currently allocated pointer returned by `my_malloc()` that has not already been freed. The implementation does not validate arbitrary pointers or detect double frees. Behavior outside this precondition is not defined by this API.

## Limits and usage notes

- The pool has a fixed capacity of **1024 bytes**, including block headers and alignment padding.
- An allocation can fail even when some free space remains, if no single suitable block is available and the unused end of the pool is too small.
- This implementation provides no synchronization for concurrent callers.
- These functions are defined directly in `Src/main.cpp`; the project does not currently provide a separate public API header.

For block layout, linker placement, and allocation algorithms, see [IMPLEMENTATION.md](IMPLEMENTATION.md).
