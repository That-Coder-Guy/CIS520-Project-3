#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "bitmap.h"
#include "block_store.h"

// include more if you need


// You might find this handy. I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)

// This declares the block store structure with an integrated free block map
// \attribute: free_block_map - A bitmap representing all curently free storage blocks
// \attribute: blocks - An array of memory blocks to store data in
struct block_store
{
	bitmap_t* free_block_map;
	uint8_t blocks[BLOCK_STORE_NUM_BLOCKS][BLOCK_SIZE_BYTES];
};

/// This creates a new block store device, ready to go
/// \return Pointer to a new block store device, NULL on error
block_store_t *block_store_create()
{
	// Allocate memory for a block store structure
	block_store_t* store = malloc(sizeof(block_store_t));
	if (store == NULL) { return NULL; }

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

/// Destroys the provided block store device
/// This is an idempotent operation, so there is no return value
/// \param: bs - A block storage device
void block_store_destroy(block_store_t* const bs)
{
	// Validate input value
	if (bs == NULL) { return; }

	// Free the block store device
	if (bs->free_block_map != NULL) { bitmap_destroy(bs->free_block_map); }
	free(bs);
}

/// Searches for a free block, marks it as in use, and returns the block's id
/// \param: bs - A block store device
/// \return: Allocated block's id, SIZE_MAX on error
size_t block_store_allocate(block_store_t *const bs)
{
	// Validate input value
	if (bs == NULL || bs->free_block_map == NULL) { return SIZE_MAX; }

	// Identify the first 0 bit in the bit map and use its index as a block ID
	size_t block_id = bitmap_ffz(bs->free_block_map);
	if (block_id == SIZE_MAX) { return SIZE_MAX; }

	// Mark the block with the acquired ID as in use
	bitmap_set(bs->free_block_map, block_id);

	// Return the acquired block ID
	return block_id;
}

/// Attempts to allocate the requested block id
/// \param: bs - A block store device
/// \param: block_id - The requested block identifier
/// \return: Boolean indicating success of operation
bool block_store_request(block_store_t *const bs, const size_t block_id)
{
	// Validate input values
	if (bs == NULL || bs->free_block_map == NULL) { return false; }
	if (block_id >= BLOCK_STORE_NUM_BLOCKS) { return false; }

	// Return failure if the block is marked as in use
	if (bitmap_test(bs->free_block_map, block_id)) { return false; }
	
	// Mark the block with the acquired ID as in use
	bitmap_set(bs->free_block_map, block_id);

	return true;
}

/// Frees the specified block
/// \param: bs - A block store device
/// \param: block_id - The block to free
void block_store_release(block_store_t *const bs, const size_t block_id)
{
	// Validate input values
	if (bs == NULL || bs->free_block_map == NULL) { return; }
	if (block_id >= BLOCK_STORE_NUM_BLOCKS) { return; }
	
	// Mark the block with the prodived ID as unused
	bitmap_reset(bs->free_block_map, block_id);
}

size_t block_store_get_used_blocks(const block_store_t *const bs)
{
	if(bs == NULL || bs->free_block_map==NULL) return SIZE_MAX;

	size_t count = 0;
	
	for (size_t i = 0; i< BLOCK_STORE_NUM_BLOCKS; i++)
	{

		if(bitmap_test(bs->free_block_map,i)) 
		{
			count++;
		}
	}
	
	return count;
}

size_t block_store_get_free_blocks(const block_store_t *const bs)
{
	
	if(bs ==NULL || bs->free_block_map == NULL)
	{
		return SIZE_MAX;
	}
	
	size_t used = block_store_get_used_blocks(bs);
	if(used == SIZE_MAX) return SIZE_MAX;

	return BLOCK_STORE_NUM_BLOCKS - used;
}

size_t block_store_get_total_blocks()
{
	return BLOCK_STORE_NUM_BLOCKS;
}

size_t block_store_read(const block_store_t *const bs, const size_t block_id, void *buffer)
{
	if(bs == NULL || bs->free_block_map == NULL || buffer == NULL) 
	{
		return 0; 
	}
	
	if(block_id >= BLOCK_STORE_NUM_BLOCKS)
	{
		return 0;
	}

	if(!bitmap_test(bs->free_block_map,block_id))
	{
		return 0;
	}

	memcpy(buffer, bs->blocks[block_id], BLOCK_SIZE_BYTES);
	return BLOCK_SIZE_BYTES;
}

/// Reads data from the specified buffer and writes it to the designated block
/// \param bs BS device
/// \param block_id Destination block id
/// \param buffer Data buffer to read from
/// \return Number of bytes written, 0 on error
size_t block_store_write(block_store_t *const bs, const size_t block_id, const void *buffer)
{
	
	if(bs == NULL || bs->free_block_map == NULL || buffer == NULL) 
	{
		return 0; 
	}
	
	if(block_id >= BLOCK_STORE_NUM_BLOCKS)
	{
		return 0;
	}

	if(!bitmap_test(bs->free_block_map,block_id))
	{
		return 0;
	}

	memcpy(bs->blocks[block_id], buffer, BLOCK_SIZE_BYTES);
	return BLOCK_SIZE_BYTES;
}

/// Imports BS device from the given file - for grads/bonus
/// \param filename The file to load
/// \return Pointer to new BS device, NULL on error
block_store_t *block_store_deserialize(const char *const filename)
{
	if (filename == NULL || *filename == '\n' || *filename == '\0') return NULL;

	int fd = open(filename, O_RDONLY);
	if (fd < 0) return NULL;

	block_store_t *store = malloc(sizeof(block_store_t));
	if (store == NULL)
	{
		if (close(fd) < 0) {
    		perror("close failed");
		}
		return NULL;
	}

	store->free_block_map = bitmap_overlay(BITMAP_SIZE_BITS, store->blocks[BITMAP_START_BLOCK]);
	if (store->free_block_map == NULL)
	{
		free(store);
		if (close(fd) < 0) {
    		perror("close failed");
		}
		return NULL;
	}

	ssize_t total_read = 0;
	while (total_read < BLOCK_STORE_NUM_BYTES)
	{
		ssize_t bytes_read = read(fd, ((uint8_t *)store->blocks) + total_read, BLOCK_STORE_NUM_BYTES - total_read);

		if (bytes_read <= 0)
		{
			bitmap_destroy(store->free_block_map);
			free(store);
			if (close(fd) < 0) {
    			perror("close failed");
			}
			return NULL;
		}

		total_read += bytes_read;
	}

	if (close(fd) < 0) {
		perror("close failed");
	}
	return store;
}

/// Writes the entirety of the BS device to file, overwriting it if it exists - for grads/bonus
/// \param bs BS device
/// \param filename The file to write to
/// \return Number of bytes written, 0 on error
size_t block_store_serialize(const block_store_t *const bs, const char *const filename)
{
	if (bs == NULL || bs->free_block_map == NULL || filename == NULL || *filename == '\n' || *filename == '\0') return 0;

	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
	if (fd < 0) return 0;

	ssize_t total_written = 0;
	while (total_written < BLOCK_STORE_NUM_BYTES)
	{
		ssize_t bytes_written = write(fd, ((const uint8_t *)bs->blocks) + total_written, BLOCK_STORE_NUM_BYTES - total_written);

		if (bytes_written <= 0)
		{
			if (close(fd) < 0) {
    			perror("close failed");
			}
			return 0;
		}

		total_written += bytes_written;
	}

	if (close(fd) < 0) {
		perror("close failed");
	}
	return total_written;
}
