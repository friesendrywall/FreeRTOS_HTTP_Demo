/*
 * Configuration for umm_malloc
 */

#ifndef _UMM_MALLOC_CFG_H
#define _UMM_MALLOC_CFG_H

/*
 * There are a number of defines you can set at compile time that affect how
 * the memory allocator will operate.
 * You can set them in your config file umm_malloc_cfg.h.
 * In GNU C, you also can set these compile time defines like this:
 *
 * -D UMM_TEST_MAIN
 *
 * Set this if you want to compile in the test suite at the end of this file.
 *
 * If you leave this define unset, then you might want to set another one:
 *
 * -D UMM_REDEFINE_MEM_FUNCTIONS
 *
 * If you leave this define unset, then the function names are left alone as
 * umm_malloc() umm_free() and umm_realloc() so that they cannot be confused
 * with the C runtime functions malloc() free() and realloc()
 *
 * If you do set this define, then the function names become malloc()
 * free() and realloc() so that they can be used as the C runtime functions
 * in an embedded environment.
 *
 * -D UMM_BEST_FIT (default)
 *
 * Set this if you want to use a best-fit algorithm for allocating new
 * blocks
 *
 * -D UMM_FIRST_FIT
 *
 * Set this if you want to use a first-fit algorithm for allocating new
 * blocks
 *
 * -D UMM_DBG_LOG_LEVEL=n
 *
 * Set n to a value from 0 to 6 depending on how verbose you want the debug
 * log to be
 *
 * ----------------------------------------------------------------------------
 *
 * Support for this library in a multitasking environment is provided when
 * you add bodies to the UMM_CRITICAL_ENTRY and UMM_CRITICAL_EXIT macros
 * (see below)
 *
 * ----------------------------------------------------------------------------
 */

//extern char test_umm_heap[];//0xA8000000

/* Start addresses and the size of the heap */
//#define UMM_MALLOC_CFG_HEAP_ADDR (0x88000000)//cached space
//#define UMM_MALLOC_CFG_HEAP_SIZE (32 * 1024 *1024)
//#define UMM_MALLOC_CFG_UNCACHED_OFFSET (0x20000000)
//#define DDR_BaseAddress_Uncached    0xA8000000
//#define DDR_BaseAddress_Cached      0x88000000

/* A couple of macros to make packing structures less compiler dependent */

//#define UMM_H_ATTPACKPRE
//#define UMM_H_ATTPACKSUF __attribute__((__packed__))

#define UMM_BEST_FIT
#undef  UMM_FIRST_FIT

/*
 * -D UMM_INFO :
 *
 * Enables a dup of the heap contents and a function to return the total
 * heap size that is unallocated - note this is not the same as the largest
 * unallocated block on the heap!
 */

#define UMM_INFO

#ifdef UMM_INFO
  typedef struct UMM_HEAP_INFO_t {
    unsigned long totalEntries;
    unsigned long usedEntries;
    unsigned long freeEntries;

    unsigned long totalBlocks;
    unsigned long usedBlocks;
    unsigned long freeBlocks;

    unsigned long maxFreeContiguousBlocks;
  }
  UMM_HEAP_INFO;

  extern UMM_HEAP_INFO ummHeapInfo;

  void *umm_info(_Heap * heap, void *ptr, int force,char *buff, int MaxLen);
  size_t umm_free_heap_size(_Heap * heap);

#else
#endif

/*
 * A couple of macros to make it easier to protect the memory allocator
 * in a multitasking system. You should set these macros up to use whatever
 * your system uses for this purpose. You can disable interrupts entirely, or
 * just disable task switching - it's up to you
 *
 * NOTE WELL that these macros MUST be allowed to nest, because umm_free() is
 * called from within umm_malloc()
 */

#define UMM_CRITICAL_ENTRY() vTaskSuspendAll()
#define UMM_CRITICAL_EXIT() xTaskResumeAll()

/*
 * -D UMM_INTEGRITY_CHECK :
 *
 * Enables heap integrity check before any heap operation. It affects
 * performance, but does NOT consume extra memory.
 *
 * If integrity violation is detected, the message is printed and user-provided
 * callback is called: `UMM_HEAP_CORRUPTION_CB()`
 *
 * Note that not all buffer overruns are detected: each buffer is aligned by
 * 4 bytes, so there might be some trailing "extra" bytes which are not checked
 * for corruption.
 */

#define UMM_INTEGRITY_CHECK

#ifdef UMM_INTEGRITY_CHECK
  int umm_integrity_check(_Heap *heap, char *buff, int MaxLen);
#define INTEGRITY_CHECK(x) umm_integrity_check(x)
  extern void umm_corruption(void);
#ifdef DEBUG
#define UMM_HEAP_CORRUPTION_CB() debug_printf("Heap Corruption!")
#else
#define UMM_HEAP_CORRUPTION_CB()
#endif
#else
#define INTEGRITY_CHECK() 0
#endif

/*
 * -D UMM_POISON :
 *
 * Enables heap poisoning: add predefined value (poison) before and after each
 * allocation, and check before each heap operation that no poison is
 * corrupted.
 *
 * Other than the poison itself, we need to store exact user-requested length
 * for each buffer, so that overrun by just 1 byte will be always noticed.
 *
 * Customizations:
 *
 *    UMM_POISON_SIZE_BEFORE:
 *      Number of poison bytes before each block, e.g. 2
 *    UMM_POISON_SIZE_AFTER:
 *      Number of poison bytes after each block e.g. 2
 *    UMM_POISONED_BLOCK_LEN_TYPE
 *      Type of the exact buffer length, e.g. `short`
 *
 * NOTE: each allocated buffer is aligned by 4 bytes. But when poisoning is
 * enabled, actual pointer returned to user is shifted by
 * `(sizeof(UMM_POISONED_BLOCK_LEN_TYPE) + UMM_POISON_SIZE_BEFORE)`.
 * It's your responsibility to make resulting pointers aligned appropriately.
 *
 * If poison corruption is detected, the message is printed and user-provided
 * callback is called: `UMM_HEAP_CORRUPTION_CB()`
 */

#define UMM_POISON_CHECK

#define UMM_POISON_SIZE_BEFORE 8
#define UMM_POISON_SIZE_AFTER 8
#define UMM_POISONED_BLOCK_LEN_TYPE long

#ifdef UMM_POISON_CHECK
void *umm_poison_malloc(_Heap * heap, size_t size);
   void *umm_poison_calloc(_Heap * heap, size_t num, size_t size);
   void *umm_poison_realloc(_Heap * heap, void *ptr, size_t size);
   void  umm_poison_free(_Heap * heap, void *ptr);
   int   umm_poison_check(_Heap * heap);
#  define POISON_CHECK(x) umm_poison_check(x)
#else
#  define POISON_CHECK() 0
#endif

#endif /* _UMM_MALLOC_CFG_H */