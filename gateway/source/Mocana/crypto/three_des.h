/* Version: mss_v6_3 */
/*
 * three_des.h
 *
 * 3DES Header
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/* \file three_des.h 3DES developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for 3DES encryption and decryption.

\since 1.41
\version 3.06 and later

! Flags
To enable this file's functions, the following flag must #not# be defined:
$__DISABLE_3DES_CIPHERS__$

! External Functions
This file contains the following public ($extern$) functions:
- Create3DESCtx
- Create2Key3DESCtx
- Delete3DESCtx
- Do3DES

*/

/*------------------------------------------------------------------*/

#ifndef __3DES_HEADER__
#define __3DES_HEADER__

#ifdef __cplusplus
extern "C" {
#endif

#define THREE_DES_BLOCK_SIZE        (8)
#define THREE_DES_KEY_LENGTH        (24)


/*------------------------------------------------------------------*/

typedef struct
{
    des_ctx firstKey;
    des_ctx secondKey;
    des_ctx thirdKey;
} ctx3des;

typedef struct
{
    ctx3des encryptKey;
    ctx3des decryptKey;

} DES3Ctx;


/*------------------------------------------------------------------*/

/* for white box testing */
MOC_EXTERN MSTATUS THREE_DES_initKey(ctx3des *p_3desContext, const ubyte *pKey, sbyte4 keyLen);
MOC_EXTERN MSTATUS THREE_DES_encipher(ctx3des *p_3desContext, ubyte *pSrc, ubyte *pDest, ubyte4 numBytes);
MOC_EXTERN MSTATUS THREE_DES_decipher(ctx3des *p_3desContext, ubyte *pSrc, ubyte *pDest, ubyte4 numBytes);

/* actual APIs */
#ifndef __DISABLE_3DES_CIPHERS__

MOC_EXTERN BulkCtx Create3DESCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt);

MOC_EXTERN BulkCtx Create2Key3DESCtx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt);

MOC_EXTERN MSTATUS Delete3DESCtx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);

MOC_EXTERN MSTATUS Do3DES(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength, sbyte4 encrypt, ubyte* iv);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __3DES_HEADER__ */
