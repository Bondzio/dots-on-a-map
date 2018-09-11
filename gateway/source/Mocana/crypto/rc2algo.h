/* Version: mss_v6_3 */
/*
 * rc2algo.h
 *
 * RC2 Algorithm
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file rc2algo.h RC2 developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for RC2 (stream cipher) operations.

\since 5.3
\version 5.3 and later

! Flags
There are no flag dependencies to enable the functions in this header file.

! External Functions
This file contains the following public ($extern$) functions:
- CreateRC2Ctx
- DeleteRC2Ctx
- DoRC2

*/


/*------------------------------------------------------------------*/

#ifndef __RC2ALGO_H__
#define __RC2ALGO_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __ENABLE_ARC2_CIPHERS__
MOC_EXTERN BulkCtx  CreateRC2Ctx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt);
/* this version has the same signature as the normal one and can be used to specify the effective bits
since the same key schedule is created for encrypt and decrypt */
MOC_EXTERN BulkCtx  CreateRC2Ctx2(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 effectiveBits);
MOC_EXTERN MSTATUS  DeleteRC2Ctx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);
MOC_EXTERN MSTATUS  DoRC2       (MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength, sbyte4 encrypt, ubyte* iv);
#endif

#ifdef __cplusplus
}
#endif


#endif
