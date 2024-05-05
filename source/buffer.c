#include <bufferlib/buffer.h>				//This must be included at the top for preprocessing reasons


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef log_fetal_error
#	undef log_fetal_error
#endif
#define log_fetal_error printf

#ifdef CALLTRACE_BEGIN
#	undef CALLTRACE_BEGIN
#endif
#define CALLTRACE_BEGIN() /* nothing */
#ifdef CALLTRACE_END
#	undef CALLTRACE_END
#endif
#define CALLTRACE_END() /* nothing */
#ifdef CALLTRACE_RETURN
#	undef CALLTRACE_RETURN
#endif
#define CALLTRACE_RETURN(x) return x


#ifdef BUF_DEBUG
#	define GOOD_ASSERT(bool_value, string) do { if(!(bool_value)) {  log_fetal_err("Assertion Failed: %s\n", string); } } while(false)
	static void check_pre_condition(BUFFER* buffer);
#else
#	define GOOD_ASSERT(...)
#	define check_pre_condition(buffer)
#endif

#define STACK_ALLOCATED_OBJECT  0x1
#define HEAP_ALLOCATED_OBJECT 	 0x2

static BUFFER* binded_buffer;

function_signature(static bool, buf_is_stack_allocated, BUFFER* buffer);
function_signature(static bool, buf_is_heap_allocated, BUFFER* buffer);
function_signature_void(static bool, BUFis_stack_allocated);
function_signature_void(static bool, BUFis_heap_allocated);
#define buf_is_stack_allocated(...) define_alias_function_macro(buf_is_stack_allocated, __VA_ARGS__)
#define buf_is_heap_allocated(...) define_alias_function_macro(buf_is_heap_allocated, __VA_ARGS__)
#define BUFis_stack_allocated() define_alias_function_void_macro(BUFis_stack_allocated)
#define BUFis_heap_allocated() define_alias_function_void_macro(BUFis_heap_allocated)

function_signature_void(uint64_t, BUFget_buffer_object_size) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(sizeof(BUFFER)); }

function_signature(void, BUFset_on_post_resize, void (*on_post_resize)(void)) { CALLTRACE_BEGIN(); buf_set_on_post_resize(binded_buffer, on_post_resize); CALLTRACE_END(); }
function_signature(void, buf_set_on_post_resize, BUFFER* buffer, void (*on_post_resize)(void))
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buffer->on_post_resize = on_post_resize;
	CALLTRACE_END();
}

function_signature(void, BUFset_on_pre_resize, void (*on_pre_resize)(void)) { CALLTRACE_BEGIN(); buf_set_on_pre_resize(binded_buffer, on_pre_resize); CALLTRACE_END(); }
function_signature(void, buf_set_on_pre_resize, BUFFER* buffer, void (*on_pre_resize)(void))
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buffer->on_pre_resize = on_pre_resize;
	CALLTRACE_END();
}

static inline void* call_malloc(buffer_t* buffer, buf_ucount_t size)
{
	if(buffer->mem_malloc != NULL)
		return buffer->mem_malloc(size, buffer->user_data);
	return malloc(size);
}

static inline void call_free(buffer_t* buffer, void* ptr)
{
	if(buffer->mem_free != NULL)
	{
		buffer->mem_free(ptr, buffer->user_data);
		return;
	}
	free(ptr);
}

static inline void* call_realloc(buffer_t* buffer, void* old_ptr, buf_ucount_t size)
{
	if(buffer->mem_realloc != NULL)
		return buffer->mem_realloc(old_ptr, size, buffer->user_data);
	return realloc(old_ptr, size);
}

function_signature(void, BUFpush_pseudo, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_push_pseudo(binded_buffer, count); CALLTRACE_END(); }
function_signature(void, buf_push_pseudo, BUFFER* buffer, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);

	if(count == 0)
		CALLTRACE_RETURN();

	buf_ucount_t previous_element_count = buffer->element_count;
	buffer->element_count += count;
	if(buffer->capacity <= 0)
	{
		buffer->capacity = 1;
		buffer->bytes = call_malloc(buffer, buffer->capacity * buffer->element_size);
		GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
	}
	buf_ucount_t previous_capacity = buffer->capacity;

	while(buffer->capacity < buffer->element_count)
		buffer->capacity *= 2;
	if(buffer->capacity > previous_capacity)
	{
		if(buffer->on_pre_resize != NULL)
		{
			buf_ucount_t temp = previous_capacity;
			previous_capacity = buffer->capacity;
			buffer->capacity = temp;
		 	(buffer->on_pre_resize)();
		 	temp = previous_capacity;
		 	previous_capacity = buffer->capacity;
			buffer->capacity = temp;
		}
		buffer->bytes = call_realloc(buffer, buffer->bytes , buffer->capacity * buffer->element_size);
		GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
		if(buffer->on_post_resize != NULL) (buffer->on_post_resize)();
	}
	memset(buffer->bytes + previous_element_count * buffer->element_size , 0 , buffer->element_size * count);
	CALLTRACE_END();
}

function_signature(void*, BUFpush_pseudo_get, buf_ucount_t count) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_push_pseudo_get(binded_buffer, count)); }
function_signature(void*, buf_push_pseudo_get, BUFFER* buffer, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);

	if(count == 0)
		CALLTRACE_RETURN(NULL);

	buf_ucount_t offset = buf_get_element_count(buffer);
	buf_push_pseudo(buffer, count);

	CALLTRACE_RETURN(buf_get_ptr_at(buffer, offset));
}

function_signature(void, BUFpop_pseudo, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_pop_pseudo(binded_buffer, count); CALLTRACE_END(); }
function_signature(void, buf_pop_pseudo, BUFFER* buffer, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(count <= buffer->element_count, "Buffer Underflow Exception");
	buffer->element_count -= count;
	CALLTRACE_END();
}

