/* Version: mss_v6_3 */
/*
 * ssl_ocsp.c
 *
 * OCSP code to be used in SSL Extensions to support ocsp stapling
 *
 * Copyright Mocana Corp 2005-2011. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"

#if ((defined(__ENABLE_TLSEXT_RFC6066__)) && (defined(__ENABLE_MOCANA_OCSP_CLIENT__)))

#include "../common/mtypes.h"
#include "../common/mocana.h"
#include "../crypto/hw_accel.h"

#include "../common/mdefs.h"
#include "../common/merrors.h"
#include "../crypto/secmod.h"
#include "../common/mrtos.h"
#include "../common/mstdlib.h"
#include "../common/tree.h"
#include "../common/absstream.h"
#include "../common/memfile.h"
#include "../common/vlong.h"
#include "../common/random.h"
#include "../common/datetime.h"
#include "../common/sizedbuffer.h"
#include "../crypto/crypto.h"
#include "../crypto/rsa.h"
#include "../crypto/md5.h"
#include "../crypto/sha1.h"
#include "../crypto/sha256.h"
#include "../crypto/sha512.h"
#include "../crypto/primefld.h"
#include "../crypto/primeec.h"
#include "../crypto/pubcrypto.h"
#include "../asn1/oiddefs.h"
#include "../asn1/parseasn1.h"
#include "../crypto/ca_mgmt.h"
#include "../crypto/cert_store.h"
#include "../asn1/parsecert.h"
#include "../asn1/derencoder.h"
#include "../crypto/pkcs_common.h"
#include "../crypto/base64.h"
#include "../asn1/ASN1TreeWalker.h"
#include "../crypto/asn1cert.h"
#include "../common/uri.h"
#include "../common/mudp.h"
#include "../common/mtcp.h"
#include "../common/debug_console.h"
#include "../http/http_context.h"
#include "../http/http.h"
#include "../http/http_common.h"
#include "../http/client/http_request.h"
#include "../ocsp/ocsp.h"
#include "../ocsp/ocsp_context.h"
#include "../ocsp/ocsp_message.h"
#include "../ocsp/client/ocsp_client.h"
#include "../ocsp/ocsp_http.h"
#include "../common/sizedbuffer.h"
#include "../common/mem_pool.h"
#include "../common/hash_value.h"
#include "../common/hash_table.h"
#include "../ssl/ssl.h"
#include "../harness/harness.h"
#include "../ssl/sslsock.h"


/* These are added to remove warnings */
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/*------------------------------------------------------------------*/

extern MSTATUS
SSL_OCSP_initContext(void** ppContext)
{
    MSTATUS status  = OK;
    ocspContext *pOcspContext = NULL;

    if (OK > (status = OCSP_CONTEXT_createContext(&pOcspContext, OCSP_CLIENT)))
        goto exit;

    *ppContext = (void *)pOcspContext;

exit:
    return status;
}

