/* Version: mss_v6_3 */
/*
 * pkcs_key.c
 *
 * PKCS Utilities (PKCS1 and PKCS8 )
 *
 * Copyright Mocana Corp 2006-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/* \file pkcs_key.c Mocana certificate store factory.
This file contains PKCS key functions.

\since 2.02
\version 2.02 and later

! Flags
Whether the following flags are defined determines which additional header files
are included:
- $__ENABLE_MOCANA_ECC__$
- $__ENABLE_MOCANA_PKCS12__$
- $__ENABLE_MOCANA_PKCS5__$
- $__ENABLE_MOCANA_PKCS7__$

! External Functions
This file contains the following public ($extern$) functions:
- PKCS_getPKCS8Key
- PKCS_getPKCS8KeyEx

*/

#include "../common/moptions.h"

#if !defined( __DISABLE_MOCANA_CERTIFICATE_PARSING__) || defined(__ENABLE_MOCANA_DER_CONVERSION__)

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mstdlib.h"
#include "../common/mrtos.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../common/memfile.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../crypto/crypto.h"
#include "../crypto/rsa.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#ifdef __ENABLE_MOCANA_DSA__
#include "../crypto/dsa.h"
#endif
#include "../crypto/pubcrypto.h"
#include "../harness/harness.h"
#include "../asn1/parseasn1.h"
#include "../asn1/ASN1TreeWalker.h"
#include "../asn1/oiddefs.h"
#include "../asn1/derencoder.h"
#include "../crypto/pkcs_common.h"
#if defined( __ENABLE_MOCANA_PKCS7__ ) || defined(__ENABLE_MOCANA_PKCS12__)
#include "../crypto/pkcs7.h"
#endif
#ifdef __ENABLE_MOCANA_PKCS12__
#include "../crypto/pkcs12.h"
#endif
#ifdef __ENABLE_MOCANA_PKCS5__
#include "../crypto/pkcs5.h"
#endif
#include "../crypto/ca_mgmt.h"
#ifdef __ENABLE_MOCANA_ECC__
#include "../crypto/sec_key.h"
#endif

#include "../crypto/pkcs_key.h"

#endif


#if !defined( __DISABLE_MOCANA_CERTIFICATE_PARSING__)
/*

PKCS#1:

RSAPublicKey ::= SEQUENCE {
    modulus           INTEGER,  -- n
    publicExponent    INTEGER   -- e
}

RSAPrivateKey ::= SEQUENCE {
    version           Version,
    modulus           INTEGER,  -- n
    publicExponent    INTEGER,  -- e
    privateExponent   INTEGER,  -- d
    prime1            INTEGER,  -- p
    prime2            INTEGER,  -- q
    exponent1         INTEGER,  -- d mod (p-1)
    exponent2         INTEGER,  -- d mod (q-1)
    coefficient       INTEGER,  -- (inverse of q) mod p
    otherPrimeInfos   OtherPrimeInfos OPTIONAL
}

PKCS#8:

PrivateKeyInfo ::= SEQUENCE {
  version Version,
  privateKeyAlgorithm AlgorithmIdentifier {{PrivateKeyAlgorithms}},
  privateKey PrivateKey,
  attributes [0] Attributes OPTIONAL }

  Version ::= INTEGER {v1(0)} (v1,...)

  PrivateKey ::= OCTET STRING

EncryptedPrivateKeyInfo ::= SEQUENCE {
  encryptionAlgorithm  EncryptionAlgorithmIdentifier,
  encryptedData        EncryptedData }

  EncryptionAlgorithmIdentifier ::= AlgorithmIdentifier

  EncryptedData ::= OCTET STRING

*/


/*-----------------------------------------------------------------------------*/

