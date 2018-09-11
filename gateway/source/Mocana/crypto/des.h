/* Version: mss_v6_3 */
/*
 * des.h
 *
 * DES Header
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file des.h DES developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for DES encryption and decryption.

\since 5.3
\version 5.3 and later

! Flags
To enable this file's functions, define the following flag:
$__ENABLE_DES_CIPHER__$

! External Functions
This file contains the following public ($extern$) functions:
- Create3DESCtx
- Create2Key3DESCtx
- Delete3DESCtx
- Do3DES

*/


/*------------------------------------------------------------------*/

#ifndef __DES_HEADER__
#define __DES_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define DES_BLOCK_SIZE      (8)
#define DES_KEY_LENGTH      (8)


/*------------------------------------------------------------------*/

typedef struct {
    ubyte4 ek[32];
    ubyte4 dk[32];

} des_ctx, DES_CTX;


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS DES_initKey(des_ctx *p_desContext, const ubyte *pKey, sbyte4 keyLen);
MOC_EXTERN MSTATUS DES_encipher(des_ctx *p_desContext, ubyte *pSrc, ubyte *pDest, ubyte4 numBytes);
MOC_EXTERN MSTATUS DES_decipher(des_ctx *p_desContext, ubyte *pSrc, ubyte *pDest, ubyte4 numBytes);

#ifdef __ENABLE_DES_CIPHER__
MOC_EXTERN BulkCtx CreateDESCtx (MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt);
MOC_EXTERN MSTATUS DeleteDESCtx (MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx* ctx);
MOC_EXTERN MSTATUS DoDES        (MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength, sbyte4 encrypt, ubyte* iv);
#endif /* __ENABLE_DES_CIPHER__ */

#ifdef __cplusplus
}
#endif

#endif /* __DES_HEADER__ */
