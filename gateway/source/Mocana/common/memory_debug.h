/* Version: mss_v6_3 */
/*
 * memory_debug.h
 *
 * Mocana Memory Leak Detection Code
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __MEMORY_DEBUG_HEADER__
#define __MEMORY_DEBUG_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ENABLE_MOCANA_DEBUG_MEMORY__
#define DEBUG_RELABEL_MEMORY(PTR)       dbg_relabel_memory(PTR,(ubyte *)__FILE__,__LINE__)
#define DEBUG_CHECK_MEMORY(PTR)         dbg_check_memory  (PTR,(ubyte *)__FILE__,__LINE__)
#else
#define DEBUG_RELABEL_MEMORY(PTR)
#define DEBUG_CHECK_MEMORY(PTR)
#endif


/*------------------------------------------------------------------*/

MOC_EXTERN void dbg_relabel_memory(void *pBlockToRelabel, ubyte *pFile, ubyte4 lineNum);
MOC_EXTERN void dbg_check_memory  (void *pBlockToCheck, ubyte *pFile, ubyte4 lineNum);

extern ubyte4 MEMORY_DEBUG_resetHighWaterMark(void);

#ifdef __cplusplus
}
#endif

#endif /* __MEMORY_DEBUG_HEADER__ */

