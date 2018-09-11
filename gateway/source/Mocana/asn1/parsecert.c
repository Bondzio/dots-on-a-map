/* Version: mss_v6_3 */
/*
 * parsecert.c
 *
 * X.509v3 Certificate Parser
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */


/*------------------------------------------------------------------*/

#include "../common/moptions.h"

#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__

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
#include "../common/debug_console.h"
#include "../crypto/crypto.h"
#include "../crypto/rsa.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/dsa.h"
#include "../crypto/dsa2.h"
#include "../crypto/pubcrypto.h"
#include "../crypto/md5.h"
#include "../crypto/md2.h"
#include "../crypto/md4.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#include "../crypto/ca_mgmt.h"
#include "../harness/harness.h"
#include "../asn1/oiddefs.h"
#include "../asn1/parseasn1.h"
#include "../asn1/parsecert.h"
#include "../asn1/ASN1TreeWalker.h"


/*------------------------------------------------------------------*/

#ifndef MAX_DNE_STRING_LENGTH
#define MAX_DNE_STRING_LENGTH       (128)
#endif

#ifndef MOCANA_MAX_MODULUS_SIZE
#define MOCANA_MAX_MODULUS_SIZE     (512)
#endif

#define kCommonName                 (0x03)
#define kSerialNumber				(0x05)
#define kCountryName                (0x06)
#define kStateOrProvidenceName      (0x08)
#define kLocality                   (0x07)
#define kOrganizationName           (0x0a)
#define kOrganizationUnitName       (0x0b)

/* routines to navigate to specific parts of the certificate */
static MSTATUS GetCertificatePart( ASN1_ITEMPTR rootItem, int startIsRoot, ASN1_ITEMPTR* ppCertificate);
static MSTATUS GetTimeElementValue( const ubyte* buffer, ubyte* value, ubyte min, ubyte max);
static MSTATUS GetCertOID( ASN1_ITEMPTR pAlgoId, CStream s, const ubyte* whichOID,
                          ubyte* whichOIDSubType, ASN1_ITEMPTR *ppOID);
extern MSTATUS CERT_getCertExtension( ASN1_ITEMPTR pExtensionsSeq, CStream s,
                            const ubyte* whichOID, intBoolean* critical,
                            ASN1_ITEMPTR* ppExtension);
static MSTATUS CERT_checkForUnknownCriticalExtensions(ASN1_ITEMPTR pExtensionsSeq, CStream s);
static MSTATUS CERT_getCertificateExtensionsAux(ASN1_ITEM* startItem, int startIsRoot,
                                ASN1_ITEM** ppExtensions);


/*------------------------------------------------------------------*/

