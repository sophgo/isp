#include "clog_ringbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/*******************************************************************************/
/* Atomic operations using GCC __atomic built-ins (libatomic) */
/*******************************************************************************/

static inline uint32_t atomic_load_acquire_u32(uint32_t *ptr)
{
	return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
}

static inline void atomic_store_release_u32(uint32_t *ptr, uint32_t val)
{
	__atomic_store_n(ptr, val, __ATOMIC_RELEASE);
}

static inline int atomic_cas_acq_rel_u32(uint32_t *ptr, uint32_t *expected, uint32_t desired)
{
	return __atomic_compare_exchange_n(ptr, expected, desired, 0,
		__ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}

static inline void atomic_thread_fence_release(void)
{
	__atomic_thread_fence(__ATOMIC_RELEASE);
}

/*******************************************************************************/
/* Internal helpers */
/*******************************************************************************/

/*
 * Calculate available space for writing (considering wrap-around)
 * Always keep at least 1 byte gap to distinguish full from empty
 */
static inline uint32_t clog_ringbuf_available_space(clog_ringbuf_t *rb)
{
	uint32_t write_idx = atomic_load_acquire_u32(&rb->write_idx);
	uint32_t read_idx = atomic_load_acquire_u32(&rb->read_idx);

	if (write_idx >= read_idx) {
		/* Write is ahead: available = total - used - 1 */
		return rb->size - (write_idx - read_idx) - 1;
	} else {
		/* Write wrapped around: available = gap - 1 */
		return read_idx - write_idx - 1;
	}
}

/*******************************************************************************/
/* Ring buffer initialization and cleanup */
/*******************************************************************************/

int clog_ringbuf_init(clog_ringbuf_t *rb, uint32_t size)
{
	if (!rb || size == 0) {
		return -1;
	}

	/* Allocate buffer memory */
	rb->buffer = (uint8_t *)malloc(size);
	if (!rb->buffer) {
		return -1;
	}

	/* Initialize fields */
	rb->size = size;
	rb->write_idx = 0;
	rb->read_idx = 0;

	return 0;
}

void clog_ringbuf_deinit(clog_ringbuf_t *rb)
{
	if (!rb) {
		return;
	}

	if (rb->buffer) {
		free(rb->buffer);
		rb->buffer = NULL;
	}
}

/*******************************************************************************/
/* Producer APIs (thread-safe for multiple producers) */
/*******************************************************************************/

int clog_ringbuf_write(clog_ringbuf_t *rb, const char *data, uint32_t len)
{
	uint32_t write_idx, read_idx;
	uint32_t new_write_idx;
	uint32_t available;

	if (!rb || !data || len == 0) {
		return -1;
	}

	/* Try to reserve space atomically using compare-and-swap loop */
	while (1) {
		write_idx = atomic_load_acquire_u32(&rb->write_idx);
		read_idx = atomic_load_acquire_u32(&rb->read_idx);

		/* Check available space */
		if (write_idx >= read_idx) {
			available = rb->size - (write_idx - read_idx) - 1;
		} else {
			available = read_idx - write_idx - 1;
		}

		if (available < len) {
			return -1;
		}

		/* Calculate new write position with wrap-around */
		new_write_idx = write_idx + len;
		if (new_write_idx >= rb->size) {
			new_write_idx -= rb->size;
		}

		/* Try to atomically update write index */
		if (atomic_cas_acq_rel_u32(&rb->write_idx, &write_idx, new_write_idx)) {
			break; /* Successfully reserved space */
		}
		/* Another thread modified write_idx, retry */
	}

	/* Successfully reserved space [write_idx, new_write_idx) */
	/* Now write data, handling wrap-around if needed */

	if (write_idx + len <= rb->size) {
		/* No wrap needed - single contiguous write */
		memcpy(rb->buffer + write_idx, data, len);
	} else {
		/* Wrap needed - split into two writes */
		uint32_t first_part = rb->size - write_idx;
		uint32_t second_part = len - first_part;

		/* Write first part to end of buffer */
		memcpy(rb->buffer + write_idx, data, first_part);

		/* Write second part to beginning of buffer */
		memcpy(rb->buffer, data + first_part, second_part);
	}

	/* Memory barrier to ensure all writes complete */
	atomic_thread_fence_release();

	return 0;
}

/*******************************************************************************/
/* Consumer APIs (single-thread only) */
/*******************************************************************************/

int clog_ringbuf_read_zerocopy(clog_ringbuf_t *rb, const char **data, uint32_t *len)
{
	uint32_t read_idx, write_idx;
	uint32_t available_len;

	if (!rb || !data || !len) {
		return -1;
	}

	read_idx = atomic_load_acquire_u32(&rb->read_idx);
	write_idx = atomic_load_acquire_u32(&rb->write_idx);

	/* Empty buffer check: no data available */
	if (read_idx == write_idx) {
		return -1;
	}

	/* Calculate available data length */
	if (write_idx > read_idx) {
		/* Normal case: write is ahead of read */
		/* Return all data from read_idx to write_idx */
		available_len = write_idx - read_idx;
	} else {
		/* Wrapped case: write wrapped around */
		/* Return data from read_idx to end of buffer */
		/* Next call will return data from beginning to write_idx */
		available_len = rb->size - read_idx;
	}

	/* Return direct pointer to data (zero-copy) */
	*data = (const char *)(rb->buffer + read_idx);
	*len = available_len;

	return 0;
}

void clog_ringbuf_read_commit(clog_ringbuf_t *rb, uint32_t len)
{
	uint32_t read_idx;
	uint32_t new_read_idx;

	if (!rb) {
		return;
	}

	read_idx = atomic_load_acquire_u32(&rb->read_idx);

	/* Advance read index by the consumed length */
	new_read_idx = read_idx + len;

	/* Handle wrap-around */
	if (new_read_idx >= rb->size) {
		new_read_idx -= rb->size;
	}

	atomic_store_release_u32(&rb->read_idx, new_read_idx);
}
