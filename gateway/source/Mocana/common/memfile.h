/* Version: mss_v6_3 */
/*
 * memfile.h
 *
 * Mocana Memory File System Abstraction Layer
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __MEMFILE_H__
#define __MEMFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MemFile
{
    ubyte*      buff; /* data */
    sbyte4      size; /* size of m_buff */
    sbyte4      pos;  /* pos in file */

} MemFile;


/*------------------------------------------------------------------*/

MOC_EXTERN const AbsStreamFuncs gMemFileAbsStreamFuncs;

MOC_EXTERN sbyte4 MF_attach( MemFile* pMF, sbyte4 size, ubyte* buff);


#ifdef __cplusplus
}
#endif


#endif

