#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "xmem.h"
#include "FreeRTOS.h"
#include "task.h"
#include "umm_malloc/umm_malloc.h"
//#include <sys/kmem.h>
//#include "logs.h"

//#include "system/devcon/sys_devcon.h"

extern void *__real_malloc(size_t);
extern void __real_free(void *);
extern void *__real_realloc(void *, size_t);
extern void *__real_calloc(size_t, size_t);

#define HEAPSLOW_CFG_HEAP_SIZE 0x20000
#define HEAPFAST_CFG_HEAP_SIZE 0x10000

__attribute__ ((aligned (32)))
__attribute__ ((section(".slow_heap")))
uint8_t heap_arraySlow[HEAPSLOW_CFG_HEAP_SIZE];

__attribute__ ((aligned (32)))
__attribute__ ((section(".fast_run")))
uint8_t heap_arrayFast[HEAPFAST_CFG_HEAP_SIZE];

_Heap HeapSlow = { .umm_heap = (umm_block *)heap_arraySlow, .heap_size = HEAPSLOW_CFG_HEAP_SIZE, .umm_numblocks = 0, .HeapReady = 0 };
_Heap HeapFast = { .umm_heap = (umm_block *)heap_arrayFast, .heap_size = HEAPFAST_CFG_HEAP_SIZE, .umm_numblocks = 0, .HeapReady = 0 };
void xmem_init(void) {
  umm_init(&HeapFast);
  umm_init(&HeapSlow);
}

static inline uint32_t calcAlign32(uint32_t size) {
  uint32_t extra = (32 - (size % 32));
  if (extra == 32) {
    extra = 0;
  }
  return size + extra + 32;
}

static unsigned char *calcAlign32Ptr(unsigned char *ptr) {
  //Its either 8 or 24 bit aligned.
  uint32_t shift = (32 - (uint32_t)ptr % 32);
  unsigned char *newPtr = ptr + shift;
  *((uint32_t *)(newPtr)-1) = shift;
  return newPtr;
}

void *__wrap_strdup(const char *str1) {
  uint32_t len = strlen(str1) + 1;
  void *ret = x_malloc(len, XMEM_HEAP_SLOW);
  strcpy((char *)ret, str1);
  return ret;
}

void *__wrap_malloc(int xWantedSize) {
  return x_malloc(xWantedSize, XMEM_HEAP_FAST);
}

void __wrap_free(void *ptr) {
  x_free(ptr);
}

void *__wrap_realloc(void *ptr, size_t size) {
  return x_realloc(ptr, size);
}

void *__wrap_calloc(size_t num, size_t size) {
  return x_calloc(num, size, XMEM_HEAP_FAST);
}

void *x_malloc(uint32_t size, uint32_t flags) {
  void *pvReturn = NULL;
  if (flags & XMEM_HEAP_ALIGN32) {
    size = calcAlign32(size);
  }
  if (flags & XMEM_HEAP_SLOW) {
    pvReturn = umm_malloc(&HeapSlow, size);
  } else if (flags & XMEM_HEAP_FAST) {
    pvReturn = umm_malloc(&HeapFast, size);
  }  else { //XMEM_HEAP_1
    pvReturn = umm_malloc(&HeapSlow, size);
  }
  if (pvReturn) {
    if (flags & XMEM_HEAP_ALIGN32) {
      pvReturn = (void *)calcAlign32Ptr(pvReturn);
    }
  }
#if (configUSE_MALLOC_FAILED_HOOK == 1)
  {
    if (pvReturn == NULL) {
      extern void vApplicationMallocFailedHook(void);
      if (!(flags & XMEM_HEAP_SKIP_HOOK)) {
        vApplicationMallocFailedHook();
      }
    }
  }
#endif
  return pvReturn;
}

