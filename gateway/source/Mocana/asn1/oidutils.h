/* Version: mss_v6_3 */
/*
 * oidutils.h
 *
 * OIDutils.h
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __OIDUTILS_HEADER__
#define __OIDUTILS_HEADER__

/*------------------------------------------------------------------*/

/* exported routines */

/* convert a OID string in the format "2.16.840.1.113719.1.2.8.132"
    to an complete DER encoding (includes tag and length).
    The oid buffer was allocated with MALLOC and must be freed using FREE */
MOC_EXTERN MSTATUS BEREncodeOID( const sbyte* oidStr, byteBoolean* wildCard, ubyte** oid);

#endif /* #ifndef __OIDUTILS_HEADER__ */