function_signature(void, BUFinsert_pseudo, buf_ucount_t index, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_insert_pseudo(binded_buffer, index, count); CALLTRACE_END(); }
function_signature(void, buf_insert_pseudo, BUFFER* buffer, buf_ucount_t index, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Binded Buffer Is NULL Exception");
	GOOD_ASSERT(index <= buffer->element_count,"Buffer Overflow Exception");

	// if space is insufficient then re-allocate
	buf_ucount_t previous_capacity = buffer->capacity;
	buffer->capacity = (buffer->capacity == 0) ? 1 : buffer->capacity;
	while(buffer->capacity < (buffer->element_count + count))
		buffer->capacity <<= 1;
	if(buffer->capacity > previous_capacity)
	{
		buffer->bytes = call_realloc(buffer, buffer->bytes, (buffer->element_size + count) * buffer->capacity);
		GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
	}

	/*
		There are number of elements which are prone to be overwritten if we just directly copy from src to dst, as
		both would overlap. We need to somehow avoid overwriting the those values (staring from (insert index + insert count)).

		There are two methods of doing this:

		First, we can shift the elements by one stride (element_size) at a time for the 'number of elements to be shifted' times, to the right
		But that would result in O(num_shift_elements * insert_count) time complexity -- which is not much efficient.

		Second, we can allocate a temporary buffer which would preserve the prone to overwrite data (starting from (inset index + insert count)
		up to 'element_count'). And then we can directly copy the elements staring from 'insert index' (upto 'insert count')
		to 'insert index + insert count'. After that, copy the data from the temporary buffer to the memory starting at 'insert index + 2 * insert count'
		upto 'element_count'.

		In the second, if the size requirements for the temporary buffer is very large, greater than the permitted program stack size,
		then we would have to allocate the memory in the heap. A warning should be issued as the user might be expecting no heap allocation.
	*/

	void* src = buffer->bytes + index * buffer->element_size;

	if(index < buffer->element_count)
	{
			/* allocate temporary buffer to store the overwrite prone data */
			buf_ucount_t overwrite_prone_size = ((index + count) > buffer->element_count) ? 0 : (buffer->element_count - (index + count)) * buffer->element_size;
			void* overwrite_prone_data = NULL;
			if(overwrite_prone_size > 2048)
			{
				log_wrn("Overwrite prone data is greater than 2 KB, allocating the memory in the heap");
				overwrite_prone_data = call_malloc(buffer, overwrite_prone_size);
			}
			else if(overwrite_prone_size > 0)
				overwrite_prone_data = alloca(overwrite_prone_size);

			/* copy the overwrite prone data to the temporary buffer before initiating any copy operation to it. */
			if(overwrite_prone_size > 0)
				memcpy(overwrite_prone_data, buffer->bytes + (index + count) * buffer->element_size, overwrite_prone_size);

			/* shift the elements to the right via copy */
			void* dst = buffer->bytes + (index + count) * buffer->element_size;
			memcpy(dst, src, count * buffer->element_size);
			if(overwrite_prone_size > 0)
				memcpy(dst + count * buffer->element_size, overwrite_prone_data, overwrite_prone_size);

			if(overwrite_prone_size > 2048)
				call_free(buffer, overwrite_prone_data);
			/* else any memory allocated on the stack automatically freed after the function returns */
	}

	/* set the inserted elements to zero */
	memset(src, 0, count * buffer->element_size);

	buffer->element_count += count;

	CALLTRACE_END();
}

function_signature(void, BUFremove_pseudo, buf_ucount_t index, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_remove_pseudo(binded_buffer, index, count);  CALLTRACE_END(); }
function_signature(void, buf_remove_pseudo, BUFFER* buffer, buf_ucount_t index, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT((index + count) <= buffer->element_count, "You're trying to remove elements out of the bounds of the buffer");

	/* if there is any element left on the right most side */
	if((index + count) < buffer->element_count)
	{
		/* shift the elements to the left */
		void* dst_ptr = buffer->bytes + index * buffer->element_size;
		void* src_ptr = dst_ptr + count * buffer->element_size;
		memmove(dst_ptr, src_ptr, (buffer->element_count - index - count) * buffer->element_size);
	}

	/* zero out the trailing elements (on the left) */
	memset(buffer->bytes + (buffer->element_count - count) * buffer->element_size, 0, count * buffer->element_size);

	buffer->element_count -= count;

	CALLTRACE_END();
}

function_signature(void, BUFset_auto_managed, bool value) { CALLTRACE_BEGIN(); buf_set_auto_managed(binded_buffer, value); CALLTRACE_END(); }
function_signature(void, buf_set_auto_managed, BUFFER* buffer, bool value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	if(value && (buffer->auto_managed_empty_blocks == BUF_INVALID))
			buffer->auto_managed_empty_blocks = BUFcreate(BUF_INVALID, sizeof(void*), 0, 0);
	else if(!value && (buffer->auto_managed_empty_blocks != BUF_INVALID))
				buf_free(buffer->auto_managed_empty_blocks);
 	buffer->is_auto_managed = value;
 	CALLTRACE_END();
}

function_signature_void(buf_ucount_t, BUFget_offset) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_offset(binded_buffer)); }
function_signature(buf_ucount_t, buf_get_offset, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN(buffer->offset);
}

function_signature_void(buf_ucount_t, BUFget_capacity) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_capacity(binded_buffer)); }
function_signature(buf_ucount_t, buf_get_capacity, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN(buffer->capacity);
}

function_signature_void(buf_ucount_t, BUFget_element_count) { CALLTRACE_BEGIN(); CALLTRACE_RETURN( buf_get_element_count(binded_buffer)); }
function_signature(buf_ucount_t, buf_get_element_count, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN(buffer->element_count);
}

function_signature_void(buf_ucount_t, BUFget_element_size) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_element_size(binded_buffer)); }
function_signature(buf_ucount_t, buf_get_element_size, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN(buffer->element_size);
}

function_signature_void(void*, BUFget_ptr) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_ptr(binded_buffer)); }
function_signature(void*, buf_get_ptr, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN(buffer->bytes);
}

function_signature(void, BUFset_offset, buf_ucount_t offset) { CALLTRACE_BEGIN(); buf_set_offset(binded_buffer, offset); CALLTRACE_END(); }
function_signature(void, buf_set_offset, BUFFER* buffer, buf_ucount_t offset)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buffer->offset = offset;
	CALLTRACE_END();
}

function_signature(void, BUFset_capacity, buf_ucount_t capacity) { CALLTRACE_BEGIN(); buf_set_capacity(binded_buffer, capacity); CALLTRACE_END(); }
function_signature(void, buf_set_capacity, BUFFER* buffer, buf_ucount_t capacity)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
 	buffer->capacity = capacity;
 	CALLTRACE_END();
}

function_signature(void, BUFset_element_count, buf_ucount_t element_count) { CALLTRACE_BEGIN(); buf_set_element_count(binded_buffer, element_count); CALLTRACE_END(); }
function_signature(void, buf_set_element_count, BUFFER* buffer, buf_ucount_t element_count)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buffer->element_count = element_count;
	CALLTRACE_END();
}

function_signature(void, BUFset_element_size, buf_ucount_t element_size) { CALLTRACE_BEGIN(); buf_set_element_size(binded_buffer, element_size); CALLTRACE_END(); }
function_signature(void, buf_set_element_size, BUFFER* buffer, buf_ucount_t element_size)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buffer->element_size = element_size;
	CALLTRACE_END();
}

function_signature(void, BUFset_ptr, void* ptr) { CALLTRACE_BEGIN(); buf_set_ptr(binded_buffer, ptr); CALLTRACE_END(); }
function_signature(void, buf_set_ptr, BUFFER* buffer, void* ptr)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buffer->bytes = ptr;
	CALLTRACE_END();
}

function_signature_void(BUFFER*, BUFget_binded_buffer)
{
	CALLTRACE_BEGIN();
	CALLTRACE_RETURN(binded_buffer);
}

