/* Version: mss_v6_3 */
/*
 * rc4algo.h
 *
 * RC4 Algorithm
 *
 * Copyright Mocana Corp 2003-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file rc4algo.h RC4 developer API header.
This header file contains definitions, enumerations, structures, and function
declarations used for RC4 (stream cipher) operations.

\since 5.3
\version 5.3 and later

! Flags
There are no flag dependencies to enable the functions in this header file.

! External Functions
This file contains the following public ($extern$) functions:
- CreateRC4Ctx
- DeleteRC4Ctx
- DoRC4

*/

/*------------------------------------------------------------------*/

#ifndef __RC4ALGO_H__
#define __RC4ALGO_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __DISABLE_ARC4_CIPHERS__
MOC_EXTERN BulkCtx  CreateRC4Ctx(MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt);
MOC_EXTERN MSTATUS  DeleteRC4Ctx(MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx *ctx);
MOC_EXTERN MSTATUS  DoRC4       (MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength, sbyte4 encrypt, ubyte* iv);
#endif

#ifdef __cplusplus
}
#endif


#endif
