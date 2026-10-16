#include <stdint.h>
#include <stddef.h>
#include <new>

#define POOL_SIZE 1024
#define BLOCK_INFO_SIZE 64  // Maximum number of blocks tracked for debugging.
#define MIN_SPLIT_SIZE 16

alignas(8) __attribute__((section(".allocator_pool")))
static uint8_t memory_pool[POOL_SIZE];

static size_t used = 0;

// Fragmentation statistics for reusable free blocks.
static size_t total_free_blocks_memory = 0;
static size_t largest_free_block = 0;
static float fragmentation_percent = 0.0f;

// Metadata stored before each allocated memory block.
struct BlockHeader{
	size_t block_size;
	bool is_free;
};

// Information used to inspect blocks in the debugger.
struct BlockInfo{

	BlockHeader* address;
	size_t block_size;
	bool is_free;

};

// Stores information about up to 64 blocks.
static BlockInfo block_info_array[BLOCK_INFO_SIZE];
static int block_counter = 0;



void update_memory_state() {
    size_t offset = 0;

    // Reset block tracking and fragmentation statistics.
    block_counter = 0;
    total_free_blocks_memory = 0;
    largest_free_block = 0;
    fragmentation_percent = 0.0f;

    // Traverse allocated blocks and collect metadata.
    while (offset < used && block_counter < BLOCK_INFO_SIZE) {

        BlockHeader* header =
            reinterpret_cast<BlockHeader*>(&memory_pool[offset]);

        // Store metadata for debugger inspection.
        block_info_array[block_counter].address = header;
        block_info_array[block_counter].block_size = header->block_size;
        block_info_array[block_counter].is_free = header->is_free;

        // Track reusable memory and the largest free block.
        if (header->is_free) {
            total_free_blocks_memory += header->block_size;

            if (header->block_size > largest_free_block) {
                largest_free_block = header->block_size;
            }
        }

        // Move to the next block.
        offset += header->block_size;
        block_counter++;
    }

    // Calculate fragmentation among reusable free blocks.
    if (total_free_blocks_memory > 0) {
        fragmentation_percent =
            (1.0f - static_cast<float>(largest_free_block) /
            static_cast<float>(total_free_blocks_memory)) * 100.0f;
    }
}


void split_block(BlockHeader* new_block_header,
                 size_t original_size,
                 size_t allocated_size) {

	//such nice syntax, this line deserves its own comment.
    // Construct metadata for the remaining free portion.
    new (new_block_header) BlockHeader{original_size - allocated_size,true};
}


BlockHeader* find_free_block(size_t* required_size) {
    size_t offset = 0;

    // First-fit search: return the first free block large enough.
    while (offset < used) {
        BlockHeader* header =
            reinterpret_cast<BlockHeader*>(&memory_pool[offset]);

        if (header->is_free && header->block_size >= *required_size) {

            // Avoid splitting when the remainder is too small.
            if (header->block_size - *required_size < MIN_SPLIT_SIZE) {
                *required_size = header->block_size;
            }

            else {

                // Create a free block from the remaining memory.
                BlockHeader* remaining_block = reinterpret_cast<BlockHeader*>(reinterpret_cast<uint8_t*>(header) + *required_size);

                split_block(remaining_block, header->block_size, *required_size);
            }

            return header;
        }

        // Advance to the next block header.
        offset += header->block_size;
    }

    return nullptr;
}



void coalesce() {
    size_t offset = 0;

    // Merge adjacent free blocks to reduce fragmentation.
    while (offset < used) {

    	BlockHeader* current_header = reinterpret_cast<BlockHeader*>(&memory_pool[offset]);
        size_t next_offset = offset + current_header->block_size;

        // No more blocks to inspect so we stop
        if (next_offset >= used) {
            break;
        }

        BlockHeader* next_header =
            reinterpret_cast<BlockHeader*>(&memory_pool[next_offset]);

        if (current_header->is_free && next_header->is_free) {
            current_header->block_size += next_header->block_size;

            // Recheck the same block for additional free neighbors.
            continue;
        }

        offset = next_offset;
    }
}



void* my_malloc(size_t requested_size) {

    // return nullptr for zero-size requests and allocations more than total memory pool size.
    if (requested_size == 0 ||
        requested_size > POOL_SIZE - sizeof(BlockHeader)) {
        return nullptr;
    }

    //Calculating total block size needed and round up to 8-byte alignment.
    size_t raw_size = sizeof(BlockHeader) + requested_size;
    size_t padding = (8 - (raw_size % 8)) % 8;
    size_t block_size = raw_size + padding;

    // Reuse an existing free block using first-fit allocation.
    BlockHeader* free_block = find_free_block(&block_size);

    if (free_block != nullptr) {
        new (free_block) BlockHeader{block_size, false};

        void* user_data =
            reinterpret_cast<uint8_t*>(free_block) + sizeof(BlockHeader);

        update_memory_state();
        return user_data;
    }

    // Check remaining pool capacity before extending the allocated region.
    size_t remaining_space = POOL_SIZE - used;

    if (remaining_space < block_size) {
        return nullptr;
    }

    // Initialize the new block header directly inside the memory pool.
    new (&memory_pool[used]) BlockHeader{block_size, false};

    void* user_data = &memory_pool[used + sizeof(BlockHeader)];

    used += block_size;

    update_memory_state();
    return user_data;
}


void my_free(void* user_ptr) {

    // check
    if (user_ptr == nullptr) {
        return;
    }

    // Locate the block header before the user data.
    BlockHeader* header = reinterpret_cast<BlockHeader*>(static_cast<uint8_t*>(user_ptr) - sizeof(BlockHeader));

    // Mark the block reusable.
    header->is_free = true;

    // Coalesce blocks to reduce memory fragmentation.
    coalesce();
}



int main(void) {

    //Test alignment.
    uint8_t* ptr1 = static_cast<uint8_t*>(my_malloc(100));
    uint8_t* ptr2 = static_cast<uint8_t*>(my_malloc(25));
    uint8_t* ptr3 = static_cast<uint8_t*>(my_malloc(25));

    //Test first-fit allocation by reusing a freed block.
    my_free(ptr1);
    uint8_t* ptr4 = static_cast<uint8_t*>(my_malloc(8));
    uint8_t* ptr5 = static_cast<uint8_t*>(my_malloc(25));

    //Test allocation after freeing another block.
    my_free(ptr5);
    uint8_t* ptr6 = static_cast<uint8_t*>(my_malloc(35));

    // Keeping the MCU running for debugger inspection.
    while (true) {
    }
}

