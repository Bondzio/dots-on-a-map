/* Version: mss_v6_3 */
/*
 * sec_key.c
 *
 *
 *
 * Copyright Mocana Corp 2009. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#if defined(__ENABLE_MOCANA_ECC__)

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
#include "../common/random.h"
#include "../common/vlong.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/pubcrypto.h"
#include "../crypto/rsa.h"
#include "../harness/harness.h"
#include "../asn1/parseasn1.h"
#include "../asn1/ASN1TreeWalker.h"
#include "../asn1/oiddefs.h"
#include "../asn1/derencoder.h"
#include "../crypto/ca_mgmt.h"
#include "../asn1/parsecert.h"
#include "../crypto/asn1cert.h"
#include "../crypto/sec_key.h"

#endif


#if !defined( __DISABLE_MOCANA_CERTIFICATE_PARSING__)


/*---------------------------------------------------------------------------*/

MSTATUS
SEC_getPrivateKey(ASN1_ITEMPTR pSeq, CStream cs, ubyte curveId,
                  AsymmetricKey* pECCKey)
{
    MSTATUS status;
    ASN1_ITEMPTR pTmp, pPrivateKey, pOID;
    const ubyte* pk = 0;

    pTmp = ASN1_FIRST_CHILD(pSeq);
    if (pTmp->data.m_intVal != 1)
    {
        status = ERR_EC_UNKNOWN_KEY_FILE_VERSION;
        goto exit;
    }

    pPrivateKey = ASN1_NEXT_SIBLING( pTmp);
    if ( OK > ASN1_VerifyType( pPrivateKey, OCTETSTRING))
    {
        status = ERR_EC_INVALID_KEY_FILE_FORMAT;
        goto exit;
    }

    /* we require a tag [0] if curveId = 0 */
    if ( 0 == curveId)
    {
        /* go to 0 tag */
        if (OK > ( status = ASN1_GoToTag( pSeq, 0, &pTmp)))
            goto exit;

        if ( !pTmp)
        {
            status = ERR_EC_INCOMPLETE_KEY_FILE;
            goto exit;
        }

        pOID = ASN1_FIRST_CHILD(pTmp);
        /* this should be one of the OID for the curves we support */
        status = ASN1_VerifyOIDRoot( pOID, cs, ansiX962CurvesPrime_OID, &curveId);
        if ( OK > status) /* try another ASN1 arc */
        {
            status = ASN1_VerifyOIDRoot( pOID, cs, certicomCurve_OID, &curveId);
        }

        if (OK > status)
        {
            status = ERR_EC_UNSUPPORTED_CURVE;
            goto exit;
        }
    }

    /* go to 1 tag */
    if (OK > ( status = ASN1_GoToTag( pSeq, 1, &pTmp)))
        goto exit;

    /* required always */
    if ( !pTmp)
    {
        status = ERR_EC_INCOMPLETE_KEY_FILE;
        goto exit;
    }

    pTmp = ASN1_FIRST_CHILD(pTmp);
    if (OK > ASN1_VerifyType( pTmp, BITSTRING))
    {
        status = ERR_EC_INVALID_KEY_FILE_FORMAT;
        goto exit;
    }

    /* access the data */
    pk = CS_memaccess( cs, pPrivateKey->dataOffset,
                       pTmp->dataOffset + pTmp->length - pPrivateKey->dataOffset);
    if (!pk)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = CRYPTO_setECCParameters( pECCKey, curveId,
                            pk + pTmp->dataOffset - pPrivateKey->dataOffset,
                            pTmp->length,
                            pk, pPrivateKey->length,
                            NULL)))
    {
        goto exit;
    }

exit:

    if (pk)
    {
        CS_stopaccess( cs, pk);
    }

    return status;
}


/*---------------------------------------------------------------------------*/

MSTATUS
SEC_getKey(const ubyte* sec1DER, ubyte4 sec1DERLen, AsymmetricKey* pECCKey)
{
    CStream         cs;
    MemFile         mf;
    ASN1_ITEMPTR    pRoot = 0;
    ASN1_ITEMPTR    pSeq, pTmp;
    MSTATUS         status;

    if (!sec1DER || !pECCKey)
        return ERR_NULL_POINTER;

    /* parse the DER */
    MF_attach( &mf, sec1DERLen, (ubyte*) sec1DER);

    CS_AttachMemFile( &cs, &mf);

    if ( OK > (status = ASN1_Parse(cs, &pRoot)))
        goto exit;

    /* public or private key ? */
    pSeq = ASN1_FIRST_CHILD( pRoot);
    if (OK > ASN1_VerifyType(pSeq, SEQUENCE))
    {
        status = ERR_EC_INVALID_KEY_FILE_FORMAT;
        goto exit;
    }

    pTmp = ASN1_FIRST_CHILD(pSeq);

    if (OK <= ASN1_VerifyType(pTmp, SEQUENCE))
    {
        status = CERT_extractECCKey( pSeq, cs, pECCKey);
        goto exit;
    }
    else if (OK <= ASN1_VerifyType(pTmp, INTEGER))
    {
        status = SEC_getPrivateKey( pSeq, cs, 0, pECCKey);
        goto exit;
    }
    else
    {
        status = ERR_EC_INVALID_KEY_FILE_FORMAT;
        goto exit;
    }

exit:
    if ( pRoot)
    {
        TREE_DeleteTreeItem((TreeItem*) pRoot);
    }
    return status;
}

#endif


#if defined(__ENABLE_MOCANA_DER_CONVERSION__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__)

