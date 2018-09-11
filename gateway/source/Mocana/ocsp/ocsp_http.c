/* Version: mss_v6_3 */
/*
 * ocsp_http.c
 *
 * OCSP Client with HTTP as the transport
 *
 * Copyright Mocana Corp 2005-2008. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

/*! \file ocsp_http.c OCSP HTTP %client API.
This file contains OCSP HTTP %client API functions.

\since 5.3
\version 5.3 and later

! Flags
To enable any of this file's functions, the following flag must be defined in
moptions.h:
- $__ENABLE_MOCANA_OCSP_CLIENT__$

! External Functions
This file contains the following public ($extern$) functions:
- OCSP_CLIENT_httpInit
- OCSP_CLIENT_recv
- OCSP_CLIENT_sendRequest

*/
#include "../common/moptions.h"

#if (defined(__ENABLE_MOCANA_OCSP_CLIENT__)  &&  \
    ((defined(__ENABLE_MOCANA_PKI_CLIENT_OCSP__) || defined(__ENABLE_MOCANA_SSH_SERVER__) || \
     defined (__ENABLE_MOCANA_SSL_SERVER__)) || defined(__ENABLE_MOCANA_SSL_ASYNC_SERVER_API__)))

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
#include "../asn1/parsecert.h"
#include "../asn1/derencoder.h"
#include "../crypto/pkcs_common.h"
#include "../crypto/base64.h"
#include "../asn1/ASN1TreeWalker.h"
#include "../crypto/asn1cert.h"
#include "../common/uri.h"
#include "../common/mudp.h"
#include "../common/mtcp.h"
#include "../common/mtcp_async.h"
#include "../common/debug_console.h"
#include "../http/http_context.h"
#include "../http/http.h"
#include "../http/http_common.h"
#include "../http/client/http_request.h"
#include "../ocsp/ocsp.h"
#include "../ocsp/ocsp_context.h"
#include "../ocsp/client/ocsp_client.h"
#include "../ocsp/ocsp_http.h"
#include "../harness/harness.h"


/*------------------------------------------------------------------*/

typedef struct requestBodyCookie
{
    ubyte*    data;
    ubyte4    dataLen;
    ubyte4    curPos;

} requestBodyCookie;


/*------------------------------------------------------------------*/

/* Methods to handle HTTP as a transport medium for OCSP messages   */
/* The example is illustrated usign MOCANA HTTP client example      */
static sbyte4
my_HttpTcpSend(httpContext *pHttpContext, sbyte4 socket, ubyte *pDataToSend,
               ubyte4 numBytesToSend, ubyte4 *pRetNumBytesSent, sbyte4 isContinueFromBlock)
{

    TCP_WRITE(socket, (sbyte *)pDataToSend,numBytesToSend, pRetNumBytesSent);
    return 0;

}

/*------------------------------------------------------------------*/

