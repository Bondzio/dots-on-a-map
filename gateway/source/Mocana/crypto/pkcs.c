/* Version: mss_v6_3 */
/*
 * pkcs.c
 *
 * PKCS7 Utilities
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#ifdef __ENABLE_MOCANA_PKCS7__

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
#include "../asn1/oiddefs.h"
#include "../asn1/parseasn1.h"
#include "../asn1/ASN1TreeWalker.h"
#include "../asn1/derencoder.h"
#include "../common/random.h"
#include "../crypto/crypto.h"
#include "../crypto/rsa.h"
#if (defined(__ENABLE_MOCANA_ECC__))
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#endif
#include "../crypto/pubcrypto.h"
#include "../crypto/ca_mgmt.h"
#include "../crypto/pkcs_common.h"
#include "../crypto/pkcs7.h"
#include "../crypto/pkcs.h"

#if defined(__ENABLE_MOCANA_HARNESS__)
#include "../harness/harness.h"
#endif

/* the global g_pRandomContext shared across applications */
extern randomContext* g_pRandomContext;


/*--------------------------------------------------------------------------*/

/* This API returns an DER encoded PKCS7 message that contains the
payload enveloped using the provided certificate. This is just a
high level wrapper, with less flexibility of PKCS7_EnvelopData */
extern MSTATUS
PKCS7_EnvelopWithCertificate( const ubyte* cert,
                                ubyte4 certLen,
                                const ubyte* encryptAlgoOID,
                                const ubyte* pPayLoad,
                                ubyte4 payLoadLen,
                                ubyte** ppEnveloped,
                                ubyte4* pEnvelopedLen)
{
    hwAccelDescr    hwAccelCtx;
    ASN1_ITEMPTR    pCertificate = NULL;
    CStream         s;
    MemFile         certMemFile;
    MSTATUS         status;

    if ( !cert || !encryptAlgoOID ||
        !pPayLoad || !ppEnveloped || !pEnvelopedLen)
    {
        return ERR_NULL_POINTER;
    }

    if (OK > ( status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;

    MF_attach(&certMemFile, certLen, (ubyte*) cert);
    CS_AttachMemFile(&s, &certMemFile );

    /* parse the certificate */
    if (OK > (status = ASN1_Parse( s, &pCertificate)))
        goto exit;

    status = PKCS7_EnvelopData( MOC_RSA_SYM( hwAccelCtx)
                                NULL, NULL, &pCertificate,
                                &s, 1, encryptAlgoOID,
                                RANDOM_rngFun, g_pRandomContext,
                                pPayLoad, payLoadLen,
                                ppEnveloped, pEnvelopedLen);
exit:

    if (pCertificate)
    {
        TREE_DeleteTreeItem( &pCertificate->treeItem);
    }

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return status;
}


/*--------------------------------------------------------------------------*/

/* This API returns an DER encoded PKCS7 message that contains the
payload enveloped using the provided certificates. This is just a
high level wrapper for PKCS7_EnvelopData */

MSTATUS
PKCS7_EnvelopWithCertificates( ubyte4 numCerts, const ubyte* certs[/*numCerts*/],
                                ubyte4 certLens[/*numCerts*/],
                                const ubyte* encryptAlgoOID,
                                const ubyte* pPayLoad, ubyte4 payLoadLen,
                                ubyte** ppEnveloped, ubyte4* pEnvelopedLen)
{
    hwAccelDescr    hwAccelCtx;
    ASN1_ITEMPTR*   pCertificates = NULL;
    CStream*        streams = NULL;
    MemFile*        certMemFiles = NULL;
    ubyte4          i;
    MSTATUS         status;

    if ( !certs || !encryptAlgoOID ||
        !pPayLoad || !ppEnveloped || !pEnvelopedLen)
    {
        return ERR_NULL_POINTER;
    }

    if ( !numCerts)
        return ERR_INVALID_ARG;

    if (OK > ( status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;

    /* array allocations */
    pCertificates = MALLOC( numCerts * sizeof( ASN1_ITEMPTR));
    if (pCertificates)
        MOC_MEMSET((ubyte*)pCertificates, 0, numCerts * sizeof( ASN1_ITEMPTR));

    streams = MALLOC( numCerts * sizeof( CStream));
    if (streams)
        MOC_MEMSET((ubyte*)streams, 0, numCerts * sizeof( CStream));

    certMemFiles = MALLOC( numCerts * sizeof( MemFile));
    if (certMemFiles)
        MOC_MEMSET((ubyte*)certMemFiles, 0, numCerts * sizeof( MemFile));

    if ((NULL == pCertificates) || (NULL == streams) || (NULL == certMemFiles))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    for ( i = 0; i < numCerts; ++i)
    {
        MF_attach(certMemFiles+i, certLens[i], (ubyte*) certs[i]);
        CS_AttachMemFile(streams+i, certMemFiles+i );

        /* parse the certificate */
        if (OK > (status = ASN1_Parse( streams[i], pCertificates+i)))
            goto exit;
    }

    status = PKCS7_EnvelopData( MOC_RSA_SYM( hwAccelCtx)
                                NULL, NULL, pCertificates,
                                streams, numCerts, encryptAlgoOID,
                                RANDOM_rngFun, g_pRandomContext,
                                pPayLoad, payLoadLen,
                                ppEnveloped, pEnvelopedLen);
exit:

    if (pCertificates)
    {
        for ( i = 0; i < numCerts; ++i)
        {
            if ( pCertificates[i])
            {
                TREE_DeleteTreeItem( &(pCertificates[i]->treeItem));
            }
        }
        FREE( pCertificates);
    }

    if ( certMemFiles)
    {
        FREE( certMemFiles);
    }

    if ( streams)
    {
        FREE( streams);
    }


    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return status;
}

/*--------------------------------------------------------------------------*/

/* This API decrypts the Enveloped Data part of a PKCS7 message
This is a high level wrapper for PKCS7_DecryptEnvelopedData */
MSTATUS
PKCS7_DecryptEnvelopedDataPart( const ubyte* pkcs7Msg,
                                            ubyte4 pkcs7MsgLen,
                                            PKCS7_GetPrivateKey getPrivateKeyFun,
                                            ubyte** decryptedInfo,
                                            sbyte4* decryptedInfoLen)
{
    hwAccelDescr    hwAccelCtx;
    CStream         s;
    MemFile         certMemFile;
    ASN1_ITEMPTR    rootItem = NULL, pItem, pEnvelopedData;
    MSTATUS         status;

    if ( !pkcs7Msg || !decryptedInfo || !decryptedInfoLen)
        return ERR_NULL_POINTER;

    if (OK > ( status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;


    MF_attach(&certMemFile, pkcs7MsgLen, (ubyte*) pkcs7Msg);
    CS_AttachMemFile(&s, &certMemFile );

    /* parse the PKCS7 message */
    if (OK > (status = ASN1_Parse( s, &rootItem)))
        goto exit;

    /* now look for the PKCS7 OID for Enveloped data */
    if ( OK > ( status = ASN1_GetChildWithOID( rootItem, s, pkcs7_envelopedData_OID, &pItem)))
        goto exit;

    if ( pItem) /* found */
    {
        /* pkcs7 enveloped data -> we need to content type: child of tag 0*/
        if ( OK > ( status = ASN1_GetChildWithTag( pItem, 0, &pEnvelopedData)))
            goto exit;
    }
    else /* assume the whole thing is an enveloped msg */
    {
        pEnvelopedData = ASN1_FIRST_CHILD( rootItem);
    }

    if (!pEnvelopedData)
    {
        status = ERR_PKCS7_INVALID_STRUCT;
        goto exit;
    }

    /*  call the middle level routine */
    status = PKCS7_DecryptEnvelopedData( MOC_RSA_SYM( hwAccelCtx)
                                           pEnvelopedData, s, getPrivateKeyFun,
                                            decryptedInfo,decryptedInfoLen);

exit:

    if ( rootItem)
    {
        TREE_DeleteTreeItem( &rootItem->treeItem);
    }

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return status;
}


/*--------------------------------------------------------------------------*/

extern MSTATUS
PKCS7_SignWithCertificateAndKeyBlob( const ubyte* cert,
                                    ubyte4 certLen,
                                    const ubyte* keyBlob,
                                    ubyte4 keyBlobLen,
                                    ASN1_ITEMPTR pCACertificates[/*numCACerts*/],
                                    CStream pCAStreams[/*numCACerts*/],
                                    sbyte4 numCACerts,
                                    ASN1_ITEMPTR pCrls[/*numCrls*/],
                                    CStream pCrlStreams[/*numCrls*/],
                                    sbyte4 numCrls,
                                    const ubyte* digestAlgoOID,
                                    const ubyte* payLoadType,
                                    ubyte* pPayLoad, /* removed const to get rid of compiler warning */
                                    ubyte4 payLoadLen,
                                    Attribute* pAuthAttrs,
                                    ubyte4 authAttrsLen,
                                    RNGFun rngFun,
                                    void* rngFunArg,
                                    ubyte** ppSigned,
                                    ubyte4* pSignedLen)
{
    hwAccelDescr    hwAccelCtx;
    ASN1_ITEMPTR    pCertificate = NULL;
    CStream         s;
    MemFile         certMemFile;
    AsymmetricKey   key;
    signerInfo  mySignerInfo;
    signerInfoPtr mySignerInfoPtr[1];
    MSTATUS         status;
    randomContext   *pRandomContext = 0;

    if ( !cert || !keyBlob || !digestAlgoOID || !pPayLoad || !ppSigned || !pSignedLen)
    {
        return ERR_NULL_POINTER;
    }


    if (OK > ( status = (MSTATUS)HARDWARE_ACCEL_OPEN_CHANNEL(MOCANA_MSS, &hwAccelCtx)))
        return status;

    /* if no random function passed in, get one */
    if ( 0 == rngFun)
    {
        rngFun = RANDOM_rngFun;
        if (OK > ( status = RANDOM_acquireContext( &pRandomContext)))
            goto exit;
        rngFunArg = pRandomContext;
    }

    MF_attach(&certMemFile, certLen, (ubyte*) cert);
    CS_AttachMemFile(&s, &certMemFile );

    /* parse the certificate */
    if (OK > (status = ASN1_Parse( s, &pCertificate)))
        goto exit;

    /* load the key */
    CA_MGMT_extractKeyBlobEx(keyBlob, keyBlobLen, &key);

    /* need to initialize issuerandserialno */
    mySignerInfo.digestAlgoOID = digestAlgoOID;
    mySignerInfo.pKey = &key;
    mySignerInfo.pAuthAttrs = pAuthAttrs;
    mySignerInfo.authAttrsLen = authAttrsLen;
    mySignerInfo.unauthAttrsLen = 0;
    mySignerInfoPtr[0]=&mySignerInfo;

    status = PKCS7_SignData( MOC_RSA_SYM( hwAccelCtx)
                                NULL, NULL, pCACertificates, pCAStreams, numCACerts,
                                pCrls, pCrlStreams, numCrls, mySignerInfoPtr,
                                1,
                                payLoadType, pPayLoad, payLoadLen,
                                rngFun, rngFunArg, ppSigned, pSignedLen);
exit:

    if (pCertificate)
    {
        TREE_DeleteTreeItem( &pCertificate->treeItem);
    }
    RANDOM_releaseContext(&pRandomContext);

    HARDWARE_ACCEL_CLOSE_CHANNEL(MOCANA_MSS, &hwAccelCtx);

    return status;

}

#endif /* __ENABLE_MOCANA_PKCS__ */
