#ifdef UMM_INFO

/* ----------------------------------------------------------------------------
 * One of the coolest things about this little library is that it's VERY
 * easy to get debug information about the memory heap by simply iterating
 * through all of the memory blocks.
 *
 * As you go through all the blocks, you can check to see if it's a free
 * block by looking at the high order bit of the next block index. You can
 * also see how big the block is by subtracting the next block index from
 * the current block number.
 *
 * The umm_info function does all of that and makes the results available
 * in the ummHeapInfo structure.
 * ----------------------------------------------------------------------------
 */

UMM_HEAP_INFO ummHeapInfo;

void *umm_info(_Heap * heap, void *ptr, int force, char *buff, int MaxLen) {
    int PrintLen;
#define DBGLOG_FORCE( force, format, ... ) {if(force) {PrintLen = snprintf(buff, MaxLen, format, ## __VA_ARGS__  );MaxLen-=PrintLen;buff+=PrintLen;}}
  unsigned long blockNo = 0;

  /* Protect the critical section... */
  UMM_CRITICAL_ENTRY();

  /*
   * Clear out all of the entries in the ummHeapInfo structure before doing
   * any calculations..
   */
  memset( &ummHeapInfo, 0, sizeof( ummHeapInfo ) );

  DBGLOG_FORCE( force, "+----------+---------+----------+----------+---------+----------+----------+\r\n" );
  DBGLOG_FORCE( force, "|0x%08lx|B %7i|NB %7i|PB %7i|Z %7i|NF %7i|PF %7i|\r\n",
      (unsigned long)(&UMM_BLOCK(blockNo)),
      blockNo,
      UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK,
      UMM_PBLOCK(blockNo),
      (UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK )-blockNo,
      UMM_NFREE(blockNo),
      UMM_PFREE(blockNo) );

  /*
   * Now loop through the block lists, and keep track of the number and size
   * of used and free blocks. The terminating condition is an nb pointer with
   * a value of zero...
   */

  blockNo = UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK;

  while( UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK ) {
    while(UartTxSpace()<512);
    size_t curBlocks = (UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK )-blockNo;

    ++ummHeapInfo.totalEntries;
    ummHeapInfo.totalBlocks += curBlocks;

    /* Is this a free block? */

    if( UMM_NBLOCK(blockNo) & UMM_FREELIST_MASK ) {
      ++ummHeapInfo.freeEntries;
      ummHeapInfo.freeBlocks += curBlocks;

      if (ummHeapInfo.maxFreeContiguousBlocks < curBlocks) {
        ummHeapInfo.maxFreeContiguousBlocks = curBlocks;
      }

      DBGLOG_FORCE( force, "|0x%08lx|B %7i|NB %7i|PB %7i|Z %7u|NF %7i|PF %7i|\r\n",
          (unsigned long)(&UMM_BLOCK(blockNo)),
          blockNo,
          UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK,
          UMM_PBLOCK(blockNo),
          (unsigned int)curBlocks,
          UMM_NFREE(blockNo),
          UMM_PFREE(blockNo) );

      /* Does this block address match the ptr we may be trying to free? */

      if( ptr == &UMM_BLOCK(blockNo) ) {

        /* Release the critical section... */
        UMM_CRITICAL_EXIT();

        return( ptr );
      }
    } else {
      ++ummHeapInfo.usedEntries;
      ummHeapInfo.usedBlocks += curBlocks;

      DBGLOG_FORCE( force, "|0x%08lx|B %7i|NB %7i|PB %7i|Z %7u|\r\n",
          (unsigned long)(&UMM_BLOCK(blockNo)),
          blockNo,
          UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK,
          UMM_PBLOCK(blockNo),
          (unsigned int)curBlocks );
    }

    blockNo = UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK;
  }

  /*
   * Update the accounting totals with information from the last block, the
   * rest must be free!
   */

  {
    size_t curBlocks = UMM_NUMBLOCKS-blockNo;
    ummHeapInfo.freeBlocks  += curBlocks;
    ummHeapInfo.totalBlocks += curBlocks;

    if (ummHeapInfo.maxFreeContiguousBlocks < curBlocks) {
      ummHeapInfo.maxFreeContiguousBlocks = curBlocks;
    }
  }

  DBGLOG_FORCE( force, "|0x%08lx|B %7i|NB %7i|PB %7i|Z %7i|NF %7i|PF %7i|\r\n",
      (unsigned long)(&UMM_BLOCK(blockNo)),
      blockNo,
      UMM_NBLOCK(blockNo) & UMM_BLOCKNO_MASK,
      UMM_PBLOCK(blockNo),
      UMM_NUMBLOCKS-blockNo,
      UMM_NFREE(blockNo),
      UMM_PFREE(blockNo) );

  DBGLOG_FORCE( force, "+----------+---------+----------+----------+---------+----------+----------+\r\n" );

  DBGLOG_FORCE( force, "Total Entries %7i    Used Entries %7i    Free Entries %7i\r\n",
      ummHeapInfo.totalEntries,
      ummHeapInfo.usedEntries,
      ummHeapInfo.freeEntries );

  DBGLOG_FORCE( force, "Total Blocks  %7i    Used Blocks  %7i    Free Blocks  %7i\r\n",
      ummHeapInfo.totalBlocks,
      ummHeapInfo.usedBlocks,
	    ummHeapInfo.freeBlocks);
    DBGLOG_FORCE(force, "Total Bytes %7i    Used Bytes %7i    Free Bytes  %7i\r\n",
	    ummHeapInfo.totalBlocks * 16,
	    ummHeapInfo.usedBlocks * 16,
	    ummHeapInfo.freeBlocks * 16);

    DBGLOG_FORCE(force, "+--------------------------------------------------------------------------+\r\n" );

  /* Release the critical section... */
  UMM_CRITICAL_EXIT();

  return( NULL );
}

/* ------------------------------------------------------------------------ */

size_t umm_free_heap_size( _Heap * heap ) {
	umm_info(heap, NULL, 0, 0, 0);
  return (size_t)ummHeapInfo.freeBlocks * sizeof(umm_block);
}

/* ------------------------------------------------------------------------ */
#endif