void *x_calloc(size_t num, size_t size, uint32_t flags) {
  void *pvReturn = NULL;
  size_t Allocate = (size_t)(size * num);
  if (flags & (XMEM_HEAP_ALIGN32)) {
    Allocate = calcAlign32(Allocate);
  }
  if (flags & XMEM_HEAP_SLOW) {
    pvReturn = umm_malloc(&HeapSlow, Allocate);
  } else if (flags & XMEM_HEAP_FAST) {
    pvReturn = umm_malloc(&HeapFast, Allocate);
  } else { //XMEM_HEAP_1
    pvReturn = umm_malloc(&HeapSlow, Allocate);
  }
  if (pvReturn) {
    memset(pvReturn, 0x00, (size_t)Allocate);
    if (flags & XMEM_HEAP_ALIGN32) {
      pvReturn = (void *)calcAlign32Ptr(pvReturn);
    }
  }

#if (configUSE_MALLOC_FAILED_HOOK == 1)
  {
    if (pvReturn == NULL) {
      extern void vApplicationMallocFailedHook(void);
      if (!(flags & XMEM_HEAP_SKIP_HOOK)) {
        vApplicationMallocFailedHook();
      }
    }
  }
#endif
  return pvReturn;
}

void *x_realloc(void *ptr, size_t size) {
  void *pvReturn = NULL;
  int WasAligned = 0;

  if (ptr && (uint32_t)ptr % 32 == 0) {
#if 1
    configASSERT(0);//No cache aligned functions should use realloc due to changing offsets

#else
    //This is an aligned pointer, we need to downshift
    uint32_t shiftValue = *((uint32_t *)(ptr)-1);
    ptr = (void *)((unsigned char *)ptr) - shiftValue;
    WasAligned = 1;
    size = calcAlign32(size);
#endif
  }
  if (ptr == NULL || ((unsigned char *)ptr >= (unsigned char *)HeapSlow.umm_heap && (unsigned char *)ptr < ((unsigned char *)HeapSlow.umm_heap) + HeapSlow.heap_size)) {
    pvReturn = umm_realloc(&HeapSlow, ptr, size);
  } else if ((unsigned char *)ptr >= (unsigned char *)HeapFast.umm_heap && (unsigned char *)ptr < ((unsigned char *)HeapFast.umm_heap) + HeapFast.heap_size) {
    pvReturn = umm_realloc(&HeapFast, ptr, size);
  } 

#if (configUSE_MALLOC_FAILED_HOOK == 1)
  {
    if (pvReturn == NULL) {
      extern void vApplicationMallocFailedHook(void);
      vApplicationMallocFailedHook();
    }
  }
#endif
  if (pvReturn && WasAligned) {
    pvReturn = (void *)calcAlign32Ptr(pvReturn);
  }
  return pvReturn;
}

void x_free(void *ptr) {
  if (ptr && (uint32_t)ptr % 32 == 0) {
    //This is an aligned pointer, we need to downshift
    uint32_t shiftValue = *((uint32_t *)(ptr)-1);
    ptr = (void *)((unsigned char *)ptr) - shiftValue;
  }
  if ((unsigned char *)ptr >= (unsigned char *)HeapSlow.umm_heap && (unsigned char *)ptr < ((unsigned char *)HeapSlow.umm_heap) + HeapSlow.heap_size) {
    umm_free(&HeapSlow, ptr);
  } else if ((unsigned char *)ptr >= (unsigned char *)HeapFast.umm_heap && (unsigned char *)ptr < ((unsigned char *)HeapFast.umm_heap) + HeapFast.heap_size) {
    umm_free(&HeapFast, ptr);
  } 
}

void x_info(uint32_t heap, char *buff, int MaxLen) {
  switch (heap) {
  default:
    snprintf(buff, MaxLen, "Unspecified Heap");
    //lint -fallthrough
  case XMEM_HEAP_SLOW:
    (void)umm_info(&HeapSlow, 0, 1, buff, MaxLen);
    break;
  case XMEM_HEAP_FAST:
    (void)umm_info(&HeapFast, 0, 1, buff, MaxLen);
    break;
  }
}

int x_integrity(uint32_t heap, char *buff, int MaxLen) {
  switch (heap) {
  default:
    snprintf(buff, MaxLen, "Unspecified Heap");
    return 0;
  case XMEM_HEAP_SLOW:
    return umm_integrity_check(&HeapSlow, buff, MaxLen);
  case XMEM_HEAP_FAST:
    return umm_integrity_check(&HeapFast, buff, MaxLen);
  }
}