static MSTATUS
PKCS1_ExtractPublicKey( CStream cs, ASN1_ITEMPTR pFirst,
                        ASN1_ITEMPTR pSecond, RSAKey* pRSAKey)
{
    MSTATUS status;
    const ubyte* buffer = 0;

    /* public key */
    if ( OK > ASN1_VerifyType(pFirst, INTEGER) ||
        OK > ASN1_VerifyType( pSecond, INTEGER) ||
        pSecond->length > sizeof (ubyte4) )
    {
        status = ERR_RSA_INVALID_PKCS1;
        goto exit;
    }

    /*Set the public key parameters*/
    buffer = CS_memaccess( cs, pFirst->dataOffset, pFirst->length);
    if (!buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    status = RSA_setPublicKeyParameters(pRSAKey,
                                        pSecond->data.m_intVal,
                                        buffer,
                                        pFirst->length,
                                        NULL);
exit:

    CS_stopaccess( cs, buffer);

    return status;
}


/*-----------------------------------------------------------------------------*/

static MSTATUS
PKCS1_ExtractPrivateKey(MOC_RSA(hwAccelDescr hwAccelCtx) CStream cs,
                        ASN1_ITEMPTR pFirst,
                        ASN1_ITEMPTR pSecond, RSAKey* pRSAKey)
{
    MSTATUS         status;
    ubyte4          version;
    ASN1_ITEMPTR    pExponent;
    ASN1_ITEMPTR    pIgnore;
    ASN1_ITEMPTR    pPrime1;
    ASN1_ITEMPTR    pPrime2;
    const ubyte*    modulusBuffer = 0;
    const ubyte*    prime1Buffer = 0;
    const ubyte*    prime2Buffer = 0;


    /* private key */
    if ( OK > ASN1_VerifyType(pFirst, INTEGER) ||
        OK > ASN1_VerifyType( pSecond, INTEGER) )
    {
        status = ERR_RSA_INVALID_PKCS1;
        goto exit;
    }

    version = pFirst->data.m_intVal;

    if ( 0 != version && 1 != version)
    {
        status = ERR_RSA_INVALID_PKCS1_VERSION;
        goto exit;
    }

    pExponent = ASN1_NEXT_SIBLING( pSecond);
    if ( !pExponent || pExponent->length > sizeof(ubyte4))
    {
        status = ERR_RSA_INVALID_PKCS1;
        goto exit;
    }

    /* get prime1 and prime2 after jumping
        over the private exponent */
    if ( !(pIgnore = ASN1_NEXT_SIBLING( pExponent)) ||
            !(pPrime1 = ASN1_NEXT_SIBLING(pIgnore)) ||
            !(pPrime2 = ASN1_NEXT_SIBLING(pPrime1)))
    {
        status = ERR_RSA_INVALID_PKCS1;
        goto exit;
    }

    modulusBuffer = CS_memaccess( cs, pSecond->dataOffset, pSecond->length);
    if (!modulusBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    prime1Buffer = CS_memaccess( cs, pPrime1->dataOffset, pPrime1->length);
    if (!prime1Buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    prime2Buffer = CS_memaccess( cs, pPrime2->dataOffset, pPrime2->length);
    if (!prime2Buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    status = RSA_setAllKeyParameters(MOC_RSA(hwAccelCtx) pRSAKey,
                            pExponent->data.m_intVal,
                            modulusBuffer,
                            pSecond->length,
                            prime1Buffer,
                            pPrime1->length,
                            prime2Buffer,
                            pPrime2->length, NULL);

exit:

    CS_stopaccess( cs, modulusBuffer);
    CS_stopaccess( cs, prime1Buffer);
    CS_stopaccess( cs, prime2Buffer);

    return status;
}


/*-----------------------------------------------------------------------------*/

static MSTATUS
PKCS_getPKCS1KeyAux(MOC_RSA(hwAccelDescr hwAccelCtx) ASN1_ITEMPTR pSequence,
                    CStream cs, AsymmetricKey* pRSAKey)
{
    MSTATUS         status;
    ASN1_ITEMPTR    pFirst,
                    pSecond;

    /* is it a public key or a private key ?*/
    if (!(pFirst = ASN1_FIRST_CHILD( pSequence)) ||
        !(pSecond = ASN1_NEXT_SIBLING( pFirst)))
    {
        status = ERR_RSA_INVALID_PKCS1;
        goto exit;
    }

    if (OK >( status = CRYPTO_createRSAKey(pRSAKey, NULL)))
        goto exit;

    if ( ASN1_NEXT_SIBLING( pSecond))
    {
        /* private key */
        status = PKCS1_ExtractPrivateKey(MOC_RSA(hwAccelCtx) cs, pFirst, pSecond, pRSAKey->key.pRSA);
    }
    else
    {
#ifdef __ENABLE_MOCANA_OPENSSL_PUBKEY_COMPATIBILITY__
        status = CERT_extractRSAKey(MOC_RSA(hwAccelCtx) pSequence, cs, pRSAKey);
#else
        status = PKCS1_ExtractPublicKey( cs, pFirst, pSecond, pRSAKey->key.pRSA);
#endif
    }

exit:

    if (OK > status)
    {
        CRYPTO_uninitAsymmetricKey( pRSAKey, NULL);
    }

    return status;
}


/*-----------------------------------------------------------------------------*/

extern MSTATUS
PKCS_getPKCS1Key(MOC_HASH(hwAccelDescr hwAccelCtx) const ubyte* pPKCS1DER, ubyte4 pkcs1DERLen,
                 AsymmetricKey* pRSAKey)
{
    CStream         cs;
    MemFile         mf;
    ASN1_ITEMPTR    pRoot = 0;
    ASN1_ITEMPTR    pSequence;
    MSTATUS         status;

    if ( !pPKCS1DER || !pRSAKey)
        return ERR_NULL_POINTER;

    /* parse the DER */
    MF_attach( &mf, pkcs1DERLen, (ubyte*) pPKCS1DER);

    CS_AttachMemFile( &cs, &mf);

    if ( OK > (status = ASN1_Parse(cs, &pRoot)))
        goto exit;

    pSequence = ASN1_FIRST_CHILD( pRoot);

    if (!pSequence || OK > ASN1_VerifyType(pSequence, SEQUENCE))
    {
        status = ERR_RSA_INVALID_PKCS1;
        goto exit;
    }

    status = PKCS_getPKCS1KeyAux(MOC_RSA(hwAccelCtx) pSequence, cs, pRSAKey);

exit:
    if ( pRoot)
    {
        TREE_DeleteTreeItem((TreeItem*) pRoot);
    }

    return status;
}


#ifdef __ENABLE_MOCANA_DSA__
/*-----------------------------------------------------------------------------*/

static MSTATUS
PKCS_getDSAKeyAux(ASN1_ITEMPTR pOID, CStream s, AsymmetricKey* pKey)
{
    MSTATUS         status;
    ASN1_ITEMPTR    pTemp;
    const ubyte     *p = 0,
                    *q = 0,
                    *g = 0,
                    *x = 0;
    ubyte4 pLen, qLen, gLen, xLen;

    static WalkerStep dsaOIDToX[] =
    {
        { GoParent, 0, 0},
        { GoNextSibling, 0, 0},
        { VerifyType, OCTETSTRING, 0},
        { GoFirstChild, 0, 0},
        { VerifyType, INTEGER, 0},
        { Complete, 0, 0}
    };


    pTemp = ASN1_NEXT_SIBLING(pOID); /* SEQUENCE */
    if ( OK > ASN1_VerifyType( pTemp, SEQUENCE))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* p */
    pTemp = ASN1_FIRST_CHILD( pTemp);
    if ( OK > ASN1_VerifyType( pTemp, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    p = CS_memaccess( s, pTemp->dataOffset, pLen = pTemp->length);
    if (!p)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* q */
    pTemp = ASN1_NEXT_SIBLING( pTemp);
    if ( OK > ASN1_VerifyType( pTemp, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    q = CS_memaccess( s, pTemp->dataOffset, qLen = pTemp->length);
    if (!q)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    /* g */
    pTemp = ASN1_NEXT_SIBLING( pTemp);
    if ( OK > ASN1_VerifyType( pTemp, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    g = CS_memaccess( s, pTemp->dataOffset, gLen = pTemp->length);
    if (!g)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* x */
    if (OK >( status = ASN1_WalkTree( pOID, s, dsaOIDToX, &pTemp)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    x = CS_memaccess( s, pTemp->dataOffset, xLen = pTemp->length);
    if (!x)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK >( status = CRYPTO_createDSAKey(pKey, NULL)))
        goto exit;

    if (OK > ( status = CRYPTO_setDSAParameters( pKey, p, pLen,
                                                q, qLen, g, gLen,
                                                NULL, 0, x, xLen, NULL)))
    {
        goto exit;
    }

exit:

    if (p)
    {
        CS_stopaccess( s, p);
    }
    if (q)
    {
        CS_stopaccess( s, q);
    }
    if (g)
    {
        CS_stopaccess( s, g);
    }
    if (x)
    {
        CS_stopaccess( s, x);
    }

    if (OK > status)
    {
        CRYPTO_uninitAsymmetricKey( pKey, NULL);
    }

    return status;
}
#endif


#ifdef __ENABLE_MOCANA_DSA__
/*-----------------------------------------------------------------------------*/

MOC_EXTERN MSTATUS PKCS_getDSAKey( const ubyte* pDSAKeyDer, ubyte4 pDSAKeyDerLen,
                                  AsymmetricKey* pKey)
{
    CStream         s;
    MemFile         mf;
    ASN1_ITEMPTR    pRoot = 0;
    ASN1_ITEMPTR    pTemp;
    MSTATUS         status;
    const ubyte     *p = 0,
                    *q = 0,
                    *g = 0,
                    *x = 0;
    ubyte4 pLen, qLen, gLen, xLen;

    if ( !pDSAKeyDer || !pKey)
        return ERR_NULL_POINTER;

    /* parse the DER */
    MF_attach( &mf, pDSAKeyDerLen, (ubyte*) pDSAKeyDer);

    CS_AttachMemFile( &s, &mf);

    if ( OK > (status = ASN1_Parse(s, &pRoot)))
        goto exit;

    pTemp = ASN1_FIRST_CHILD( pRoot);

    if (OK > ASN1_VerifyType(pTemp, SEQUENCE))
    {
        status = ERR_RSA_INVALID_PKCS1;
        goto exit;
    }

    /* version */
    pTemp = ASN1_FIRST_CHILD( pTemp);
    if (OK > ASN1_VerifyType( pTemp, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }


    /* p */
    pTemp = ASN1_NEXT_SIBLING( pTemp);
    if (OK > ASN1_VerifyType( pTemp, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    p = CS_memaccess( s, pTemp->dataOffset, pLen = pTemp->length);
    if (!p)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* q */
    pTemp = ASN1_NEXT_SIBLING( pTemp);
    if (OK > ASN1_VerifyType( pTemp, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    q = CS_memaccess( s, pTemp->dataOffset, qLen = pTemp->length);
    if (!q)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* g */
    pTemp = ASN1_NEXT_SIBLING( pTemp);
    if (OK > ASN1_VerifyType( pTemp, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    g = CS_memaccess( s, pTemp->dataOffset, gLen = pTemp->length);
    if (!g)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* y -- no read */
    pTemp = ASN1_NEXT_SIBLING(pTemp);
    if (OK > ASN1_VerifyType( pTemp, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* x */
    pTemp = ASN1_NEXT_SIBLING( pTemp);
    if (OK > ASN1_VerifyType( pTemp, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    x = CS_memaccess( s, pTemp->dataOffset, xLen = pTemp->length);
    if (!x)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK >( status = CRYPTO_createDSAKey(pKey, NULL)))
        goto exit;

    if (OK > ( status = DSA_setAllKeyParameters( pKey->key.pDSA, p, pLen, q, qLen,
                                                 g, gLen, x, xLen, NULL)))
    {
        goto exit;
    }

exit:

    if (p)
    {
        CS_stopaccess( s, p);
    }
    if (q)
    {
        CS_stopaccess( s, q);
    }
    if (g)
    {
        CS_stopaccess( s, g);
    }
    if (x)
    {
        CS_stopaccess( s, x);
    }
    if ( pRoot)
    {
        TREE_DeleteTreeItem((TreeItem*) pRoot);
    }

    if (OK > status)
    {
        CRYPTO_uninitAsymmetricKey( pKey, NULL);
    }

    return status;
}
#endif


/*-----------------------------------------------------------------------------*/

static MSTATUS
PKCS_getPKCS8KeyAux(MOC_RSA(hwAccelDescr hwAccelCtx) ASN1_ITEMPTR pPrivateKeyInfo,
                    CStream cs, AsymmetricKey* pKey)
{
    MSTATUS         status;
    ASN1_ITEMPTR    pOID;
    ASN1_ITEMPTR    pPKItem;

    static WalkerStep pcks8WalkToOID[] =
    {
        { GoFirstChild, 0, 0},
        { VerifyType, SEQUENCE, 0},
        { GoFirstChild, 0, 0},          /* version */
        { VerifyInteger, 0, 0},
        { GoNextSibling, 0, 0},         /* AlgorithmIdentifier */
        { VerifyType, SEQUENCE, 0},
        { GoFirstChild, 0, 0},          /* OID */
        { Complete, 0, 0}
    };

    static WalkerStep pkcs8OIDToPrivateKey[] =
    {
        { GoParent, 0, 0},
        { GoNextSibling, 0, 0 },           /* Private Key */
        { VerifyType, OCTETSTRING, 0},
        { GoFirstChild, 0, 0},
        { Complete, 0, 0}
    };

    if ( OK >  ASN1_WalkTree( pPrivateKeyInfo, cs, pcks8WalkToOID,
                                &pOID))
    {
        status = ERR_RSA_INVALID_PKCS8;
        goto exit;
    }

    if ( OK >  ASN1_WalkTree( pOID, cs, pkcs8OIDToPrivateKey,
                                &pPKItem))
    {
        status = ERR_RSA_INVALID_PKCS8;
        goto exit;
    }

    /* look at the pOID */
    if ( OK <= ASN1_VerifyOID( pOID, cs, rsaEncryption_OID))
    {
        if (OK > ASN1_VerifyType( pPKItem, SEQUENCE))
        {
            status = ERR_RSA_INVALID_PKCS8;
            goto exit;
        }
        status = PKCS_getPKCS1KeyAux(MOC_RSA(hwAccelCtx) pPKItem, cs,
                                    pKey);
    }
#ifdef __ENABLE_MOCANA_DSA__
    else if (OK <= ASN1_VerifyOID( pOID, cs, dsa_OID))
    {
        status = PKCS_getDSAKeyAux( pOID, cs, pKey);
    }
#endif
#ifdef __ENABLE_MOCANA_ECC__
    else if ( OK <= ASN1_VerifyOID( pOID, cs, ecPublicKey_OID))
    {
        /* curve is the next OID */
        ASN1_ITEMPTR pCurveOID;
        ubyte curveId = 0;


        pCurveOID = ASN1_NEXT_SIBLING( pOID);
        if (pCurveOID)
        {
            status = ASN1_VerifyOIDRoot( pCurveOID, cs, ansiX962CurvesPrime_OID, &curveId);
            if ( OK > status) /* try another ASN1 arc */
            {
                status = ASN1_VerifyOIDRoot( pCurveOID, cs, certicomCurve_OID, &curveId);
            }

            if (OK > status)
            {
                status = ERR_EC_UNSUPPORTED_CURVE;
                goto exit;
            }
        }

        if (OK > ASN1_VerifyType( pPKItem, SEQUENCE))
        {
            status = ERR_RSA_INVALID_PKCS8;
            goto exit;
        }
        status = SEC_getPrivateKey(pPKItem, cs, curveId, pKey);
    }
#endif
    else
    {
        status = ERR_RSA_UNKNOWN_PKCS8_ALGOID;
        goto exit;
    }

exit:

    return status;

}


/*-----------------------------------------------------------------------------*/

/* Extract key from unencrypted PKCS8 DER file.
This function extracts a key from an unencrypted PKCS8 DER file.

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- __DISABLE_MOCANA_CERTIFICATE_PARSING__

\param hwAccelCtx       For future use.
\param pPKCS8DER        Pointer to buffer containing PKCS8 DER file contents.
\param pkcs8DERLen      Number of bytes in PKCS8 DER file ($pPKCS8DER$).
\param pKey             On return, pointer to Mocana-formatted key.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.
*/
extern MSTATUS
PKCS_getPKCS8Key(MOC_RSA(hwAccelDescr hwAccelCtx) const ubyte* pPKCS8DER, ubyte4 pkcs8DERLen,
                 AsymmetricKey* pKey)
{
    MSTATUS         status;
    CStream         cs;
    MemFile         mf;
    ASN1_ITEMPTR    pRoot = 0;

    if ( !pPKCS8DER || !pKey)
        return ERR_NULL_POINTER;

    /* parse the DER */
    MF_attach( &mf, pkcs8DERLen, (ubyte*) pPKCS8DER);

    CS_AttachMemFile( &cs, &mf);

    if ( OK > (status = ASN1_Parse(cs, &pRoot)))
        goto exit;

    status = PKCS_getPKCS8KeyAux(MOC_RSA(hwAccelCtx) pRoot, cs, pKey);

exit:

    if ( pRoot)
    {
        TREE_DeleteTreeItem((TreeItem*) pRoot);
    }

    return status;
}


/*-----------------------------------------------------------------------------*/

/* Extract key from PKCS8 DER file (encrypted or unencrypted).
This function extracts a key from a PKCS8 DER file (which can be encrypted or unencrypted).

\since 1.41
\version 1.41 and later

! Flags
To enable this function, the following flag must #not# be defined:
- __DISABLE_MOCANA_CERTIFICATE_PARSING__

\param hwAccelCtx       For future use.
\param pPKCS8DER        Pointer to buffer containing PKCS8 DER file contents..
\param pkcs8DERLen      Number of bytes in PKCS8 DER file ($pPKCS8DER$).
\param password         For an encrypted file, pointer to buffer contaning
password; otherwise NULL.
\param passwordLen      For an encrypted file, number of bytes in the password
($password$); otherwise not used.
\param pKey             On return, pointer to Mocana-formatted key.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error status, use the
$DISPLAY_ERROR$ macro.
*/
extern MSTATUS
PKCS_getPKCS8KeyEx(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx) const ubyte* pPKCS8DER,
                   ubyte4 pkcs8DERLen, const ubyte* password, ubyte4 passwordLen,
                   AsymmetricKey* pKey)
{
    MSTATUS         status;
    CStream         cs;
    MemFile         mf;
    ASN1_ITEMPTR    pRoot = 0;
    ASN1_ITEMPTR    pSequence, pFirstChild;
    ubyte*          plainText = 0;
#if defined(__ENABLE_MOCANA_PKCS5__) || defined(__ENABLE_MOCANA_PKCS12__)
    sbyte4          ptLen;
#endif
    if ( !pPKCS8DER || !pKey)
        return ERR_NULL_POINTER;

    /* parse the DER */
    MF_attach( &mf, pkcs8DERLen, (ubyte*) pPKCS8DER);

    CS_AttachMemFile( &cs, &mf);

    if ( OK > (status = ASN1_Parse(cs, &pRoot)))
        goto exit;

    /* look at the type: encrypted or unencrypted */
    pSequence = ASN1_FIRST_CHILD( pRoot);
    if (OK > ASN1_VerifyType( pSequence, SEQUENCE))
    {
        status = ERR_RSA_INVALID_PKCS8;
        goto exit;
    }

    pFirstChild =  ASN1_FIRST_CHILD( pSequence);

    if (OK <= ASN1_VerifyType( pFirstChild, INTEGER))
    {
        /* unencrypted */
        status = PKCS_getPKCS8KeyAux(MOC_RSA(hwAccelCtx) pRoot, cs, pKey);
    }
    else if (OK <= ASN1_VerifyType( pFirstChild, SEQUENCE))
    {
#if defined(__ENABLE_MOCANA_PKCS5__) || defined(__ENABLE_MOCANA_PKCS12__)
        /* encrypted? */
        if (OK > (status = PKCS_DecryptPKCS8Key( MOC_SYM_HASH(hwAccelCtx)
                pSequence, cs, password, passwordLen,
                &plainText, &ptLen)))
        {
            goto exit;
        }

        status = PKCS_getPKCS8Key(MOC_RSA(hwAccelCtx) plainText, ptLen, pKey);

#else
        status = ERR_RSA_BUILT_WITH_NO_PKCS8_DECRYPTION;
#endif

    }
    else
    {
        status = ERR_RSA_INVALID_PKCS8;
    }

exit:

    if (plainText)
    {
        FREE(plainText);
    }

    if ( pRoot)
    {
        TREE_DeleteTreeItem((TreeItem*) pRoot);
    }

    return status;
}

#endif /* __DISABLE_MOCANA_CERTIFICATE_PARSING__ */


/*------------------------------------------------------------------*/

#if defined( __ENABLE_MOCANA_DER_CONVERSION__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__)

static MSTATUS
PKCS_getRSAPrivateExponent(MOC_HASH(hwAccelDescr hwAccelCtx) RSAKey *pRSAKey, vlong **ppRetD, vlong **ppVlongQueue)
{
    MSTATUS status = ERR_MEM_ALLOC_FAIL;
    vlong* po = NULL;
    vlong* qo = NULL;
    vlong* pq = NULL;
    vlong* rem = NULL;
    vlong* gcd = NULL;
    vlong* lamda = NULL;

    if ( (OK > VLONG_allocVlong(&pq, ppVlongQueue)) ||
         (OK > VLONG_allocVlong(&lamda, ppVlongQueue)) ||
         (OK > VLONG_allocVlong(&rem, ppVlongQueue)) )
    {
        goto exit;
    }

    /* po = p - 1; */
    if (OK > (status = VLONG_makeVlongFromVlong(RSA_P(pRSAKey), &po,
                                                ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_decrement(po, ppVlongQueue)))
        goto exit;

    /* qo = q - 1; */
    if (OK > (status = VLONG_makeVlongFromVlong(RSA_Q(pRSAKey), &qo,
                                                ppVlongQueue)))
    {
        goto exit;
    }

    if (OK > (status = VLONG_decrement(qo, ppVlongQueue)))
        goto exit;

    /* pq = (p - 1)(q - 1) */
    if (OK > (status = VLONG_unsignedMultiply(pq, po, qo)))
        goto exit;

#if 1
    /* gcd = GCD(po, qo) */
    if (OK > (status = VLONG_greatestCommonDenominator(MOC_MOD(hwAccelCtx) po, qo, &gcd, ppVlongQueue)))
        goto exit;

    /* lamda = pq/gcd */
    if (OK > (status = VLONG_unsignedDivide(lamda, pq, gcd, rem, ppVlongQueue)))
        goto exit;

    /* check if rem is 0 */
    if (!VLONG_isVlongZero(rem))
    {
        status = ERR_INTERNAL_ERROR;
        goto exit;
    }

    /* lamda = LCM(p-1, q-1) = (p-1)*(q-1)/GCD(p-1, q-1) */
    status = VLONG_modularInverse(MOC_MOD(hwAccelCtx) RSA_E(pRSAKey), lamda, ppRetD, ppVlongQueue);
#else

    status = VLONG_modularInverse( MOC_MOD(hwAccelCtx) RSA_E(pRSAKey), pq, ppRetD, ppVlongQueue);

#endif

exit:
    VLONG_freeVlong(&po, ppVlongQueue);
    VLONG_freeVlong(&qo, ppVlongQueue);
    VLONG_freeVlong(&pq, ppVlongQueue);
    VLONG_freeVlong(&rem, ppVlongQueue);
    VLONG_freeVlong(&gcd, ppVlongQueue);
    VLONG_freeVlong(&lamda, ppVlongQueue);

    return status;
}


/*------------------------------------------------------------------*/

/*
   PKCS1 standard for RSA private key:
    --
    -- Representation of RSA private key with information for the CRT algorithm.
    --
    RSAPrivateKey ::= SEQUENCE {
        version           Version,
        modulus           INTEGER,  -- n
        publicExponent    INTEGER,  -- e
        privateExponent   INTEGER,  -- d
        prime1            INTEGER,  -- p
        prime2            INTEGER,  -- q
        exponent1         INTEGER,  -- d mod (p-1)
        exponent2         INTEGER,  -- d mod (q-1)
        coefficient       INTEGER,  -- (inverse of q) mod p
        otherPrimeInfos   OtherPrimeInfos OPTIONAL
    }

    Version ::= INTEGER { two-prime(0), multi(1) }
        (CONSTRAINED BY {-- version must be multi if otherPrimeInfos present --})

    OtherPrimeInfos ::= SEQUENCE SIZE(1..MAX) OF OtherPrimeInfo


    OtherPrimeInfo ::= SEQUENCE {
        prime             INTEGER,  -- ri
        exponent          INTEGER,  -- di
        coefficient       INTEGER   -- ti
    }
*/

extern MSTATUS
PKCS_setPKCS1Key(MOC_RSA(hwAccelDescr hwAccelCtx) AsymmetricKey* pKey, ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    MSTATUS       status = OK;
    DER_ITEMPTR   pSequence = NULL;
    RSAKey*       pRSAKey = NULL;
    ubyte4        i, tmpLen;
    ubyte4        numLeadZero;
    ubyte*        pBufStart = NULL;
    ubyte*        pTmpBuf = NULL;
    ubyte4        tmpBufLen;
    ubyte4        keyParamLen[NUM_RSA_VLONG + 1];
    vlong*        d = NULL;
    vlong*        v[NUM_RSA_VLONG + 1] = {0}; /* extra 1 for D */

    if (!pKey || !ppRetKeyDER || !pRetKeyDERLength)
        return ERR_NULL_POINTER;

    if (akt_rsa != pKey->type)
        return ERR_BAD_KEY_TYPE;

    pRSAKey = pKey->key.pRSA;

    /* create a tmp buffer large enough to hold all components of a key */
    /* need space for leading zeros, see comment for DER_AddInteger in derencoder.h */
    numLeadZero = 2;

    /* organize key components in specified order */
    v[0] = RSA_N(pRSAKey);
    v[1] = RSA_E(pRSAKey);

    if (pRSAKey->keyType == RsaPrivateKey)
    {
        if (OK > PKCS_getRSAPrivateExponent(MOC_RSA(hwAccelCtx) pRSAKey, &d, NULL))
            goto exit;

        v[2] = d;
        v[3] = RSA_P(pRSAKey);
        v[4] = RSA_Q(pRSAKey);
        v[5] = RSA_DP(pRSAKey);
        v[6] = RSA_DQ(pRSAKey);
        v[7] = RSA_QINV(pRSAKey);

        /* add extra 1 for leading zero in version */
        numLeadZero = numLeadZero + 6 + 1;
    }

    tmpBufLen = numLeadZero;

    for (i=0; (i < NUM_RSA_VLONG + 1) && (NULL != v[i]); i++)
    {
        if (OK > (status = VLONG_byteStringFromVlong(v[i], NULL, (sbyte4 *)&tmpLen)))
            goto exit;

        keyParamLen[i] = tmpLen;
        tmpBufLen = tmpBufLen + tmpLen;
    }

    if (NULL == (pBufStart = (ubyte *)MALLOC(tmpBufLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    pTmpBuf = pBufStart;
    MOC_MEMSET(pTmpBuf, 0x00, tmpBufLen);

    /* create an empty sequence */
    if (OK > (status = DER_AddSequence(NULL, &pSequence)))
        goto exit;

    /* only need version if this is a private key */
    if (pRSAKey->keyType == RsaPrivateKey)
    {
        /* version is 0 */
        if (OK > (status = DER_AddInteger(pSequence, 1, pTmpBuf, NULL)))
            goto exit;

        pTmpBuf = pTmpBuf + 1;
    }

    for (i=0; (i < NUM_RSA_VLONG + 1) && (NULL != v[i]); i++)
    {
        tmpLen = keyParamLen[i];

        if (OK > (status = VLONG_byteStringFromVlong(v[i], pTmpBuf + 1, (sbyte4 *)&tmpLen)))
            goto exit;

        if (OK > (status = DER_AddInteger(pSequence, tmpLen + 1, pTmpBuf, NULL)))
            goto exit;

        pTmpBuf = pTmpBuf + tmpLen + 1;
    }

    status = DER_Serialize(pSequence, ppRetKeyDER, pRetKeyDERLength);

exit:
    VLONG_freeVlong(&d, NULL);

    if (NULL != pBufStart)
        FREE(pBufStart);

    if (NULL != pSequence)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSequence);
    }

    return status;
}


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_DSA__
extern MSTATUS
PKCS_setDsaDerKey( AsymmetricKey* pKey, ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    MSTATUS       status = OK;
    DER_ITEMPTR   pSequence = NULL;
    DSAKey*       pDSAKey = NULL;
    ubyte4        i, tmpLen;
    ubyte4        numLeadZero;
    ubyte*        pBufStart = NULL;
    ubyte*        pTmpBuf = NULL;
    ubyte4        tmpBufLen;
    ubyte4        keyParamLen[NUM_DSA_VLONG];

    if (!pKey || !ppRetKeyDER || !pRetKeyDERLength)
        return ERR_NULL_POINTER;

    if (akt_dsa != pKey->type)
        return ERR_BAD_KEY_TYPE;

    pDSAKey = pKey->key.pDSA;

    /* create a tmp buffer large enough to hold all components of a key */
    /* need space for leading zeros, see comment for DER_AddInteger in derencoder.h */
    /* add extra 1 for leading zero in version */
    numLeadZero = NUM_DSA_VLONG + 1;

    tmpBufLen = numLeadZero;

    for (i=0; (i < NUM_DSA_VLONG) && (NULL != pDSAKey->dsaVlong[i]); i++)
    {
        if (OK > (status = VLONG_byteStringFromVlong(pDSAKey->dsaVlong[i], NULL, (sbyte4*)&tmpLen)))
            goto exit;

        keyParamLen[i] = tmpLen;
        tmpBufLen = tmpBufLen + tmpLen;
    }

    if (NULL == (pBufStart = (ubyte *)MALLOC(tmpBufLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    pTmpBuf = pBufStart;
    MOC_MEMSET(pTmpBuf, 0x00, tmpBufLen);

    /* create an empty sequence */
    if (OK > (status = DER_AddSequence(NULL, &pSequence)))
        goto exit;

    /* version is 0 */
    if (OK > (status = DER_AddInteger(pSequence, 1, pTmpBuf, NULL)))
        goto exit;

    pTmpBuf = pTmpBuf + 1;

    for (i=0; (i < NUM_DSA_VLONG) && (NULL != pDSAKey->dsaVlong[i]); i++)
    {
        tmpLen = keyParamLen[i];

        if (OK > (status = VLONG_byteStringFromVlong(pDSAKey->dsaVlong[i], pTmpBuf + 1, (sbyte4*)&tmpLen)))
            goto exit;

        if (OK > (status = DER_AddInteger(pSequence, tmpLen + 1, pTmpBuf, NULL)))
            goto exit;

        pTmpBuf = pTmpBuf + tmpLen + 1;
    }

    status = DER_Serialize(pSequence, ppRetKeyDER, pRetKeyDERLength);

exit:
    if (NULL != pBufStart)
        FREE(pBufStart);

    if (NULL != pSequence)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSequence);
    }

    return status;
}
#endif /* __ENABLE_MOCANA_DSA__ */

#endif /* defined( __ENABLE_MOCANA_DER_CONVERSION__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__) */


/*------------------------------------------------------------------*/

#if defined( __ENABLE_MOCANA_DER_CONVERSION__)

static MSTATUS
PKCS_makePrivateKeyInfo(MOC_RSA(hwAccelDescr hwAccelCtx)
                        AsymmetricKey* pKey, ubyte4 paddingTo,
                        ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    MSTATUS status;
    DER_ITEMPTR   pSequence = 0;
    ubyte* pPKBuffer = 0;
    ubyte* serializeBuffer = 0;
    ubyte4 pkLen;
    ubyte leadZero = 0;

    /* create sequence */
    if (OK > (status = DER_AddSequence(NULL, &pSequence)))
        goto exit;

    if (OK > ( status = DER_AddInteger( pSequence, 1, &leadZero, NULL)))
        goto exit;

    if ( akt_rsa == pKey->type)
    {
        if (OK > (status = DER_StoreAlgoOID( pSequence, rsaEncryption_OID, 1)))
            goto exit;

        /* generate the pkcs1 buffer */
        if (OK > (status = PKCS_setPKCS1Key( MOC_RSA(hwAccelCtx) pKey,
                            &pPKBuffer, &pkLen)))
        {
            goto exit;
        }
    }
#ifdef __ENABLE_MOCANA_ECC__
    else if ( akt_ecc == pKey->type)
    {
        const ubyte* curveOID;
        DER_ITEMPTR pAlgoSeq;

        if (!pKey->key.pECC->privateKey)
        {
            status = ERR_EC_PUBLIC_KEY;
            goto exit;
        }

        if (OK > ( status = DER_AddSequence( pSequence, &pAlgoSeq)))
            goto exit;

        if (OK > ( status = DER_AddOID( pAlgoSeq, ecPublicKey_OID, 0)))
            goto exit;

        /* add oid for the curve */
        if (OK > ( status = CRYPTO_getECCurveOID( pKey->key.pECC, &curveOID)))
            goto exit;

        if (OK > ( status = DER_AddOID( pAlgoSeq, curveOID, 0)))
            goto exit;

        /* generate the SEC buffer */
        if (OK > (status = SEC_setKeyEx( pKey, E_SEC_omitCurveOID, &pPKBuffer, &pkLen)))
        {
            goto exit;
        }

    }
#endif
    else
    {
        status = ERR_BAD_KEY_TYPE;
        goto exit;
    }

    /* add it embedded as an OCTETSTRING */
    if (OK > ( status = DER_AddItem( pSequence, OCTETSTRING, pkLen, pPKBuffer, NULL)))
    {
        goto exit;
    }

    if (paddingTo) /* need to pad to a multiple of paddingTo -> encryption */
    {
        ubyte4 origLen, padding;

        if (OK > ( status = DER_GetLength( pSequence, &origLen)))
            goto exit;

        padding = paddingTo - (origLen % paddingTo);

        origLen += padding;
        *pRetKeyDERLength = origLen; /* return the total allocated length */
        serializeBuffer = MALLOC( origLen);
        if (!serializeBuffer)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        if ( OK > ( status = DER_SerializeInto( pSequence, serializeBuffer, &origLen)))
            goto exit;

        /* pad the rest -- origLen was set to the used value by DER_SerializeInto */
        MOC_MEMSET( serializeBuffer + origLen, (ubyte) padding, (sbyte4) padding);

        *ppRetKeyDER = serializeBuffer;
        serializeBuffer = 0;

    }
    else
    {
        /* serialize now */
        if (OK > ( status = DER_Serialize( pSequence, ppRetKeyDER, pRetKeyDERLength)))
            goto exit;
    }

exit:

    if (serializeBuffer)
    {
        FREE( serializeBuffer);
    }

    if (pPKBuffer)
    {
        FREE( pPKBuffer);
    }

    if (pSequence)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSequence);
    }

    return status;
}


#ifdef __ENABLE_MOCANA_PKCS5__
/*------------------------------------------------------------------*/

static MSTATUS
PKCS_makePKCS5V1PKInfo(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                 AsymmetricKey* pKey,
                                 randomContext* pRandomContext,
                                 enum PKCS8EncryptionType encType,
                                 const ubyte* password, ubyte4 passwordLen,
                                 ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    MSTATUS status;
    DER_ITEMPTR   pSequence = 0;
    DER_ITEMPTR   pAlgoSequence, pInitSequence;
    ubyte*        pPrivateKeyInfo = 0;
    ubyte4        privateKeyInfoLen;
    ubyte         salt[8];
    ubyte         pkcs5_algo_oid[10]=
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x05, 0x00 };


    /* create sequence */
    if (OK > ( status = DER_AddSequence(NULL, &pSequence)))
        goto exit;

    /* algo */
    if (OK > ( status = DER_AddSequence(pSequence, &pAlgoSequence)))
        goto exit;

    /* algo oid */
    pkcs5_algo_oid[9] = encType; /* the enum PKCS8EncryptionType values
                                    are set up to allow this */

    if (OK > ( status = DER_AddOID( pAlgoSequence, pkcs5_algo_oid, NULL)))
        goto exit;

    /* init */
    if (OK > ( status = DER_AddSequence(pAlgoSequence, &pInitSequence)))
        goto exit;

    /* generate salt */
    if (OK > (status = RANDOM_numberGenerator( pRandomContext, salt, 8)))
        goto exit;

    if (OK > ( status = DER_AddItem( pInitSequence, OCTETSTRING, 8, salt, NULL)))
        goto exit;

    /* count = 2048 */
    if (OK > ( status = DER_AddIntegerEx( pInitSequence, 2048, NULL)))
        goto exit;

    /* get the private key info in a buffer with padding: DES or RC2 use a 8 byte pad */
    if (OK > ( status = PKCS_makePrivateKeyInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                                    pKey, 8, &pPrivateKeyInfo,
                                                    &privateKeyInfoLen)))
    {
        goto exit;
    }

    /* encrypt using PKCS5 v1 */
    if (OK > ( status = PKCS5_encryptV1( MOC_SYM_HASH( hwAccelCtx)
                                            encType, password, passwordLen,
                                            salt, 8, 2048,
                                            pPrivateKeyInfo, privateKeyInfoLen)))
    {
        goto exit;
    }

    /* add it as OCTETSTRING */
    if (OK > ( status = DER_AddItem(pSequence, OCTETSTRING, privateKeyInfoLen,
                                    pPrivateKeyInfo, NULL)))
    {
        goto exit;
    }

    /* serialize now */
    if (OK > ( status = DER_Serialize( pSequence, ppRetKeyDER, pRetKeyDERLength)))
        goto exit;

exit:

    if (pPrivateKeyInfo)
    {
        FREE( pPrivateKeyInfo);
    }

    if (pSequence)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSequence);
    }

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
PKCS_makePKCS5V2PKInfo(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                 AsymmetricKey* pKey,
                                 randomContext* pRandomContext,
                                 enum PKCS8EncryptionType encType,
                                 const ubyte* password, ubyte4 passwordLen,
                                 ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    MSTATUS status;
    DER_ITEMPTR   pSequence = 0;
    DER_ITEMPTR   pAlgoSequence, pPBESequence, pKDFSequence;
    DER_ITEMPTR   pInitSequence, pEncryptionSequence;
    ubyte*        pPrivateKeyInfo = 0;
    ubyte4        privateKeyInfoLen;
    ubyte         random[16];
    const         ubyte* algoOID = 0;
    const BulkEncryptionAlgo *pAlgo;
    ubyte4        keyLength;
    ubyte4        effectiveKeyBits = -1;

    /* create sequence */
    if (OK > ( status = DER_AddSequence(NULL, &pSequence)))
        goto exit;

    /* algo */
    if (OK > ( status = DER_AddSequence(pSequence, &pAlgoSequence)))
        goto exit;

    /* PBES2 OID */
    if (OK > ( status = DER_AddOID(pAlgoSequence, pkcs5_PBES2_OID, NULL)))
        goto exit;

    if (OK > ( status = DER_AddSequence(pAlgoSequence, &pPBESequence)))
        goto exit;

    /* Key Derivation Function */
    if (OK > ( status = DER_AddSequence(pPBESequence, &pKDFSequence)))
        goto exit;

    /* PKDEF2 OID */
    if (OK > ( status = DER_AddOID(pKDFSequence, pkcs5_PBKDF2_OID, NULL)))
        goto exit;

    /* Init Sequence */
    if (OK > ( status = DER_AddSequence(pKDFSequence, &pInitSequence)))
        goto exit;

    /* generate salt and IV */
    if (OK > (status = RANDOM_numberGenerator( pRandomContext, random, 16)))
        goto exit;

    if (OK > ( status = DER_AddItem( pInitSequence, OCTETSTRING, 8, random, NULL)))
        goto exit;

    if (OK > ( status = DER_AddIntegerEx( pInitSequence, 2048, NULL)))
        goto exit;


#ifdef __ENABLE_ARC2_CIPHERS__
    if (PCKS8_EncryptionType_pkcs5_v2_rc2 == encType)
    {
        /* add key size if RC2 */
        if (OK > ( status = DER_AddIntegerEx( pInitSequence, 16, NULL)))
            goto exit;
    }
#endif

    /* Encryption Scheme */
    if (OK > ( status = DER_AddSequence(pPBESequence, &pEncryptionSequence)))
        goto exit;

    switch (encType)
    {
#ifdef __ENABLE_ARC2_CIPHERS__
    case PCKS8_EncryptionType_pkcs5_v2_rc2:
        algoOID = rc2CBC_OID;
        pAlgo = &CRYPTO_RC2EffectiveBitsSuite;
        keyLength = 16;
        effectiveKeyBits = 128;
        break;
#endif

#ifdef __ENABLE_DES_CIPHER__
    case PCKS8_EncryptionType_pkcs5_v2_des:
        algoOID = desCBC_OID;
        pAlgo = &CRYPTO_DESSuite;
        keyLength = 8;
        break;
#endif

#ifndef __DISABLE_3DES_CIPHERS__
    case PCKS8_EncryptionType_pkcs5_v2_3des:
        algoOID = desEDE3CBC_OID;
        pAlgo = &CRYPTO_TripleDESSuite;
        keyLength = 24;
        break;
#endif

    default:
        status = ERR_INTERNAL_ERROR;
        goto exit;
        break;
    }

    /* encryption algo OID */
    if (OK > ( status = DER_AddOID(pEncryptionSequence, algoOID, NULL)))
        goto exit;

#ifdef __ENABLE_ARC2_CIPHERS__
    /* rc2 special case */
    if (PCKS8_EncryptionType_pkcs5_v2_rc2 == encType)
    {
        DER_ITEMPTR pRC2Params;

        if (OK > ( status = DER_AddSequence(pEncryptionSequence,
                                                &pRC2Params)))
        {
            goto exit;
        }

        /* add effective bits if RC2  --weird : 128 is encoded as 58 */
        if (OK > ( status = DER_AddIntegerEx( pRC2Params, 58, NULL)))
        {
            goto exit;
        }
        if (OK > ( status = DER_AddItem(pRC2Params, OCTETSTRING,
                                         8, random + 8, NULL)))
        {
            goto exit;
        }
    }
    else
#endif
    {
        if (OK > ( status = DER_AddItem(pEncryptionSequence, OCTETSTRING,
                                        8, random + 8, NULL)))
        {
            goto exit;
        }
    }

    /* get the private key info in a buffer with padding: DES, 3DES or RC2 use a 8 byte pad */
    if (OK > ( status = PKCS_makePrivateKeyInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                                    pKey, 8, &pPrivateKeyInfo,
                                                    &privateKeyInfoLen)))
    {
        goto exit;
    }

    /* encrypt using PKCS5 v1 */
    if (OK > ( status = PKCS5_encryptV2( MOC_SYM_HASH( hwAccelCtx)
                                            pAlgo, keyLength, effectiveKeyBits,
                                            password, passwordLen,
                                            random, 8, 2048, random+8,
                                            pPrivateKeyInfo, privateKeyInfoLen)))
    {
        goto exit;
    }

    /* add it as OCTETSTRING */
    if (OK > ( status = DER_AddItem(pSequence, OCTETSTRING, privateKeyInfoLen,
                                    pPrivateKeyInfo, NULL)))
    {
        goto exit;
    }

    /* serialize now */
    if (OK > ( status = DER_Serialize( pSequence, ppRetKeyDER, pRetKeyDERLength)))
        goto exit;


exit:

    if (pPrivateKeyInfo)
    {
        FREE( pPrivateKeyInfo);
    }

    if (pSequence)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSequence);
    }

    return status;

}
#endif /* __ENABLE_MOCANA_PKCS5__ */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_PKCS12__) && (!defined(__DISABLE_3DES_CIPHERS__) || defined(__ENABLE_ARC2_CIPHERS__) || (!defined(__DISABLE_ARC4_CIPHERS__))))

static MSTATUS
PKCS_makePKCS12PKInfo(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                 AsymmetricKey* pKey,
                                 randomContext* pRandomContext,
                                 enum PKCS8EncryptionType encType,
                                 const ubyte* password, ubyte4 passwordLen,
                                 ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    MSTATUS status;
    DER_ITEMPTR   pSequence = 0;
    DER_ITEMPTR   pAlgoSequence, pInitSequence;
    ubyte*        pPrivateKeyInfo = 0;
    ubyte4        privateKeyInfoLen;
    //    ubyte4        paddingTo;
    ubyte         salt[8];
    ubyte         count[4];
    const         BulkEncryptionAlgo* pBulkAlgo;
    ubyte         pkcs12_algo_oid[11] =
    { 10, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x0C, 0x01, 0x00 };


    encType -= PKCS8_EncryptionType_pkcs12;
    pBulkAlgo = PKCS12_GetEncryptionAlgo( encType);
    if (!pBulkAlgo)
    {
        return ERR_INTERNAL_ERROR;
    }

    //paddingTo = pBulkAlgo->blockSize;

    /* create sequence */
    if (OK > ( status = DER_AddSequence(NULL, &pSequence)))
        goto exit;

    /* algo */
    if (OK > ( status = DER_AddSequence(pSequence, &pAlgoSequence)))
        goto exit;

    pkcs12_algo_oid[10] = (ubyte) encType;

    if (OK > ( status = DER_AddOID( pAlgoSequence, pkcs12_algo_oid, NULL)))
        goto exit;

    /* init */
    if (OK > ( status = DER_AddSequence(pAlgoSequence, &pInitSequence)))
        goto exit;

    /* generate salt */
    if (OK > (status = RANDOM_numberGenerator( pRandomContext, salt, 8)))
        goto exit;

    if (OK > ( status = DER_AddItem( pInitSequence, OCTETSTRING, 8, salt, NULL)))
        goto exit;

    /* count = 2048 */
    BIGEND32(count, 2048);
    if (OK > ( status = DER_AddInteger( pInitSequence, 4, count, NULL)))
        goto exit;


    /* get the private key info in a buffer with padding: DES or RC2 uses a 8 byte pad */
    if (OK > ( status = PKCS_makePrivateKeyInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                                    pKey, 8, &pPrivateKeyInfo,
                                                    &privateKeyInfoLen)))
    {
        goto exit;
    }

    /* encrypt using PKCS5 v1 */
    if (OK > ( status = PKCS12_encrypt( MOC_SYM_HASH( hwAccelCtx)
                                        encType, password, passwordLen,
                                        salt, 8, 2048,
                                        pPrivateKeyInfo, privateKeyInfoLen)))
    {
        goto exit;
    }

    /* add it as OCTETSTRING */
    if (OK > ( status = DER_AddItem(pSequence, OCTETSTRING, privateKeyInfoLen,
                                    pPrivateKeyInfo, NULL)))
    {
        goto exit;
    }

    /* serialize now */
    if (OK > ( status = DER_Serialize( pSequence, ppRetKeyDER, pRetKeyDERLength)))
        goto exit;


exit:

    if (pPrivateKeyInfo)
    {
        FREE( pPrivateKeyInfo);
    }

    if (pSequence)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSequence);
    }

    return status;
}

#endif /* (defined(__ENABLE_MOCANA_PKCS12__) && (!defined(__DISABLE_3DES_CIPHERS__) || defined(__ENABLE_ARC2_CIPHERS__) || (!defined(__DISABLE_ARC4_CIPHERS__)))) */

/*------------------------------------------------------------------*/

static MSTATUS
PKCS_makeEncryptionPrivateKeyInfo(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                 AsymmetricKey* pKey,
                                 randomContext* pRandomContext,
                                 enum PKCS8EncryptionType encType,
                                 const ubyte* password, ubyte4 passwordLen,
                                 ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    if (!pRandomContext)
    {
        return ERR_NULL_POINTER;
    }

    switch (encType)
    {
#if defined(__ENABLE_MOCANA_PKCS5__)
#if defined(__ENABLE_DES_CIPHER__)
        case PCKS8_EncryptionType_pkcs5_v1_sha1_des:
            return PKCS_makePKCS5V1PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if defined(__ENABLE_ARC2_CIPHERS__)
        case PCKS8_EncryptionType_pkcs5_v1_sha1_rc2:
            return PKCS_makePKCS5V1PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if defined(__ENABLE_DES_CIPHER__) && defined(__ENABLE_MOCANA_MD2__)
        case PCKS8_EncryptionType_pkcs5_v1_md2_des:
            return PKCS_makePKCS5V1PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if defined(__ENABLE_ARC2_CIPHERS__) && defined(__ENABLE_MOCANA_MD2__)
        case PCKS8_EncryptionType_pkcs5_v1_md2_rc2:
            return PKCS_makePKCS5V1PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if defined(__ENABLE_DES_CIPHER__)
        case PCKS8_EncryptionType_pkcs5_v1_md5_des:
            return PKCS_makePKCS5V1PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if defined(__ENABLE_ARC2_CIPHERS__)
        case PCKS8_EncryptionType_pkcs5_v1_md5_rc2:
            return PKCS_makePKCS5V1PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if !defined(__DISABLE_3DES_CIPHERS__)
        case PCKS8_EncryptionType_pkcs5_v2_3des:
            return PKCS_makePKCS5V2PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if defined(__ENABLE_DES_CIPHER__)
        case PCKS8_EncryptionType_pkcs5_v2_des:
            return PKCS_makePKCS5V2PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if defined(__ENABLE_ARC2_CIPHERS__)
        case PCKS8_EncryptionType_pkcs5_v2_rc2:
            return PKCS_makePKCS5V2PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#endif /*  __ENABLE_MOCANA_PKCS5__  */

#if defined(__ENABLE_MOCANA_PKCS12__)
#if !defined(__DISABLE_3DES_CIPHERS__)
        case PCKS8_EncryptionType_pkcs12_sha_2des:
        case PCKS8_EncryptionType_pkcs12_sha_3des:
            return PKCS_makePKCS12PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if defined(__ENABLE_ARC2_CIPHERS__)
        case PCKS8_EncryptionType_pkcs12_sha_rc2_40:
        case PCKS8_EncryptionType_pkcs12_sha_rc2_128:
            return PKCS_makePKCS12PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif

#if !defined(__DISABLE_ARC4_CIPHERS__)
        case PCKS8_EncryptionType_pkcs12_sha_rc4_40:
        case PCKS8_EncryptionType_pkcs12_sha_rc4_128:
            return PKCS_makePKCS12PKInfo( MOC_RSA_SYM_HASH(hwAccelCtx)
                                            pKey, pRandomContext,
                                            encType, password, passwordLen,
                                            ppRetKeyDER, pRetKeyDERLength);
#endif
#endif /* __ENABLE_MOCANA_PKCS12__ */

        default:
            return ERR_RSA_UNSUPPORTED_PKCS8_ALGO;
    }
}


/*------------------------------------------------------------------*/

MOC_EXTERN MSTATUS PKCS_setPKCS8Key(MOC_RSA_SYM_HASH(hwAccelDescr hwAccelCtx)
                                    AsymmetricKey* pKey,
                                    randomContext* pRandomContext,
                                    enum PKCS8EncryptionType encType,
                                    const ubyte* password, ubyte4 passwordLen,
                                    ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    if (!pKey || !ppRetKeyDER || !pRetKeyDERLength)
        return ERR_NULL_POINTER;

#ifdef __ENABLE_MOCANA_ECC__
    if (akt_rsa != pKey->type && akt_ecc != pKey->type)
#else
    if (akt_rsa != pKey->type)
#endif
        return ERR_BAD_KEY_TYPE;

    if (!password || !passwordLen)
    {
         return PKCS_makePrivateKeyInfo(MOC_RSA(hwAccelCtx)
                            pKey, 0, ppRetKeyDER, pRetKeyDERLength);
    }
    else /* encrypted key */
    {
        return PKCS_makeEncryptionPrivateKeyInfo(MOC_RSA_SYM_HASH(hwAccelCtx)
                                                pKey, pRandomContext,
                                                encType, password,
                                                passwordLen, ppRetKeyDER,
                                                pRetKeyDERLength);
    }
}


#endif /* defined( __ENABLE_MOCANA_DER_CONVERSION__) */