function_signature_void(bool, BUFis_auto_managed) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_is_auto_managed(binded_buffer)); }
function_signature(bool, buf_is_auto_managed, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN(buffer->is_auto_managed);
}

function_signature(void, BUFbind, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
  binded_buffer = buffer;
  CALLTRACE_END();
}

function_signature_void(void, BUFunbind)
{
	CALLTRACE_BEGIN();
	binded_buffer = NULL;
	CALLTRACE_END();
}

function_signature_void(void, BUFlog) { CALLTRACE_BEGIN(); buf_log(binded_buffer); CALLTRACE_END(); }
function_signature(void, buf_log, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	log_msg(
			 "----------------------\n"
			 "ElementCount :\t%u\n"
		   "Capacity :\t%u\n"
		   "ElementSize :\t%u\n"
		   "Offset: \t%u\n"
		   "----------------------\n",
		    buffer->element_count,
		    buffer->capacity,
		    buffer->element_size,
		    buffer->offset
			);
	CALLTRACE_END();
}

function_signature(void, BUFtraverse_elements, buf_ucount_t start, buf_ucount_t end, void (*func)(void* /*element ptr*/, void* /*args ptr*/), void* args) { CALLTRACE_BEGIN(); buf_traverse_elements(binded_buffer, start, end, func, args); CALLTRACE_END(); }
function_signature(void, buf_traverse_elements, BUFFER* buffer, buf_ucount_t start, buf_ucount_t end, void (*func)(void* /*element ptr*/, void* /*args ptr*/), void* args)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT((start <= end) && (start < buffer->element_count) && (end < buffer->element_count), "(start <= end) && (start < buffer->element_count) && (end < buffer->element_count) evaulates to false");
	for(buf_ucount_t i = start; i <= end; i++)
	 		func(buf_getptr_at(buffer, i), args);
	CALLTRACE_END();
}

function_signature_void(static bool, BUFis_stack_allocated) { CALLTRACE_BEGIN();  CALLTRACE_RETURN(buf_is_stack_allocated(binded_buffer)); }
function_signature(static bool, buf_is_stack_allocated, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN((buffer->info & STACK_ALLOCATED_OBJECT) == STACK_ALLOCATED_OBJECT);
}

function_signature_void(static bool, BUFis_heap_allocated) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_is_heap_allocated(binded_buffer)); }
function_signature(static bool, buf_is_heap_allocated, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN((buffer->info & HEAP_ALLOCATED_OBJECT) == HEAP_ALLOCATED_OBJECT);
}

function_signature_void(void, BUFfree) { CALLTRACE_BEGIN(); buf_free(binded_buffer); CALLTRACE_END(); }
function_signature(void, buf_free, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	if((buffer->free != NULL) && (buffer->element_count > 0))
		buf_traverse_elements(buffer, 0, buf_get_element_count(buffer)- 1, (void (*)(void*, void*))(buffer->free), NULL);
	if(buffer->bytes != NULL)
	{
		call_free(buffer, buffer->bytes);
		buffer->bytes == NULL;
	}

  if(buffer->info & HEAP_ALLOCATED_OBJECT)
  { buffer->info = 0x00; call_free(buffer, buffer); }
	buffer = NULL;
	CALLTRACE_END();
}

function_signature_void(BUFFER*, BUFget_clone)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(binded_buffer != NULL, "binded buffer is NULL Exception");
	CALLTRACE_RETURN(BUFcopy_construct(binded_buffer));
}
function_signature(BUFFER, buf_get_clone, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN(buf_copy_construct(buffer));
}

function_signature(void, BUFmove_to, BUFFER* destination) { CALLTRACE_BEGIN(); buf_move_to(binded_buffer, destination); CALLTRACE_END(); }
function_signature(void, buf_move_to, BUFFER* buffer, BUFFER* destination)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(destination != NULL, "destination buffer is NULL Exception");
	buf_copy_to(buffer, destination);
	buf_free(buffer);
	CALLTRACE_END();
}

function_signature(void, BUFmove, BUFFER* destination) { CALLTRACE_BEGIN(); buf_move(binded_buffer, destination); CALLTRACE_END(); }
function_signature(void, buf_move, BUFFER* buffer, BUFFER* destination)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(destination != NULL, "destination buffer is NULL Exception");
	memcpy(destination, buffer, sizeof(BUFFER));
	buffer->bytes = NULL;
	buffer->element_size = 0;
	buffer->element_count = 0;
	buffer->capacity = 0;
	buffer->auto_managed_empty_blocks = NULL;
	buffer->offset = 0;
	CALLTRACE_END();
}

function_signature(void, BUFcopy_to, BUFFER* destination) { CALLTRACE_BEGIN(); buf_copy_to(binded_buffer, destination); CALLTRACE_END(); }
function_signature(void, buf_copy_to, BUFFER* buffer, BUFFER* destination)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(destination != NULL, "destination buffer is NULL Exception");
	GOOD_ASSERT(buffer != destination, "source and destination buffers are referencing to the same memory location");
	GOOD_ASSERT(destination->element_size == buffer->element_size, "element size of the source and destination buffers are not identical");
	GOOD_ASSERT(buf_get_buffer_size(buffer) != 0, "buffer size of the source buffer is zero");
	if(destination->capacity < buffer->element_count)
		buf_resize(destination, buffer->element_count);
	memcpy(destination->bytes, buffer->bytes, buf_get_buffer_size(buffer));
	buf_set_offset(destination, buffer->offset);
	buf_set_capacity(destination, buffer->capacity);
	buf_set_element_count(destination, buffer->element_count);
	buf_set_auto_managed(destination, buffer->is_auto_managed);
	buf_set_on_pre_resize(destination, buffer->on_pre_resize);
	buf_set_on_post_resize(destination, buffer->on_post_resize);
	buf_set_on_free(destination, buffer->free);
	CALLTRACE_END();
}

function_signature(BUFFER*, BUFcopy_construct, BUFFER* source)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(source != NULL, "source buffer Is NULL Exception");
	BUFFER* buffer = BUFcreate(NULL, source->element_size, source->capacity, source->offset);
	buf_copy_to(source, buffer);
	CALLTRACE_RETURN(buffer);
}

function_signature(BUFFER, buf_copy_construct, BUFFER* source)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(source != NULL, "source buffer is NULL Exception");
	BUFFER buffer = buf_create(source->element_size, source->capacity, source->offset);
	buf_copy_to(source, &buffer);
	CALLTRACE_RETURN(buffer);
}

function_signature(void, BUFset_on_free, void (*free)(void*)) { CALLTRACE_BEGIN(); buf_set_on_free(binded_buffer, free); CALLTRACE_END(); }
function_signature(void, buf_set_on_free, BUFFER* buffer, void (*free)(void*))
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buffer->free = free;
	CALLTRACE_END();
}

