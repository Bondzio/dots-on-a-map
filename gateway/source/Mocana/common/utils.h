/* Version: mss_v6_3 */
/*
 * utils.h
 *
 * Utility header for storing and retrieving keys
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __UTILS_HEADER__
#define __UTILS_HEADER__

#ifdef __cplusplus
extern "C" {
#endif


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS UTILS_readFile(const char* pFilename, ubyte **ppRetBuffer, ubyte4 *pRetBufLength);
MOC_EXTERN MSTATUS UTILS_readFileRaw(const ubyte* pFileObj, ubyte **ppRetBuffer, ubyte4 *pRetBufLength);
MOC_EXTERN MSTATUS UTILS_freeReadFile(ubyte **ppRetBuffer);
MOC_EXTERN MSTATUS UTILS_writeFile(const char* pFilename, ubyte *pBuffer, ubyte4 bufLength);


#ifdef __cplusplus
}
#endif

#endif /* __UTILS_HEADER__ */

