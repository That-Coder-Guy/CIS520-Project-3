#include <stdio.h>
#include <stdint.h>

#include "bitmap.h"
#include "block_store.h"
// include more if you need


// You might find this handy. I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

struct block_store
{
	bitmap_t* free_block_map;
	size_t block_size;
	size_t block_count;
	uint8_t blocks[BLOCK_STORE_NUM_BLOCKS][BLOCK_SIZE_BYTES];
};

block_store_t *block_store_create()
{
	// Allocate memory for a block store structure
	block_store_t* store = malloc(sizeof(block_store_t));
	if (store == NULL) { return NULL; }
	
	// Initialize size attributes
	store->block_size = BLOCK_SIZE_BYTES;
	store->block_count = BLOCK_STORE_NUM_BLOCKS;

	// Store the free block map in the middle blocks of the block store
	store->free_block_map = bitmap_overlay(BITMAP_SIZE_BITS, store->blocks[BITMAP_START_BLOCK]);
	if (store->free_block_map == NULL)
	{
		free(store);
		return NULL;
	}
	bitmap_format(store->free_block_map, 0x00);

	// Mark the blocks used to represent the free block map as taken to prevent overwriting
	for (size_t i = 0; i < BITMAP_NUM_BLOCKS; i++) { bitmap_set(store->free_block_map, BITMAP_START_BLOCK + i); }

	// Return the constructed block store
	return store;
}

void block_store_destroy(block_store_t* const bs)
{
	free(bs);
}

size_t block_store_allocate(block_store_t *const bs)
{
	UNUSED(bs);
	return 0;
}

bool block_store_request(block_store_t *const bs, const size_t block_id)
{
	UNUSED(bs);
	UNUSED(block_id);
	return false;
}

void block_store_release(block_store_t *const bs, const size_t block_id)
{
	UNUSED(bs);
	UNUSED(block_id);
}

size_t block_store_get_used_blocks(const block_store_t *const bs)
{
	UNUSED(bs);
	return 0;
}

size_t block_store_get_free_blocks(const block_store_t *const bs)
{
	UNUSED(bs);
	return 0;
}

size_t block_store_get_total_blocks()
{
	return 0;
}

size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
	UNUSED(bs);
	UNUSED(block_id);
	UNUSED(buffer);
	return 0;
}

size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{
	UNUSED(bs);
	UNUSED(block_id);
	UNUSED(buffer);
	return 0;
}

block_store_t *block_store_deserialize(const char *const filename)
{
	UNUSED(filename);
	return NULL;
}

size_t block_store_serialize(const block_store_t *const bs, const char *const filename)
{
	UNUSED(bs);
	UNUSED(filename);
	return 0;
}