function_signature(BUFFER, buf_create_a, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	CALLTRACE_BEGIN();
	BUFFER buffer;
	BUFcreate_object_a(&buffer, _malloc, _free, _realloc, user_data);
	BUFcreate_a(&buffer, element_size, capacity, offset, _malloc, _free, _realloc, user_data);
	CALLTRACE_RETURN(buffer);
}

function_signature(BUFFER, buf_create, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset)
{
	CALLTRACE_BEGIN();
	BUFFER buffer;
	BUFcreate_object(&buffer);
	BUFcreate(&buffer, element_size, capacity, offset);
	CALLTRACE_RETURN(buffer);
}

function_signature(BUFFER*, BUFcreate_object_a, void* bytes, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	CALLTRACE_BEGIN();
	BUFcreate_object(bytes);
	BUFFER* buffer = bytes;
	buffer->mem_malloc = _malloc;
	buffer->mem_free = _free;
	buffer->mem_realloc = _realloc;
	buffer->user_data = user_data;
	CALLTRACE_RETURN(buffer);
}

//TODO: Replace the name with BUFcreate_object_from_bytes
function_signature(BUFFER*, BUFcreate_object, void* bytes)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(bytes != NULL, "bytes equals to NULL");
	BUFFER* buffer = bytes;
	buffer->bytes = NULL;
	buffer->info = 0x00;
	buffer->info |= STACK_ALLOCATED_OBJECT;
	buffer->element_size = 0;
	buffer->element_count = 0;
	buffer->capacity = 0;
	buffer->auto_managed_empty_blocks = NULL;
	buffer->is_auto_managed = false;
	buffer->on_pre_resize = NULL;
	buffer->on_post_resize = NULL;
	buffer->offset = 0;
	buffer->free = NULL;
	buffer->mem_malloc = NULL;
	buffer->mem_free = NULL;
	buffer->mem_realloc = NULL;
	buffer->user_data = NULL;
#ifdef BUF_DEBUG
	buffer->is_ptr_queried = false;
#endif /* BUF_DEBUG */
	CALLTRACE_RETURN(buffer);
}

function_signature(BUFFER*, BUFcreate_a, BUFFER* buffer, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset, buf_malloc_t _malloc, buf_free_t _free, buf_realloc_t _realloc, void* user_data)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(((int64_t)element_size) > 0, "element_size cannot be negative or zero");
	GOOD_ASSERT(((int64_t)capacity) >= 0, "capacity cannot be negative");
	GOOD_ASSERT(((int64_t)offset) >= 0, "offset cannot be negative");
	if(buffer == NULL)
	{
		buffer = (_malloc != NULL) ? _malloc(sizeof(BUFFER), user_data) : malloc(sizeof(BUFFER));
		GOOD_ASSERT(buffer != NULL, "Memory Allocation Failure Exception");
		buffer->info = 0x00;
		buffer->info |= HEAP_ALLOCATED_OBJECT;
		buffer->auto_managed_empty_blocks = NULL;
		buffer->is_auto_managed = false;
		buffer->on_pre_resize = NULL;
		buffer->on_post_resize = NULL;
		buffer->free = NULL;
		buffer->mem_malloc = _malloc;
		buffer->mem_free = _free;
		buffer->mem_realloc = _realloc;
		buffer->user_data = user_data;
	#ifdef BUF_DEBUG
		buffer->is_ptr_queried = false;
	#endif /* BUF_DEBUG */
	}
	if((capacity > 0) || (offset > 0))
	{
		buf_ucount_t alloc_size = element_size * capacity + offset;
		buffer->bytes = (void*)((_malloc != NULL) ? _malloc(alloc_size, user_data) : malloc(alloc_size));
		GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
	}
	else
		buffer->bytes = NULL;
	buffer->element_size = element_size;
	buffer->capacity = capacity;
	buffer->element_count = 0;
	buffer->offset = offset;
	CALLTRACE_RETURN(buffer);
}

function_signature(BUFFER*, BUFcreate, BUFFER* buffer, buf_ucount_t element_size, buf_ucount_t capacity, buf_ucount_t offset)
{
	CALLTRACE_BEGIN();
	CALLTRACE_RETURN(BUFcreate_a(buffer, element_size, capacity, offset, NULL, NULL, NULL, NULL));
}

function_signature(void, BUFget_at, buf_ucount_t index, void* out_value) { CALLTRACE_BEGIN(); buf_get_at(binded_buffer, index, out_value); CALLTRACE_END(); }
function_signature(void, buf_get_at, BUFFER* buffer, buf_ucount_t index, void* out_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(index < buffer->element_count,"index >= buffer->element_count, Index Out of Range Exception");
	memcpy(out_value , buffer->bytes + index * buffer->element_size, buffer->element_size);
	CALLTRACE_END();
}

#if defined(BUF_DEBUG) && defined(BUF_ENABLE_BUFFER_RESIZE_WARNING)
#	define PTR_QUERIED(buffer) (buffer)->is_ptr_queried = true
#	define WARN_IF_PTR_QUERIED(buffer) if((buffer)->is_ptr_queried) { (buffer)->is_ptr_queried = false; log_wrn("Bufferlib: buffer has been resized since a pointer into the buffer was queried!\n"); }
#else
#	define PTR_QUERIED(buffer)
#	define WARN_IF_PTR_QUERIED(buffer)
#endif

function_signature(void*, BUFgetptr_at, buf_ucount_t index) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_getptr_at(binded_buffer, index)); }
function_signature(void*, buf_getptr_at, BUFFER* buffer, buf_ucount_t index)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(index < buffer->element_count,"index >= buffer->element_count, Index Out of Range Exception");
	PTR_QUERIED(buffer);
	CALLTRACE_RETURN(buffer->bytes + index * buffer->element_size);
}

function_signature(void, BUFset_at, buf_ucount_t index , void* in_value) { CALLTRACE_BEGIN(); buf_set_at(binded_buffer, index, in_value); CALLTRACE_END(); }
function_signature(void, buf_set_at, BUFFER* buffer, buf_ucount_t index , void* in_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(index < buffer->element_count,"Index Out of Range Exception");
	memcpy(buffer->bytes + index * buffer->element_size, in_value , buffer->element_size);
	CALLTRACE_END();
}

function_signature_void(void*, BUFget_offset_bytes) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_offset_bytes(binded_buffer)); }
function_signature(void*, buf_get_offset_bytes, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->offset != 0, "buffer->offset equals to Zero!");
	PTR_QUERIED(buffer);
	CALLTRACE_RETURN(buffer->bytes + buffer->capacity * buffer->element_size);
}

function_signature(void, BUFset_offset_bytes, void* offset_bytes) { CALLTRACE_BEGIN(); buf_set_offset_bytes(binded_buffer, offset_bytes); CALLTRACE_END(); }
function_signature(void, buf_set_offset_bytes, BUFFER* buffer, void* offset_bytes)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(offset_bytes != NULL, "offset_bytes is NULL Exception");
	GOOD_ASSERT(buffer->offset != 0, "buffer->offset equals to Zero!");
	memcpy(buf_get_offset_bytes(buffer), offset_bytes, buffer->offset);
	CALLTRACE_END();
}