static sbyte4
OCSP_CLIENT_http_requestBodyCallback(httpContext *pHttpContext, ubyte **ppDataToSend,
                                     ubyte4 *pDataLength, void *pRequestBodyCookie)
{

    MSTATUS                status       = OK;
    static const ubyte4    BLOCKSIZE    = 2000;
    requestBodyCookie*     cookie;

    if (pRequestBodyCookie)
    {
        cookie       = (requestBodyCookie*)pRequestBodyCookie;
        *pDataLength = (cookie->dataLen - cookie->curPos) > BLOCKSIZE? BLOCKSIZE : (cookie->dataLen - cookie->curPos);

        if (NULL == (*ppDataToSend = MALLOC(*pDataLength)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMCPY(*ppDataToSend, cookie->data + cookie->curPos, *pDataLength);
        cookie->curPos = cookie->curPos + (*pDataLength);

        if (cookie->dataLen == cookie->curPos)
            pHttpContext->isBodyDone = TRUE;
        else
            pHttpContext->isBodyDone = FALSE;
    }
    else
    {

        *ppDataToSend            = NULL;
        *pDataLength             = 0;
        pHttpContext->isBodyDone = TRUE;
    }

exit:
    return status;

}

/*------------------------------------------------------------------*/

static sbyte4
OCSP_CLIENT_http_responseHeaderCallback(httpContext *pHttpContext, sbyte4 isContinueFromBlock)
{
    /* do nothing */
    return OK;
}

/*------------------------------------------------------------------*/

static sbyte4
OCSP_CLIENT_http_responseBodyCallback(httpContext *pHttpContext,
                                      ubyte *pDataReceived,
                                      ubyte4 dataLength,
                                      sbyte4 isContinueFromBlock)
{
    /* the index for ContentLength */
    static const ubyte4 index      = NUM_HTTP_RESPONSES + NUM_HTTP_GENERALHEADERS + ContentLength;
    ubyte*              pNewBuffer = NULL;
    MSTATUS             status     = OK;

    /* if contentlength known, allocate memory only once */
    if ((0 >= pHttpContext->receivedPendingDataLength) &&
        (pHttpContext->responseBitmask[index / 8] & (1 << (index & 7))))
    {
        sbyte*            stop;
        sbyte4            contentLength;
        HTTP_stringDescr* strDescr = &(pHttpContext->responses[index]);

        contentLength = MOC_ATOL((sbyte*)strDescr->pHttpString, (const sbyte**)&stop);

        /* This needs to be freed by the application */
        pHttpContext->pReceivedPendingDataFree = pHttpContext->pReceivedPendingData = (ubyte*) MALLOC(contentLength);
    }

    /* accumulate response body in httpContext pReceivedDataPending */
    if (!(pHttpContext->responseBitmask[index/8] & (1<<(index & 7))))
    {
        pNewBuffer = (ubyte*)MALLOC(pHttpContext->receivedPendingDataLength + dataLength);

        if (NULL == pNewBuffer)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        if (0 < pHttpContext->receivedPendingDataLength)
        {
            /* copy existing data */
            MOC_MEMCPY(pNewBuffer, pHttpContext->pReceivedPendingDataFree, pHttpContext->receivedPendingDataLength);
        }

        MOC_MEMCPY(pNewBuffer + pHttpContext->receivedPendingDataLength, pDataReceived, dataLength);

        if (pHttpContext->pReceivedPendingDataFree)
        {
            FREE(pHttpContext->pReceivedPendingDataFree);
        }

        pHttpContext->pReceivedPendingDataFree = pHttpContext->pReceivedPendingData = pNewBuffer;
        pNewBuffer = NULL;
    }
    else
    {
        MOC_MEMCPY(pHttpContext->pReceivedPendingDataFree+pHttpContext->receivedPendingDataLength, pDataReceived, dataLength);
    }

    pHttpContext->receivedPendingDataLength += dataLength;

exit:
    return (sbyte4)status;
}


/*------------------------------------------------------------------*/
/*! Create an HTTP OCSP connection context.
This function creates (and returns) an HTTP OCSP connection context to the HTTP
server, which can be passed to subsequent OCSP_CLIENT_http* function calls.

\since 5.3
\version 5.3 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_OCSP_CLIENT__$

#Include %file:#&nbsp;&nbsp;ocsp_http.h

\param ppHttpContext    On return, double pointer to HTTP context.
\param pOcspContext     Pointer to HTTP OCSP server context, returned from
OCSP_CLIENT_createContext.

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error %status, use the
$DISPLAY_ERROR$ macro.

*/
MOC_EXTERN MSTATUS
OCSP_CLIENT_httpInit(httpContext **ppHttpContext, ocspContext *pOcspContext)
{

    httpContext*          pHttpContext;
    sbyte*                uriStrLocal    = NULL;
    URI*                  uri            = NULL;
    sbyte *               host           = NULL;
    sbyte                 pIpStr[40];
    sbyte2                port;
    TCP_SOCKET            socketServer;
    MSTATUS               status         = OK;

    uriStrLocal = pOcspContext->pOcspSettings->pResponderUrl;
    if (OK > (status = URI_ParseURI(uriStrLocal, &uri)))
        goto exit;

    if (OK > (status = URI_GetHost(uri, &host)))
        goto exit;

    if (OK > (status = URI_GetPort(uri, &port)))
        goto exit;

    if (port == 0)
        port = 80;

    if (OK > (status = TCP_GETHOSTBYNAME((char *)host, (char *)pIpStr)))
        goto exit;

    if (OK > (status = TCP_CONNECT(&socketServer, pIpStr, port)))
        goto exit;

    if (OK > (status = HTTP_connect(&pHttpContext, socketServer)))
        goto exit;

    HTTP_httpSettings()->funcPtrHttpTcpSend            = my_HttpTcpSend;
    HTTP_httpSettings()->funcPtrResponseHeaderCallback = OCSP_CLIENT_http_responseHeaderCallback;
    HTTP_httpSettings()->funcPtrResponseBodyCallback   = OCSP_CLIENT_http_responseBodyCallback;
    HTTP_httpSettings()->funcPtrRequestBodyCallback    = OCSP_CLIENT_http_requestBodyCallback;

    *ppHttpContext = pHttpContext;
    pHttpContext   = NULL;

exit:
    if (host)
        FREE(host);
    if (uri)
        URI_DELETE(uri);

    return status;
}


/*------------------------------------------------------------------*/
/*! Send an OCSP request via HTTP transport.
This function sends an OCSP request using HTTP as the transport.

\since 5.3
\version 5.3 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_OCSP_CLIENT__$

#Include %file:#&nbsp;&nbsp;ocsp_http.h

\param pOcspContext     Pointer to HTTP OCSP server context, returned from
OCSP_CLIENT_createContext.
\param ppHttpContext    Pointer to HTTP context returned from
OCSP_CLIENT_httpInit.
\param pRequest         Pointer to request to send.
\param requestLen       Number of bytes in the request ($pRequest$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error %status, use the
$DISPLAY_ERROR$ macro.

*/
MOC_EXTERN MSTATUS
OCSP_CLIENT_sendRequest(ocspContext *pOcspContext, httpContext *pHttpContext, ubyte *pRequest, ubyte4 requestLen)
{
    ubyte4                  index;
    requestBodyCookie*      pCookie        = NULL;
/*    ubyte4                i; */


    MSTATUS                 status         = OK;

    if (!pOcspContext || !pHttpContext)
    {
        status = ERR_CMP_INVALID_CONTEXT;
        goto exit;
    }

    if ((!pRequest) || (0 == requestLen))
    {
        status = ERR_OCSP_INVALID_INPUT;
        goto exit;
    }

    if (OK > (status = HTTP_REQUEST_setRequestMethodIfNotSet(pHttpContext, &mHttpMethods[POST])))
        goto exit;

    index = Host;


    index = NUM_HTTP_REQUESTS + NUM_HTTP_GENERALHEADERS + ContentType;

    if (OK > (status = HTTP_COMMON_setHeaderIfNotSet(pHttpContext, index, (ubyte *)"application/ocsp-request", 24)))
        goto exit;

    if (OK > (status = HTTP_REQUEST_setContentLengthIfNotSet(pHttpContext, requestLen)))
        goto exit;

    HTTP_httpSettings()->funcPtrHttpTcpSend            = my_HttpTcpSend;
    HTTP_httpSettings()->funcPtrResponseHeaderCallback = OCSP_CLIENT_http_responseHeaderCallback;
    HTTP_httpSettings()->funcPtrResponseBodyCallback   = OCSP_CLIENT_http_responseBodyCallback;
    HTTP_httpSettings()->funcPtrRequestBodyCallback    = OCSP_CLIENT_http_requestBodyCallback;

    /* set request URI */
    if (OK > (status = HTTP_REQUEST_setRequestUriIfNotSet(pHttpContext, (sbyte *)pOcspContext->pOcspSettings->pResponderUrl)))
        goto exit;
/* Can be enabled for debugging, if required */
#if 0
    if (OK > (status = MOCANA_writeFile("../src/ocsp/client/generatedrequest.der", pRequest, requestLen)))
        goto exit;
#endif

    /* create cookie for requestBody callback */
    if (NULL == (pCookie = MALLOC(sizeof(requestBodyCookie))))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    pCookie->curPos  = 0;
    pCookie->data    = pRequest;
    pCookie->dataLen = requestLen;

    if (OK > (status = HTTP_setCookie(pHttpContext, pCookie)))
        goto exit;

    /* send request */
    if (OK > (status = HTTP_recv(pHttpContext, NULL, 0)))
        goto exit;

    /* finish sending the request via transport... */
    while (!HTTP_REQUEST_isDoneSendingRequest(pHttpContext))
    {
        if (OK > (status = HTTP_continue(pHttpContext)))
            goto exit;
    }

exit:
    if (pCookie)
        FREE (pCookie);

    return status;
}


/*------------------------------------------------------------------*/
/*! Receive an HTTP OCSP message.
This function receives an HTTP OCSP message.

\since 5.3
\version 5.3 and later

! Flags
To enable this function, the following flag must be defined in moptions.h:
- $__ENABLE_MOCANA_OCSP_CLIENT__$

#Include %file:#&nbsp;&nbsp;ocsp_http.h

\param pOcspContext     Pointer to HTTP OCSP server context, returned from
OCSP_CLIENT_createContext.
\param ppHttpContext    Pointer to HTTP context returned from
OCSP_CLIENT_httpInit.
\param pHttpContext     Pointer to HTTP connection context.
\param isDone           On return, pointer to $TRUE$ if the entire message is
received; otherwise pointer to $FALSE$.
\param ppResponse       On return, pointer to response message structure.
\param pResponseLen     On return, number of bytes in the response ($ppResponse$).

\return $OK$ (0) if successful; otherwise a negative number error code
definition from merrors.h. To retrieve a string containing an English text
error identifier corresponding to the function's returned error %status, use the
$DISPLAY_ERROR$ macro.

*/
MOC_EXTERN MSTATUS
OCSP_CLIENT_recv(ocspContext *pOcspContext, httpContext *pHttpContext, intBoolean *isDone, ubyte **ppResponse, ubyte4 *pResponseLen)
{
    sbyte                    tcpBuffer[512];
    ubyte4                   nRet;
    ubyte*                   pHttpResp      = NULL;
    ubyte4                   httpRespLen    = 0;
    MSTATUS                  status         = OK;


    if ((NULL == pOcspContext) || (NULL == pHttpContext) || (NULL == ppResponse) || (NULL == pResponseLen))
    {
        status = ERR_OCSP_INVALID_INPUT;
        goto exit;
    }

    *isDone = FALSE;
    *ppResponse = NULL;
    *pResponseLen = 0;


    status = TCP_READ_AVL(pHttpContext->socket, tcpBuffer, 512, &nRet, 500000);

    if ((status == ERR_TCP_READ_TIMEOUT) || (nRet <= 0))
    {
        goto exit;
    }
    else if (status < OK)
    {
        goto exit;
    }

    if (OK > (status = HTTP_recv(pHttpContext, (ubyte*)tcpBuffer, nRet)))
        goto exit;

    if (HTTP_isDone(pHttpContext))
    {
        ubyte4 statusCode;

        *isDone = TRUE;

        if (OK > (status = HTTP_REQUEST_getStatusCode(pHttpContext, &statusCode)))
            goto exit;

        if (OK > (status = HTTP_REQUEST_getResponseContent(pHttpContext, &pHttpResp, &httpRespLen)))
            goto exit;

        /* Status code < 300;      indicates that the client's request was successfully
        received, understood, and accepted. */
        if (statusCode < 300)
        {
            const ubyte*        pContentType;
            ubyte4              contentTypeLen;

            if (OK > (status = HTTP_REQUEST_getContentType(pHttpContext, &pContentType, &contentTypeLen)))
                goto exit;

            *ppResponse = pHttpResp;
            *pResponseLen = httpRespLen;
        }
        else
        {
            /* Status code > 300; indicates that further action needs to be taken by the */
            /* user agent in order to fulfill the request */

            const ubyte* reasonPhrase;
            ubyte4       reasonPhraseLength;
            ubyte*       str;

            if (OK > (status = HTTP_REQUEST_getStatusPhrase(pHttpContext, &reasonPhrase, &reasonPhraseLength)))
                goto exit;

            if (NULL == (str = MALLOC(reasonPhraseLength + 1)))
            {
                status = ERR_MEM_ALLOC_FAIL;
                goto exit;
            }

            if (OK > (status = MOC_MEMCPY(str, reasonPhrase, reasonPhraseLength)))
                goto exit;

            *(str + reasonPhraseLength) = '\0';

            status = ERR_HTTP;
            FREE(str);
        }
    }
exit:

    if ((FALSE == *isDone) && (pHttpResp))
        FREE (pHttpResp);

    return status;
}


MOC_EXTERN MSTATUS
OCSP_CLIENT_httpUninit(httpContext **ppHttpContext)
{
    MSTATUS status = OK;

    if (ppHttpContext)
    {
        if ((*ppHttpContext) && ((*ppHttpContext)->socket))
        {
            TCP_CLOSE_SOCKET((*ppHttpContext)->socket);
        }
        status = HTTP_close(ppHttpContext);
    }

    return status;
}

/*------------------------------------------------------------------*/

#endif /* #ifdef __ENABLE_MOCANA_OCSP_CLIENT__ */