extern MSTATUS
SSL_OCSP_createResponderIdList(void* pContext, sbyte** ppTrustedResponderCertPath,
    ubyte4 trustedResponderCertCount, ubyte** ppRetRespIdList, ubyte4* pRetRespIdListLen)
{
    MSTATUS      status = OK;

    ResponderID* pResponderId = NULL;
    DER_ITEMPTR  pRespId     = NULL;
    ubyte*       pRetData    = NULL;
    ubyte4       retDataLen  = 0;
    ubyte        i, offset;

    ocspContext* pOcspContext = (ocspContext *)pContext;

    if (NULL == (pOcspContext->pOcspSettings->pTrustedResponders = MALLOC(sizeof(OCSP_certInfo)*
                                                                   trustedResponderCertCount)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    for (i = 0; i < trustedResponderCertCount; i++)
    {
        if (NULL == ppTrustedResponderCertPath[i])
        {
            status = ERR_OCSP_INIT_FAIL;
            goto exit;
        }

        if (OK > (status = MOCANA_readFile(ppTrustedResponderCertPath[i],
                                           &pOcspContext->pOcspSettings->pTrustedResponders[i].pCertPath,
                                           &pOcspContext->pOcspSettings->pTrustedResponders[i].certLen)))
        {
            /* File read failed */
            goto exit;
        }
    }

    pOcspContext->pOcspSettings->trustedResponderCount = trustedResponderCertCount;

    if (NULL == (pResponderId = MALLOC(sizeof(ResponderID) * pOcspContext->pOcspSettings->trustedResponderCount)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* Now get the common name to be put in Responder Id */
    for (i = 0; i < pOcspContext->pOcspSettings->trustedResponderCount; i++)
    {
        ubyte4 subjectNameOffset;
        ubyte4 subjectNameLen;

        /* Create RespondeId structure */
        if (OK > (status = DER_AddSequence(NULL, &pRespId)))
            goto exit;

        if (OK > (status = DER_AddTag(pRespId, 1, &pRespId)))
            goto exit;


        if (OK > (status = CA_MGMT_extractCertASN1Name(pOcspContext->pOcspSettings->pTrustedResponders[i].pCertPath,
                                                       pOcspContext->pOcspSettings->pTrustedResponders[i].certLen,
                                                       TRUE, FALSE, &subjectNameOffset,
                                                       &subjectNameLen)))
        {
            goto exit;
        }

        if (OK > (status = DER_AddItem(pRespId, SEQUENCE|CONSTRUCTED, subjectNameLen,
                                       pOcspContext->pOcspSettings->pTrustedResponders[i].pCertPath,
                                       &pRespId)))
        {
            goto exit;
        }

        if (OK > (status = DER_Serialize(pRespId, (ubyte **) &pResponderId[0].pResponderID,
                (ubyte4 *)&pResponderId[0].responderIDlen)))
        {
            goto exit;
        }

        retDataLen += pResponderId[0].responderIDlen;

        if (pRespId)
        {
            TREE_DeleteTreeItem((TreeItem *) pRespId);
            pRespId = NULL;
        }

    }

    if (NULL == (pRetData = MALLOC(retDataLen)))
    {
        status =  ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMSET(pRetData, 0x00, retDataLen);

    offset = 0;

    /* copy contents */
    for (i = 0; i < pOcspContext->pOcspSettings->trustedResponderCount; i++)
    {

        MOC_MEMCPY(pRetData + offset, pResponderId[i].pResponderID,  pResponderId[i].responderIDlen);
        offset += pResponderId[i].responderIDlen;
    }

    *ppRetRespIdList = pRetData;
    *pRetRespIdListLen = retDataLen;

exit:
    /* Need to free data here */
    if (pResponderId)
    {
        for (i = 0; i < pOcspContext->pOcspSettings->trustedResponderCount; i++)
        {
            if (pResponderId[i].pResponderID)
                FREE (pResponderId[i].pResponderID);
        }

        FREE (pResponderId);
    }

    return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
SSL_OCSP_createExtensionsList(void* pContext, extensions* pExts, ubyte4 extCount,
    intBoolean shouldAddNonceExtension, ubyte** ppRetExtensionsList, ubyte4* pRetExtensionListLen)
{
    MSTATUS         status      = OK;
    DER_ITEMPTR     pExtensions = NULL;
    ubyte           count       = 0;
    ubyte*          pExt        = NULL;
    ubyte4          extLen      = 0;

    ocspContext* pOcspContext = (ocspContext *)pContext;

    if ((0 < extCount) || shouldAddNonceExtension)
    {

        if (OK > (status = DER_AddSequence(NULL, &pExtensions)))
            goto exit;

        for (count = 0; count < extCount; count++)
        {
            if (OK > (status = OCSP_MESSAGE_addExtension(pExtensions, pExts+count)))
                goto exit;
        }

        if (shouldAddNonceExtension)
        {
            if (OK > (status = OCSP_MESSAGE_addNonce(pOcspContext, pExtensions)))
                goto exit;

            pOcspContext->pOcspSettings->shouldAddNonce = TRUE;
        }

        if (OK > (status = DER_Serialize(pExtensions, &pExt, &extLen)))
            goto exit;

        *ppRetExtensionsList = pExt;
        *pRetExtensionListLen = extLen;
    }

exit:
    if (pExtensions)
    {
        TREE_DeleteTreeItem((TreeItem *) pExtensions);
        pExtensions = NULL;
    }

    return status;
}

/*------------------------------------------------------------------*/

extern MSTATUS
SSL_OCSP_addCertificates(void* pContext, certDescriptor* pCertChain, ubyte4 certNum)
{
    MSTATUS      status         = OK;
    ocspContext *pOcspContext   = (ocspContext *)pContext;
    ubyte        issuerCertNum  = 0;

    if (0 < certNum)
    {
        pOcspContext->pOcspSettings->certCount = 1;
    }
    else
    {
        status = ERR_OCSP_INVALID_INPUT;
        goto exit;
    }

    if (NULL == (pOcspContext->pOcspSettings->pCertInfo = MALLOC(sizeof(OCSP_singleRequestInfo)*
                                                                pOcspContext->pOcspSettings->certCount)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = MOC_MEMSET((ubyte *)pOcspContext->pOcspSettings->pCertInfo, 0x00, sizeof(OCSP_singleRequestInfo))))
        goto exit;

    if (NULL == (pOcspContext->pOcspSettings->pCertInfo[0].pCert = MALLOC(pCertChain[0].certLength)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pOcspContext->pOcspSettings->pCertInfo[0].pCert,
                pCertChain[0].pCertificate, pCertChain[0].certLength);

    pOcspContext->pOcspSettings->pCertInfo[0].certLen = pCertChain[0].certLength;

    if (NULL == (pOcspContext->pOcspSettings->pIssuerCertInfo = MALLOC(sizeof(OCSP_singleRequestInfo) *
                                                                pOcspContext->pOcspSettings->certCount)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = MOC_MEMSET((ubyte *)pOcspContext->pOcspSettings->pIssuerCertInfo, 0x00, sizeof(OCSP_singleRequestInfo))))
        goto exit;

    if (1 < certNum)
    {
        issuerCertNum = 1;
    }

    if (NULL == (pOcspContext->pOcspSettings->pIssuerCertInfo[0].pCertPath = MALLOC(pCertChain[issuerCertNum].certLength)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pOcspContext->pOcspSettings->pIssuerCertInfo[0].pCertPath,
                pCertChain[issuerCertNum].pCertificate, pCertChain[issuerCertNum].certLength);
    pOcspContext->pOcspSettings->pIssuerCertInfo[0].certLen = pCertChain[issuerCertNum].certLength;


exit:
    return status;
}


#if defined(__ENABLE_MOCANA_SSL_SERVER__) || defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)
/*------------------------------------------------------------------*/
/* This method to be called from SSL server code to add OCSP response in the    */
/* Certificate Status Message to be send to the client                                      */

extern MSTATUS
SSL_OCSP_getOcspResponse(SSLSocket* pSSLSock, ubyte** ppResponse, ubyte4* pRetResponseLen)

{

    ocspContext*        pOcspContext  = (ocspContext *)pSSLSock->pOcspContext;
    httpContext*        pHttpContext  = NULL;
    ASN1_ITEMPTR        pIssuerCert   = NULL;
    ubyte*              pRequest      = NULL;
    ubyte4              requestLen    = 0;
    intBoolean          isDone        = FALSE;
    ubyte               i             = 0;
    MSTATUS             status        = OK;

    /* Create a Client Context */
    if (OK > (status = OCSP_CLIENT_createContext(&pOcspContext)))
        goto exit;


    /* Check for cached response; if no extensions present */
    if ((NULL == pSSLSock->pExts) &&
        (NULL != SSL_sslSettings()->pCachedOcspResponse) &&
         (0 < SSL_sslSettings()->cachedOcspResponseLen))
    {
        /* Check if cached response is valid */
        TimeDate curTime;
        sbyte4   timeDiff = 0;

        /* Get the system time in GMT */
        if (OK > (status = RTOS_timeGMT(&curTime)))
            goto exit;

        if (OK > (status = DATETIME_diffTime(&SSL_sslSettings()->nextUpdate,
            &curTime,&timeDiff)))
        {
            goto exit;
        }

        /* 60 sec; offset difference less than this would mean re-fetch of response */
        if (60 < timeDiff)
        {
            *ppResponse = SSL_sslSettings()->pCachedOcspResponse;
            *pRetResponseLen = SSL_sslSettings()->cachedOcspResponseLen;

            /* No need to fetch new response */
            goto exit;

        } else {
            /* Reset the cached response */
            if (SSL_sslSettings()->pCachedOcspResponse)
                FREE(SSL_sslSettings()->pCachedOcspResponse);

            SSL_sslSettings()->pCachedOcspResponse = NULL;
            SSL_sslSettings()->cachedOcspResponseLen = 0;
        }
    }

    /* Set the user configuration */
    pOcspContext->pOcspSettings->certCount                  = 1;

    pOcspContext->pOcspSettings->hashAlgo                   = sha1_OID;
    pOcspContext->pOcspSettings->pResponderUrl              = (sbyte *)pSSLSock->roleSpecificInfo.server.pResponderUrl;
    pOcspContext->pOcspSettings->shouldAddServiceLocator    = FALSE;
    pOcspContext->pOcspSettings->shouldSign                 = FALSE;
    pOcspContext->pOcspSettings->signingAlgo                = FALSE;
    pOcspContext->pOcspSettings->timeSkewAllowed            = 60;

    /* check if responder URL has been configured */
    if ((0 >= pSSLSock->roleSpecificInfo.server.numCertificates))
    {
        status = ERR_OCSP_INIT_FAIL;
        goto exit;
    }

    if (!pOcspContext->pOcspSettings->pResponderUrl)
    {
        if (OK > (status = OCSP_CLIENT_getResponderIdfromCert(pSSLSock->roleSpecificInfo.server.certificates[0].data,
                                                              pSSLSock->roleSpecificInfo.server.certificates[0].length,
                                                              (ubyte **)&pOcspContext->pOcspSettings->pResponderUrl)))
        {
            status = ERR_OCSP_BAD_AIA;
            goto exit;
        }
    }

    /* END OF USER CONFIG                                          */

    if (OK > (status = OCSP_CLIENT_httpInit(&pHttpContext, pOcspContext)))
        goto exit;

    /* Populating data in internal format based on user config     */
    if (NULL == (pOcspContext->pOcspSettings->pCertInfo = MALLOC(sizeof(OCSP_singleRequestInfo)*
                                                                pOcspContext->pOcspSettings->certCount)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = MOC_MEMSET((ubyte *)pOcspContext->pOcspSettings->pCertInfo, 0x00, sizeof(OCSP_singleRequestInfo))))
        goto exit;

    if (NULL == (pOcspContext->pOcspSettings->pIssuerCertInfo = MALLOC(sizeof(OCSP_singleRequestInfo) *
                                                                pOcspContext->pOcspSettings->certCount)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = MOC_MEMSET((ubyte *)pOcspContext->pOcspSettings->pIssuerCertInfo, 0x00, sizeof(OCSP_singleRequestInfo))))
        goto exit;

    if (0 < pSSLSock->roleSpecificInfo.server.numCertificates)
    {
        /* leaf node would be the cert in question; multiple response not supported */
        pOcspContext->pOcspSettings->pCertInfo[i].pCert   = pSSLSock->roleSpecificInfo.server.certificates[0].data;
        pOcspContext->pOcspSettings->pCertInfo[i].certLen = pSSLSock->roleSpecificInfo.server.certificates[0].length;

        ubyte issuercert = (1 < pSSLSock->roleSpecificInfo.server.numCertificates)? 1 : 0;

        pOcspContext->pOcspSettings->pIssuerCertInfo[i].pCertPath = pSSLSock->roleSpecificInfo.server.certificates[issuercert].data;
        pOcspContext->pOcspSettings->pIssuerCertInfo[i].certLen = pSSLSock->roleSpecificInfo.server.certificates[issuercert].length;

        if (0 == issuercert) {
            /* Check if the certificate is a root certificate */
            ubyte*  pTempCert        = pSSLSock->roleSpecificInfo.server.certificates[issuercert].data;
            ubyte4  tempLen          = pSSLSock->roleSpecificInfo.server.certificates[issuercert].length;
            MemFile mf;
            CStream cs;

            MF_attach(&mf, tempLen, pTempCert);
            CS_AttachMemFile(&cs, &mf );

            if (OK > (status = ASN1_Parse(cs, &pIssuerCert)))
            {
                goto exit;
            }

            if (OK > (status = CERT_isRootCertificate(pIssuerCert, cs)))
            {
                if (ERR_FALSE != status)
                {
                    goto exit;
                }
                else {
                    /* Not a root certificate
                                   * Check if we have its root in certstore
                                   */

                    certDistinguishedName *pIssuer = NULL;
                    certDescriptor        caCertificate;

                    caCertificate.pCertificate     = NULL;
                    caCertificate.certLength       = 0;

                    if (OK > (status = (MSTATUS)CA_MGMT_allocCertDistinguishedName(&pIssuer)))
                        goto exit;

                    status = CERT_extractDistinguishedNames(pIssuerCert, cs, 0, pIssuer);

                    if (OK > status)
                    {
                        CA_MGMT_freeCertDistinguishedName(&pIssuer);
                        goto exit;
                    }

                    status = SSL_sslSettings()->funcPtrCertificateStoreLookup(SSL_findConnectionInstance(pSSLSock),
                                                                              pIssuer, &caCertificate);
                    CA_MGMT_freeCertDistinguishedName(&pIssuer);

                    if (!(OK > status))
                    {
                        pOcspContext->pOcspSettings->pIssuerCertInfo[i].pCertPath = caCertificate.pCertificate;
                        pOcspContext->pOcspSettings->pIssuerCertInfo[i].certLen = caCertificate.certLength;
                    }
                }
            }
        }
    }

    /* Call the API to generate OCSP request */
    if (OK > (status = OCSP_CLIENT_generateRequest(pOcspContext,
            pSSLSock->pExts, pSSLSock->numOfExtension, &pRequest, &requestLen)))
        goto exit;

    if (OK > (status = OCSP_CLIENT_sendRequest(pOcspContext, pHttpContext, pRequest, requestLen)))
       goto exit;

    while (!isDone)
    {
        if (OK > (status = OCSP_CLIENT_recv(pOcspContext, pHttpContext, &isDone, ppResponse, pRetResponseLen)))
            goto exit;
    }

    /* Cache response */
    SSL_sslSettings()->pCachedOcspResponse = *ppResponse;
    SSL_sslSettings()->cachedOcspResponseLen = *pRetResponseLen;

    /* Need to parse response in order to get the nextUpdate value */
    if (OK > (status = OCSP_CLIENT_parseResponse(pOcspContext,
        SSL_sslSettings()->pCachedOcspResponse,
        SSL_sslSettings()->cachedOcspResponseLen)))
    {
        goto exit;
    }

    byteBoolean isNextUpdate = TRUE;
    if (OK > (status = OCSP_CLIENT_getCurrentNextUpdate(pOcspContext,
        &SSL_sslSettings()->nextUpdate, &isNextUpdate)))
    {
        goto exit;
    }
exit:
    if (pIssuerCert)
    {
        TREE_DeleteTreeItem( (TreeItem*) pIssuerCert);
    }

    /* API to free the context and shutdown MOCANA OCSP CLIENT */
    OCSP_CLIENT_releaseContext(&pOcspContext);
    return status;

}/* SSL_OCSP_getOcspResponse */

/*------------------------------------------------------------------*/

static ubyte4
SSL_OCSP_getChildCount(ASN1_ITEMPTR pItem)
{
    ubyte4       count  = 0;
    ASN1_ITEMPTR pChild = NULL;

    if (!pItem)
        return count;

    pChild = ASN1_FIRST_CHILD(pItem);
    while (pChild)
    {
        count++;
        pChild = ASN1_NEXT_SIBLING(pChild);
    }

    return count;

} /* SSL_OCSP_getChildCount */

/*------------------------------------------------------------------*/

extern MSTATUS
SSL_OCSP_parseExtensions(ubyte* pExtensions, ubyte4 extLen, extensions** ppExts, ubyte4* pExtCount)
{
    MSTATUS status = OK;
    MemFile       memFile;
    CStream       cs;
    ASN1_ITEMPTR  pExtRoot      = NULL;
    ASN1_ITEMPTR  pExtRootChild = NULL;
    extensions*   pTempExts     = NULL;
    ASN1_ITEMPTR  pItem         = NULL;

    MF_attach(&memFile, extLen, pExtensions);
    CS_AttachMemFile(&cs, &memFile);

    if (OK > (status = ASN1_Parse(cs, &pExtRoot)))
        goto exit;

    if (pExtRoot)
    {
        ASN1_ITEMPTR pExt;
        ubyte4       count = 0;

        if (NULL == (pExtRootChild = ASN1_FIRST_CHILD(pExtRoot)))
        {
            status = ERR_OCSP_INVALID_STRUCT;
            goto exit;
        }

        *pExtCount = SSL_OCSP_getChildCount(pExtRootChild);

        if (0 == *pExtCount)
            goto exit;

        if (NULL == (pTempExts = MALLOC((*pExtCount)*sizeof(extensions))))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        if (NULL == (pExt = ASN1_FIRST_CHILD(pExtRootChild)))
        {
            status = ERR_OCSP_INVALID_STRUCT;
            goto exit;
        }

        while (pExt)
        {
            ubyte *buf = NULL;

            if (NULL == (pItem = ASN1_FIRST_CHILD(pExt)))
            {
                status = ERR_OCSP_INVALID_STRUCT;
                goto exit;
            }

            if (OK != ASN1_VerifyType(pItem, OID))
            {
                status = ERR_OCSP_INVALID_STRUCT;
                goto exit;
            }

            if (OK == ASN1_VerifyOID(pItem, cs, id_pkix_ocsp_nonce_OID))
            {
                (pTempExts+count)->oid = (ubyte *)id_pkix_ocsp_nonce_OID;
            }
            else if (OK == ASN1_VerifyOID(pItem, cs, id_pkix_ocsp_crl_OID))
            {
                (pTempExts+count)->oid = (ubyte *)id_pkix_ocsp_crl_OID;
            }
            else
            {
                /* unknown extension, ignore? */
            }

            if (NULL == (pItem = ASN1_NEXT_SIBLING(pItem)))
            {
                status = ERR_OCSP_INVALID_STRUCT;
                goto exit;
            }

            if (OK == ASN1_VerifyType(pItem, BOOLEAN))
            {
                (pTempExts+count)->isCritical = pItem->data.m_boolVal;
                if (NULL == (pItem = ASN1_NEXT_SIBLING(pItem)))
                {
                    status = ERR_OCSP_INVALID_STRUCT;
                    goto exit;
                }
            }
            else
            {
                (pTempExts+count)->isCritical = FALSE;
            }

            /* unwrap OCTETSTRING */
            if (NULL == (pItem = ASN1_FIRST_CHILD(pItem)))
            {
                status = ERR_OCSP_INVALID_STRUCT;
                goto exit;
            }

            buf = (ubyte*)CS_memaccess(cs,
                                       pItem->dataOffset, pItem->length);

            if (NULL ==((pTempExts+count)->value = MALLOC(pItem->length)))
            {
                status = ERR_MEM_ALLOC_FAIL;
                goto exit;
            }

            MOC_MEMCPY((pTempExts+count)->value, buf, pItem->length);
            (pTempExts+count)->valueLen = pItem->length;

            if (buf)
            {
                CS_stopaccess(cs, buf);
            }

            count = count + 1;
            pExt = ASN1_NEXT_SIBLING(pExt);
        }

        *ppExts   = pTempExts;
        pTempExts = NULL;
    }

exit:
    if (pTempExts)
    {
        ubyte count;

        for (count = 0; count < *pExtCount; count++)
        {
            if ((pTempExts+count)->value)
                FREE ((pTempExts+count)->value);
        }

        FREE (pTempExts);
    }

    if (pExtRoot)
        TREE_DeleteTreeItem((TreeItem*)pExtRoot);

    return status;
}

#endif /* defined(__ENABLE_MOCANA_SSL_SERVER__) || defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__) */

/*------------------------------------------------------------------*/
/* This method is to be called from the SSL client to validate the OCSP response  */
/* received in the Certificate Status  message from the server                            */

#if defined(__ENABLE_MOCANA_SSL_CLIENT__) || defined(__ENABLE_MOCANA_SSL_ASYNC_CLIENT_API__)
extern MSTATUS
SSL_OCSP_validateOcspResponse(SSLSocket *pSSLSock, ubyte* pResponse, ubyte4 responseLen)
{

    ocspContext*        pOcspContext    = (ocspContext *)pSSLSock->pOcspContext;

    ubyte*              pRequest        = NULL;
    ubyte4              requestLen      = 0;
    extensions*         pExts           = NULL;
    ubyte4              extCount        = 0;

    OCSP_certStatus*    pCertStatus     = NULL;
    OCSP_responseStatus respStatus;
    TimeDate            producedAt;

#ifdef __ENABLE_MOCANA_DEBUG_CONSOLE__
    OCSP_certID*        pCertId         = NULL;
    TimeDate            thisUpdate;
    TimeDate            nextUpdate;
    byteBoolean         isNextUpdate;
    ubyte               i               = 0;
    ubyte               j               = 0;
#endif

    MSTATUS             status       =  OK;

    pOcspContext->pOcspSettings->hashAlgo                   = sha1_OID;
    pOcspContext->pOcspSettings->shouldAddServiceLocator    = FALSE;
    pOcspContext->pOcspSettings->shouldSign                 = FALSE;
    pOcspContext->pOcspSettings->signingAlgo                = NULL;
    pOcspContext->pOcspSettings->timeSkewAllowed            = 360;
    pOcspContext->pOcspSettings->shouldAddNonce             = FALSE;

    /* Populating data in internal format based on user config     */
    if ((0 >= pOcspContext->pOcspSettings->certCount))
    {
       status = ERR_OCSP_INIT_FAIL;
       goto exit;
    }

    /* Call the API to generate OCSP request just to make sure our internal state is correct */
    if (OK > (status = OCSP_CLIENT_generateRequest(pOcspContext, pExts, extCount, &pRequest, &requestLen)))
        goto exit;

    /* API to parse the response */
    if (OK > (status = OCSP_CLIENT_parseResponse(pOcspContext, pResponse, responseLen)))
        goto exit;

    /* API to check the OCSP response status */
    if (OK > (status = OCSP_CLIENT_getResponseStatus(pOcspContext,  &respStatus)))
        goto exit;

    if (ocsp_successful == respStatus)
    {

        DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "OCSP Response Details:");

        /* API to get the producedAt attribute in the OCSP response */
        if (OK > (status = OCSP_CLIENT_getProducedAt(pOcspContext, &producedAt)))
                goto exit;

        DEBUG_CONSOLE_printf ("Produced At       -> %2d:%2d:%2d %2d %2d %4d \n",producedAt.m_hour,
                                                                  producedAt.m_minute,
                                                                  producedAt.m_second,
                                                                  producedAt.m_day,
                                                                  producedAt.m_month,
                                                                  (producedAt.m_year + 1970));

        if (OK > (status = OCSP_CLIENT_getCurrentCertStatus(pOcspContext, &pCertStatus)))
            goto exit;

        /* Note: Free pCertStatus before exiting to avoid memory leaks */

        if (NULL == pCertStatus)
        {
            status = ERR_NULL_POINTER;
            goto exit;
        }

        switch (pCertStatus->flag)
        {
            case ocsp_good:
                 DEBUG_CONSOLE_printf ("Certificate status-> GOOD\n");
                 break;

            case ocsp_revoked:
                 status       =  ERR_SSL_EXTENSION_CERTIFICATE_STATUS_RESPONSE;
                 DEBUG_CONSOLE_printf ("Certificate status-> Revoked\n");
                 DEBUG_CONSOLE_printf ("Revocation Time   -> %2d:%2d:%2d %2d %2d %4d \n",pCertStatus->revocationTime.m_hour,
                                                                           pCertStatus->revocationTime.m_minute,
                                                                           pCertStatus->revocationTime.m_second,
                                                                           pCertStatus->revocationTime.m_day,
                                                                           pCertStatus->revocationTime.m_month,
                                                                           (pCertStatus->revocationTime.m_year + 1970));
                 break;

            case ocsp_unknown:
                 status       =  ERR_SSL_EXTENSION_CERTIFICATE_STATUS_RESPONSE;
                 DEBUG_CONSOLE_printf ("Certificate status-> UnKnown\n");
                 break;
        }


        if (OK > status)
        {
            goto exit;
        }

        DEBUG_CONSOLE_printf("\nCERT %d\n", i+1);

        /* API to get the certificate specific info present in CertId field of response */
        if (OK > (status = OCSP_CLIENT_getCurrentCertId(pOcspContext,&pCertId)))
            goto exit;

                 DEBUG_CONSOLE_printf ("Serial Number:    ->");

        for (j = 0; j < pCertId->serialNumberLength; j++)
            DEBUG_CONSOLE_printf(" %X ",pCertId->serialNumber[j]);

        DEBUG_CONSOLE_printf("\n");

        if (OK > (status = OCSP_CLIENT_getCurrentThisUpdate(pOcspContext, &thisUpdate)))
            goto exit;

        DEBUG_CONSOLE_printf ("This Update       -> %2d:%2d:%2d %2d %2d %4d \n",thisUpdate.m_hour,
                                                                  thisUpdate.m_minute,
                                                                  thisUpdate.m_second,
                                                                  thisUpdate.m_day,
                                                                  thisUpdate.m_month,
                                                                  (thisUpdate.m_year + 1970));


        if (OK > (status = OCSP_CLIENT_getCurrentNextUpdate(pOcspContext, &nextUpdate, &isNextUpdate)))
            goto exit;

        if (isNextUpdate)
        {
            DEBUG_CONSOLE_printf ("Next Update       -> %2d:%2d:%2d %2d %2d %4d \n",nextUpdate.m_hour,
                                                                      nextUpdate.m_minute,
                                                                      nextUpdate.m_second,
                                                                      nextUpdate.m_day,
                                                                      nextUpdate.m_month,
                                                                      (nextUpdate.m_year + 1970));



            DEBUG_CONSOLE_printf ("\n");
        }

        if (pCertId)
        {
            if (pCertId->keyHash)
                FREE(pCertId->keyHash);

            if (pCertId->nameHash)
                FREE(pCertId->nameHash);

            if (pCertId->serialNumber)
                FREE(pCertId->serialNumber);

            FREE(pCertId);
        }


    }
    else
    {
        /* In case the response status is not successful */
        switch (respStatus)
        {
            case ocsp_malformedRequest:
            {
                 DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "OCSP_CLIENT: Illegal confirmation request(malformedRequest)");
                 break;
            }

            case ocsp_internalError:
            {
                 DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "OCSP_CLIENT: Internal Error in Issuer(internalError)");
                 break;
            }

            case ocsp_tryLater:
            {
                 DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "OCSP_CLIENT: Try again later(tryLater)");
                 break;
            }

            case ocsp_sigRequired:
            {
                 DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "OCSP_CLIENT: Must sign the request(sigRequired)");
                 break;
            }

            case ocsp_unauthorized:
            {
                 DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "OCSP_CLIENT: Request Unauthorized(unauthorized)");
                 break;
            }

            default:
            {
                 DEBUG_PRINTNL(DEBUG_SSL_MESSAGES, "OCSP_CLIENT: Failed with unknown status");
                 break;
            }

        }
    }
exit:
    if (pCertStatus)
        FREE(pCertStatus);

    /* API to free the context and shutdown MOCANA OCSP CLIENT */
    if (pRequest)
        FREE(pRequest);

    OCSP_CLIENT_releaseContext(&pOcspContext);

    return status;
}
#endif /* __ENABLE_MOCANA_SSL_CLIENT__ */
/*------------------------------------------------------------------*/
#endif /* #ifdef __ENABLE_TLSEXT_RFC6066__ */