function_signature_void(buf_ucount_t, BUFget_buffer_size) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_get_buffer_size(binded_buffer)); }
function_signature(buf_ucount_t, buf_get_buffer_size, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	CALLTRACE_RETURN(buffer->capacity * buffer->element_size  + buffer->offset);
}

function_signature(void, BUFresize, buf_ucount_t new_capacity) { CALLTRACE_BEGIN(); buf_resize(binded_buffer, new_capacity); CALLTRACE_END(); }
function_signature(void, buf_resize, BUFFER* buffer, buf_ucount_t new_capacity)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(new_capacity != 0, "new_capacity == 0");
	if(new_capacity == buffer->capacity)
		CALLTRACE_RETURN();
	buf_ucount_t new_buffer_size = new_capacity * buffer->element_size + buffer->offset;
	buf_ucount_t buffer_size = buffer->capacity * buffer->element_size + buffer->offset;
	void* new_buffer = call_malloc(buffer, new_buffer_size);
	GOOD_ASSERT(new_buffer != NULL, "Failed to allocate memory");
	if(new_buffer_size > buffer_size)

	{
		if((buffer_size - buffer->offset) != 0)
		//copy only the elments, excluding the offset
		memcpy(new_buffer, buffer->bytes, buffer_size - buffer->offset);
		if(buffer->offset != 0)
		//copy the offset bytes at the end of the new buffer
		memcpy(new_buffer + new_buffer_size - buffer->offset, buffer->bytes + buffer_size - buffer->offset, buffer->offset);
		//set the intermediate bytes to zero
		memset(new_buffer + buffer_size - buffer->offset, 0, new_buffer_size - buffer_size);
	}
	else//if (new_buffer_size < buffer_size)
	{
		//copy only the elements, excluding the offset
		memcpy(new_buffer, buffer->bytes, new_buffer_size - buffer->offset);
		if(buffer->offset != 0)
		//copy the offset bytes at the end of the new buffer
		memcpy(new_buffer + new_buffer_size - buffer->offset, buffer->bytes + buffer_size - buffer->offset, buffer->offset);
	}
	if(buffer->bytes != NULL)
		call_free(buffer, buffer->bytes);
	buffer->bytes = new_buffer;
	if((new_buffer_size < buffer_size) && (buffer->element_count >= buffer->capacity))
		buffer->element_count = new_capacity;
	buffer->capacity = new_capacity;
	WARN_IF_PTR_QUERIED(buffer);
	CALLTRACE_END();
}

function_signature(void, BUFensure_capacity, buf_ucount_t min_capacity) { CALLTRACE_BEGIN(); buf_ensure_capacity(binded_buffer, min_capacity); CALLTRACE_END(); }
function_signature(void, buf_ensure_capacity, BUFFER* buffer, buf_ucount_t min_capacity)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	if(min_capacity <= buf_get_capacity(buffer))
		CALLTRACE_RETURN();
	buf_resize(buffer, min_capacity);
	CALLTRACE_END();
}

function_signature(void, BUFclear_buffer, void* clear_value) { CALLTRACE_BEGIN(); buf_clear_buffer(binded_buffer, clear_value); CALLTRACE_END(); }
function_signature(void, buf_clear_buffer, BUFFER* buffer, void* clear_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	if(clear_value == NULL)
		memset(buffer->bytes, 0, buffer->capacity * buffer->element_size + buffer->offset);
	else
	{
		buf_ucount_t capacity = buffer->capacity;
		while(capacity > 0)
		{
			--capacity;
			memcpy(buffer->bytes + capacity * buffer->element_size, clear_value, buffer->element_size);
		}
		memset(buffer->bytes + buffer->capacity * buffer->element_size, 0, buffer->offset);
	}
	buffer->element_count = 0;
	CALLTRACE_END();
}

function_signature(void, BUFclear, void* clear_value) { CALLTRACE_BEGIN(); buf_clear(binded_buffer, clear_value); CALLTRACE_END(); }
function_signature(void, buf_clear, BUFFER* buffer, void* clear_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);

	if((buffer->free != NULL) && (buffer->element_count > 0))
		buf_traverse_elements(buffer, 0, buf_get_element_count(buffer) - 1, (void (*)(void*, void*))(buffer->free), NULL);

	if(clear_value == NULL)
		memset(buffer->bytes , 0 , buffer->element_count * buffer->element_size) ;
	else
	{
		buf_ucount_t element_count = buffer->element_count;
		while(element_count > 0)
		{
			--element_count;
			memcpy(buffer->bytes + element_count * buffer->element_size, clear_value , buffer->element_size);
		}
	}
 	buffer->element_count = 0;
 	CALLTRACE_END();
}

function_signature(void, BUFinsert_at_noalloc, buf_ucount_t index , void* in_value , void* out_value) { CALLTRACE_BEGIN(); buf_insert_at_noalloc(binded_buffer, index, in_value, out_value); CALLTRACE_END(); }
function_signature(void, buf_insert_at_noalloc, BUFFER* buffer, buf_ucount_t index , void* in_value , void* out_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(index < buffer->capacity,"Buffer Overflow Exception");
	GOOD_ASSERT(buffer->element_count > index ,"Index should be less than buffer->element_count");
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + index * buffer->element_size , buffer->element_size) ;
	memcpy(buffer->bytes + index * buffer->element_size, in_value , buffer->element_size) ;
	CALLTRACE_END();
}

function_signature(void, BUFinsert_at, buf_ucount_t index , void* in_value) { CALLTRACE_BEGIN(); buf_insert_at(binded_buffer, index, in_value); CALLTRACE_END(); }
function_signature(void, buf_insert_at, BUFFER* buffer, buf_ucount_t index , void* in_value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buffer != NULL, "Binded Buffer Is NULL Exception");
	GOOD_ASSERT(index <= buffer->element_count,"Buffer Overflow Exception");

	// calculate the number of elements to shift towards right
	buf_ucount_t num_shift_elements = buffer->element_count - index;

	// increase the element_count by count
	buffer->element_count += 1;

	// if space is insufficient then re-allocate
	buf_ucount_t previous_capacity = buffer->capacity;
	buffer->capacity = (buffer->capacity == 0) ? 1 : buffer->capacity;
	while(buffer->capacity < buffer->element_count)
		buffer->capacity <<= 1;
	if(buffer->capacity > previous_capacity)
	{
		buffer->bytes = call_realloc(buffer, buffer->bytes, buffer->element_size * buffer->capacity);
		GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
	}

	// shift the elements to the right
	void* dst_ptr = buffer->bytes + (buffer->element_count - 1) * buffer->element_size;
	uint8_t offset = buffer->element_size * 1;
	while(num_shift_elements)
	{
		memcpy(dst_ptr , dst_ptr - offset, buffer->element_size);
		dst_ptr -= buffer->element_size;
		--num_shift_elements;
	}

	// copy in_value to the inserted block
	memcpy(buffer->bytes + index * buffer->element_size, in_value, buffer->element_size);

	WARN_IF_PTR_QUERIED(buffer);
	CALLTRACE_END();
}