/*---------------------------------------------------------------------------*/

static MSTATUS
SEC_setPublicKey( const AsymmetricKey* pECCKey, ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    MSTATUS status;
    DER_ITEMPTR pRoot = 0;
    DER_ITEMPTR pPublicKeyInfo;

    /* create a root */
    if (OK > ( status = DER_AddSequence( NULL, &pRoot)))
        goto exit;

    /* use the usual routine to store the key info under the root */
    if (OK > ( status = ASN1CERT_storePublicKeyInfo( pECCKey, pRoot)))
        goto exit;

    /* serialize the public key info */
    pPublicKeyInfo = DER_FIRST_CHILD( pRoot);
    if (OK > ( status = DER_Serialize( pPublicKeyInfo, ppRetKeyDER, pRetKeyDERLength)))
        goto exit;

exit:

    if ( pRoot)
    {
        TREE_DeleteTreeItem( (TreeItem*) pRoot);
    }

    return status;
}


/*---------------------------------------------------------------------------*/

static MSTATUS
SEC_setPrivateKey( const ECCKey* pECCKey, ubyte4 options,
                  ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    MSTATUS status;
    DER_ITEMPTR pRoot = 0;
    DER_ITEMPTR pTag;
    PrimeFieldPtr pFld;
    ubyte* kbuffer = 0;
    ubyte* pbuffer = 0;
    ubyte4 bufLen;
    ubyte4 offset;
    const ubyte* curveOID;

    /* create a root */
    if (OK > ( status = DER_AddSequence( NULL, &pRoot)))
        goto exit;

    /* create an integer */
    if (OK > ( status = DER_AddIntegerEx( pRoot, 1, NULL)))
        goto exit;

    pFld = EC_getUnderlyingField( pECCKey->pCurve);
    /* add the OCTETSTRING with the k value */
    if (OK > ( status = PRIMEFIELD_getAsByteString( pFld, pECCKey->k, &kbuffer, (sbyte4*)&bufLen)))
    {
        goto exit;
    }

    /* remove the leading zeroes PRIMEFIELD_getAsByteString MUST preserve them */
    for (offset = 0; offset < bufLen && 0 == kbuffer[offset]; ++offset) {}
    if (OK > ( status = DER_AddItem( pRoot, OCTETSTRING, bufLen - offset, kbuffer + offset, NULL)))
        goto exit;

    if ( 0 == (options & E_SEC_omitCurveOID))
    {
        /* add tag [0] */
        if (OK > ( status = DER_AddTag( pRoot, 0, &pTag)))
            goto exit;

        if (OK > ( status = CRYPTO_getECCurveOID( pECCKey, &curveOID)))
            goto exit;

        /* add OID for the curve */
        if (OK > ( status = DER_AddOID( pTag, curveOID, NULL)))
            goto exit;
    }

    /* add tag [1] */
    if (OK > ( status = DER_AddTag( pRoot, 1, &pTag)))
        goto exit;

    /* allocate a buffer for the key parameter */
    if (OK > (status = EC_getPointByteStringLen( pECCKey->pCurve, (sbyte4*)&bufLen)))
        goto exit;

    if (0 == bufLen)
    {
        status = ERR_BAD_KEY;
        goto exit;
    }

    /* add an extra byte = 0 (unused bits) */
    pbuffer = MALLOC( bufLen+1);
    if (!pbuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    pbuffer[0] = 0; /* unused bits */

    if (OK > ( status = EC_writePointToBuffer( pECCKey->pCurve, pECCKey->Qx,
                                                pECCKey->Qy, pbuffer+1, bufLen)))
    {
        goto exit;
    }

    if (OK > ( status = DER_AddItem( pTag, BITSTRING, bufLen+1, pbuffer, NULL)))
        goto exit;

	/* at this point pbuffer should be added to pTag */
	pbuffer = NULL;

    /* serialize the public key info */
    if (OK > ( status = DER_Serialize( pRoot, ppRetKeyDER, pRetKeyDERLength)))
        goto exit;

exit:

    if (kbuffer)
    {
        FREE(kbuffer);
    }

    if (pbuffer)
    {
        FREE(pbuffer);
    }

    if ( pRoot)
    {
        TREE_DeleteTreeItem( (TreeItem*) pRoot);
    }

    return status;
}


/*---------------------------------------------------------------------------*/

MSTATUS SEC_setKeyEx(const AsymmetricKey* pKey, ubyte4 options,
                     ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    if (!pKey || !ppRetKeyDER || !pRetKeyDERLength)
        return ERR_NULL_POINTER;

    if (akt_ecc != pKey->type)
        return ERR_EC_INVALID_KEY_TYPE;

    if (!pKey->key.pECC)
        return ERR_NULL_POINTER;

    return pKey->key.pECC->privateKey ? SEC_setPrivateKey( pKey->key.pECC, options, ppRetKeyDER, pRetKeyDERLength) :
                                        SEC_setPublicKey( pKey, ppRetKeyDER, pRetKeyDERLength);
}


/*---------------------------------------------------------------------------*/

MSTATUS SEC_setKey(const AsymmetricKey* pKey, ubyte **ppRetKeyDER, ubyte4 *pRetKeyDERLength)
{
    return SEC_setKeyEx( pKey, 0, ppRetKeyDER, pRetKeyDERLength);
}

#endif /* defined( __ENABLE_MOCANA_DER_CONVERSION__) || defined(__ENABLE_MOCANA_PEM_CONVERSION__) */


#endif /* defined( __ENABLE_MOCANA_ECC__) */
