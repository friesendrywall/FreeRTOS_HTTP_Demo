/* ----------------------------------------------------------------------------
 * umm_malloc.h - a memory allocator for embedded systems (microcontrollers)
 *
 * See copyright notice in LICENSE.TXT
 * ----------------------------------------------------------------------------
 */

#ifndef UMM_MALLOC_H
#define UMM_MALLOC_H
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif

#define UMM_H_ATTPACKPRE
#define UMM_H_ATTPACKSUF __attribute__((__packed__))

	UMM_H_ATTPACKPRE typedef struct umm_ptr_t {
		unsigned long next;
		unsigned long prev;
	} UMM_H_ATTPACKSUF umm_ptr;


	UMM_H_ATTPACKPRE typedef struct umm_block_t {
		union {
			umm_ptr used;
		} header;
		union {
			umm_ptr free;
			unsigned char data[8];
		} body;
	} UMM_H_ATTPACKSUF umm_block;

	typedef struct {
		umm_block * umm_heap;
		unsigned long heap_size;
		unsigned long umm_numblocks;
		int HeapReady;
	} _Heap;

	/* ------------------------------------------------------------------------ */

	void  umm_init(_Heap * heap);
	void *umm_malloc(_Heap * heap, size_t size);
	void *umm_calloc(_Heap * heap, size_t num, size_t size);
	void *umm_realloc(_Heap * heap, void *ptr, size_t size);
	void  umm_free(_Heap * heap, void *ptr);
	void *umm_info(_Heap * heap, void *ptr, int force, char *buff, int MaxLen);
    int umm_integrity_check(_Heap * heap, char * buff, int MaxLen);


	/* ------------------------------------------------------------------------ */
#ifdef __cplusplus
}
#endif
#endif /* UMM_MALLOC_H */