static bool ptr_comparer(void* ptr1, void* ptr2) { return *((uint8_t*)ptr1) == *((uint8_t*)ptr2); }

function_signature(bool, BUFremove_at_noshift, buf_ucount_t index , void* out_value) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_remove_at_noshift(binded_buffer, index, out_value)); }
function_signature(bool, buf_remove_at_noshift, BUFFER* buffer, buf_ucount_t index , void* out_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	GOOD_ASSERT(index < buffer->element_count,"index >= buffer->element_count, Index Out of Range Exception");
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + index * buffer->element_size , buffer->element_size) ;
	if(buf_is_auto_managed(buffer))
	{
		BUFFER* previous_buffer = buffer;
		void* ptr = buffer->bytes + index * buffer->element_size;
		if(buf_find_index_of(buffer->auto_managed_empty_blocks, &ptr, ptr_comparer) == BUF_INVALID_INDEX) /*if ptr is not found in the auto_managed_empty_blocks BUFFER*/
			buf_push(buffer->auto_managed_empty_blocks, &ptr);
	}
	memset(buffer->bytes + index * buffer->element_size , 0 , buffer->element_size);
	CALLTRACE_RETURN(true);
}

function_signature(bool, BUFremove_at, buf_ucount_t index , void* out_value) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_remove_at(binded_buffer, index, out_value)); }
function_signature(bool, buf_remove_at, BUFFER* buffer, buf_ucount_t index , void* out_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	GOOD_ASSERT(index < buffer->element_count,"Index Out of Range Exception");
	--(buffer->element_count);
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + index * buffer->element_size , buffer->element_size);

	void* dst_ptr = buffer->bytes + index * buffer->element_size;
	if(index <= buffer->element_count)
	{
		buf_ucount_t num_shift_elements = (buffer->element_count - index) ;
		while(num_shift_elements > 0)
		{
			memcpy(dst_ptr , dst_ptr + buffer->element_size, buffer->element_size);
			dst_ptr += buffer->element_size;
			--num_shift_elements;
		}
	}
	memset(dst_ptr , 0 , buffer->element_size);
	CALLTRACE_RETURN(true);
}

function_signature(bool, BUFremove_noshift, void* object, bool (*comparer)(void*, void*)) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_remove_noshift(binded_buffer, object, comparer)); }
function_signature(bool, buf_remove_noshift, BUFFER* buffer, void* object, bool (*comparer)(void*, void*))
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	void* cursor = buffer->bytes;
	for(buf_ucount_t i = 0; i < buffer->element_count; i++, cursor += buffer->element_size)
	{
		if(comparer(object, cursor))
		{
			memset(cursor, 0, buffer->element_size);
			if(buf_is_auto_managed(buffer))
				if(buf_find_index_of(buffer->auto_managed_empty_blocks, &cursor, ptr_comparer) == BUF_INVALID_INDEX) /*if ptr is not found in the auto_managed_empty_blocks BUFFER*/
					buf_push(buffer->auto_managed_empty_blocks, &cursor);
			CALLTRACE_RETURN(true);
		}
	}
	CALLTRACE_RETURN(false);
}

function_signature(bool, BUFremove, void* object, bool (*comparer)(void*, void*)) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_remove(binded_buffer, object, comparer)); }
function_signature(bool, buf_remove, BUFFER* buffer, void* object, bool (*comparer)(void*, void*))
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	void* cursor = buffer->bytes;
	for(buf_ucount_t i = 0; i < buffer->element_count; i++, cursor += buffer->element_size)
	{
		if(comparer(object, cursor))
		{
			/* NOTICE: do not use memcpy here, as overlapping can't be handled by memcpy */
			memmove(cursor, cursor + buffer->element_size, (buffer->element_count - i - 1) * buffer->element_size);
			memset(buf_peek_ptr(buffer), 0, buffer->element_size);
			--(buffer->element_count);
			CALLTRACE_RETURN(true);
		}
	}
	CALLTRACE_RETURN(false);
}

function_signature_void(void, BUFfit) { CALLTRACE_BEGIN(); buf_fit(binded_buffer); CALLTRACE_END(); }
function_signature(void, buf_fit, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	//TODO: Replace this with BUFresize(binded_buffer->element_count)
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	buffer->bytes = call_realloc(buffer, buffer->bytes , buffer->element_count * buffer->element_size);
	GOOD_ASSERT(buffer->bytes != NULL, "Memory Allocation Failure Exception");
	buffer->capacity = buffer->element_count;
	WARN_IF_PTR_QUERIED(buffer);
	CALLTRACE_END();
}

function_signature_void(void*, BUFpeek_ptr) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_peek_ptr(binded_buffer)); }
function_signature(void*, buf_peek_ptr, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	PTR_QUERIED(buffer);
	CALLTRACE_RETURN(buffer->bytes + (buffer->element_count - 1) * buffer->element_size);
}

function_signature(void, BUFpeek, void* out_value) { CALLTRACE_BEGIN(); buf_peek(binded_buffer, out_value); CALLTRACE_END(); }
function_signature(void, buf_peek, BUFFER* buffer, void* out_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	memcpy(out_value, buffer->bytes + (buffer->element_count - 1) * buffer->element_size , buffer->element_size);
	CALLTRACE_END();
}

function_signature(void, BUFpop, void* out_value) { CALLTRACE_BEGIN(); buf_pop(binded_buffer, out_value); CALLTRACE_END(); }
function_signature(void, buf_pop, BUFFER* buffer, void* out_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	GOOD_ASSERT(buffer->element_count > 0, "Buffer is Empty!");
	--(buffer->element_count);
	if(out_value != NULL)
		memcpy(out_value , buffer->bytes + buffer->element_count * buffer->element_size , buffer->element_size);
	if(buffer->free != NULL)
		buffer->free(buffer->bytes + buffer->element_count * buffer->element_size);
	CALLTRACE_END();
}

function_signature_void(void*, BUFpop_get_ptr) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_pop_get_ptr(binded_buffer)); }
function_signature(void*, buf_pop_get_ptr, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	void* ptr = buf_peek_ptr(buffer);
	buf_pop_pseudo(buffer, 1);
	PTR_QUERIED(buffer);
	CALLTRACE_RETURN(ptr);
}


