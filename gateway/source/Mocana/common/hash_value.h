/* Version: mss_v6_3 */
/*
 * hash_value.h
 *
 * Generate Hash Value Header
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __HASH_VALUE_HEADER__
#define __HASH_VALUE_HEADER__

MOC_EXTERN MSTATUS HASH_VALUE_hashWord(const ubyte4 *pHashKeyData, ubyte4 hashKeyDataLength, ubyte4 initialHashValue, ubyte4 *pRetHashValue);
MOC_EXTERN void HASH_VALUE_hashGen(const void *pHashKeyData, ubyte4 hashKeyDataLength, ubyte4 initialHashValue, ubyte4 *pRetHashValue);

#endif /* __HASH_VALUE_HEADER__ */