static MSTATUS
GetCertificatePart( ASN1_ITEM* startItem, int startIsRoot, ASN1_ITEM** ppCertificate)
{
    ASN1_ITEM*  pItem;
    MSTATUS     status = ERR_CERT_INVALID_STRUCT;

    if ((0 == startItem) || (0 == ppCertificate))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if ( startIsRoot)
    {
        /* go to signed struct */
        pItem = ASN1_FIRST_CHILD(startItem);
        if ( NULL == pItem)
            goto exit;
    }
    else
    {
        pItem = startItem;
    }
    /* otherwise already there */

    /* go to certificate object */
    pItem = ASN1_FIRST_CHILD(pItem);
    if ( NULL == pItem)
        goto exit;

    *ppCertificate = pItem;
    status = OK;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertExtension(ASN1_ITEM* pExtensionsSeq, CStream s, const ubyte* whichOID,
                 intBoolean* critical, ASN1_ITEM** ppExtension)
{
    ASN1_ITEM*  pOID;
    MSTATUS     status;

    if ((NULL == pExtensionsSeq) || (NULL == whichOID) ||
    (NULL == critical) || (NULL == ppExtension))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *critical = 0;
    *ppExtension = 0;

    status = ASN1_GetChildWithOID( pExtensionsSeq, s, whichOID, &pOID);
    if (OK > status )
        goto exit;

    if (pOID)
    {
        /* Extension ::= SEQUENCE {
                extnId     EXTENSION.&id({ExtensionSet}),
                critical   BOOLEAN DEFAULT FALSE,
                extnValue  OCTET STRING }  */
        ASN1_ITEM* pSibling = ASN1_NEXT_SIBLING( pOID);

        status = ERR_CERT_INVALID_STRUCT;

        if (NULL == pSibling || UNIVERSAL != (pSibling->id & CLASS_MASK ))
            goto exit;

        if ( BOOLEAN == pSibling->tag)
        {
            *critical = pSibling->data.m_boolVal;
            pSibling = ASN1_NEXT_SIBLING(pSibling);
            if ( NULL == pSibling || UNIVERSAL != (pSibling->id & CLASS_MASK ))
                goto exit;
        }

        if (OCTETSTRING != pSibling->tag || !pSibling->encapsulates)
            goto exit;

        *ppExtension = ASN1_FIRST_CHILD(pSibling);

        if ( 0 == *ppExtension)
            goto exit;
    }

    status = OK;

exit:
    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
GetTimeElementValue( const ubyte* buffer, ubyte* value, ubyte min, ubyte max)
{
    sbyte4     i;
    ubyte2  temp = 0;
    MSTATUS status = ERR_CERT_INVALID_STRUCT;

    *value = 0;
    for (i = 0; i < 2; ++i)
    {
        if ((buffer[i] < '0') || (buffer[i] > '9'))
            goto exit;

        temp *= 10;
        temp = (ubyte2)(temp + (buffer[i] - '0'));
    }

    if (temp < (ubyte2) min || temp > (ubyte2)max)
        goto exit;

    *value = (ubyte) temp;
    status = OK;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_GetCertTime(ASN1_ITEM* pTime, CStream s, TimeDate* pGMTTime)
{
    const ubyte*    buffer = 0;
    sbyte4             i;
    const ubyte*    rest;
    MSTATUS         status;

    if (NULL == s.pFuncs->m_memaccess)
    {
        status = ERR_ASN_NULL_FUNC_PTR;
        goto exit;
    }

    buffer = CS_memaccess(s, (/*FSL*/sbyte4)pTime->dataOffset, (/*FSL*/sbyte4)pTime->length);

    if (0 == buffer)
    {
        status = ERR_MEM_;
        goto exit;
    }

    switch ( pTime->tag)
    {
        case UTCTIME:
            if ( pTime->length != 13)
            {
                status = ERR_CERT_INVALID_STRUCT;
                goto exit;
            }

            pGMTTime->m_year = 0;
	    /* XXX: m_year us a ubyte2 for some odd reason */
            status = GetTimeElementValue( buffer, (ubyte*)&pGMTTime->m_year, 0, 99);
            if ( OK > status)
            {
                goto exit;
            }

            if ( pGMTTime->m_year < 50)  /* 21st century */
            {
                pGMTTime->m_year += 30;
            }
            else if ( pGMTTime->m_year >= 70) /* 20th century */
            {
                pGMTTime->m_year -= 70;
            }
            else
            {
                /* refuse dates from 1950 to 1970 */
                status = ERR_CERT_INVALID_STRUCT;
                goto exit;
            }
            rest = buffer + 2;
            break;

        case GENERALIZEDTIME:
        {
            ubyte2  temp = 0;

            if (pTime->length != 15)
            {
                status = ERR_CERT_INVALID_STRUCT;
                goto exit;
            }

            for (i = 0; i < 4; ++i)
            {
                if ( buffer[i] < '0' || buffer[i] > '9')
                {
                    status = ERR_CERT_INVALID_STRUCT;
                    goto exit;
                }
                temp *= 10;
                temp = (ubyte2)(temp + (buffer[i] - '0'));
            }
            if ( temp >= 1970)
            {
                pGMTTime->m_year = (ubyte2)(temp - 1970);
            }
            else
            {
                /* refuse dates earlier than 1970 */
                status = ERR_CERT_INVALID_STRUCT;
                goto exit;
            }
            rest = buffer + 4;
            break;
        }

        default:
            status = ERR_CERT_INVALID_STRUCT;
            goto exit;
    }

    /* read the rest: 10 bytes */
    /* month */
    status = GetTimeElementValue( rest, &pGMTTime->m_month, 1, 12);
    if ( OK > status)
    {
        goto exit;
    }
    rest += 2;

    /* day */
    status = GetTimeElementValue( rest, &pGMTTime->m_day, 1, 31);
    if ( OK > status)
    {
        goto exit;
    }
    rest += 2;

    /* hour */
    status = GetTimeElementValue( rest, &pGMTTime->m_hour, 0, 23);
    if ( OK > status)
    {
        goto exit;
    }
    rest += 2;

    /* minute */
    status = GetTimeElementValue( rest, &pGMTTime->m_minute, 0, 59);
    if ( OK > status)
    {
        goto exit;
    }
    rest += 2;

    /* second */
    status = GetTimeElementValue( rest, &pGMTTime->m_second, 0, 59);

exit:

    if (buffer)
    {
        CS_stopaccess(s, buffer);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
GetCertOID(ASN1_ITEM* pAlgoId, CStream s, const ubyte* whichOID,
           ubyte* whichOIDSubType, ASN1_ITEM** ppOID)
{
    ASN1_ITEM*  pOID;
    ubyte4      i;
    ubyte4      oidLen;
    ubyte       digit;
    MSTATUS     status;

    /* whichOIDSubType and ppOID can be null */
    if ( NULL == whichOID)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    oidLen = *whichOID;

    status = ERR_CERT_INVALID_STRUCT;

    if ((NULL == pAlgoId) ||
        ((pAlgoId->id & CLASS_MASK) != UNIVERSAL) ||
        (pAlgoId->tag != SEQUENCE))
    {
        goto exit;
    }

    pOID = ASN1_FIRST_CHILD( pAlgoId);
    if (NULL == pOID ||
        ( (pOID->id & CLASS_MASK) != UNIVERSAL) ||
        (pOID->tag != OID) )
    {
        goto exit;
    }

    if (pOID->length != oidLen + ((whichOIDSubType) ? 1 : 0))
    {
        /* not the expected OID...*/
        status = ERR_CERT_NOT_EXPECTED_OID;
        goto exit;
    }

    /* compare OID */
    CS_seek(s, pOID->dataOffset, MOCANA_SEEK_SET);
    for (i = 0; i < oidLen; ++i)
    {
        if (OK > (status = CS_getc(s, &digit)))
            goto exit;

        if (whichOID[i+1] != digit)
        {
            status = ERR_CERT_NOT_EXPECTED_OID;
            goto exit;
        }
    }

    if (whichOIDSubType)
    {
        if (OK > (status = CS_getc(s, whichOIDSubType)))
            goto exit;
    }

    if (ppOID)
    {
        *ppOID = pOID;
    }

    status = OK;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertSignAlgoType(ASN1_ITEM* pSignAlgoId, CStream s,
                    ubyte4* hashType, ubyte4* pubKeyType)
{
    ubyte    subType;
    MSTATUS  status;

    *hashType = 0;
    *pubKeyType = 0;

    status = GetCertOID( pSignAlgoId, s, pkcs1_OID, &subType, NULL);
    if ( OK <= status)
    {
        *hashType = subType;
        *pubKeyType = akt_rsa;
    }
    else if ( OK <= (status = GetCertOID( pSignAlgoId, s,
                                  sha1withRsaSignature_OID, NULL, NULL)))
    {
        /* sha1withRSAEncryption_OID sub-type  */
        *hashType = sha1withRSAEncryption;
        *pubKeyType = akt_rsa;
    }
#if (defined(__ENABLE_MOCANA_DSA__))
    else if ( OK <= (status = GetCertOID( pSignAlgoId, s, dsaWithSHA1_OID, NULL, NULL)))
    {
        *hashType = ht_sha1;
        *pubKeyType = akt_dsa;
    }
    else if ( OK <= (status = GetCertOID( pSignAlgoId, s, dsaWithSHA2_OID, &subType, NULL)))
    {
        *pubKeyType = akt_dsa;
        switch (subType)
        {
#ifndef __DISABLE_MOCANA_SHA224__
        case 1:
            *hashType = ht_sha224;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA256__
        case 2:
            *hashType = ht_sha256;
            break;
#endif
        default:
            status = ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO;
            break;
        }
    }
#endif  /* defined(__ENABLE_MOCANA_DSA__ */
#if (defined(__ENABLE_MOCANA_ECC__))
    else if ( OK <= (status = GetCertOID( pSignAlgoId, s, ecdsaWithSHA1_OID, NULL, NULL)))
    {
        *hashType = ht_sha1;
        *pubKeyType = akt_ecc;
    }
    else if ( OK <= (status = GetCertOID( pSignAlgoId, s, ecdsaWithSHA2_OID, &subType, NULL)))
    {
       *pubKeyType = akt_ecc;
        switch (subType)
        {
#ifndef __DISABLE_MOCANA_SHA224__
        case 1:
            *hashType = ht_sha224;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA256__
        case 2:
            *hashType = ht_sha256;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA384__
        case 3:
            *hashType = ht_sha384;
            break;
#endif

#ifndef __DISABLE_MOCANA_SHA512__
        case 4:
            *hashType = ht_sha512;
            break;
#endif
        default:
            status = ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO;
            break;
        }
    }
#endif  /* defined(__ENABLE_MOCANA_ECC__ */

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_checkForUnknownCriticalExtensions(ASN1_ITEMPTR pExtensionsSeq, CStream s)
{
    static WalkerStep gotoExtensionOID[] =
    {
        { VerifyType, SEQUENCE, 0},
        { GoFirstChild, 0, 0},
        { VerifyType, OID, 0 },
        { Complete, 0, 0}
    };

    static const ubyte* knownExtensions[] =
    {
        basicConstraints_OID,
        keyUsage_OID,
        nameConstraints_OID
    };

    ASN1_ITEMPTR pExtension;

    if ( NULL == pExtensionsSeq)
    {
        return ERR_NULL_POINTER;
    }

    pExtension = ASN1_FIRST_CHILD( pExtensionsSeq);
    while (pExtension)
    {
        ASN1_ITEMPTR pOID;
        ASN1_ITEMPTR pCritical;

        if (OK > ASN1_WalkTree( pExtension, s, gotoExtensionOID, &pOID))
        {
            return ERR_CERT_INVALID_STRUCT;
        }

        pCritical = ASN1_NEXT_SIBLING( pOID);
        /* pCritical is OPTIONAL and FALSE by default, check type */
        if ( OK == ASN1_VerifyType( pCritical, BOOLEAN) &&
         pCritical->data.m_boolVal)
        {
            /* critical extension -> look for OID we checks */
            int i;
            for (i = 0; i < COUNTOF(knownExtensions); ++i)
            {
                if ( OK == ASN1_VerifyOID( pOID, s, knownExtensions[i]))
                {
                    break; /* found */
                }
            }

            /* found ? */
            if ( i >= COUNTOF( knownExtensions))
            {
                return ERR_CERT_UNKNOWN_CRITICAL_EXTENSION;
            }
        }

        pExtension = ASN1_NEXT_SIBLING( pExtension);
    }

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_checkCertificateIssuerSerialNumber( ASN1_ITEM* pIssuer,
                                ASN1_ITEM* pSerialNumber,
                                CStream pIssuerStream,
                                ASN1_ITEM* pCertificate,
                                CStream pCertStream)
{
    ASN1_ITEMPTR pVersion;
    ASN1_ITEMPTR pCertIssuer;
    ASN1_ITEMPTR pCertSerialNumber;
    MSTATUS status;

    /* need to see if there is the optional version (tag 0) */
    if (OK > (status = ASN1_GetChildWithTag(pCertificate, 0, &pVersion)))
    {
        return status;
    }

    /* serial number is first or second child */
    if (OK > (status = ASN1_GetNthChild( pCertificate, (pVersion) ? 2 : 1, &pCertSerialNumber)))
    {
        return ERR_CERT_INVALID_STRUCT;
    }

    /* issuer is 3rd or 4th child */
    if (OK > (status = ASN1_GetNthChild( pCertificate, (pVersion) ? 4 : 3, &pCertIssuer)))
    {
        return ERR_CERT_INVALID_STRUCT;
    }
    /* compare serial number if provided */
    if (pSerialNumber)
    {
        if ( OK > (status = ASN1_CompareItems( pCertSerialNumber, pCertStream,
                                    pSerialNumber, pIssuerStream)))
        {
            return status;
        }
    }

    return ASN1_CompareItems( pCertIssuer, pCertStream, pIssuer, pIssuerStream);
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_getCertificateSubjectAux( ASN1_ITEM* startItem, int startIsRoot, ASN1_ITEM** ppSubject)
{
    ASN1_ITEMPTR pVersion;
    ASN1_ITEMPTR pCertificate = NULL;
    MSTATUS status;

    if ( NULL == startItem)
        return ERR_NULL_POINTER;

    if (OK > (status = GetCertificatePart(startItem, startIsRoot, &pCertificate)))
        return ERR_CERT_INVALID_STRUCT;

    /* need to see if there is the optional version (tag 0) */
    if (OK > (status = ASN1_GetChildWithTag(pCertificate, 0, &pVersion)))
    {
        return status;
    }

    /* subject is 5th or 6th child */
    if ( ppSubject)
    {
        if (OK > ASN1_GetNthChild( pCertificate, (pVersion) ? 6 : 5, ppSubject))
        {
            return ERR_CERT_INVALID_STRUCT;
        }
    }

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertificateSubject( ASN1_ITEM* pRootItem, ASN1_ITEM** ppSubject)
{
    return CERT_getCertificateSubjectAux( pRootItem, 1, ppSubject);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertificateSubject2( ASN1_ITEM* pCertificate, ASN1_ITEM** ppSubject)
{
    return CERT_getCertificateSubjectAux( pCertificate, 0, ppSubject);
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_getCertificateIssuerSerialNumberAux( ASN1_ITEM* pStartItem,
                                            int startIsRoot,
                                            ASN1_ITEM** ppIssuer,
                                            ASN1_ITEM** ppSerialNumber)
{
    ASN1_ITEMPTR pVersion;
    ASN1_ITEMPTR pCertificate = NULL;
    MSTATUS status;

    if ( NULL == pStartItem)
        return ERR_NULL_POINTER;

    if (OK > (status = GetCertificatePart(pStartItem, startIsRoot, &pCertificate)))
        return ERR_CERT_INVALID_STRUCT;

    /* need to see if there is the optional version (tag 0) */
    if (OK > (status = ASN1_GetChildWithTag(pCertificate, 0, &pVersion)))
    {
        return status;
    }

    /* serial number is first or second child */
    if ( ppSerialNumber)
    {
        if (OK > ASN1_GetNthChild( pCertificate, (pVersion) ? 2 : 1, ppSerialNumber))
        {
            return ERR_CERT_INVALID_STRUCT;
        }
    }

    /* issuer is 3rd or 4th child */
    if ( ppIssuer)
    {
        if (OK > ASN1_GetNthChild( pCertificate, (pVersion) ? 4 : 3, ppIssuer))
        {
            return ERR_CERT_INVALID_STRUCT;
        }
    }

    return OK;
}



/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertificateIssuerSerialNumber( ASN1_ITEM* pRootItem,
                                            ASN1_ITEM** ppIssuer,
                                            ASN1_ITEM** ppSerialNumber)
{
    return CERT_getCertificateIssuerSerialNumberAux( pRootItem, 1,
                                                    ppIssuer, ppSerialNumber);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertificateIssuerSerialNumber2( ASN1_ITEM* pCertificate,
                                            ASN1_ITEM** ppIssuer,
                                            ASN1_ITEM** ppSerialNumber)
{
    return CERT_getCertificateIssuerSerialNumberAux( pCertificate, 0,
                                                    ppIssuer, ppSerialNumber);
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_checkCertificateIssuerAux(ASN1_ITEM* pPrevStartItem, int prevStartIsRoot,
                                CStream pPrevCertStream,
                                ASN1_ITEM* pStartItem, int startIsRoot,
                                CStream pCertStream)
{
    /* compare issuer of prev certificate with subject of certificate */
    MSTATUS status;
    ASN1_ITEM* pPrevCertificate = NULL;
    ASN1_ITEM* pCertificate = NULL;
    ASN1_ITEM* pVersion;
    ASN1_ITEM* pSubject;

    /* move to start of certificates */
    if (OK > (status = GetCertificatePart(pStartItem, startIsRoot, &pCertificate)))
        return ERR_CERT_INVALID_STRUCT;

    if (OK > (status = GetCertificatePart(pPrevStartItem, prevStartIsRoot, &pPrevCertificate)))
        return ERR_CERT_INVALID_STRUCT;

    /* need to see if there is the optional version (tag 0) */
    if (OK > (status = ASN1_GetChildWithTag(pCertificate, 0, &pVersion)))
        return ERR_CERT_INVALID_STRUCT;

    /* subject is 5th or 6th child */
    if (OK > (status = ASN1_GetNthChild( pCertificate, (pVersion) ? 6 : 5, &pSubject)))
    {
        return ERR_CERT_INVALID_STRUCT;
    }

    status = CERT_checkCertificateIssuerSerialNumber( pSubject, NULL,
                              pCertStream,
                              pPrevCertificate,
                              pPrevCertStream);

    if (ERR_FALSE == status)
    {
        status = ERR_CERT_INVALID_PARENT_CERTIFICATE;
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_checkCertificateIssuer(ASN1_ITEM* pPrevRootItem,
                                CStream pPrevCertStream,
                                ASN1_ITEM* pRootItem,
                                CStream pCertStream)
{
    return CERT_checkCertificateIssuerAux(pPrevRootItem, 1,
                                            pPrevCertStream,
                                            pRootItem, 1,
                                            pCertStream);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_checkCertificateIssuer2(ASN1_ITEM* pPrevCert,
                                CStream pPrevCertStream,
                                ASN1_ITEM* pCertificate,
                                CStream pCertStream)
{
    return CERT_checkCertificateIssuerAux(pPrevCert, 0,
                                            pPrevCertStream,
                                            pCertificate, 0,
                                            pCertStream);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_extractRSAKey(MOC_RSA(hwAccelDescr hwAccelCtx)
             ASN1_ITEM* pSubjectKeyInfo, CStream s, AsymmetricKey* pKey)
{
    ubyte4          i;
    sbyte4          startModulus;
    ubyte4          exponent, modulusLen;
    const ubyte*    modulus = 0;
    ubyte           rsaAlgoIdSubType;
    ASN1_ITEMPTR    pItem;
    MSTATUS         status;

    pItem = ASN1_FIRST_CHILD(pSubjectKeyInfo);
    if ( 0 == pItem)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* verify pItem is the OID for PKCS#1 */
    if (OK > (status = GetCertOID( pItem, s, pkcs1_OID, &rsaAlgoIdSubType, NULL)))
        goto exit;

    if ( rsaEncryption != rsaAlgoIdSubType)
    {
        status = ERR_CERT_NOT_EXPECTED_OID;
        goto exit;
    }

    /* the public key is in a bit string that encapsulates a PKCS struct */
    pItem = ASN1_NEXT_SIBLING( pItem);

    if ( NULL == pItem ||
            (pItem->id & CLASS_MASK) != UNIVERSAL ||
            pItem->tag != BITSTRING ||
            0 == pItem->encapsulates)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    /* extract the key parameters */
    pItem = ASN1_FIRST_CHILD( pItem);
    /* according to pkcs1-v2-1.asn the public key is a sequence of two integers
    the modulus and then the public exponent */
    if ( NULL == pItem ||
            (pItem->id & CLASS_MASK) != UNIVERSAL ||
            pItem->tag != SEQUENCE)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* modulus */
    pItem = ASN1_FIRST_CHILD( pItem);
    if ( NULL == pItem ||
            (pItem->id & CLASS_MASK) != UNIVERSAL ||
            pItem->tag != INTEGER)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    modulusLen = pItem->length;

    modulus = CS_memaccess( s, pItem->dataOffset, pItem->length);
    if ( NULL == modulus)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* ASN1 INTEGERs are signed so it's possible 0x00 are added to make sure the
     value is represented as positive so check for that */
    startModulus = 0;
    while ((startModulus < ((sbyte4)modulusLen)) && (0 == modulus[startModulus]))
    {
        ++startModulus;
    }

    /* we support only modulus up to 512 (4096 bits) bytes long */
    if (MOCANA_MAX_MODULUS_SIZE < (modulusLen - startModulus) )
    {
        /* prevent parasitic public key attack */
        status = ERR_CERT_RSA_MODULUS_TOO_BIG;
        goto exit;
    }

    /* exponent */
    pItem = ASN1_NEXT_SIBLING( pItem);
    if (NULL == pItem ||
            (pItem->id & CLASS_MASK) != UNIVERSAL ||
            pItem->tag != INTEGER)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* we support only exponent up to 4 bytes long */
    if (pItem->length > (ubyte4)sizeof(exponent))
    {
        status = ERR_CERT_RSA_EXPONENT_TOO_BIG;
        goto exit;
    }

    CS_seek( s, pItem->dataOffset, MOCANA_SEEK_SET);

    exponent = 0;
    for (i = 0; i < pItem->length; ++i)
    {
        ubyte digit;

        if (OK > (status = CS_getc(s, &digit)))
            goto exit;

        exponent = ((exponent << 8) | digit);
    }

    status = CRYPTO_setRSAParameters( MOC_RSA(hwAccelCtx) pKey,
                                        exponent,
                                        (ubyte*) (modulus + startModulus),
                                        modulusLen - startModulus,
                                        NULL, 0, NULL, 0,
                                        NULL);

exit:
    if (modulus)
    {
        CS_stopaccess(s, modulus);
    }

    return status;
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
extern MSTATUS
CERT_extractECCKey(ASN1_ITEM* pSubjectKeyInfo, CStream s, AsymmetricKey* pKey)
{
    ubyte           curveId;
    ASN1_ITEMPTR    pAlgorithmIdentifier, pTemp;
    const ubyte*    point = 0;
    MSTATUS         status;

    pAlgorithmIdentifier = ASN1_FIRST_CHILD(pSubjectKeyInfo);
    if ( 0 == pAlgorithmIdentifier)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* verify pItem is the OID for EC public Key */
    if (OK > (status = GetCertOID( pAlgorithmIdentifier, s, ecPublicKey_OID, NULL, &pTemp)))
        goto exit;

    /* get the OID for the Curve ( 2nd child of pItem) */
    pTemp = ASN1_NEXT_SIBLING( pTemp);
    if ( 0 == pTemp)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    /* this should be one of the OID for the curves we support */
    status = ASN1_VerifyOIDRoot( pTemp, s, ansiX962CurvesPrime_OID, &curveId);
    if ( OK > status) /* try another ASN1 arc */
    {
        status = ASN1_VerifyOIDRoot( pTemp, s, certicomCurve_OID, &curveId);
    }

    if (OK > status)
    {
        goto exit;
    }

    /* then the public key is the content of the BIT string -- a point on the curve
    encoded in the usual way */
    pTemp = ASN1_NEXT_SIBLING( pAlgorithmIdentifier);
    if (!pTemp ||
        UNIVERSAL != (pTemp->id & CLASS_MASK) ||
        BITSTRING != pTemp->tag)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* access the BITSTRING */
    point = CS_memaccess( s, pTemp->dataOffset, pTemp->length);
    if (!point)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = CRYPTO_setECCParameters( pKey, curveId, point,
                                                pTemp->length, NULL, 0, NULL)))
    {
        goto exit;
    }

exit:

    if (point)
    {
        CS_stopaccess( s, point);
    }

    return status;
}
#endif



/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DSA__))
extern MSTATUS
CERT_extractDSAKey(ASN1_ITEM* pSubjectKeyInfo, CStream s, AsymmetricKey* pKey)
{
    ASN1_ITEMPTR    pAlgorithmIdentifier, pTemp;
    ASN1_ITEMPTR    pRoot = 0;
    MSTATUS         status;
    const ubyte     *p = 0,
                    *q = 0,
                    *g = 0,
                    *bitStr = 0;
    ubyte4 pLen, qLen, gLen;
    CStream s2;
    MemFile mf;

/*
    DSAPublicKey ::= INTEGER  -- public key, y

    Dss-Parms  ::=  SEQUENCE  {
      p             INTEGER,
      q             INTEGER,
      g             INTEGER  }
*/

    pAlgorithmIdentifier = ASN1_FIRST_CHILD(pSubjectKeyInfo);
    if ( 0 == pAlgorithmIdentifier)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* verify pItem is the OID for dsa */
    if (OK > (status = GetCertOID( pAlgorithmIdentifier, s, dsa_OID, NULL, &pTemp)))
        goto exit;

    /* next item is the sequence Dss-Parms */
    pTemp = ASN1_NEXT_SIBLING( pTemp);
    if ( 0 == pTemp || OK > ASN1_VerifyType( pTemp, SEQUENCE))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* p */
    pTemp = ASN1_FIRST_CHILD( pTemp);
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

    /* y -- public key is a INTEGER inside a bit string */
    pTemp = ASN1_NEXT_SIBLING(pAlgorithmIdentifier);
    if ( OK > ( ASN1_VerifyType( pTemp, BITSTRING)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    bitStr = CS_memaccess( s, pTemp->dataOffset + pTemp->data.m_unusedBits,
                                pTemp->length - pTemp->data.m_unusedBits);
    if (!bitStr)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MF_attach( &mf, pTemp->length - pTemp->data.m_unusedBits, (ubyte*)bitStr);
    CS_AttachMemFile( &s2, &mf);
    if (OK > ( status = ASN1_Parse( s2, &pRoot)))
        goto exit;

    pTemp = ASN1_FIRST_CHILD(pRoot);
    if ( OK > ( ASN1_VerifyType( pTemp, INTEGER)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if (OK > (status = CRYPTO_setDSAParameters( pKey, p, pLen, q, qLen,
                                                g, gLen,
                                                bitStr + pTemp->dataOffset,
                                                pTemp->length, NULL, 0, NULL)))
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
    if (bitStr)
    {
        CS_stopaccess( s, bitStr);
    }

    if (pRoot)
    {
        TREE_DeleteTreeItem( (TreeItem*) pRoot);
    }

    return status;
}
#endif


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_setKeyFromSubjectPublicKeyInfoCert(MOC_RSA(hwAccelDescr hwAccelCtx)
                                        ASN1_ITEM* pCertificate, CStream s, AsymmetricKey* pPubKey)
{
    /* this is the structure of a certificate = Signed Certificate Object */
    /* Certificate Object */
        /* Version */
        /* Serial Number */
        /* Signature Algorithm */
        /* Issuer */
        /* Validity */
        /* Subject */
        /* Subject Public Key Info */
    /* Signature Algorithm */
    /* Signature */
    ASN1_ITEM*      pSubjectKeyInfo;
    ASN1_ITEM*      pVersion;
    MSTATUS         status;

    if (NULL == s.pFuncs->m_memaccess)
    {
        status = ERR_ASN_NULL_FUNC_PTR;
        goto exit;
    }

    if ((NULL == pCertificate) || (NULL == pPubKey))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pVersion)))
        goto exit;

    if (OK > (status = ASN1_GetNthChild( pCertificate, pVersion ? 7 : 6, &pSubjectKeyInfo)))
        goto exit;

    /* verify the type */
    if ((UNIVERSAL != (pSubjectKeyInfo->id & CLASS_MASK)) || (SEQUENCE != pSubjectKeyInfo->tag))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    status = CERT_extractRSAKey(MOC_RSA(hwAccelCtx) pSubjectKeyInfo, s, pPubKey);

#if (defined(__ENABLE_MOCANA_DSA__))
    if (ERR_CERT_NOT_EXPECTED_OID == status)
    {
        status = CERT_extractDSAKey( pSubjectKeyInfo, s, pPubKey);
    }
#endif
#if (defined(__ENABLE_MOCANA_ECC__))
    if ( ERR_CERT_NOT_EXPECTED_OID == status)
    {
        /* not a RSA or DSA key -> try a ECC key */
        status = CERT_extractECCKey( pSubjectKeyInfo, s, pPubKey);
    }
#endif

exit:
    return status;

} /* CERT_setKeyFromSubjectPublicKeyInfoCert */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(hwAccelDescr hwAccelCtx) ASN1_ITEM* rootItem, CStream s,
                                            AsymmetricKey* pPubKey)
{
    ASN1_ITEM*      pCertificate = NULL;
    MSTATUS         status;

    if (NULL == s.pFuncs->m_memaccess)
    {
        return ERR_ASN_NULL_FUNC_PTR;
    }

    if ((NULL == rootItem) || (NULL == pPubKey))
    {
        return ERR_NULL_POINTER;
    }

    if (OK > (status = GetCertificatePart( rootItem, 1, &pCertificate)))
        return status;

    return CERT_setKeyFromSubjectPublicKeyInfoCert(MOC_RSA(hwAccelCtx)
         pCertificate, s, pPubKey);
}


/*------------------------------------------------------------------*/
extern MSTATUS
CERT_extractVersion(ASN1_ITEM* startItem, CStream s, intBoolean startIsRoot, sbyte4 *pRetVersion)
{
    ASN1_ITEM*  pCertificate;
    ASN1_ITEM*  pVersion;
    MSTATUS     status;

    if ((NULL == startItem) || (NULL == pRetVersion))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *pRetVersion = 0;

    if (OK > (status = GetCertificatePart( startItem, startIsRoot, &pCertificate)))
        goto exit;

    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pVersion)))
        goto exit;

    if (pVersion)
    {
        if (UNIVERSAL == (pVersion->id & CLASS_MASK) &&
	    pVersion->tag == INTEGER &&
	    pVersion->length <= sizeof(sbyte4))
	{
	    *pRetVersion = pVersion->data.m_intVal;
	}
    }
    else
    {
        *pRetVersion = 1;
    }

 exit:
    return status;
}

/*------------------------------------------------------------------*/

static MSTATUS
CERT_getSubjectEntryByOIDAux( ASN1_ITEM* startItem, int startIsRoot,
                              CStream s, const ubyte* oid, ASN1_ITEM** ppEntryItem)
{
    ASN1_ITEM*  pCertificate = NULL;
    ASN1_ITEM*  pSubject;
    ASN1_ITEM*  pCurrChild;
    ASN1_ITEM*  pVersion;
    MSTATUS     status;

    if (OK > (status = GetCertificatePart( startItem, startIsRoot, &pCertificate)))
        goto exit;

    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pVersion)))
        goto exit;

    /* subject is sixth child of certificate object */
    if (OK > (status = ASN1_GetNthChild( pCertificate, pVersion ? 6 : 5, &pSubject)))
        goto exit;

    /* now get the child with the passed-in OID */
    /*  Name ::= SEQUENCE of RelativeDistinguishedName
        RelativeDistinguishedName = SET of AttributeValueAssertion
        AttributeValueAssertion = SEQUENCE { attributeType OID; attributeValue ANY }
    */

    /* Name is a sequence */
    if ( NULL == pSubject ||
            (pSubject->id & CLASS_MASK) != UNIVERSAL ||
            pSubject->tag != SEQUENCE)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    pCurrChild = ASN1_FIRST_CHILD( pSubject);

    while (pCurrChild)
    {
        ASN1_ITEM* pGrandChild;
        ASN1_ITEM* pOID;

        status = ERR_CERT_INVALID_STRUCT;

        /* child should be a SET */
        if ( (pCurrChild->id & CLASS_MASK) != UNIVERSAL ||
                pCurrChild->tag != SET)
        {
            goto exit;
        }

        /* GrandChild should be a SEQUENCE */
        pGrandChild = ASN1_FIRST_CHILD( pCurrChild);

        while (pGrandChild)
        {
            if ( NULL == pGrandChild ||
                 (pGrandChild->id & CLASS_MASK) != UNIVERSAL ||
                  pGrandChild->tag != SEQUENCE)
            {
                goto exit;
            }

            /* get the OID */
            pOID = ASN1_FIRST_CHILD(pGrandChild);
            if ( NULL == pOID ||
                (pOID->id & CLASS_MASK) != UNIVERSAL ||
                pOID->tag != OID )
            {
                goto exit;
            }

            /* is it the right OID ?*/
            if (OK == ASN1_VerifyOID(pOID, s, oid))
            {
                *ppEntryItem = ASN1_NEXT_SIBLING(pOID);

                status = ( *ppEntryItem) ? OK: ERR_CERT_INVALID_STRUCT;
                goto exit;
            }
            pGrandChild = ASN1_NEXT_SIBLING(pGrandChild);
        }

        pCurrChild = ASN1_NEXT_SIBLING( pCurrChild);
    }

    status = ERR_CERT_INVALID_STRUCT;

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getSubjectCommonName( ASN1_ITEM* rootItem,
                          CStream s, ASN1_ITEM** ppCommonNameItem)
{
    return CERT_getSubjectEntryByOIDAux( rootItem, 1, s, commonName_OID, ppCommonNameItem);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getSubjectCommonName2( ASN1_ITEM* pCertificate,
                          CStream s, ASN1_ITEM** ppCommonNameItem)
{
    return CERT_getSubjectEntryByOIDAux( pCertificate, 0, s, commonName_OID, ppCommonNameItem);
}

/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getSubjectEntryByOID( ASN1_ITEM* rootItem, CStream s, const ubyte* oid, ASN1_ITEM** ppEntryItem)
{
    return CERT_getSubjectEntryByOIDAux( rootItem, 1, s, oid, ppEntryItem);
}

/*------------------------------------------------------------------*/
extern MSTATUS
CERT_getSubjectEntryByOID2( ASN1_ITEM* pCertificate, CStream s, const ubyte* oid, ASN1_ITEM** ppEntryItem)
{
    return CERT_getSubjectEntryByOIDAux( pCertificate, 0, s, oid, ppEntryItem);
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_matchCommonNameSuffix( ASN1_ITEM* pCommonName, CStream s,
                                const sbyte* nameToMatch, ubyte4 flags)
{
    ubyte       ch;
    MSTATUS     status;
    ubyte4      index;
    ubyte4      nameToMatchLen;

    nameToMatchLen = MOC_STRLEN( nameToMatch);

    CS_seek(s, pCommonName->dataOffset, MOCANA_SEEK_SET);

    /* the certificate common name is either a full name or
        wildcard "*.acme.com" */
    if ( OK > ( status = CS_getc( s, &ch)))
        goto exit;

     /* wild card matching */
    if ('*' == ch && (0 == (flags & matchFlagNoWildcard)))
    {
        /* match the smaller of the two */
        if ( nameToMatchLen < pCommonName->length - 1)
        {
            index = 0;
            CS_seek(s, pCommonName->length - nameToMatchLen - 1,
                    MOCANA_SEEK_CUR);
        }
        else
        {
            index = nameToMatchLen - pCommonName->length + 1;
        }
    }
    else if ( nameToMatchLen <= pCommonName->length)
    {
        /* match the end */
        index = 0;
        CS_seek(s, pCommonName->length - nameToMatchLen - 1, MOCANA_SEEK_CUR);
    }
    else
    {
        status = ERR_CERT_BAD_COMMON_NAME;
        goto exit;
    }

    if (flags & matchFlagDotSuffix)
    {
        sbyte4 dotIndex = CS_tell(s)-1;

        if (dotIndex < pCommonName->dataOffset)
        {
            status = ERR_CERT_BAD_COMMON_NAME;
            goto exit;
        }
        CS_seek(s, dotIndex, MOCANA_SEEK_SET);
        if ( OK > (status = CS_getc(s, &ch)))
            goto exit;
        if (ch != '.')
        {
            status = ERR_CERT_BAD_COMMON_NAME;
            goto exit;
        }
    }

    /* match */
    for (; index < nameToMatchLen; index++)
    {
        if (OK > (status = CS_getc(s, &ch)))
            goto exit;

        /* case insensitive comparison */
        ch = MTOLOWER(ch);

        if ((ubyte)MTOLOWER(nameToMatch[index]) != ch)
        {
            status = ERR_CERT_BAD_COMMON_NAME;
            goto exit;
        }
    }

exit:

    return status;
}


/*------------------------------------------------------------------*/

static MSTATUS
wild_cardMatch(sbyte* DNSName, ubyte4 DNSNameLen, sbyte* HostName, ubyte4 flags)
{
    ubyte4   HostNameLen;
    ubyte4   i = 0;
    sbyte   *pHostName = HostName;
    sbyte   *pDNSName = DNSName;
    sbyte   *temp = NULL;
    sbyte   *dotCheck = NULL;
    HostNameLen = MOC_STRLEN(HostName);

    if (NULL == MOC_STRCHR(pDNSName, '*', DNSNameLen))
    {
        /* no wildcards present */
        if (DNSNameLen != HostNameLen)
            return -1;

        if (0 != MOC_STRNICMP(pDNSName, pHostName, DNSNameLen))
            return -1;
        else 
        	return 0;

    }

    if (flags & noWildcardMatch)
        return -1;

    if (flags & matchFlagSuffix)
    {
        if (NULL == (pHostName = MOC_STRCHR(pHostName, '.', HostNameLen)))
            return -1;
        if (NULL == (temp = MOC_STRCHR(pDNSName, '.', DNSNameLen)))
            return -1;
        i = (ubyte4) (temp - pDNSName);
    }

    while((i < DNSNameLen) && (HostNameLen > (ubyte4)(pHostName - HostName)))
    {
        if (pDNSName[i] == '*')
        {
            if (DNSNameLen == (i+1))
            {
                if (NULL != MOC_STRCHR(pHostName, '.', (HostNameLen - (ubyte4)(pHostName - HostName))))
                    return -1;
                else
                    return 0;
            }
            dotCheck = pHostName;
            while ((NULL != pHostName) && (NULL != (pHostName = MOC_STRCHR(pHostName, pDNSName[i+1], (HostNameLen - (ubyte4)(pHostName - HostName))))))
            {
                if (NULL == (temp = MOC_STRCHR((pDNSName+i+1), '*',(DNSNameLen - i - 1))))
                {
                    temp = pDNSName + DNSNameLen;              
                }
                if (0 == MOC_STRNICMP(pHostName, pDNSName+i+1, ((ubyte4)(temp-pDNSName) - i - 1)))
                {
                    if (NULL != MOC_STRCHR(dotCheck, '.', (ubyte4)(pHostName - dotCheck)))
                        return -1;
                    i++;
                    break;
                }
                else
                {
                    pHostName++;
                }
            }

            if (i > DNSNameLen || pDNSName[i] == '*')
                return -1;
        }
        else if (MTOLOWER(pDNSName[i]) == MTOLOWER(*pHostName))
        {
            i++;
            pHostName++;
            continue;
        }
        else
        {
            return -1;
        }
    }

    if (DNSNameLen == i &&  (HostNameLen >= ((ubyte4)(pHostName - HostName) + 1)))
    {
        return -1;
    }

    if ((DNSNameLen > i+1) && (HostNameLen == (ubyte4)(pHostName - HostName)))
        return -1;
    
    return 0;
}

/*------------------------------------------------------------------*/

static MSTATUS
CERT_matchCommonName(ASN1_ITEM* pCommonName, CStream s,
                     const sbyte* nameToMatch, ubyte4 flags)
{
    MSTATUS     status;
    sbyte       DNSName[128];
    
    MOC_MEMSET((ubyte*)DNSName, 0x00, 128);

    CS_seek(s, pCommonName->dataOffset, MOCANA_SEEK_SET);

    if (OK > (status = CS_read(DNSName, sizeof(ubyte), pCommonName->length, s)))
        goto exit;

    if (OK != wild_cardMatch(DNSName, pCommonName->length, (sbyte*)nameToMatch, flags))
    {
        status = ERR_CERT_BAD_COMMON_NAME;
        goto exit;
    }
    else
        status = OK;
exit:
    return status;
}



/*------------------------------------------------------------------*/

static MSTATUS
CERT_CompSubjectAltNamesAux(ASN1_ITEM* startItem, int startIsRoot, CStream s,
                            const sbyte* nameToMatch, ubyte4 tagMask, intBoolean exactStatus)
{
    /* tag mask is used to limit comparison to names that have only the
        tags in the mask ex: 1<<2 as a mask makes sure only DNS names
        are used in the comparison

            subjectAltName EXTENSION ::= {
              SYNTAX         GeneralNames
              IDENTIFIED BY  id-ce-subjectAltName
            }

            GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName

            GeneralName ::= CHOICE {
              otherName                  [0]  INSTANCE OF OTHER-NAME,
              rfc822Name                 [1]  IA5String,
              dNSName                    [2]  IA5String,
              x400Address                [3]  ORAddress,
              directoryName              [4]  Name,
              ediPartyName               [5]  EDIPartyName,
              uniformResourceIdentifier  [6]  IA5String,
              iPAddress                  [7]  OCTET STRING,
              registeredID               [8]  OBJECT IDENTIFIER
            }
    */

    ASN1_ITEM*  pExtensionsSeq;
    ASN1_ITEM*  pSubjectAltNames;
    ASN1_ITEM*  pGeneralName;
    MSTATUS     status = ERR_CERT_BAD_SUBJECT_NAME;
    intBoolean  critical;

    if ((NULL == startItem) || (NULL == nameToMatch))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > ( status = CERT_getCertificateExtensionsAux( startItem,
                                                            startIsRoot,
                                                            &pExtensionsSeq)))
    {
        goto exit;
    }

    if ( !pExtensionsSeq)
    {
        status = ERR_CERT_BAD_SUBJECT_NAME;
        goto exit;
    }

    if (OK > (status = CERT_getCertExtension( pExtensionsSeq, s,
            subjectAltName_OID, &critical, &pSubjectAltNames)))
    {
        goto exit;
    }

    if ( !pSubjectAltNames)
    {
        status = ERR_CERT_BAD_SUBJECT_NAME;
        goto exit;
    }

    if  (OK > ( status = ASN1_VerifyType( pSubjectAltNames, SEQUENCE)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    /* pSubjectAltNames is a sequence of general names; filter out by tags
    and see if the name matches */
	status = ERR_CERT_BAD_SUBJECT_NAME;
    pGeneralName = ASN1_FIRST_CHILD( pSubjectAltNames);
    while (pGeneralName)
    {
        ubyte4 tag;
        if ( OK > (status = ASN1_GetTag( pGeneralName, &tag)))
        {
            status = ERR_CERT_INVALID_STRUCT;
            goto exit;
        }
        if ( (1 << tag) & tagMask)
        {
            if ( OK == (status = CERT_matchCommonName(pGeneralName, s, nameToMatch, matchFlagNoWildcard)))
            {
                goto exit;
            }
        }

        pGeneralName = ASN1_NEXT_SIBLING(pGeneralName);
    }

    /* return more specific status if 'exactStatus' is true */
    status = ((exactStatus) ? status : ERR_CERT_BAD_SUBJECT_NAME);

exit:

    return status;

} /* CERT_CompSubjectAltNames */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_CompSubjectAltNames(ASN1_ITEM* rootItem, CStream s,
                           const sbyte* nameToMatch, ubyte4 tagMask)
{
    return CERT_CompSubjectAltNamesAux(rootItem, 1, s,
                                       nameToMatch, tagMask, FALSE);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_CompSubjectAltNames2(ASN1_ITEM* pCertificate, CStream s,
                           const sbyte* nameToMatch, ubyte4 tagMask)
{
    return CERT_CompSubjectAltNamesAux(pCertificate, 0, s,
                                       nameToMatch, tagMask, FALSE);
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_CompSubjectCommonNameAux(ASN1_ITEM* startItem, int startIsRoot,
                              CStream s, const sbyte* nameToMatch)
{
    ASN1_ITEM*  pCommonName;
    MSTATUS     status;

    if ((NULL == startItem) || (NULL == nameToMatch))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CERT_getSubjectEntryByOIDAux( startItem, startIsRoot, s, commonName_OID, &pCommonName)))
    {
        goto exit;
    }

    status = CERT_matchCommonName( pCommonName, s, nameToMatch, 0);

exit:

    return status;

} /* CERT_CompSubjectCommonName */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_CompSubjectCommonName(ASN1_ITEM* rootItem,
                              CStream s, const sbyte* nameToMatch)
{
    return CERT_CompSubjectCommonNameAux(rootItem, 1, s, nameToMatch);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_CompSubjectCommonName2(ASN1_ITEM* pCertificate,
                              CStream s, const sbyte* nameToMatch)
{
    return CERT_CompSubjectCommonNameAux(pCertificate, 0, s, nameToMatch);
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_CompSubjectAltNamesExAux(ASN1_ITEM* startItem, int startIsRoot, CStream s,
                           const CNMatchInfo* namesToMatch, ubyte4 tagMask, intBoolean exactStatus)
{
    /* tag mask is used to limit comparison to names that have only the
        tags in the mask ex: 1<<2 as a mask makes sure only DNS names
        are used in the comparison

            subjectAltName EXTENSION ::= {
              SYNTAX         GeneralNames
              IDENTIFIED BY  id-ce-subjectAltName
            }

            GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName

            GeneralName ::= CHOICE {
              otherName                  [0]  INSTANCE OF OTHER-NAME,
              rfc822Name                 [1]  IA5String,
              dNSName                    [2]  IA5String,
              x400Address                [3]  ORAddress,
              directoryName              [4]  Name,
              ediPartyName               [5]  EDIPartyName,
              uniformResourceIdentifier  [6]  IA5String,
              iPAddress                  [7]  OCTET STRING,
              registeredID               [8]  OBJECT IDENTIFIER
            }
    */

    ASN1_ITEM*  pExtensionsSeq;
    ASN1_ITEM*  pSubjectAltNames;
    ASN1_ITEM*  pGeneralName;
    MSTATUS     status;
    intBoolean  critical;

    if ((NULL == startItem) || (NULL == namesToMatch))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > ( status = CERT_getCertificateExtensionsAux( startItem,
                                            startIsRoot, &pExtensionsSeq)))
    {
        goto exit;
    }

    if ( !pExtensionsSeq)
    {
        status = ERR_CERT_BAD_SUBJECT_NAME;
        goto exit;
    }

    if (OK > (status = CERT_getCertExtension( pExtensionsSeq, s,
            subjectAltName_OID, &critical, &pSubjectAltNames)))
    {
        goto exit;
    }

    if ( !pSubjectAltNames)
    {
        status = ERR_CERT_BAD_SUBJECT_NAME;
        goto exit;
    }

    if  (OK > ( status = ASN1_VerifyType( pSubjectAltNames, SEQUENCE)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    /* pSubjectAltNames is a sequence of general names; filter out by tags
    and see if the name matches */
    pGeneralName = ASN1_FIRST_CHILD( pSubjectAltNames);
    while (pGeneralName)
    {
        ubyte4 tag;
        if ( OK > (status = ASN1_GetTag( pGeneralName, &tag)))
        {
            status = ERR_CERT_INVALID_STRUCT;
            goto exit;
        }
        if ( (1 << tag) & tagMask)
        {
            const CNMatchInfo* tmp = namesToMatch;

            while ( tmp->name)
            {
                if ( tmp->flags & matchFlagSuffix)
                {
                    status = CERT_matchCommonNameSuffix( pGeneralName, s,
                                tmp->name, tmp->flags);
                }
                else
                {
                    status = CERT_matchCommonName( pGeneralName, s,
                                tmp->name, tmp->flags);
                }
                if ( OK == status)
                {
                    goto exit;
                }
                tmp++;
            }
        }
        pGeneralName = ASN1_NEXT_SIBLING(pGeneralName);
    }

    /* return more specific status if 'exactStatus' is true */
    status = ((exactStatus) ? status : ERR_CERT_BAD_SUBJECT_NAME);

exit:

    return status;

} /* CERT_CompSubjectAltNamesAux */


/*------------------------------------------------------------------*/
/*
   API added to give more specific status, The return status differntiates
   1. tag in the extention found but strings are not matching
   2. there is no tag of given type present in the extention
*/
MOC_EXTERN MSTATUS
CERT_CompSubjectAltNamesExact(ASN1_ITEM* rootItem, CStream s,
                              const sbyte* nameToMatch, ubyte4 tagMask)
{
    //return CERT_CompSubjectAltNamesAux(rootItem, 1, s,
    //                                   nameToMatch, tagMask, TRUE);
    CNMatchInfo cn[2];
    cn[0].name = nameToMatch;
    cn[0].flags = 0;
    MOC_MEMSET((ubyte *)&cn[1], 0, sizeof(CNMatchInfo));

    return CERT_CompSubjectAltNamesExAux(rootItem, 1, s,
                                       cn, tagMask, TRUE);
}



/*---------------------------------------------------------------------------*/

extern MSTATUS
CERT_CompSubjectAltNamesEx(ASN1_ITEM* rootItem, CStream s,
                           const CNMatchInfo* namesToMatch, ubyte4 tagMask)
{
    return CERT_CompSubjectAltNamesExAux( rootItem, 1, s, namesToMatch, tagMask, FALSE);
}


/*---------------------------------------------------------------------------*/
extern MSTATUS
CERT_CompSubjectAltNamesEx2(ASN1_ITEM* pCertificate, CStream s,
                           const CNMatchInfo* namesToMatch, ubyte4 tagMask)
{
    return CERT_CompSubjectAltNamesExAux( pCertificate, 0, s, namesToMatch, tagMask, FALSE);
}



/*------------------------------------------------------------------*/
#if defined(__ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__)
static MSTATUS
CERT_CompSubjectCommonNameExAux(ASN1_ITEM* startItem, int startIsRoot,
                                CStream s, const CNMatchInfo* namesToMatch)
{
    ASN1_ITEM*  pCommonName;
    MSTATUS     status;

    if ((NULL == startItem) || (NULL == namesToMatch))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = CERT_getSubjectEntryByOIDAux( startItem, startIsRoot,
                                                     s, commonName_OID, &pCommonName)))
    {
        goto exit;
    }

    status = ERR_CERT_BAD_COMMON_NAME;

    while ( OK > status && namesToMatch->name)
    {
        if ( namesToMatch->flags & matchFlagSuffix)
        {
            status = CERT_matchCommonNameSuffix( pCommonName, s,
                        namesToMatch->name, namesToMatch->flags);
        }
        else
        {
            status = CERT_matchCommonName( pCommonName, s,
                        namesToMatch->name, namesToMatch->flags);
        }
        namesToMatch++;
    }

exit:

    return status;
}


/*------------------------------------------------------------------------*/

extern MSTATUS
CERT_CompSubjectCommonNameEx(ASN1_ITEM* rootItem, CStream s,
                             const CNMatchInfo* namesToMatch)
{
    return CERT_CompSubjectCommonNameExAux( rootItem, 1, s, namesToMatch);
}


/*------------------------------------------------------------------------*/

extern MSTATUS
CERT_CompSubjectCommonNameEx2(ASN1_ITEM* pCertificate, CStream s,
                              const CNMatchInfo* namesToMatch)
{
    return CERT_CompSubjectCommonNameExAux( pCertificate, 0, s, namesToMatch);
}

#endif


/*------------------------------------------------------------------*/

static MSTATUS
CERT_VerifyValidityTimeAux(ASN1_ITEM* startItem, CStream s, int startIsRoot, intBoolean chkBefore, intBoolean chkAfter)
{
    ASN1_ITEM*  pCertificate = NULL;
    ASN1_ITEM*  pValidity;
    ASN1_ITEM*  pTime;
    ASN1_ITEM*  pVersion;
    TimeDate    certTime;
    TimeDate    currTime;
#if ((!defined __MOCANA_DISABLE_CERT_TIME_VERIFY__) && (!defined(__MOCANA_DISABLE_CERT_STOP_TIME_VERIFY__)))
    sbyte4      res;
#endif
    MSTATUS     status;

    if ((NULL == startItem))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

#if (defined(MOC_DYNAMIC_VERIFY_CERT_TIME))
    if (0 == MOC_DYNAMIC_VERIFY_CERT_TIME())
    {
        /* 0 == bypass time verify; non-zero == do normal verification */
        status = OK;
        goto exit;
    }
#endif

    if (OK > (status = GetCertificatePart( startItem, startIsRoot, &pCertificate)))
        goto exit;

    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pVersion)))
        goto exit;

    /* validity is fifth child of certificate object */
    if (OK > (status = ASN1_GetNthChild( pCertificate, pVersion ? 5 : 4, &pValidity)))
        goto exit;

    /* validity is a sequence of two items */
    if ( NULL == pValidity ||
            (pValidity->id & CLASS_MASK) != UNIVERSAL ||
            pValidity->tag != SEQUENCE)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if (OK > (status = RTOS_timeGMT(&currTime)))
        goto exit;

    if (OK > (status = ASN1_GetNthChild( pValidity, 2, &pTime)))
        goto exit;

    if (OK > (status = CERT_GetCertTime( pTime, s, &certTime)))
        goto exit;

#if ((!defined __MOCANA_DISABLE_CERT_TIME_VERIFY__) && (!defined(__MOCANA_DISABLE_CERT_STOP_TIME_VERIFY__)))
    /* verify expiration date */
    if(chkAfter)
    {
        if (certTime.m_year == currTime.m_year)
        	MOC_MEMCMP((ubyte*) &certTime+2, (ubyte*) &currTime+2, sizeof(TimeDate)-2, &res);
     	else 
        	res = certTime.m_year - currTime.m_year;

    	if ( res <= 0)
        {
            status = ERR_CERT_EXPIRED;
            goto exit;
        }
    }
#endif

#if ((!defined __MOCANA_DISABLE_CERT_TIME_VERIFY__) && (!defined(__MOCANA_DISABLE_CERT_START_TIME_VERIFY__)))
    /* verify start date */
    if (OK > (status = ASN1_GetNthChild( pValidity, 1, &pTime)))
        goto exit;

    if (OK > (status = CERT_GetCertTime( pTime, s, &certTime)))
        goto exit;

    if(chkBefore)
    {
        if (certTime.m_year == currTime.m_year)
            MOC_MEMCMP( (ubyte*) &certTime+2, (ubyte*) &currTime+2, sizeof(TimeDate)-2, &res);
        else
            res = certTime.m_year - currTime.m_year;

        if (res >= 0)
        {
            status = ERR_CERT_START_TIME_VALID_IN_FUTURE;
            goto exit;
        }
    }
#endif

    status = OK;

exit:
    return status;

} /* CERT_VerifyValidityTime */

/*------------------------------------------------------------------*/

extern MSTATUS
CERT_VerifyValidityTimeWithConf(ASN1_ITEM* rootItem, CStream s, intBoolean chkBefore, intBoolean chkAfter)
{
    return CERT_VerifyValidityTimeAux(rootItem, s, 1, chkBefore, chkAfter);
} /* CERT_VerifyValidityTime2 */

/*------------------------------------------------------------------*/

extern MSTATUS
CERT_VerifyValidityTime(ASN1_ITEM* rootItem, CStream s)
{
    return CERT_VerifyValidityTimeAux(rootItem, s, 1, TRUE, TRUE);
} /* CERT_VerifyValidityTime */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_VerifyValidityTime2(ASN1_ITEM* pCertificate, CStream s)
{
    return CERT_VerifyValidityTimeAux(pCertificate, s, 0, TRUE, TRUE);
} /* CERT_VerifyValidityTime */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_ComputeBufferHash(MOC_HASH(hwAccelDescr hwAccelCtx) ubyte*  buffer,
                            ubyte4  bytesToHash,
                            ubyte   hash[CERT_MAXDIGESTSIZE],
                            sbyte4  *hashSize,
                            ubyte4   rsaAlgoIdSubType)
{
    MSTATUS status;

    if ((NULL == buffer) || (NULL == hash) || (NULL == hashSize))
    {
        return ERR_NULL_POINTER;
    }

    MOC_MEMSET( hash, 0, CERT_MAXDIGESTSIZE);

    switch ( rsaAlgoIdSubType)
    {
#ifdef __ENABLE_MOCANA_MD2__
    case md2withRSAEncryption:
    {
        status = MD2_completeDigest( MOC_HASH(hwAccelCtx) (ubyte*)buffer, bytesToHash, hash);
        *hashSize = MD2_RESULT_SIZE;
        break;
    }
#endif

#ifdef __ENABLE_MOCANA_MD4__
    case md4withRSAEncryption:
    {
        status = MD4_completeDigest( MOC_HASH(hwAccelCtx) (ubyte*)buffer, bytesToHash, hash);
        *hashSize = MD4_RESULT_SIZE;
        break;
    }
#endif

    case md5withRSAEncryption:
    {
        status = MD5_completeDigest(MOC_HASH(hwAccelCtx) (ubyte*)buffer, bytesToHash, hash);
        *hashSize = MD5_RESULT_SIZE;
        break;
    }

    case sha1withRSAEncryption:
    {
        status = SHA1_completeDigest(MOC_HASH(hwAccelCtx) (ubyte*)buffer, bytesToHash, hash);
        *hashSize = SHA1_RESULT_SIZE;
        break;
    }

#ifndef __DISABLE_MOCANA_SHA224__
    case sha224withRSAEncryption:
    {
        status = SHA224_completeDigest( MOC_HASH(hwAccelCtx) (ubyte*) buffer, bytesToHash, hash);
        *hashSize = SHA224_RESULT_SIZE;
        break;
    }
#endif

#ifndef __DISABLE_MOCANA_SHA256__
    case sha256withRSAEncryption:
    {
        status = SHA256_completeDigest( MOC_HASH(hwAccelCtx) (ubyte*) buffer, bytesToHash, hash);
        *hashSize = SHA256_RESULT_SIZE;
        break;
    }
#endif

#ifndef __DISABLE_MOCANA_SHA384__
    case sha384withRSAEncryption:
    {
        status = SHA384_completeDigest( MOC_HASH(hwAccelCtx) (ubyte*) buffer, bytesToHash, hash);
        *hashSize = SHA384_RESULT_SIZE;
        break;
    }
#endif

#ifndef __DISABLE_MOCANA_SHA512__
    case sha512withRSAEncryption:
    {
        status = SHA512_completeDigest( MOC_HASH(hwAccelCtx) (ubyte*) buffer, bytesToHash, hash);
        *hashSize = SHA512_RESULT_SIZE;
        break;
    }
#endif

    default:
        status = ERR_CERT_UNSUPPORTED_DIGEST;
        break;
    }

    return status;

} /* CERT_ComputeBufferHash */


/*------------------------------------------------------------------*/

static MSTATUS
CERT_ComputeCertificateHash(MOC_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* rootItem,
                            CStream s,
                            ubyte* hash,
                            sbyte4* hashLen,
                            ubyte4* hashType,
                            ubyte4* pubKeyType)
{
    MSTATUS status;
    ASN1_ITEM* pCertificate = NULL;
    ASN1_ITEM* pSeqAlgoId;
    ASN1_ITEM* pItem;
    sbyte4 bytesToHash;
    const ubyte* buffer = 0;

    if ((NULL == rootItem) || (NULL == hash) || (NULL == hashType) || (NULL == pubKeyType) )
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (NULL == s.pFuncs->m_memaccess)
    {
        status = ERR_ASN_NULL_FUNC_PTR;
        goto exit;
    }

    /* get the algorithm identifier */
    /* go to signed struct */
    pItem = ASN1_FIRST_CHILD( rootItem);
    if ( NULL == pItem)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    /* algo id is the second child of signed */
    status = ASN1_GetNthChild( pItem, 2, &pSeqAlgoId);
    if (OK > status)
    {
        goto exit;
    }

    status = CERT_getCertSignAlgoType( pSeqAlgoId, s, hashType, pubKeyType);
    if ( OK > status)
    {
        goto exit;
    }

    status = GetCertificatePart( rootItem, 1, &pCertificate);
    if ( OK > status)
    {
        goto exit;
    }

    /* now we need to compute the hash of the whole certificate */
    bytesToHash = pCertificate->length + pCertificate->headerSize;

    buffer = CS_memaccess( s, pCertificate->dataOffset - pCertificate->headerSize,
                                    bytesToHash);
    if ( 0 == buffer)
    {
        status = ERR_MEM_;
        goto exit;
    }

    status = CERT_ComputeBufferHash( MOC_HASH( hwAccelCtx) (ubyte*)buffer,
                                        bytesToHash,
                                        hash,
                                        hashLen,
                                        *hashType);

exit:

    if (buffer)
    {
        CS_stopaccess( s, buffer);
    }
    return status;

} /* CERT_ComputeCertificateHash */


/*------------------------------------------------------------------*/

static MSTATUS
CERT_getCertificateExtensionsAux(ASN1_ITEM* startItem, int startIsRoot,
                                ASN1_ITEM** ppExtensions)
{
    MSTATUS status;
    ASN1_ITEM* pCertificate = NULL;
    ASN1_ITEM* pItem;

    if (NULL == startItem || NULL == ppExtensions)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *ppExtensions = NULL;

    status = GetCertificatePart( startItem, startIsRoot, &pCertificate);
    if ( OK > status)
    {
        goto exit;
    }

    /* version */
    status = ASN1_GetChildWithTag( pCertificate, 0, &pItem);
    if ( OK > status)
    {
        goto exit;
    }

    if ( NULL == pItem) /* not found */
    {
        /* version 1 by default nothing to do*/
        goto exit;
    }

    if ((pItem->id & CLASS_MASK) != UNIVERSAL ||
            pItem->tag != INTEGER)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if ( 2 != pItem->data.m_intVal)  /*v3 = 2 */
    {
        goto exit;
    }

    /* version 3 -> look for the CONTEXT tag = 3 */
    status = ASN1_GetChildWithTag( pCertificate, 3, &pItem);
    if ( OK > status)
    {
        goto exit;
    }

    if ( NULL == pItem) /* not found */
    {
        goto exit;
    }

    if ((pItem->id & CLASS_MASK) != UNIVERSAL ||
            pItem->tag != SEQUENCE)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    *ppExtensions = pItem;

exit:
    return status;

} /* CERT_getCertificateExtensionsAux */



/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertificateExtensions(ASN1_ITEM* rootItem,
                                ASN1_ITEM** ppExtensions)
{
    return CERT_getCertificateExtensionsAux( rootItem, 1, ppExtensions);
} /* CERT_getCertificateExtensions */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertificateExtensions2(ASN1_ITEM* pCertificate,
                                ASN1_ITEM** ppExtensions)
{
    return CERT_getCertificateExtensionsAux( pCertificate, 0, ppExtensions);
} /* CERT_getCertificateExtensionsCert */


/*------------------------------------------------------------------*/

static MSTATUS
CERT_VerifyCertificatePoliciesAux(ASN1_ITEM* startItem, int startIsRoot,
                                CStream s,
                                sbyte4 checkOptions,
                                sbyte4 chainLength,
                                intBoolean rootCertificate)
{
    /* basically we check the v3 X.509 extension for Basic Constraints
    !!!!!We require Basic Constraints for non root certificates!!!!!
    we also check the chain length. We also check the key usage for certificate signing.
    Depending on the value of checkOption, additional checks are performed to
    improve 3280 compliance */
    MSTATUS status;
    ASN1_ITEM* pExtensions;
    ASN1_ITEM* pExtension;
    ASN1_ITEM* pExtPart;
    intBoolean criticalExtension;
    ubyte keyBitString;

    if ( OK > (status = CERT_getCertificateExtensionsAux( startItem, startIsRoot, &pExtensions)))
        goto exit;

    /* any extensions: if no and this is not a root certificate, reject it */
    if ( NULL == pExtensions)
    {
        status = (MSTATUS) ((rootCertificate) ? OK : ERR_CERT_INVALID_INTERMEDIATE_CERTIFICATE);
        goto exit;
    }

    /* look for the children with the basicConstraint OID */
    status = CERT_getCertExtension( pExtensions, s, basicConstraints_OID,
                                &criticalExtension, &pExtension);
    if ( OK > status)
    {
        goto exit;
    }

    if ((NULL == pExtension) || (0 == pExtension->length))
    {
        status = (MSTATUS) ((rootCertificate) ? OK : ERR_CERT_INVALID_INTERMEDIATE_CERTIFICATE);
        goto exit;
    }

    /* BasicConstraintsSyntax ::= SEQUENCE {
                            cA                 BOOLEAN DEFAULT FALSE,
                            pathLenConstraint  INTEGER(0..MAX) OPTIONAL }*/

    if (  (pExtension->id & CLASS_MASK) != UNIVERSAL ||
            pExtension->tag != SEQUENCE)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* verify that it is for a CA */
    pExtPart = ASN1_FIRST_CHILD( pExtension);
    if ( NULL == pExtPart)
    {
        status = ERR_CERT_INVALID_CERT_POLICY; /* cA  BOOLEAN DEFAULT FALSE */
        goto exit;
    }
    if ( (pExtPart->id & CLASS_MASK) != UNIVERSAL ||
            pExtPart->tag != BOOLEAN)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    if ( !pExtPart->data.m_boolVal) /* not a CA */
    {
        status = ERR_CERT_INVALID_CERT_POLICY;
        goto exit;
    }

    /* verify the maximum chain length if there */
    pExtPart = ASN1_NEXT_SIBLING( pExtPart);
    if ( pExtPart)
    {
        if ( (pExtPart->id & CLASS_MASK) != UNIVERSAL ||
                pExtPart->tag != INTEGER)
        {
            status = ERR_CERT_INVALID_STRUCT;
            goto exit;
        }

        if (chainLength > (sbyte4)(1 + pExtPart->data.m_intVal))   /* chain length too big */
        {
            status = ERR_CERT_INVALID_CERT_POLICY;
            goto exit;
        }
    }

    /* look for the child with the keyUsage OID */
    status = CERT_getCertExtension( pExtensions, s, keyUsage_OID,
                                &criticalExtension, &pExtension);
    if ( OK > status)
    {
        goto exit;
    }

    if ( NULL == pExtension)
    {
        if ( checkOptions & keyUsageMandatory)
        {
            /* used to okay, now it depends on options. Can be set at compile time
             for SSL, if the certificate does not follow RFC 3280, one can override the values of
            __MOCANA_PARENT_CERT_CHECK_OPTIONS__ and __MOCANA_SELFSIGNED_CERT_CHECK_OPTIONS__ */
            status = ERR_CERT_KEYUSAGE_MISSING;
            goto exit;
        }
    }
    else /* look at the key usage extension */
    {
        /* KeyUsage ::= BIT STRING {
            digitalSignature(0), nonRepudiation(1), keyEncipherment(2),
            dataEncipherment(3), keyAgreement(4), keyCertSign(5), cRLSign(6),
            encipherOnly(7), decipherOnly(8)}

        The bit string is represented with bit 0 first.
        Examples:
        Certificate Signing, Off-line CRL Signing, CRL Signing (06)  00000110
        Digital Signature, Non-Repudiation (c0)  11000000
        Digital Signature, Key Encipherment (a0) 10100000
        Digital Signature, Non-Repudiation, Certificate Signing,
        Off-line CRL Signing, CRL Signing (c6)   11000110
        Digital Signature, Certificate Signing, Off-line CRL Signing, CRL Signing (86)
            10000110
        */

        if (  (pExtension->id & CLASS_MASK) != UNIVERSAL ||
                pExtension->tag != BITSTRING)
        {
            status = ERR_CERT_INVALID_STRUCT;
            goto exit;
        }

        /* we just look for the Certificate Signing bit (bit 5 => mask = 4) */
        CS_seek( s, pExtension->dataOffset, MOCANA_SEEK_SET);

        if (OK > (status = CS_getc(s, &keyBitString)))
            goto exit;

        if (  0 == (keyBitString & 4))    /* not supposed to be signing certificate */
        {
            status = ERR_CERT_INVALID_CERT_POLICY;
            goto exit;
        }
    }

    if ( rejectUnknownCriticalExtensions & checkOptions)
    {
        if ( OK > (status = CERT_checkForUnknownCriticalExtensions( pExtensions, s)))
        {
            goto exit;
        }
    }

    status = OK;

exit:

    return status;

} /* CERT_VerifyCertificatePoliciesAux */



/*------------------------------------------------------------------*/

extern MSTATUS
CERT_VerifyCertificatePolicies(ASN1_ITEM* rootItem,
                                CStream s,
                                sbyte4 checkOptions,
                                sbyte4 chainLength,
                                intBoolean rootCertificate)
{
    return CERT_VerifyCertificatePoliciesAux( rootItem, 1, s, checkOptions,
                                            chainLength, rootCertificate);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_VerifyCertificatePolicies2(ASN1_ITEM* pCertificate,
                                CStream s,
                                sbyte4 checkOptions,
                                sbyte4 chainLength,
                                intBoolean rootCertificate)
{
    return CERT_VerifyCertificatePoliciesAux( pCertificate, 0, s, checkOptions,
                                            chainLength, rootCertificate);
}



/*------------------------------------------------------------------*/

static MSTATUS
CERT_getCertificateKeyUsageAux(ASN1_ITEM* startItem, int rootIsStart, CStream s,
                               sbyte4 checkOptions,
                               ASN1_ITEM** ppKeyUsage)
{
    MSTATUS status;
    ASN1_ITEM* pExtensions;
    ASN1_ITEM* pExtension;
    intBoolean criticalExtension;

    if (!ppKeyUsage || !startItem )
        return ERR_NULL_POINTER;

    *ppKeyUsage = 0;

    if ( OK > (status = CERT_getCertificateExtensionsAux( startItem, rootIsStart, &pExtensions)))
        goto exit;

    /* any extensions: if no and keyUsage is mandatory, reject it */
    if ( NULL == pExtensions)
    {
        status = (MSTATUS) ((checkOptions & keyUsageMandatory) ? ERR_CERT_KEYUSAGE_MISSING : OK);
        goto exit;
    }

    /* look for the child with the keyUsage OID */
    status = CERT_getCertExtension( pExtensions, s, keyUsage_OID,
                                &criticalExtension, &pExtension);
    if ( OK > status)
    {
        goto exit;
    }

    if ( NULL == pExtension)
    {
        if ( checkOptions & keyUsageMandatory)
        {
            /* used to okay, now it depends on options. Should it be compile time? */
            status = ERR_CERT_KEYUSAGE_MISSING;
            goto exit;
        }
    }
    else /* retrieve the key usage extension */
    {
        /* KeyUsage ::= BIT STRING {
            digitalSignature(0), nonRepudiation(1), keyEncipherment(2),
            dataEncipherment(3), keyAgreement(4), keyCertSign(5), cRLSign(6),
            encipherOnly(7), decipherOnly(8)}

        The bit string is represented with bit 0 first.
        Examples:
        Certificate Signing, Off-line CRL Signing, CRL Signing (06)  00000110
        Digital Signature, Non-Repudiation (c0)  11000000
        Digital Signature, Key Encipherment (a0) 10100000
        Digital Signature, Non-Repudiation, Certificate Signing,
        Off-line CRL Signing, CRL Signing (c6)   11000110
        Digital Signature, Certificate Signing, Off-line CRL Signing, CRL Signing (86)
            10000110
        */

        if (  (pExtension->id & CLASS_MASK) != UNIVERSAL ||
                pExtension->tag != BITSTRING)
        {
            status = ERR_CERT_INVALID_STRUCT;
            goto exit;
        }

        *ppKeyUsage = pExtension;
    }

    if ( rejectUnknownCriticalExtensions & checkOptions)
    {
        if ( OK > (status = CERT_checkForUnknownCriticalExtensions( pExtensions, s)))
        {
            goto exit;
        }
    }

    status = OK;

exit:

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertificateKeyUsage(ASN1_ITEM* rootItem, CStream s,
                               sbyte4 checkOptions,
                               ASN1_ITEM** ppKeyUsage)
{
    return CERT_getCertificateKeyUsageAux( rootItem, 1, s,
                                            checkOptions, ppKeyUsage);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getCertificateKeyUsage2(ASN1_ITEM* pCertificate, CStream s,
                               sbyte4 checkOptions,
                               ASN1_ITEM** ppKeyUsage)
{
    return CERT_getCertificateKeyUsageAux( pCertificate, 0, s,
                                            checkOptions, ppKeyUsage);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_decryptRSASignatureBuffer(MOC_RSA(hwAccelDescr hwAccelCtx) RSAKey* pRSAKey,
                            const ubyte* pSignature, ubyte4 signatureLen,
                            ubyte hash[CERT_MAXDIGESTSIZE], sbyte4 *pHashLen,
                            ubyte4* rsaAlgoIdSubType)
{
    ubyte*       decrypt             = NULL;
    ubyte4       plainTextLen;
    sbyte4       cipherTextLen;
    ubyte4       digestLen = 0;
    MemFile      signatureFile;
    CStream      signatureFileStream;
    ubyte        digestSubType;
    vlong*       pVlongQueue = NULL;
    ASN1_ITEM*   pItem;
    ASN1_ITEM*   pDigest;
    ASN1_ITEM*   pAlgoId;
    ASN1_ITEM*   pDecryptedSignature = NULL;

    MSTATUS      status;

    if ((NULL == pSignature) || (NULL == hash) ||
        (NULL == pRSAKey) || (NULL == rsaAlgoIdSubType))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    status = RSA_getCipherTextLength(pRSAKey, &cipherTextLen);
    if ( OK > status)
    {
        goto exit;
    }

    if (signatureLen != (ubyte4)cipherTextLen)
    {
        status = ERR_CERT_INVALID_SIGNATURE;
        goto exit;
    }

    if (10 > cipherTextLen)     /*!-!-!-! to prevent static analyzer warnings, maybe we should have a function to sanity check cipher len?  */
    {
        status = ERR_CERT_INVALID_SIGNATURE;
        goto exit;
    }

    decrypt = MALLOC( (/*FSL*/ubyte4)cipherTextLen);
    if (NULL == decrypt)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    status = RSA_verifySignature(MOC_RSA(hwAccelCtx) pRSAKey, pSignature, decrypt, &plainTextLen, &pVlongQueue);
    if ( OK > status )
    {
        goto exit;
    }

    /* decrypt: the first plainTextLen bytes contains a ASN.1 encoded sequence */

#if 0
    MOCANA_writeFile( "signature.der", decrypt, plainTextLen);
#endif

    MF_attach( &signatureFile, (/*FSL*/sbyte4)plainTextLen, decrypt);
    CS_AttachMemFile( &signatureFileStream, &signatureFile);

    status = ASN1_Parse( signatureFileStream, &pDecryptedSignature);
    if (OK > status)
    {
        goto exit;
    }

    pItem = ASN1_FIRST_CHILD( pDecryptedSignature);
    if ( 0 == pItem)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    /* we need to verify that the ASN1 DER is plainTextLen bytes long...
        to defeat the BleichenBacher technique of certificate forgery --
        RSA Signature Forgery (CVE-2006-4339)
    */
    if (pItem->headerSize + pItem->length != plainTextLen)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    pAlgoId = ASN1_FIRST_CHILD( pItem);
    if ( 0 == pAlgoId)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* REVIEW: fix this so that GetCertOID is not called several times */
    /* the signature is a again a sequence with the first item a sequence
        with an OID */

    if (OK <= (status = GetCertOID( pAlgoId, signatureFileStream, rsaDSI_OID,
                                    &digestSubType, NULL)))
    {
        /* convert digestSubType */
        switch (digestSubType)
        {
            case md2Digest:
                *rsaAlgoIdSubType = md2withRSAEncryption;
                digestLen = MD2_RESULT_SIZE;
                break;

            case md4Digest:
                *rsaAlgoIdSubType = md4withRSAEncryption;
                digestLen = MD4_RESULT_SIZE;
                break;

            case md5Digest:
                *rsaAlgoIdSubType = md5withRSAEncryption;
                digestLen = MD5_RESULT_SIZE;
                break;

            default:
                *rsaAlgoIdSubType = (ubyte4) -1;
                break;
        }
    }
    else if (OK <= (status = GetCertOID( pAlgoId, signatureFileStream, sha2_OID,
                                        &digestSubType, NULL)))
    {
        switch (digestSubType)
        {
        case sha224Digest:
            *rsaAlgoIdSubType = sha224withRSAEncryption;
            digestLen = SHA224_RESULT_SIZE;
            break;

        case sha256Digest:
            *rsaAlgoIdSubType = sha256withRSAEncryption;
            digestLen = SHA256_RESULT_SIZE;
            break;

        case sha384Digest:
            *rsaAlgoIdSubType = sha384withRSAEncryption;
            digestLen = SHA384_RESULT_SIZE;
            break;

        case sha512Digest:
            *rsaAlgoIdSubType = sha512withRSAEncryption;
            digestLen = SHA512_RESULT_SIZE;
            break;

        default:
            *rsaAlgoIdSubType = (ubyte4) -1;
            break;
        }
    }
    else if (OK <= (status = GetCertOID( pAlgoId, signatureFileStream, sha1_OID,
                                        NULL, NULL)))
    {
        *rsaAlgoIdSubType = sha1withRSAEncryption;
        digestLen = SHA1_RESULT_SIZE;
    }
    else if (OK <= (status = GetCertOID( pAlgoId, signatureFileStream, sha1withRsaSignature_OID,
                                        NULL, NULL)))
    {
        *rsaAlgoIdSubType = sha1withRSAEncryption;
        digestLen = SHA1_RESULT_SIZE;
    }
    else /* no match */
    {
        goto exit;
    }

    status = ASN1_GetNthChild( pItem, 2, &pDigest);
    if ( OK > status)
    {
        goto exit;
    }

    if (pDigest->length != digestLen)
    {
        status = ERR_CERT_INVALID_SIGNATURE;
        goto exit;
    }

    if ((plainTextLen - pDigest->dataOffset) != digestLen)
    {
        /* Prevent Bleichenbacher's RSA signature forgery */
        /* the hash should be right-justified in the signature buffer */
        status = ERR_CERT_INVALID_SIGNATURE;
        goto exit;
    }

    /* copy the Hash */
    MOC_MEMSET( hash, 0, CERT_MAXDIGESTSIZE);
    MOC_MEMCPY( hash, decrypt + pDigest->dataOffset, (/*FSL*/sbyte4)digestLen);
    *pHashLen = (/*FSL*/sbyte4)digestLen;

exit:
    VLONG_freeVlongQueue(&pVlongQueue);

    if ( pDecryptedSignature)
    {
        TREE_DeleteTreeItem( (TreeItem*) pDecryptedSignature);
    }

    if (decrypt)
    {
        FREE(decrypt);
    }

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_getSignatureItem(ASN1_ITEM* rootItem, CStream s, ASN1_ITEM** ppSignature)
{
    static WalkerStep signatureWalkInstructions[] =
    {
        { GoFirstChild, 0, 0},
        { GoNthChild, 3, 0},
        { VerifyType, BITSTRING, 0 },
        { Complete, 0, 0}
    };

    if ( OK > ASN1_WalkTree( rootItem, s, signatureWalkInstructions, ppSignature))
    {
        return ERR_CERT_INVALID_STRUCT;
    }

    return OK;

} /* CERT_getSignatureItem */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_decryptRSASignature(MOC_RSA(hwAccelDescr hwAccelCtx) ASN1_ITEM* rootItem,
                      CStream s,
                      RSAKey* pRSAKey,
                      ubyte* hash,
                      sbyte4* hashLen,
                      ubyte4* rsaAlgoIdSubType)
{
    const ubyte* buffer              = NULL;
    ASN1_ITEMPTR pSignature;
    MSTATUS      status;

    if (NULL == s.pFuncs->m_memaccess)
    {
        status = ERR_ASN_NULL_FUNC_PTR;
        goto exit;
    }

    if ((NULL == rootItem) || (NULL == hash) || (NULL == rsaAlgoIdSubType))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* get the signature */
    if (OK > ( status = CERT_getSignatureItem( rootItem, s, &pSignature)))
        goto exit;

    /* access the buffer */
    buffer = CS_memaccess( s, (/*FSL*/sbyte4)pSignature->dataOffset, (/*FSL*/sbyte4)pSignature->length);
    if (NULL == buffer)
    {
        status = ERR_MEM_;
        goto exit;
    }

    status = CERT_decryptRSASignatureBuffer(MOC_RSA(hwAccelCtx) pRSAKey, buffer, pSignature->length,
                                            hash, hashLen, rsaAlgoIdSubType);
    if ( OK > status )
    {
        goto exit;
    }

exit:

    if ( buffer)
    {
        CS_stopaccess( s, buffer);
    }

    return status;

} /* CERT_DecryptSignature */


/*---------------------------------------------------------------------------*/

static MSTATUS
CERT_verifyRSACertSignature( MOC_RSA(hwAccelDescr hwAccelCtx)
                            ASN1_ITEM* pPrevCertificate,
                            CStream pPrevCertStream,
                            RSAKey* pRSAKey,
                            ubyte4 computedHashType,
                            sbyte4 computedHashLen,
                            const ubyte computedHash[CERT_MAXDIGESTSIZE])
{
    MSTATUS status;
    ubyte   decryptedHash[CERT_MAXDIGESTSIZE];
    ubyte4  decryptedHashType;
    sbyte4  decryptedHashLen;
    sbyte4  result;

    /* decrypt the signature in the certificate */
    if (OK > (status = CERT_decryptRSASignature(MOC_RSA(hwAccelCtx)
                            pPrevCertificate,
                            pPrevCertStream,
                            pRSAKey,
                            decryptedHash,
                            &decryptedHashLen,
                            &decryptedHashType)))
    {
        return status;
    }

    if (decryptedHashType != computedHashType ||
        decryptedHashLen != computedHashLen)
    {
        return ERR_CERT_INVALID_SIGNATURE;
    }

    if (OK > ( status = MOC_MEMCMP( computedHash, decryptedHash,
            (/*FSL*/ubyte4)decryptedHashLen, &result)))
    {
        return status;
    }

    return (( 0 == result) ? OK : ERR_CERT_INVALID_SIGNATURE);
}


/*---------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
extern MSTATUS
CERT_verifyECDSASignature( ASN1_ITEM* pSequence, CStream cs, ECCKey* pECCKey,
                            sbyte4 computedHashLen, const ubyte computedHash[])
{
    MSTATUS         status;
    ASN1_ITEMPTR    pItem;
    const ubyte*    buffer = 0;
    PFEPtr          r = 0, s = 0;
    PrimeFieldPtr   pPF = 0;

    if ( !pSequence || !pECCKey || !computedHash)
        return ERR_NULL_POINTER;

    pPF = EC_getUnderlyingField( pECCKey->pCurve);

    if (OK > ( status = PRIMEFIELD_newElement( pPF, &r)))
        goto exit;

    if (OK > ( status = PRIMEFIELD_newElement( pPF, &s)))
        goto exit;

    /* read R */
    pItem = ASN1_FIRST_CHILD( pSequence);
    if (OK > (status = ASN1_VerifyType(pItem, INTEGER)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    buffer = CS_memaccess( cs, pItem->dataOffset, pItem->length);
    if (!buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    if (OK > ( status = PRIMEFIELD_setToByteString( pPF, r, buffer, pItem->length)))
        goto exit;

    CS_stopaccess( cs, buffer);
    buffer = 0;

    /* read S */
    pItem = ASN1_NEXT_SIBLING(pItem);
    if ( OK > ASN1_VerifyType( pItem, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    buffer = CS_memaccess( cs, pItem->dataOffset, pItem->length);
    if (!buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    if (OK > ( status = PRIMEFIELD_setToByteString( pPF, s, buffer, pItem->length)))
        goto exit;

    CS_stopaccess( cs, buffer);
    buffer = 0;

    if ( OK > (status = ECDSA_verifySignature( pECCKey->pCurve, pECCKey->Qx, pECCKey->Qy,
                                    computedHash, computedHashLen, r, s)))
    {
        status = ERR_CERT_INVALID_SIGNATURE;
        goto exit;
    }
exit:

    if ( buffer)
    {
        CS_stopaccess( cs, buffer);
    }

    PRIMEFIELD_deleteElement( pPF, &r);
    PRIMEFIELD_deleteElement( pPF, &s);

    return status;
}
#endif


/*---------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_ECC__))
static MSTATUS
CERT_verifyECDSACertSignature( ASN1_ITEM* pCertificate,
                            CStream pCertStream,
                            ECCKey* pECCKey,
                            ubyte4 computedHashType,
                            sbyte4 computedHashLen,
                            const ubyte computedHash[CERT_MAXDIGESTSIZE])
{
    MSTATUS         status;
    ASN1_ITEM*      pSignature;
    ASN1_ITEM*      pSequence;

    MOC_UNUSED(computedHashType);

    if (OK > ( status = CERT_getSignatureItem( pCertificate, pCertStream, &pSignature)))
    {
        goto exit;
    }

    pSequence = ASN1_FIRST_CHILD( pSignature);
    if ( OK > ( status = ASN1_VerifyType( pSequence, SEQUENCE)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* call the exported routine */
    status = CERT_verifyECDSASignature( pSequence, pCertStream, pECCKey,
                                        computedHashLen, computedHash);
exit:
    return status;
}
#endif




/*---------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DSA__))
extern MSTATUS
CERT_verifyDSASignature( ASN1_ITEM* pSequence, CStream cs, DSAKey* pDSAKey,
                            sbyte4 computedHashLen, const ubyte computedHash[])
{
    MSTATUS         status;
    ASN1_ITEMPTR    pItem;
    intBoolean      good;
    const ubyte*    buffer = 0;
    vlong*          pR = 0;
    vlong*          pS = 0;

    if ( !pSequence || !pDSAKey || !computedHash)
        return ERR_NULL_POINTER;

    /* read R */
    pItem = ASN1_FIRST_CHILD( pSequence);
    if (OK > (status = ASN1_VerifyType(pItem, INTEGER)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    buffer = CS_memaccess( cs, pItem->dataOffset, pItem->length);
    if (!buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    if (OK > ( status = VLONG_vlongFromByteString(buffer, pItem->length, &pR, NULL)))
        goto exit;

    CS_stopaccess( cs, buffer);
    buffer = 0;

    /* read S */
    pItem = ASN1_NEXT_SIBLING(pItem);
    if ( OK > ASN1_VerifyType( pItem, INTEGER))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }
    buffer = CS_memaccess( cs, pItem->dataOffset, pItem->length);
    if (!buffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    if (OK > ( status = VLONG_vlongFromByteString( buffer, pItem->length, &pS, NULL)))
        goto exit;

    CS_stopaccess( cs, buffer);
    buffer = 0;

    if ( OK > (status = DSA_verifySignature2( pDSAKey,
                                                computedHash, computedHashLen,
                                                pR, pS, &good, NULL)))
    {
        goto exit;
    }

    if (!good)
    {
        status = ERR_CERT_INVALID_SIGNATURE;
        goto exit;
    }

exit:

    if ( buffer)
    {
        CS_stopaccess( cs, buffer);
    }

    VLONG_freeVlong( &pR, NULL);
    VLONG_freeVlong( &pS, NULL);

    return status;
}
#endif


/*---------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_DSA__))
static MSTATUS
CERT_verifyDSACertSignature( ASN1_ITEM* pCertificate,
                            CStream pCertStream,
                            DSAKey* pECCKey,
                            ubyte4 computedHashType,
                            sbyte4 computedHashLen,
                            const ubyte computedHash[CERT_MAXDIGESTSIZE])
{
    MSTATUS         status;
    ASN1_ITEM*      pSignature;
    ASN1_ITEM*      pSequence;

    MOC_UNUSED(computedHashType);

    if (OK > ( status = CERT_getSignatureItem( pCertificate, pCertStream, &pSignature)))
    {
        goto exit;
    }

    pSequence = ASN1_FIRST_CHILD( pSignature);
    if ( OK > ( status = ASN1_VerifyType( pSequence, SEQUENCE)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* call the exported routine */
    status = CERT_verifyDSASignature( pSequence, pCertStream, pECCKey,
                                        computedHashLen, computedHash);
exit:
    return status;
}
#endif


/*---------------------------------------------------------------------------*/

extern MSTATUS
CERT_verifySignature( MOC_RSA(hwAccelDescr hwAccelCtx) ASN1_ITEM *pCertOrCRL, CStream cs, AsymmetricKey *pIssuerPubKey)
{
    MSTATUS status = OK;
    ubyte          pComputedHash[CERT_MAXDIGESTSIZE];
    ubyte4          computedHashType;
    sbyte4          computedHashLen;
    ubyte4          pubKeyType;


    /* check certificate signature */
    /* compute the hash of the prevCertificate according to the algorithm */
    if ( OK > (status = CERT_ComputeCertificateHash(MOC_HASH(hwAccelCtx)
                                            pCertOrCRL,
                                            cs,
                                            pComputedHash,
                                            &computedHashLen,
                                            &computedHashType,
                                            &pubKeyType)))
    {
        goto exit;
    }

    if (pIssuerPubKey->type != pubKeyType)
    {
        status = ERR_CERT_KEY_SIGNATURE_OID_MISMATCH;
        goto exit;
    }

    switch( pIssuerPubKey->type)
    {
        case akt_rsa:
        {
            status = CERT_verifyRSACertSignature( MOC_RSA(hwAccelCtx)
                                        pCertOrCRL, cs,
                                        pIssuerPubKey->key.pRSA,
                                        computedHashType,
                                        computedHashLen,
                                        pComputedHash);
            break;
        }
#if (defined(__ENABLE_MOCANA_DSA__))
        case akt_dsa:
        {
            status = CERT_verifyDSACertSignature(pCertOrCRL,
                                        cs,
                                        pIssuerPubKey->key.pDSA,
                                        computedHashType,
                                        computedHashLen,
                                        pComputedHash);
            break;
        }
#endif
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            status = CERT_verifyECDSACertSignature(pCertOrCRL,
                                        cs,
                                        pIssuerPubKey->key.pECC,
                                        computedHashType,
                                        computedHashLen,
                                        pComputedHash);
            break;
        }
#endif
        default:
        {
            status = ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO;
            break;
        }
    }

exit:
    return status;
}


/*---------------------------------------------------------------------------*/

static MSTATUS
CERT_isRootCertificateAux(ASN1_ITEM* startItem, int startIsRoot, CStream s)
{
    /* this functions returns OK, ERR_FALSE or an error */
    MSTATUS status;
    ASN1_ITEM* pExtensions;
    ASN1_ITEM* pSKIExtension;
    ASN1_ITEM* pAKIExtension;
    ASN1_ITEM* pAKIKeyID;
    intBoolean critical;

    /* is certificate subject equal to certificate issuer? */
    if ( OK > ( status = CERT_checkCertificateIssuerAux( startItem, startIsRoot, s,
                                                        startItem, startIsRoot, s)))
    {
        return ( ERR_CERT_INVALID_PARENT_CERTIFICATE == status) ?
                ERR_FALSE: status;
    }

    /* issuer = subject so check for Authority Key Identifier and Subject Key Identifier */
    if ( OK > ( status = CERT_getCertificateExtensionsAux( startItem, startIsRoot, &pExtensions)))
    {
        return status;
    }

    /* no extensions */
    if ( !pExtensions)
    {
        return OK;
    }
    /* look for the Subject Key Extension */
    if (OK > ( status = CERT_getCertExtension( pExtensions, s,
                                            subjectKeyIdentifier_OID,
                                            &critical, &pSKIExtension)))
    {
        return status;
    }

    if (!pSKIExtension)
        return OK;

    if (OK > ( status = CERT_getCertExtension( pExtensions, s,
                                            authorityKeyIdentifier_OID,
                                            &critical, &pAKIExtension)))
    {
        return status;
    }

    if (!pAKIExtension)
        return OK;

    /* compare key identifiers */
    /* we can do that only if tag [0]
    KeyIdentifier ::= OCTET STRING
    AuthorityKeyIdentifier ::= SEQUENCE {
        keyIdentifier              [0]  KeyIdentifier OPTIONAL,
        authorityCertIssuer        [1]  GeneralNames OPTIONAL,
        authorityCertSerialNumber  [2]  CertificateSerialNumber OPTIONAL
    SubjectKeyIdentifier ::= KeyIdentifier
    */
    if ( OK > ( status = ASN1_GoToTag( pAKIExtension, 0, &pAKIKeyID)))
    {
        return status;
    }

    if (!pAKIKeyID)
    {
        return ERR_FALSE; /* assume different keys */
    }
    return ASN1_CompareItems( pAKIKeyID, s, pSKIExtension, s);
}


/*---------------------------------------------------------------------------*/

extern MSTATUS
CERT_isRootCertificate(ASN1_ITEM* rootItem, CStream s)
{
    return CERT_isRootCertificateAux(rootItem, 1, s);
}


/*---------------------------------------------------------------------------*/

extern MSTATUS
CERT_isRootCertificate2(ASN1_ITEM* pCertificate, CStream s)
{
    return CERT_isRootCertificateAux(pCertificate, 0, s);
}



/*---------------------------------------------------------------------------*/

static MSTATUS
CERT_validateCertificateAux(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pPrevCertificate,
                            CStream pPrevCertStream,
                            ASN1_ITEM* pCertificate,
                            CStream pCertStream,
                            sbyte4 checkOptions,
                            sbyte4 chainLength,
                            intBoolean rootCertificate,
                            intBoolean chkValidityBefore,
                            intBoolean chkValidityAfter)
{
    ubyte*          pComputedHash = NULL;
    AsymmetricKey   certKey;
    ubyte4          computedHashType;
    ubyte4          pubKeyType;
    sbyte4          computedHashLen;
    MSTATUS         status;

    if ((NULL == pPrevCertificate) || (NULL == pCertificate))
        return ERR_NULL_POINTER;

    if ( OK > ( status = CRYPTO_initAsymmetricKey( &certKey)))
        goto exit;

    if (OK > (status = CRYPTO_ALLOC(hwAccelCtx, CERT_MAXDIGESTSIZE, TRUE, (void *)&pComputedHash)))
        goto exit;

    /* verify time validity of pCertificate */
    if((FALSE != chkValidityBefore) || (FALSE != chkValidityAfter))
    {
        if (OK > (status = CERT_VerifyValidityTimeWithConf(pCertificate, pCertStream, chkValidityBefore, chkValidityAfter)))
            goto exit;
    }


    /* verify issuer of pPrevCertificate == subject of pCertificate */
    if (OK > (status = CERT_checkCertificateIssuer(pPrevCertificate,
                                         pPrevCertStream,
                                         pCertificate,
                                         pCertStream)))
    {
        goto exit;
    }

    /* compute the hash of the prevCertificate according to the algorithm */
    if ( OK > (status = CERT_ComputeCertificateHash(MOC_HASH(hwAccelCtx)
                                            pPrevCertificate,
                                            pPrevCertStream,
                                            pComputedHash,
                                            &computedHashLen,
                                            &computedHashType,
                                            &pubKeyType)))
    {
        goto exit;
    }
    /* verify that the certificate is authorized to be used to sign other certificates */
    if (OK > (status = CERT_VerifyCertificatePolicies( pCertificate,
                                                pCertStream,
                                                checkOptions,
                                                chainLength,
                                                rootCertificate)))
    {
        goto exit;
    }

    /* extract the public key of the certificate */
    if (OK > (status = CERT_setKeyFromSubjectPublicKeyInfo(MOC_RSA(hwAccelCtx) pCertificate, pCertStream, &certKey)))
        goto exit;

    if ( certKey.type != pubKeyType)
    {
        status = ERR_CERT_KEY_SIGNATURE_OID_MISMATCH;
        goto exit;
    }

    switch( certKey.type)
    {
        case akt_rsa:
        {
            status = CERT_verifyRSACertSignature( MOC_RSA(hwAccelCtx)
                                        pPrevCertificate, pPrevCertStream,
                                        certKey.key.pRSA,
                                        computedHashType,
                                        computedHashLen,
                                        pComputedHash);
            break;
        }
#if (defined(__ENABLE_MOCANA_DSA__))
        case akt_dsa:
        {
             status = CERT_verifyDSACertSignature(pPrevCertificate,
                                        pPrevCertStream,
                                        certKey.key.pDSA,
                                        computedHashType,
                                        computedHashLen,
                                        pComputedHash);
             break;
        }
#endif
#if (defined(__ENABLE_MOCANA_ECC__))
        case akt_ecc:
        {
            status = CERT_verifyECDSACertSignature(pPrevCertificate,
                                        pPrevCertStream,
                                        certKey.key.pECC,
                                        computedHashType,
                                        computedHashLen,
                                        pComputedHash);
            break;
        }
#endif
        default:
        {
            status = ERR_CERT_UNSUPPORTED_SIGNATURE_ALGO;
            break;
        }
    }

exit:
    CRYPTO_FREE(hwAccelCtx, TRUE, (void *)&pComputedHash);
    CRYPTO_uninitAsymmetricKey(&certKey, NULL);

    return status;
}

/*---------------------------------------------------------------------------*/

extern MSTATUS
CERT_validateCertificate(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pPrevCertificate,
                         CStream pPrevCertStream,
                         ASN1_ITEM* pCertificate,
                         CStream pCertStream,
                         sbyte4 checkOptions,
                         sbyte4 chainLength,
                         intBoolean rootCertificate)
{
    return CERT_validateCertificateAux(MOC_RSA_HASH(hwAccelCtx) pPrevCertificate,
                                       pPrevCertStream,
                                       pCertificate,
                                       pCertStream,
                                       checkOptions,
                                       chainLength,
                                       rootCertificate,
                                       TRUE, TRUE);
}

/*---------------------------------------------------------------------------*/

extern MSTATUS
CERT_validateCertificateWithConf(MOC_RSA_HASH(hwAccelDescr hwAccelCtx) ASN1_ITEM* pPrevCertificate,
                                 CStream pPrevCertStream,
                                 ASN1_ITEM* pCertificate,
                                 CStream pCertStream,
                                 sbyte4 checkOptions,
                                 sbyte4 chainLength,
                                 intBoolean rootCertificate,
                                 intBoolean chkValidityBefore,
                                 intBoolean chkValidityAfter)
{
    return CERT_validateCertificateAux(MOC_RSA_HASH(hwAccelCtx) pPrevCertificate,
                                       pPrevCertStream,
                                       pCertificate,
                                       pCertStream,
                                       checkOptions,
                                       chainLength,
                                       rootCertificate,
                                       chkValidityBefore,
                                       chkValidityAfter);
}

/*------------------------------------------------------------------*/

static MSTATUS
extractDistinguishedNameFields(ASN1_ITEM* pOID, CStream s,
                               nameAttr *pRetNameComponent)
{
    ubyte*          pDistinguishedElementCopy = NULL;
    long            dnElementLen;
    ubyte           ch;
    MSTATUS         status = OK;
    ASN1_ITEM*      pDistinguishedElement = ASN1_NEXT_SIBLING( pOID);
    const ubyte*    oid = NULL;
    long            copyIndex;

    /* match 2.5.4.n */
    if ( NULL == pDistinguishedElement)
    {
        status = ERR_CERT_UNRECOGNIZED_OID;
        goto exit;
    }

    CS_seek(s, pOID->dataOffset, MOCANA_SEEK_SET);

    if (3 == pOID->length)
    {
        ubyte   distinguishedDigit1;
        ubyte   distinguishedDigit2;

        /* verify first two digits match, 2.5.4.n */
        if (OK > (status = CS_getc(s, &distinguishedDigit1)))
            goto exit;

        if (OK > (status = CS_getc(s, &distinguishedDigit2)))
            goto exit;

        if ((0x55 == distinguishedDigit1) && (0x04 == distinguishedDigit2))
        {
            if (OK > (status = CS_getc(s, &ch)))
                goto exit;

            switch (ch)
            {
            case kCountryName:
                {
                    oid = countryName_OID;
                    break;
                }

            case kStateOrProvidenceName:
                {
                    oid = stateOrProvinceName_OID;
                    break;
                }

            case kLocality:
                {
                    oid = localityName_OID;
                    break;
                }

            case kOrganizationName:
                {
                    oid = organizationName_OID;
                    break;
                }

            case kOrganizationUnitName:
                {
                    oid = organizationalUnitName_OID;
                    break;
                }

            case kCommonName:
                {
                    oid = commonName_OID;
                    break;
                }

			case kSerialNumber:
				{
					oid = serialNumber_OID;
					break;
				}

            default:
                status = ERR_CERT_UNRECOGNIZED_OID;
                goto exit;
            }
        } else
        {
            /* unrecognized name component */
            status = ERR_CERT_UNRECOGNIZED_OID;
            goto exit;
        }
    } else if (10 == pOID->length)
    {
        /* match 0.9.2342.19200300.100.1.x */
        ubyte4 i;
        for (i = 1; i <= 9; i++)
        {
            if (OK > (status = CS_getc(s, &ch)))
                goto exit;
            if (ch != domainComponent_OID[i])
            {
                /* unrecognized name component */
                status = ERR_CERT_UNRECOGNIZED_OID;
                goto exit;
            }
        }
        if (OK > (status = CS_getc(s, &ch)))
	    goto exit;
	    switch (ch)
	    {
        case 0x01: /* userID */
            oid = userID_OID;
            break;
        case 0x19: /* domainComponent = 25 */
            oid = domainComponent_OID;
            break;
        default:
            /* unrecognized name component */
            status = ERR_CERT_UNRECOGNIZED_OID;
            goto exit;
        }
    } else if (9 == pOID->length)
    {
        /* Match 1.2.840.113549.1.9.X */
        ubyte4 i;
        for (i = 1; i <= 8; i++)
        {
            if (OK > (status = CS_getc(s, &ch)))
                goto exit;
            if (ch != pkcs9_emailAddress_OID[i])
            {
                /* unrecognized name component, ignored */
                status = ERR_CERT_UNRECOGNIZED_OID;
                goto exit;
            }
        }
		if (OK > (status = CS_getc(s, &ch)))
			goto exit;
		switch (ch)
		{
			case 0x01: /*pkcs9_emailAddress*/
				{
					oid = pkcs9_emailAddress_OID;
					break;
				}
			case 0x02: /*pkcs9_unstructuredName*/
				{
					oid = pkcs9_unstructuredName_OID;
					break;
				}
			default:
				{
					status = ERR_CERT_UNRECOGNIZED_OID;
					goto exit;
				}
		}
    }
	else
	{
		status = ERR_CERT_UNRECOGNIZED_OID;
		goto exit;
	}

    /* copy data byte by byte */
    CS_seek(s, pDistinguishedElement->dataOffset, MOCANA_SEEK_SET);

    dnElementLen = pDistinguishedElement->length;

    /* prevent over-sized mallocs */
    if (MAX_DNE_STRING_LENGTH < dnElementLen)
    {
        status = ERR_CERT_DNE_STRING_TOO_LONG;
        goto exit;
    }

    /* make sure the cert common name is same length as the expected name */
    if (0 != dnElementLen)
    {
        if (NULL == (pDistinguishedElementCopy = MALLOC(dnElementLen)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        for (copyIndex = 0; copyIndex < dnElementLen; copyIndex++)
        {
            if (OK > (status = CS_getc(s, &ch)))
                goto exit;

            pDistinguishedElementCopy[copyIndex] = ch;
        }
    }
    pRetNameComponent->oid      = oid;
    pRetNameComponent->type     = (/*FSL*/ubyte)pDistinguishedElement->tag;
    pRetNameComponent->value    = pDistinguishedElementCopy;
    pDistinguishedElementCopy   = NULL;
    pRetNameComponent->valueLen = pDistinguishedElement->length;

exit:
    if (NULL != pDistinguishedElementCopy)
        FREE(pDistinguishedElementCopy);

    return status;

} /* extractDistinguishedNameFields */


/*------------------------------------------------------------------*/

extern ubyte4
CERT_getNumberOfChild(ASN1_ITEM *pParent)
{
    ubyte4 count = 0;
    ASN1_ITEM *pItem;

    pItem = ASN1_FIRST_CHILD(pParent);
    while (NULL != pItem)
    {
        count++;
        pItem = ASN1_NEXT_SIBLING(pItem);
    }
    return count;
}

/*------------------------------------------------------------------*/

/* mocana internal API */
extern MSTATUS
CERT_extractDistinguishedNamesFromName(ASN1_ITEM* pName, CStream s,
                                       certDistinguishedName *pRetDN)
{
    MSTATUS     status;
    ASN1_ITEM*  pCurrChild;
    ubyte4      rdnOffset;
    ubyte4      nameAttrOffset;

    /* now traverse each child with OID 2.5.4.n */
    /*  Name ::= SEQUENCE of RelativeDistinguishedName
        RelativeDistinguishedName = SET of AttributeValueAssertion
        AttributeValueAssertion = SEQUENCE { attributeType OID; attributeValue ANY }
    */

    /* Name is a sequence */
    if ((NULL == pName) ||
        ((pName->id & CLASS_MASK) != UNIVERSAL) ||
        (pName->tag != SEQUENCE))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    pRetDN->dnCount = CERT_getNumberOfChild(pName);
    if (NULL == (pRetDN->pDistinguishedName = MALLOC(pRetDN->dnCount * sizeof(relativeDN))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }
    MOC_MEMSET((ubyte*)pRetDN->pDistinguishedName, 0, (/*FSL*/sbyte4)(pRetDN->dnCount * sizeof(relativeDN)));

    pCurrChild = ASN1_FIRST_CHILD( pName);

    rdnOffset = 0;
    while (pCurrChild)
    {
        ASN1_ITEM* pGrandChild;
        ASN1_ITEM* pOID;

        status = ERR_CERT_INVALID_STRUCT;

        /* child should be a SET */
        if (((pCurrChild->id & CLASS_MASK) != UNIVERSAL) ||
            (pCurrChild->tag != SET) )
        {
            goto exit;
        }
        (pRetDN->pDistinguishedName+rdnOffset)->nameAttrCount = CERT_getNumberOfChild(pCurrChild);
        if (NULL == ( (pRetDN->pDistinguishedName+rdnOffset)->pNameAttr = MALLOC((pRetDN->pDistinguishedName+rdnOffset)->nameAttrCount * sizeof(nameAttr))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }
        MOC_MEMSET((ubyte*)(pRetDN->pDistinguishedName+rdnOffset)->pNameAttr, 0, (/*FSL*/sbyte4)((pRetDN->pDistinguishedName+rdnOffset)->nameAttrCount * sizeof(nameAttr)));

        /* GrandChild should be a SEQUENCE */
        nameAttrOffset = 0;
        pGrandChild = ASN1_FIRST_CHILD( pCurrChild);
        while (pGrandChild)
        {
            if ( NULL == pGrandChild ||
                (pGrandChild->id & CLASS_MASK) != UNIVERSAL ||
                pGrandChild->tag != SEQUENCE)
            {
                goto exit;
            }

            /* get the OID */
            pOID = ASN1_FIRST_CHILD( pGrandChild);

            if ((NULL == pOID) ||
                ((pOID->id & CLASS_MASK) != UNIVERSAL) ||
                (pOID->tag != OID) )
            {
                goto exit;
            }

            /* which OID as a member of distinguished name? */
            if (3 == pOID->length || 10 == pOID->length || 9 == pOID->length) /* userID and domainComponent oids have length 10 */
            {
                if (OK > (status = extractDistinguishedNameFields(pOID, s, (pRetDN->pDistinguishedName+rdnOffset)->pNameAttr+nameAttrOffset)))
                    goto exit;
            }
			else
			{
				status = ERR_CERT_UNRECOGNIZED_OID;
				goto exit;
			}
            pGrandChild = ASN1_NEXT_SIBLING(pGrandChild);
            nameAttrOffset++;
        }
        pCurrChild = ASN1_NEXT_SIBLING( pCurrChild);
        rdnOffset++;
    }

    status = OK;
exit:
    return status;

}

/*------------------------------------------------------------------*/

static MSTATUS
CERT_extractDistinguishedNamesAux(ASN1_ITEM* startItem, int startIsRoot, CStream s,
                               intBoolean isSubject, certDistinguishedName *pRetDN)
{
    ASN1_ITEM*  pCertificate = NULL;
    ASN1_ITEM*  pIssuer;
    ASN1_ITEM*  pVersion;
    MSTATUS     status;

    if ((NULL == startItem) || (NULL == pRetDN))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = GetCertificatePart( startItem, startIsRoot, &pCertificate)))
        goto exit;

    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pVersion)))
        goto exit;

    /* extract distinguished name from subject or issuer */
    if (TRUE == isSubject)
    {
        /* subject is 5th or 6th child */
        if (OK > (status = ASN1_GetNthChild( pCertificate, pVersion ? 6 : 5, &pIssuer)))
            goto exit;
    }
    else
    {
        /* issuer is 3rd or 4th child */
        if (OK > (status = ASN1_GetNthChild( pCertificate, pVersion ? 4 : 3, &pIssuer)))
            goto exit;
    }

    status = CERT_extractDistinguishedNamesFromName(pIssuer, s, pRetDN);

exit:
    return status;

} /* CERT_extractDistinguishedNamesAux */

/*------------------------------------------------------------------*/

extern MSTATUS
CERT_extractDistinguishedNames(ASN1_ITEM* rootItem, CStream s,
                               intBoolean isSubject, certDistinguishedName *pRetDN)
{
    return CERT_extractDistinguishedNamesAux( rootItem, 1, s, isSubject, pRetDN);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_extractDistinguishedNames2(ASN1_ITEM* pCertificate, CStream s,
                               intBoolean isSubject, certDistinguishedName *pRetDN)
{
    return CERT_extractDistinguishedNamesAux( pCertificate, 0, s, isSubject, pRetDN);
}


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_EXTRACT_CERT_BLOB__
extern MSTATUS
CERT_extractDistinguishedNamesBlob(ASN1_ITEM* rootItem, CStream s,
                                   intBoolean isSubject,
                                   ubyte **ppRetDistinguishedName, ubyte4 *pRetDistinguishedNameLen)
{
    ASN1_ITEM*  pCertificate;
    ASN1_ITEM*  pIssuer;
    ASN1_ITEM*  pVersion;
    ubyte*      pMemBlock;
    ubyte4      copyIndex;
    ubyte       ch;
    MSTATUS     status;

    if ((NULL == rootItem) || (NULL == ppRetDistinguishedName) || (NULL == pRetDistinguishedNameLen))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = GetCertificatePart( rootItem, 1, &pCertificate)))
        goto exit;

    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pVersion)))
        goto exit;

    /* extract distinguished name from subject or issuer */
    if (TRUE == isSubject)
    {
        /* subject is 5th or 6th child */
        if (OK > (status = ASN1_GetNthChild( pCertificate, pVersion ? 6 : 5, &pIssuer)))
            goto exit;
    }
    else
    {
        /* issuer is 3rd or 4th child */
        if (OK > (status = ASN1_GetNthChild( pCertificate, pVersion ? 4 : 3, &pIssuer)))
            goto exit;
    }

    /* now traverse each child with OID 2.5.4.n */
    /*  Name ::= SEQUENCE of RelativeDistinguishedName
        RelativeDistinguishedName = SET of AttributeValueAssertion
        AttributeValueAssertion = SEQUENCE { attributeType OID; attributeValue ANY }
    */

    /* Name is a sequence */
    if ((NULL == pIssuer) ||
        ((pIssuer->id & CLASS_MASK) != UNIVERSAL) ||
        (pIssuer->tag != SEQUENCE))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* copy data byte by byte */
    CS_seek(s, pIssuer->dataOffset, MOCANA_SEEK_SET);

    /* allocate return memory for subject/issuer block */
    if (NULL == (pMemBlock = MALLOC(pIssuer->length)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* copy out byte by byte */
    for (copyIndex = 0; copyIndex < pIssuer->length; copyIndex++)
    {
        if (OK > (status = CS_getc(s, &ch)))
            goto exit;

        pMemBlock[copyIndex] = ch;
    }

    /* copy return values */
    *ppRetDistinguishedName   = pMemBlock;
    *pRetDistinguishedNameLen = pIssuer->length;

    /* for return */
    pMemBlock = NULL;
    status = OK;

exit:
    if (pMemBlock)
        FREE(pMemBlock);

    return status;

} /* CERT_extractDistinguishedNamesBlob */
#endif /* __ENABLE_MOCANA_EXTRACT_CERT_BLOB__ */


/*------------------------------------------------------------------*/

static void
convertTime(TimeDate *pTime, ubyte *pOutputTime)
{
    ubyte4  temp;

    temp = (ubyte4)(pTime->m_year + 1970);
    pOutputTime[0] = (ubyte)('0' + (temp / 1000));
    pOutputTime[1] = (ubyte)('0' + ((temp % 1000) / 100));
    pOutputTime[2] = (ubyte)('0' + ((temp % 100) / 10));
    pOutputTime[3] = (ubyte)('0' + ((temp % 10)));

    temp = pTime->m_month;

    pOutputTime[4] = (ubyte)('0' + (temp / 10));
    pOutputTime[5] = (ubyte)('0' + (temp % 10));

    temp = pTime->m_day;

    pOutputTime[6] = (ubyte)('0' + (temp / 10));
    pOutputTime[7] = (ubyte)('0' + (temp % 10));

    temp = pTime->m_hour;

    pOutputTime[8] = (ubyte)('0' + (temp / 10));
    pOutputTime[9] = (ubyte)('0' + (temp % 10));

    temp = pTime->m_minute;

    pOutputTime[10] = (ubyte)('0' + (temp / 10));
    pOutputTime[11] = (ubyte)('0' + (temp % 10));

    temp = pTime->m_second;

    pOutputTime[12] = (ubyte)('0' + (temp / 10));
    pOutputTime[13] = (ubyte)('0' + (temp % 10));

    pOutputTime[14] = (ubyte)('Z');
    pOutputTime[15] = (ubyte)('\0');
}


/*------------------------------------------------------------------*/
extern MSTATUS
CERT_getValidityTime(ASN1_ITEM* startItem, intBoolean startIsRoot, ASN1_ITEMPTR* pRetStart, ASN1_ITEMPTR* pRetEnd)
{
    ASN1_ITEM*  pCertificate;
    ASN1_ITEM*  pVersion;
    ASN1_ITEM*  pValidity;
    MSTATUS     status;

    if ((NULL == startItem) || (NULL == pRetStart) || (NULL == pRetEnd))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *pRetStart = NULL;
    *pRetEnd = NULL;

    if (OK > (status = GetCertificatePart( startItem, startIsRoot, &pCertificate)))
        goto exit;

    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pVersion)))
        goto exit;

    /* validity is fifth child of certificate object */
    if (OK > (status = ASN1_GetNthChild( pCertificate, pVersion ? 5 : 4, &pValidity)))
        goto exit;

    /* validity is a sequence of two items */
    if ( NULL == pValidity ||
            (pValidity->id & CLASS_MASK) != UNIVERSAL ||
            pValidity->tag != SEQUENCE)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if (OK > (status = ASN1_GetNthChild( pValidity, 1, pRetStart)))
        goto exit;

    if (OK > (status = ASN1_GetNthChild( pValidity, 2, pRetEnd)))
        goto exit;

 exit:
    return status;
}

/*------------------------------------------------------------------*/


static MSTATUS
CERT_extractValidityTimeAux(ASN1_ITEM* startItem, int startIsRoot, CStream s,
                            certDistinguishedName *pRetDN)
{
    ASN1_ITEM*  pStart;
    ASN1_ITEM*  pEnd;
    TimeDate    certTime;
    sbyte*      pAsciiStartTime = NULL;
    sbyte*      pAsciiEndTime   = NULL;
    MSTATUS     status;

    if ((NULL == startItem) || (NULL == pRetDN))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    pAsciiStartTime = MALLOC(16);
    pAsciiEndTime   = MALLOC(16);

    if ((NULL == pAsciiStartTime) || (NULL == pAsciiEndTime))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = CERT_getValidityTime(startItem, startIsRoot, &pStart, &pEnd)))
        goto exit;

    if (OK > (status = CERT_GetCertTime( pStart, s, &certTime)))
        goto exit;

    convertTime(&certTime, (ubyte *)pAsciiStartTime);

    if (OK > (status = CERT_GetCertTime( pEnd, s, &certTime)))
        goto exit;

    convertTime(&certTime, (ubyte *)pAsciiEndTime);

    /* return results */
    pRetDN->pStartDate = pAsciiStartTime; pAsciiStartTime = NULL;
    pRetDN->pEndDate   = pAsciiEndTime;   pAsciiEndTime   = NULL;

    status = OK;

exit:
    if (NULL != pAsciiStartTime)
        FREE(pAsciiStartTime);

    if (NULL != pAsciiEndTime)
        FREE(pAsciiEndTime);

    return status;

} /* CERT_extractValidityTime */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_extractValidityTime(ASN1_ITEM* rootItem, CStream s,
                            certDistinguishedName *pRetDN)
{
    return CERT_extractValidityTimeAux( rootItem, 1, s, pRetDN);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_extractValidityTime2(ASN1_ITEM* pCertificate, CStream s,
                            certDistinguishedName *pRetDN)
{
    return CERT_extractValidityTimeAux( pCertificate, 0, s, pRetDN);
}


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_CERTIFICATE_SEARCH_SUPPORT__
static MSTATUS
findOID(ASN1_ITEM* pAlgoId, CStream s, const ubyte* whichOID,
        sbyte4* oidIndex)
{
    ASN1_ITEM*  pOID;
    sbyte4      index;
    MSTATUS     status;

    if ((NULL == whichOID) || (NULL == oidIndex))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    status = ERR_CERT_INVALID_STRUCT;

    if ((NULL == pAlgoId) ||
        ((pAlgoId->id & CLASS_MASK) != UNIVERSAL) ||
        (pAlgoId->tag != SEQUENCE))
    {
        goto exit;
    }

    pOID = ASN1_FIRST_CHILD(pAlgoId);
    if (NULL == pOID)
    {
        goto exit;
    }

    status = OK;

    for (index = 0; (NULL != pOID); pOID = ASN1_NEXT_SIBLING(pOID), index++)
    {
        MSTATUS status2 = ASN1_VerifyOID( pOID, s, whichOID);

        if (OK == status2)
        {
            *oidIndex = index;
            break;
        }
    }

exit:
    return status;

} /* findOID */


/*------------------------------------------------------------------*/

static MSTATUS
CERT_rawVerifyOIDAux(ASN1_ITEM* startItem,
                  int startIsRoot,
                  CStream s,
                  const ubyte *pOidItem,
                  const ubyte *pOidValue,
                  intBoolean *pIsPresent)
{
    MSTATUS status;
    ASN1_ITEM* pCertificate = NULL;
    ASN1_ITEM* pItem;
    ASN1_ITEM* pExtension;
    intBoolean criticalExtension;
    sbyte4 oidValueIndex = -1;

    if ((NULL == startItem) || (NULL == pOidItem) || (NULL == pOidValue) || (NULL == pIsPresent))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = GetCertificatePart( startItem, startIsRoot, &pCertificate)))
        goto exit;

    /* version */
    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pItem)))
        goto exit;

    if ( NULL == pItem) /* not found */
    {
        /* version 1 by default nothing to do*/
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if ((pItem->id & CLASS_MASK) != UNIVERSAL ||
            pItem->tag != INTEGER)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if ( 2 != pItem->data.m_intVal)  /*v3 = 2 */
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* version 3 -> look for the CONTEXT tag = 3 */
    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 3, &pItem)))
        goto exit;

    if ( NULL == pItem) /* not found */
    {
        status = ERR_CERT_INVALID_INTERMEDIATE_CERTIFICATE;
        goto exit;
    }

    if ((UNIVERSAL != (pItem->id & CLASS_MASK)) || (SEQUENCE != pItem->tag))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* look for the child with OID item */
    if (OK > (status = CERT_getCertExtension(pItem, s, pOidItem,
                                        &criticalExtension, &pExtension)))
    {
        goto exit;
    }

    if ((NULL == pExtension) || (0 == pExtension->length))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if (  (pExtension->id & CLASS_MASK) != UNIVERSAL ||
            pExtension->tag != SEQUENCE)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* look for the child with OID item */
    if (OK > (status = findOID(pExtension, s, pOidValue, &oidValueIndex)))
    {
        goto exit;
    }

    if (-1 != oidValueIndex)
        *pIsPresent = TRUE;

exit:
    return status;

} /* CERT_rawVerifyOIDAux */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_rawVerifyOID(ASN1_ITEM* rootItem,
                  CStream s,
                  const ubyte *pOidItem,
                  const ubyte *pOidValue,
                  intBoolean *pIsPresent)
{
    return CERT_rawVerifyOIDAux(rootItem, 1, s, pOidItem, pOidValue, pIsPresent);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_rawVerifyOID2(ASN1_ITEM* pCertificate,
                  CStream s,
                  const ubyte *pOidItem,
                  const ubyte *pOidValue,
                  intBoolean *pIsPresent)
{
    return CERT_rawVerifyOIDAux(pCertificate, 0, s, pOidItem, pOidValue, pIsPresent);
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_extractSerialNumAux(ASN1_ITEM* startItem, int startIsRoot,
                      CStream s,
                      ubyte** ppRetSerialNum, ubyte4 *pRetSerialNumLength)
{
    MSTATUS status;
    ASN1_ITEM* pSerialNum;

    if ((NULL == startItem) || (NULL == ppRetSerialNum) || (NULL == pRetSerialNumLength))
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    *ppRetSerialNum      = NULL;
    *pRetSerialNumLength = 0;

    if (OK > (status =
	      CERT_getCertificateIssuerSerialNumberAux(startItem, startIsRoot,
						       NULL, &pSerialNum)))
        goto exit;

    if (NULL != pSerialNum)
    {
        ubyte* pSerialNumBuf;
        ubyte4 i;

        if (NULL == (pSerialNumBuf = MALLOC(pSerialNum->length)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        *pRetSerialNumLength = pSerialNum->length;
        CS_seek(s, pSerialNum->dataOffset, MOCANA_SEEK_SET);

        /* memcpy the serial name from the certificate to a return buffer */
        for (i = 0; i < pSerialNum->length; ++i)
        {
            if (OK > (status = CS_getc(s, &(pSerialNumBuf[i]))))
                goto exit;
        }

        /* store for return */
        *ppRetSerialNum = pSerialNumBuf;
    }

exit:
    return status;

} /* CERT_extractSerialNumAux */


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_extractSerialNum(ASN1_ITEM* rootItem, CStream s,
                      ubyte** ppRetSerialNum, ubyte4 *pRetSerialNumLength)
{
    return CERT_extractSerialNumAux( rootItem, 1, s, ppRetSerialNum, pRetSerialNumLength);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_extractSerialNum2(ASN1_ITEM* pCertificate, CStream s,
                      ubyte** ppRetSerialNum, ubyte4 *pRetSerialNumLength)
{
    return CERT_extractSerialNumAux( pCertificate, 0, s, ppRetSerialNum, pRetSerialNumLength);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_enumerateCRLAux( ASN1_ITEM* startItem, int startIsRoot, CStream s,
                     EnumCallbackFun ecf, void* userArg)
{
    ASN1_ITEM* pCertificate;
    ASN1_ITEM* pItem;
    ASN1_ITEM* pExtension;
    ASN1_ITEM* pCrlItem;
    intBoolean criticalExtension;
    MSTATUS    status;
    MSTATUS    cbReturn; /* return value from callback */

   /* CRLDistPointSyntax ::= SEQUENCE SIZE (1..MAX) OF DistributionPoint */

    /* DistributionPoint ::= SEQUENCE {
    distributionPoint [0] DistributionPointName OPTIONAL,
    reasons       [1] ReasonFlags OPTIONAL,
    cRLIssuer     [2] GeneralNames OPTIONAL } */


    /* DistributionPointName ::= CHOICE {
    fullname    [0] GeneralNames,
    nameRelativeToCRLIssuer [1] RelativeDistinguishedName } */

    /*  GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName
    GeneralName ::= CHOICE {
        otherName                  [0]  INSTANCE OF OTHER-NAME,
        rfc822Name                 [1]  IA5String,
        dNSName                    [2]  IA5String,
        x400Address                [3]  ORAddress,
        directoryName              [4]  Name,
        ediPartyName               [5]  EDIPartyName,
        uniformResourceIdentifier  [6]  IA5String,
        iPAddress                  [7]  OCTET STRING,
        registeredID               [8]  OBJECT IDENTIFIER
    }
    */
    static WalkerStep gotoFirstCRLGeneralName[] =
    {
        { VerifyType, SEQUENCE, 0},     /* CRLDistPointSyntax */
        { GoFirstChild, 0, 0},          /* DistributionPoint */
        { VerifyType, SEQUENCE, 0 },
        { GoToTag, 0, 0 },              /* [0]DistributionPointName */
        { GoToTag, 0, 0},               /* [0]GeneralNames */
        { GoFirstChild, 0, 0 },         /* GeneralName */
        { Complete, 0, 0}
    };

    static WalkerStep gotoFirstCRLGeneralNameOfNextDistributionPoint[] =
    {
        { GoParent, 0, 0},              /* [0]GeneralNames */
        { GoParent, 0, 0},              /* [0]DistributionPointName */
        { GoParent, 0, 0},              /* DistributionPoint */
        { GoNextSibling, 0, 0},         /* DistributionPoint */
        { VerifyType, SEQUENCE, 0 },
        { GoToTag, 0, 0 },              /* [0]DistributionPointName */
        { GoToTag, 0, 0},               /* [0]GeneralNames */
        { GoFirstChild, 0, 0 },         /* GeneralName */
        { Complete, 0, 0}
    };

    if ((NULL == startItem) || (NULL == ecf) )
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = GetCertificatePart( startItem, startIsRoot, &pCertificate)))
        goto exit;

    /* version */
    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pItem)))
        goto exit;

    if (NULL == pItem) /* not found */
    {
        /* version 1 by default nothing to do*/
        goto exit;
    }

    if ((pItem->id & CLASS_MASK) != UNIVERSAL ||
            pItem->tag != INTEGER)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if ( 2 < pItem->data.m_intVal)  /*v3 = 2 */
    {
        /* other version than version 3 -> return no error */
        goto exit;
    }

    /* version 3 -> look for the CONTEXT tag = 3 */
    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 3, &pItem)))
        goto exit;

    if (NULL == pItem) /* not found */
    {
        /* no extensions -> no error */
        goto exit;
    }

    if ((UNIVERSAL != (pItem->id & CLASS_MASK)) || (SEQUENCE != pItem->tag))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* look for the child with OID item */
    if (OK > (status = CERT_getCertExtension(pItem, s, crl_OID,
                                        &criticalExtension, &pExtension)))
    {
        goto exit;
    }

    if ((NULL == pExtension) || (0 == pExtension->length))
    {
        /* no CRL extension -> no error */
        goto exit;
    }

    if ( OK > ( status = ASN1_WalkTree( pExtension, s,
                                        gotoFirstCRLGeneralName, &pCrlItem)))
    {
        goto exit;
    }

    while ( pCrlItem)
    {
        /* call the callback */
        if (OK > (cbReturn = ecf(pCrlItem, s, userArg)))
        {
            status = OK; /* return OK */
            goto exit;
        }
        /* next sibling ? */
        pItem = ASN1_NEXT_SIBLING( pCrlItem);
        if (!pItem)
        {
            /* otherwise go to the next Distribution Point */
            if ( OK > ( status =
                        ASN1_WalkTree( pCrlItem, s,
                               gotoFirstCRLGeneralNameOfNextDistributionPoint,
                               &pItem)))
            {
                status = OK;
                goto exit;
            }
        }
        pCrlItem = pItem;
    }

exit:

    return status;

}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_enumerateCRL( ASN1_ITEM* rootItem, CStream s,  EnumCallbackFun ecf,
                  void* userArg)
{
    return CERT_enumerateCRLAux( rootItem, 1, s, ecf, userArg);
}


/*------------------------------------------------------------------*/

extern MSTATUS
CERT_enumerateCRL2( ASN1_ITEM* pCertificate, CStream s,  EnumCallbackFun ecf,
                  void* userArg)
{
    return CERT_enumerateCRLAux( pCertificate, 0, s, ecf, userArg);
}


/*------------------------------------------------------------------*/

static MSTATUS
CERT_enumerateAltNameAux( ASN1_ITEM* startItem, int startIsRoot,
                         CStream s,  sbyte4 isSubject,
                       EnumCallbackFun ecf, void* userArg)
{
    const ubyte* altNameOID;
    ASN1_ITEM*   pCertificate;
    ASN1_ITEM*   pItem;
    ASN1_ITEM*   pAltName;
    ASN1_ITEM*   pGeneralName;
    intBoolean   critical;
    ubyte4       tag;
    MSTATUS      status;
    MSTATUS      cbReturn; /* return value from callback */

    /* SubjectAltName ::= GeneralNames */
    /* IssuerAltName  ::= GeneralNames */

    /*  GeneralNames ::= SEQUENCE SIZE (1..MAX) OF GeneralName
    GeneralName ::= CHOICE {
        otherName                  [0]  INSTANCE OF OTHER-NAME,
        rfc822Name                 [1]  IA5String,
        dNSName                    [2]  IA5String,
        x400Address                [3]  ORAddress,
        directoryName              [4]  Name,
        ediPartyName               [5]  EDIPartyName,
        uniformResourceIdentifier  [6]  IA5String,
        iPAddress                  [7]  OCTET STRING,
        registeredID               [8]  OBJECT IDENTIFIER
    }
    */

    static WalkerStep findUserPrincipalName[] =
    {
        { GoFirstChild, 0, 0 },         /* type-id */
        { VerifyOID, 0, (ubyte*)userPrincipalName_OID },
        { GoNextSibling, 0, 0 },        /* value [0] */
        { GoFirstChild, 0, 0 },
        { VerifyType, UTF8STRING, 0 },
        { Complete, 0, 0 }
    };

    if ((NULL == startItem) || (NULL == ecf) )
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    if (OK > (status = GetCertificatePart( startItem, startIsRoot, &pCertificate)))
        goto exit;

    /* version */
    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 0, &pItem)))
        goto exit;

    if (NULL == pItem) /* not found */
    {
        /* version 1 by default nothing to do*/
        goto exit;
    }

    if ((pItem->id & CLASS_MASK) != UNIVERSAL ||
            pItem->tag != INTEGER)
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    if ( 2 < pItem->data.m_intVal)  /*v3 = 2 */
    {
        /* other version than version 3 -> return no error */
        goto exit;
    }

    /* version 3 -> look for the CONTEXT tag = 3 */
    if (OK > (status = ASN1_GetChildWithTag( pCertificate, 3, &pItem)))
        goto exit;

    if (NULL == pItem) /* not found */
    {
        /* no extensions -> no error */
        goto exit;
    }

    if ((UNIVERSAL != (pItem->id & CLASS_MASK)) || (SEQUENCE != pItem->tag))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    /* look for the child with OID item */
    altNameOID = isSubject ? subjectAltName_OID : issuerAltName_OID;

    if (OK > (status = CERT_getCertExtension(pItem, s, altNameOID,
                                        &critical, &pAltName)))
    {
        goto exit;
    }

    if ((NULL == pAltName) || (0 == pAltName->length))
    {
        /* no alternative name extension -> no error */
        goto exit;
    }

    if  (OK > ( status = ASN1_VerifyType( pAltName, SEQUENCE)))
    {
        status = ERR_CERT_INVALID_STRUCT;
        goto exit;
    }

    pGeneralName = ASN1_FIRST_CHILD( pAltName);

    while ( pGeneralName)
    {
        pItem = pGeneralName;

        if  (OK > ( status = ASN1_GetTag( pGeneralName, &tag)))
        {
            status = ERR_CERT_INVALID_STRUCT;
            goto exit;
        }

        /* othername, current only recognizes UPN */
        if ( 0 == tag)
        {
            if (OK > (status = ASN1_WalkTree( pGeneralName, s,
                                  findUserPrincipalName, &pItem)))
            {
                /* unsupported item, let user decide what to do */
                pItem = pGeneralName;
                status = OK;
            }
        }

        /* call the callback */
        if (OK > (cbReturn = ecf( pItem, s, userArg)))
        {
            status = OK; /* return OK */
            goto exit;
        }

        /* next sibling */
        pGeneralName = ASN1_NEXT_SIBLING( pGeneralName);
    }

exit:

    return status;

}

/*------------------------------------------------------------------*/

extern MSTATUS
CERT_enumerateAltName( ASN1_ITEM* rootItem, CStream s,  sbyte4 isSubject,
                       EnumCallbackFun ecf, void* userArg)
{
    return CERT_enumerateAltNameAux( rootItem, 1, s, isSubject, ecf, userArg);
}

/*------------------------------------------------------------------*/

extern MSTATUS
CERT_enumerateAltName2( ASN1_ITEM* pCertificate, CStream s,  sbyte4 isSubject,
                       EnumCallbackFun ecf, void* userArg)
{
    return CERT_enumerateAltNameAux( pCertificate, 0, s, isSubject, ecf, userArg);
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
CERT_getRSASignatureAlgo( ASN1_ITEM* rootItem, CStream certStream,
                      ubyte* signAlgo)
{
    static WalkerStep signatureWalkInstructions[] =
    {
        { GoFirstChild, 0, 0},          /* First Child -> SignedCert */
        { VerifyType, SEQUENCE, 0},
        { GoFirstChild, 0, 0},          /* Cert */
        { VerifyType, SEQUENCE, 0},
        { GoNextSibling, 0, 0},         /* Signature */
        { VerifyType, SEQUENCE, 0},
        { Complete, 0, 0}
    };

    MSTATUS status;
    ASN1_ITEM* pSignatureAlgo;

    if (OK > ( status = ASN1_WalkTree( rootItem, certStream,
            signatureWalkInstructions, &pSignatureAlgo)))
    {
        return status;
    }

    if (OK > (status = GetCertOID(pSignatureAlgo, certStream, pkcs1_OID, signAlgo, NULL)))
    {
        if (OK <= (status = GetCertOID(pSignatureAlgo, certStream, sha1withRsaSignature_OID, NULL, NULL)))
            *signAlgo = sha1withRSAEncryption;
    }

    return status;
}


#endif /* __ENABLE_MOCANA_CERTIFICATE_SEARCH_SUPPORT__ */

#endif /* __DISABLE_MOCANA_CERTIFICATE_PARSING__ */