function_signature(buf_ucount_t, BUFfind_index_of, void* value, bool (*comparer)(void*, void*)) { CALLTRACE_BEGIN(); CALLTRACE_RETURN(buf_find_index_of(binded_buffer, value, comparer)); }
function_signature(buf_ucount_t, buf_find_index_of, BUFFER* buffer, void* value, bool (*comparer)(void*, void*))
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	void* cursor = buffer->bytes;
	for(buf_ucount_t i = 0; i < buffer->element_count; i++, cursor += buffer->element_size)
		if(comparer(value, cursor))
			CALLTRACE_RETURN(i);
	CALLTRACE_RETURN(BUF_INVALID_INDEX);
}

function_signature(void, BUFpush, void* in_value) { CALLTRACE_BEGIN(); buf_push(binded_buffer, in_value); CALLTRACE_END(); }
function_signature(void, buf_push, BUFFER* buffer, void* in_value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buf_ucount_t new_capacity = (buffer->capacity == 0) ? 1 : buffer->capacity;
	++(buffer->element_count);
	while(new_capacity < buffer->element_count)
		new_capacity *= 2;
	buf_resize(buffer, new_capacity);
	buf_set_at(buffer, buffer->element_count - 1, in_value);
	WARN_IF_PTR_QUERIED(buffer);
	CALLTRACE_END();
}

function_signature(void, BUFpushv, void* in_value, buf_ucount_t count) { CALLTRACE_BEGIN(); buf_pushv(binded_buffer, in_value, count); CALLTRACE_END(); }
function_signature(void, buf_pushv, BUFFER* buffer, void* in_value, buf_ucount_t count)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buf_ucount_t new_capacity = (buffer->capacity == 0) ? 1 : buffer->capacity;
	buffer->element_count += count;
	while(new_capacity < buffer->element_count)
		new_capacity *= 2;
	buf_resize(buffer, new_capacity);
	for(buf_ucount_t i = 0; i < count; i++, in_value += buffer->element_size)
		buf_set_at(buffer, buffer->element_count - count + i, in_value);
	WARN_IF_PTR_QUERIED(buffer);
	CALLTRACE_END();
}

function_signature(void, buf_vprintf, BUFFER* buffer, char* stage_buffer, const char* format_string, va_list args)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	if(stage_buffer != NULL)
	{
		vsprintf(stage_buffer, format_string, args);
		buf_pushv(buffer, stage_buffer, strlen(stage_buffer));
	}
	else
	{
		buf_ucount_t offset = buf_get_element_count(buffer);
		buf_push_pseudo(buffer, 512);
		vsprintf(buf_get_ptr(buffer) + offset, format_string, args);
		buf_set_element_count(buffer, offset + strlen(buf_get_ptr_at_typeof(buffer, char, offset)) + 1);
	}
	WARN_IF_PTR_QUERIED(buffer);
	CALLTRACE_END();
}

function_signature(void, buf_printf, BUFFER* buffer, char* stage_buffer, const char* format_string, ...)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	va_list args;
	va_start(args, format_string);
	buf_vprintf(buffer, stage_buffer, format_string, args);
	va_end(args);
	WARN_IF_PTR_QUERIED(buffer);
	CALLTRACE_END();
}

function_signature(void, buf_push_string, BUFFER* buffer, const char* string)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buf_pushv(buffer, (char*)string, strlen(string));
	WARN_IF_PTR_QUERIED(buffer);
	CALLTRACE_END();
}

function_signature(void, buf_push_char, BUFFER* buffer, char value)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buf_push(buffer, &value);
	WARN_IF_PTR_QUERIED(buffer);
	CALLTRACE_END();
}

function_signature(void, _buf_get_at_s, BUFFER* buffer, buf_ucount_t index, void* out_value, uint32_t out_value_size)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(out_value_size == buf_get_element_size(buffer), "Out value size doesn't match with the Buffer's element size");
	buf_get_at(buffer, index, out_value);
	CALLTRACE_END();
}

BUF_API function_signature(void, buf_push_u8, 	BUFFER* buffer, u8 value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
	CALLTRACE_END();
}
BUF_API function_signature(void, buf_push_u16, 	BUFFER* buffer, u16 value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
	CALLTRACE_END();
}
BUF_API function_signature(void, buf_push_u32, 	BUFFER* buffer, u32 value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
	CALLTRACE_END();
}
BUF_API function_signature(void, buf_push_u64, 	BUFFER* buffer, u64 value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
	CALLTRACE_END();
}
BUF_API function_signature(void, buf_push_s8, 	BUFFER* buffer, s8 value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
	CALLTRACE_END();
}
BUF_API function_signature(void, buf_push_s16, 	BUFFER* buffer, s16 value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
	CALLTRACE_END();
}
BUF_API function_signature(void, buf_push_s32, 	BUFFER* buffer, s32 value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
	CALLTRACE_END();
}
BUF_API function_signature(void, buf_push_s64, 	BUFFER* buffer, s64 value)
{
	CALLTRACE_BEGIN();
	GOOD_ASSERT(buf_get_element_size(buffer) == sizeof(value), "Size mismatch");
	buf_push(buffer, &value);
	CALLTRACE_END();
}

static buf_ucount_t get_selection_index(void* values, u32 stride, buf_ucount_t count, buf_comparer_t compare, void* user_data)
{
	buf_ucount_t index = 0;
	void* value = values;
	for(u32 i = 1; i < count; i++)
	{
		void* cursor = values + stride * i;
		if(compare(cursor, value, user_data))
		{
			value = cursor;
			index = i;
		}
	}
	return index;
}

static void swap(void* ptr1, void* ptr2, u32 size, void* swap_buffer)
{
	if(swap_buffer == NULL)
		swap_buffer = alloca(size);
	memcpy(swap_buffer, ptr1, size);
	memcpy(ptr1, ptr2, size);
	memcpy(ptr2, swap_buffer, size);
}

BUF_API function_signature(void, buf_sort, BUFFER* buffer, buf_comparer_t compare, void* user_data)
{
	CALLTRACE_BEGIN();
	check_pre_condition(buffer);
	buf_ucount_t count = buf_get_element_count(buffer);
	uint32_t stride = buf_get_element_size(buffer);
	void* ptr = buf_get_ptr(buffer);
	uint8_t swap_buffer[stride];
	for(buf_ucount_t i = 0; i < count; i++)
	{
		void* v1 = ptr + stride * i;
		swap(v1, v1 + stride * get_selection_index(v1, stride, count - i, compare, user_data), stride, swap_buffer);
	}
	CALLTRACE_END();
}

BUF_API function_signature(void, buf_push_sort, BUFFER* buffer, void* value, buf_comparer_t compare, void* user_data)
{
	CALLTRACE_BEGIN();
	uint32_t stride = buf_get_element_size(buffer);
	void* ptr = buf_get_ptr(buffer);
	buf_ucount_t i = buf_get_element_count(buffer);
	while(i > 0)
	{
		void* cursor = ptr + (i - 1) * stride;
		if(compare(cursor, value, user_data))
			break;
		--i;
	}
	buf_insert_at(buffer, i, value);
	CALLTRACE_END();
}

