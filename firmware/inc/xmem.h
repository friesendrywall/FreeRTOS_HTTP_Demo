/* 
 * File:   xmem.h
 * Author: Erik
 *
 * Created on January 8, 2018, 11:17 AM
 */

#ifndef XMEM_H
#define	XMEM_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stddef.h>
    
    void xmem_init(void);
    void x_free(void * ptr);
    void * x_malloc(uint32_t size, uint32_t flags);
    void * x_calloc(size_t num, size_t size, uint32_t flags);
    void * x_realloc(void * ptr, size_t size);
    void x_info(uint32_t heap, char *buff, int MaxLen);
    //TCP glue
    int x_integrity(uint32_t heap, char *buff, int MaxLen);

#define XMEM_HEAP_SLOW         1
#define XMEM_HEAP_FAST         2
//#define XMEM_HEAP_COHERENT  16
#define XMEM_HEAP_ALIGN32     32
#define XMEM_HEAP_SKIP_HOOK   64
#define XMEM_SINGLE_HEAP

#ifdef	__cplusplus
}
#endif

#endif	/* XMEM_H */

