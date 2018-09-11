/* Version: mss_v6_3 */
/*
 * pkcs_key.h
 *
 * PKCS#1 PKCS#8 Parser and utilities routines
 *
 * Copyright Mocana Corp 2006-2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file pkcs_key.h PKCS1 and PKCS10 utility routines.
This header file contains definitions, enumerations, structures, and function
declarations used by PKCS1 and PKCS10 utility routines.

\since 1.41
\version 5.3 and later

! Flags
To build products using this header file, the following flag must be defined
in moptions.h:
- $__ENABLE_MOCANA_PKCS10__$

! External Functions
This file contains the following public ($extern$) function declarations:
- PKCS_getPKCS8Key
- PKCS_getPKCS8KeyEx
*/
#ifndef __PKCS_KEY_HEADER__
#define __PKCS_KEY_HEADER__


/*------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* encryption types --- do not change the values associated with the constants */
enum PKCS8EncryptionType
{
    PCKS8_EncryptionType_undefined = 0,
#if defined(__ENABLE_MOCANA_PKCS5__)
#if defined(__ENABLE_DES_CIPHER__)
    PCKS8_EncryptionType_pkcs5_v1_sha1_des  = 10, /* oid suffix */
#endif

#if defined(__ENABLE_ARC2_CIPHERS__)
    PCKS8_EncryptionType_pkcs5_v1_sha1_rc2  = 11, /* oid suffix */
#endif

#if defined(__ENABLE_DES_CIPHER__) && defined(__ENABLE_MOCANA_MD2__)
    PCKS8_EncryptionType_pkcs5_v1_md2_des   = 1,   /* oid suffix */
#endif

#if defined(__ENABLE_ARC2_CIPHERS__) && defined(__ENABLE_MOCANA_MD2__)
    PCKS8_EncryptionType_pkcs5_v1_md2_rc2   = 4,  /* oid suffix */
#endif

#if defined(__ENABLE_DES_CIPHER__)
    PCKS8_EncryptionType_pkcs5_v1_md5_des   = 3,  /* oid suffix */
#endif

#if defined(__ENABLE_ARC2_CIPHERS__)
    PCKS8_EncryptionType_pkcs5_v1_md5_rc2   = 6, /* oid suffix */
#endif

#if !defined(__DISABLE_3DES_CIPHERS__)
    PCKS8_EncryptionType_pkcs5_v2_3des      = 5000 + 1, /* no signification */
#endif

#if defined(__ENABLE_DES_CIPHER__)
    PCKS8_EncryptionType_pkcs5_v2_des       = 5000 + 2, /* no signification */
#endif

#if defined(__ENABLE_ARC2_CIPHERS__)
    PCKS8_EncryptionType_pkcs5_v2_rc2       = 5000 + 3, /* no signification */
#endif

#endif /*  __ENABLE_MOCANA_PKCS5__  */

#if defined(__ENABLE_MOCANA_PKCS12__)
    PKCS8_EncryptionType_pkcs12             = 12000,
#if !defined(__DISABLE_3DES_CIPHERS__)
   PCKS8_EncryptionType_pkcs12_sha_2des    = PKCS8_EncryptionType_pkcs12 + 4, /* 12000 + oid suffix */
    PCKS8_EncryptionType_pkcs12_sha_3des    = PKCS8_EncryptionType_pkcs12 + 3, /* 12000 + oid suffix */
#endif

#if defined(__ENABLE_ARC2_CIPHERS__)
    PCKS8_EncryptionType_pkcs12_sha_rc2_40  = PKCS8_EncryptionType_pkcs12 + 6, /* 12000 + oid suffix */
    PCKS8_EncryptionType_pkcs12_sha_rc2_128 = PKCS8_EncryptionType_pkcs12 + 5, /* 12000 + oid suffix */
#endif

#if !defined(__DISABLE_ARC4_CIPHERS__)
    PCKS8_EncryptionType_pkcs12_sha_rc4_40  = PKCS8_EncryptionType_pkcs12 + 2, /* 12000 + oid suffix */
    PCKS8_EncryptionType_pkcs12_sha_rc4_128 = PKCS8_EncryptionType_pkcs12 + 1, /* 12000 + oid suffix */
#endif
#endif /* __ENABLE_MOCANA_PKCS12__ */

};


#if !defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__)

MOC_EXTERN MSTATUS PKCS_getPKCS1Key(MOC_HASH(hwAccelDescr hwAccelCtx)const ubyte* pPKCS1DER, ubyte4 pkcs1DERLen, AsymmetricKey* pKey);
#if defined(__ENABLE_MOCANA_DSA__)
/* This read an unencrypted raw file like those produced by openssl */
MOC_EXTERN MSTATUS PKCS_getDSAKey( const ubyte* pDSAKeyDer, ubyte4 pDSAKeyDerLen, AsymmetricKey* pKey);
#endif
/* unencrypted PKCS8 */
MOC_EXTERN MSTATUS PKCS_getPKCS8Key(MOC_RSA(hwAccelDescr hwAccelCtx)const ubyte* pPKCS8DER, ubyte4 pkcs8DERLen, AsymmetricKey* pKey);
/* encrypted or unencrypted PKCS8 */
MOC_EXTERN MSTATUS PKCS_getPKCS8KeyEx(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)const ubyte* pPKCS8DER, ubyte4 pkcs8DERLen,
                                      const ubyte* password, ubyte4 passwordLen, AsymmetricKey* pKey);

#if defined( __ENABLE_MOCANA_DER_CONVERSION__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__)
MOC_EXTERN MSTATUS PKCS_setPKCS1Key(MOC_RSA(hwAccelDescr hwAccelCtx)
                                    AsymmetricKey* pKey,
                                    ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength);

#ifdef __ENABLE_MOCANA_DSA__
MOC_EXTERN MSTATUS PKCS_setDsaDerKey( AsymmetricKey* pKey,
                                     ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength);
#endif

#endif

#if defined( __ENABLE_MOCANA_DER_CONVERSION__)
MOC_EXTERN MSTATUS PKCS_setPKCS8Key(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                    AsymmetricKey* pKey,
                                    randomContext* pRandomContext,
                                    enum PKCS8EncryptionType encType,
                                    const ubyte* password, ubyte4 passwordLen,
                                    ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength);
#endif

#endif /* !defined(__DISABLE_MOCANA_CERTIFICATE_PARSING__) */


#ifdef __cplusplus
}
#endif

#endif  /* __PKCS_KEY_HEADER__ */