BUF_API function_signature(void*, buf_create_element, BUFFER* buffer)
{
	CALLTRACE_BEGIN();
	buf_push_pseudo(buffer, 1);
	CALLTRACE_RETURN(buf_peek_ptr(buffer));
}

bool buf_string_comparer(void* v1, void* v2)
{
	return strcmp(*(char**)v1, *(char**)v2) == 0;
}

bool buf_ptr_comparer(void* v1, void* v2)
{
	return (*(char**)v1) == (*(char**)v2);
}

bool buf_s8_comparer(void* v1, void* v2)
{
	return (*(int8_t*)v1) == (*(int8_t*)v2);
}

bool buf_s16_comparer(void* v1, void* v2)
{
	return (*(int16_t*)v1) == (*(int16_t*)v2);
}

bool buf_s32_comparer(void* v1, void* v2)
{
	return (*(int32_t*)v1) == (*(int32_t*)v2);
}

bool buf_s64_comparer(void* v1, void* v2)
{
	return (*(int64_t*)v1) == (*(int64_t*)v2);
}

bool buf_u8_comparer(void* v1, void* v2)
{
	return (*(uint8_t*)v1) == (*(uint8_t*)v2);
}

bool buf_u16_comparer(void* v1, void* v2)
{
	return (*(uint16_t*)v1) == (*(uint16_t*)v2);
}

bool buf_u32_comparer(void* v1, void* v2)
{
	return (*(uint32_t*)v1) == (*(uint32_t*)v2);
}

bool buf_u64_comparer(void* v1, void* v2)
{
	return (*(uint64_t*)v1) == (*(uint64_t*)v2);
}

bool buf_float_comparer(void* v1, void* v2)
{
	return (*(float*)v1) == (*(float*)v2);
}

bool buf_double_comparer(void* v1, void* v2)
{
	return (*(double*)v1) == (*(double*)v2);
}


BUF_API bool buf_string_greater_than(void* v1, void* v2, void* user_data)
{
	return strlen(*((const char* const*)v1)) > strlen(*((const char* const*)v2));
}

BUF_API bool buf_ptr_greater_than(void* v1, void* v2, void* user_data)
{
	return *(char* const*)v1 > *(char* const*)v2;
}

BUF_API bool buf_s8_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const int8_t*)v1 > *(const int8_t*)v2;
}

BUF_API bool buf_s16_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const int16_t*)v1 > *(const int16_t*)v2;
}

BUF_API bool buf_s32_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const int32_t*)v1 > *(const int32_t*)v2;
}

BUF_API bool buf_s64_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const int64_t*)v1 > *(const int64_t*)v2;
}

BUF_API bool buf_u8_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const uint8_t*)v1 > *(const uint8_t*)v2;
}

BUF_API bool buf_u16_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const uint16_t*)v1 > *(const uint16_t*)v2;
}

BUF_API bool buf_u32_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const uint32_t*)v1 > *(const uint32_t*)v2;
}

BUF_API bool buf_u64_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const uint64_t*)v1 > *(const uint64_t*)v2;
}

BUF_API bool buf_float_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const float*)v1 > *(const float*)v2;
}

BUF_API bool buf_double_greater_than(void* v1, void* v2, void* user_data)
{
	return *(const double*)v1 > *(const double*)v2;
}


BUF_API bool buf_string_less_than(void* v1, void* v2, void* user_data)
{
	return strlen(*((const char* const*)v1)) < strlen(*((const char* const*)v2));
}

BUF_API bool buf_ptr_less_than(void* v1, void* v2, void* user_data)
{
	return *(char* const*)v1 < *(char* const*)v2;
}

BUF_API bool buf_s8_less_than(void* v1, void* v2, void* user_data)
{
	return *(const int8_t*)v1 < *(const int8_t*)v2;
}

BUF_API bool buf_s16_less_than(void* v1, void* v2, void* user_data)
{
	return *(const int16_t*)v1 < *(const int16_t*)v2;
}

BUF_API bool buf_s32_less_than(void* v1, void* v2, void* user_data)
{
	return *(const int32_t*)v1 < *(const int32_t*)v2;
}

BUF_API bool buf_s64_less_than(void* v1, void* v2, void* user_data)
{
	return *(const int64_t*)v1 < *(const int64_t*)v2;
}

BUF_API bool buf_u8_less_than(void* v1, void* v2, void* user_data)
{
	return *(const uint8_t*)v1 < *(const uint8_t*)v2;
}

BUF_API bool buf_u16_less_than(void* v1, void* v2, void* user_data)
{
	return *(const uint16_t*)v1 < *(const uint16_t*)v2;
}

BUF_API bool buf_u32_less_than(void* v1, void* v2, void* user_data)
{
	return *(const uint32_t*)v1 < *(const uint32_t*)v2;
}

BUF_API bool buf_u64_less_than(void* v1, void* v2, void* user_data)
{
	return *(const uint64_t*)v1 < *(const uint64_t*)v2;
}

BUF_API bool buf_float_less_than(void* v1, void* v2, void* user_data)
{
	return *(const float*)v1 < *(const float*)v2;
}

BUF_API bool buf_double_less_than(void* v1, void* v2, void* user_data)
{
	return *(const double*)v1 < *(const double*)v2;
}


BUF_API void buf_string_print(void* value, void* user_data)
{
	printf("%s ", *((const char* const*)value));
}

BUF_API void buf_ptr_print(void* value, void* user_data)
{
	printf("%p ", *(char* const*)value);
}

BUF_API void buf_s8_print(void* value, void* user_data)
{
	printf("%d ", *(const int8_t*)value);
}

BUF_API void buf_s16_print(void* value, void* user_data)
{
	printf("%d ", *(const int16_t*)value);
}

BUF_API void buf_s32_print(void* value, void* user_data)
{
	printf("%d ", *(const int32_t*)value);
}

BUF_API void buf_s64_print(void* value, void* user_data)
{
	printf("%ld ", *(const int64_t*)value);
}

BUF_API void buf_u8_print(void* value, void* user_data)
{
	printf("%u ", *(const uint8_t*)value);
}

BUF_API void buf_u16_print(void* value, void* user_data)
{
	printf("%u ", *(const uint16_t*)value);
}

BUF_API void buf_u32_print(void* value, void* user_data)
{
	printf("%u ", *(const uint32_t*)value);
}

BUF_API void buf_u64_print(void* value, void* user_data)
{
	printf("%lu ", *(const uint64_t*)value);
}

BUF_API void buf_float_print(void* value, void* user_data)
{
	printf("%f ", *(const float*)value);
}

BUF_API void buf_double_print(void* value, void* user_data)
{
	printf("%f ", *(const double*)value);
}


#ifdef BUF_DEBUG
static void check_pre_condition(BUFFER* buffer)
{
	GOOD_ASSERT(buffer != NULL, "buffer is NULL");
}
#endif /*BUF_DEBUG*/
