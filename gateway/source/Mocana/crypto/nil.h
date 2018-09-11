/* Version: mss_v6_3 */
/*
 * nil.h
 *
 * NIL Header
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#ifndef __NIL_HEADER__
#define __NIL_HEADER__

#ifdef __ENABLE_NIL_CIPHER__

MOC_EXTERN BulkCtx CreateNilCtx (MOC_SYM(hwAccelDescr hwAccelCtx) ubyte* keyMaterial, sbyte4 keyLength, sbyte4 encrypt);
MOC_EXTERN MSTATUS DeleteNilCtx (MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx* ctx);
MOC_EXTERN MSTATUS DoNil        (MOC_SYM(hwAccelDescr hwAccelCtx) BulkCtx ctx, ubyte* data, sbyte4 dataLength, sbyte4 encrypt, ubyte* iv);

#endif /* __ENABLE_NIL_CIPHER__ */

#endif /* __NIL_HEADER__ */
