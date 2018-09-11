/* Version: mss_v6_3 */
/*
 * ansix9_63_kdf.h
 *
 * ansi x9.63 Key Derivation Function
 *
 * Copyright Mocana Corp 2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __ANSIX9_63_KDF_HEADER__
#define __ANSIX9_63_KDF_HEADER__


/*------------------------------------------------------------------*/

extern MSTATUS
ANSIX963KDF_generate( MOC_HASH(hwAccelDescr hwAccelCtx)
                        const BulkHashAlgo* pBulkHashAlgo,
                        ubyte* z, ubyte4 zLength,
                        const ubyte* sharedInfo, ubyte4 sharedInfoLen,
                        ubyte4 retLen, ubyte ret[/*retLen*/]);

#endif /* __ANSIX9_63_KDF_HEADER__*/

