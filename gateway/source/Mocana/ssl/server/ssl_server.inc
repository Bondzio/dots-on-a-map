/*
 * ssl_server.inc
 *
 * SSL Server Specific Functionality
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __SSL_SERVER_INC__
#define __SSL_SERVER_INC__

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
#include "../../dtls/server/dtls_server.inc"
#endif

/*------------------------------------------------------------------*/

extern RTOS_MUTEX gSslSessionCacheMutex;
extern MSTATUS    SSL_setServerCert(SSLSocket* pSSLSock);

/* SESSION RESUMPTION */
/* array used for session resumption */
typedef struct SessionInfo
{
    SESSIONID               m_sessionId;
    CipherSuiteInfo*        m_pCipherSuite;
    ubyte                   m_masterSecret[SSL_MASTERSECRETSIZE];
    moctime_t               startTime;
    ubyte                   m_minorVersion;
#if defined(__ENABLE_MOCANA_DTLS_SERVER__) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
    const SrtpProfileInfo*  m_pSrtpProfile;
#endif
    ubyte2                  m_signatureAlgo; /* for TLS 1.2 */
	ubyte4                  m_clientECCurves;

} SessionInfo;

#ifndef SSL_SESSION_RESUME_TIMEOUT
#define SSL_SESSION_RESUME_TIMEOUT      (24 * (60 * (60 * 1000)))
#endif

static SESSIONID gNextSessionId;

#define SESSION_CACHE_SIZE (10)
static SessionInfo gSessionCache[SESSION_CACHE_SIZE];

/* The following lines are a quick fix, but necessary since this header file makes references to SUPPORTED_CURVES_FLAGS which is only defined in ssl_sock.c
*/

#ifndef __ENABLE_MOCANA_ECC_P192__
#define EC_P192_FLAG    0
#else
#define EC_P192_FLAG    (1 << tlsExtNamedCurves_secp192r1)
#endif

#ifdef __DISABLE_MOCANA_ECC_P224__
#define EC_P224_FLAG    0
#else
#define EC_P224_FLAG    (1 << tlsExtNamedCurves_secp224r1)
#endif

#ifdef __DISABLE_MOCANA_ECC_P256__
#define EC_P256_FLAG    0
#else
#define EC_P256_FLAG    (1 << tlsExtNamedCurves_secp256r1)
#endif

#ifdef __DISABLE_MOCANA_ECC_P384__
#define EC_P384_FLAG    0
#else
#define EC_P384_FLAG    (1 << tlsExtNamedCurves_secp384r1)
#endif

#ifdef __DISABLE_MOCANA_ECC_P521__
#define EC_P521_FLAG    0
#else
#define EC_P521_FLAG    (1 << tlsExtNamedCurves_secp521r1)
#endif

#ifndef SUPPORTED_CURVES_FLAGS
#define SUPPORTED_CURVES_FLAGS (EC_P192_FLAG | EC_P224_FLAG | EC_P256_FLAG | EC_P384_FLAG | EC_P521_FLAG)
#endif

/* END SESSION RESUMPTION */

#ifndef MOCANA_MAX_MODULUS_SIZE
#define MOCANA_MAX_MODULUS_SIZE     (512)
#endif

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
static ubyte4 calculateExtraFragmentHeader(SSLSocket *pSSLSock, ubyte4 numBufs);
static MSTATUS fragmentHandshakeMessages(SSLSocket *pSSLSock, ubyte4 fragmentLen);
#if defined(__ENABLE_MOCANA_DTLS_SRTP__)
static sbyte4 SSLSOCK_selectSrtpProfile( const SSLSocket* pSSLSock, sbyte4 start,
                            ubyte* clientSrtpProfiles, sbyte4 clientSrtpProfileLen);
#endif
#endif

#ifdef __ENABLE_TLSEXT_RFC6066__
extern MSTATUS SSL_OCSP_getOcspResponse(SSLSocket * pSSLSock,ubyte * * ppResponse,ubyte4 * pRetResponseLen);
extern MSTATUS SSL_OCSP_parseExtensions(ubyte * pExtensions,ubyte4 extLen,extensions * * ppExts,ubyte4 * pExtCount);
#endif

/*------------------------------------------------------------------*/

#ifdef __ENABLE_RFC3546__
static MSTATUS
SSLSOCK_sanityServerNameList(ubyte *pServerNameList, sbyte4 serverNameListLen)
{
    ubyte4  count = 0;
    ubyte4  nameLength;
    MSTATUS status = OK;
    ubyte   nameType;
    intBoolean hostNamePresent = FALSE;

    while (0 != serverNameListLen)
    {
        /* struct {
         *     NameType name_type;
         *     select (name_type) {
         *         case host_name: HostName;
         *     } name;
         * } ServerName;
         */

        if (3 > serverNameListLen)
        {
            status = (0 == count) ? ERR_SSL_EXTENSION_SERVER_LIST_LENGTH : ERR_SSL_EXTENSION_SERVER_NAME_LENGTH;
            goto exit;
        }

        if (0 != pServerNameList[0])
        {
            /* expected name_type (0) */
            status = ERR_SSL_EXTENSION_SERVER_LIST_LENGTH;
            goto exit;
        }

        nameType = *pServerNameList;

        switch(nameType)
        {
            case 0: /* name_type = HostName */
                if (hostNamePresent)
                {
                    /* RFC 6066; only one nameType allowed, duplicates not allowed */
                    status = ERR_SSL_EXTENSION_DUPLICATE_NAMETYPE_SNI;
                    goto exit;
                }
                hostNamePresent = TRUE;
        }

        /* skip past name_type */
        pServerNameList++;
        serverNameListLen--;

        /* skip past host_name length field */
        nameLength = getShortValue(pServerNameList);
        pServerNameList += 2;    serverNameListLen -= 2;

        if ((sbyte4)nameLength > serverNameListLen)
        {
            /* they lied, bad length */
            status = ERR_SSL_EXTENSION_SERVER_NAME_LENGTH;
            goto exit;
        }

        /* skip past host_name string */
        pServerNameList   += nameLength;
        serverNameListLen -= nameLength;

        /* next! */
        count++;
    }

exit:
    return status;
}

/*------------------------------------------------------------------*/

static MSTATUS
validateServerName(SSLSocket *pSSLSock)
{
    MSTATUS status = OK;
    sbyte4 i       = 0;
    ubyte4 nameLength;
    ubyte4 serverNameListLen;
    ubyte* pServerNameList   = NULL;
    sbyte* pServerName       = NULL;
    MemFile      mf;
    CStream      cs;
    ASN1_ITEMPTR pCertRoot   = NULL;

    serverNameListLen = pSSLSock->serverNameListLength;
    pServerNameList   = pSSLSock->serverNameList;

    /* compare each serverNamel sent by client */
    while(3 < serverNameListLen)
    {
        /* skip past name_type */
        pServerNameList++;
        serverNameListLen--;

        /* skip past host_name length field */
        nameLength = getShortValue(pServerNameList);
        pServerNameList += 2;
        serverNameListLen -= 2;

        /* nameLength + 1 as the method to compare subject Names expects
               * a null terminated string */
        if (NULL == (pServerName = MALLOC(nameLength + 1)))
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        MOC_MEMSET((ubyte *)pServerName, 0x00, nameLength + 1);
        MOC_MEMCPY(pServerName, pServerNameList, nameLength);

        /* for each certificate, check if the name matches common name */
        for (i = 0; i < pSSLSock->roleSpecificInfo.server.numCertificates; i++)
        {
            ubyte2 certLen = pSSLSock->roleSpecificInfo.server.certificates[i].length;

            MF_attach(&mf, certLen, pSSLSock->roleSpecificInfo.server.certificates[i].data);
            CS_AttachMemFile(&cs, &mf);

            if (OK > (status = ASN1_Parse(cs, &pCertRoot)))
                goto exit;

            if (OK > (status = CERT_CompSubjectCommonName(pCertRoot, cs,
                                                pServerName)))
            {
                status = ERR_SSL_EXTENSION_UNRECOGNIZED_NAME;
            }

            /* clean up */
            if (pCertRoot)
            {
                TREE_DeleteTreeItem((TreeItem*)pCertRoot);
                pCertRoot = NULL;
            }

            if (OK == status)
            {
                /* one match is good enough */
                goto exit;
            }
        }

        /* skip past host_name string */
        pServerNameList   += nameLength;
        serverNameListLen -= nameLength;

        if (NULL != pServerName)
        {
            FREE(pServerName);
            pServerName = NULL;
        }
    }

exit:

    if (NULL != pServerName)
    {
        FREE(pServerName);
        pServerName = NULL;
    }

    return status;
}

/*------------------------------------------------------------------*/

#ifdef __ENABLE_TLSEXT_RFC6066__

static MSTATUS
SSLSOCK_sanityCertificateStatusRequestExtension(SSLSocket *pSSLSock, ubyte *pCertStatusRequest, sbyte4 certStatusRequestLen)
{
    MSTATUS status          = OK;
    ubyte2  respIdListLen   = 0;
    ubyte2  ocspReqExtLen   = 0;
    ubyte2  offset          = 0;

    /* We do basic sanity test here */
    if (5 > certStatusRequestLen)
    {
         /* 5 = 1 + 2 byte repId Len + 2 byte Exten Len */
         status = ERR_SSL_EXTENSION_LENGTH;
         goto exit;
    }

    /* Check for CertificateStatusType */
    if (certStatusType_ocsp != (ubyte)(*pCertStatusRequest))
    {
        status = ERR_SSL_EXTENSION_UNKNOWN_FORMAT;
        goto exit;
    }

    offset += 1;

    /* Now check for ResponderId List Length */
    respIdListLen = getShortValue(pCertStatusRequest + offset);
    offset += 2;

    if (respIdListLen > certStatusRequestLen)
    {
        status = ERR_SSL_EXTENSION_LENGTH;
        goto exit;
    }

    /* Ignore responder Ids */
    offset += respIdListLen;

    /* Now check for OCSP extensions length */
    ocspReqExtLen = getShortValue(pCertStatusRequest + offset);
    offset += 2;

    if (ocspReqExtLen > certStatusRequestLen)
    {
        status = ERR_SSL_EXTENSION_LENGTH;
        goto exit;
    }

    if (ocspReqExtLen) {
        /* Now extract the extensions */
        if (OK > (status = SSL_OCSP_parseExtensions(pCertStatusRequest + offset,
                    ocspReqExtLen, &pSSLSock->pExts, &pSSLSock->numOfExtension)))
        {
            goto exit;
        }
    }

exit:
    return status;
}

#endif /* __ENABLE_TLSEXT_RFC6066__ */
#endif

/*------------------------------------------------------------------*/

#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)    )
static PEllipticCurvePtr
SSL_SOCK_getCurveFromName( ubyte4 tlsExtNamedCurve)
{
    switch (tlsExtNamedCurve)
    {
#ifndef __DISABLE_MOCANA_ECC_P256__
        case tlsExtNamedCurves_secp256r1:
            return EC_P256;
#endif
#ifndef __DISABLE_MOCANA_ECC_P384__
        case tlsExtNamedCurves_secp384r1:
            return EC_P384;
#endif
#ifndef __DISABLE_MOCANA_ECC_P521__
        case tlsExtNamedCurves_secp521r1:
            return EC_P521;
#endif
#ifndef __DISABLE_MOCANA_ECC_P224__
        case tlsExtNamedCurves_secp224r1:
            return EC_P224;
#endif
#ifdef __ENABLE_MOCANA_ECC_P192__
        case tlsExtNamedCurves_secp192r1:
            return EC_P192;
#endif
    }

    return 0;
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_EAP_FAST__) && defined(__ENABLE_MOCANA_SSL_SERVER__) && defined(__ENABLE_RFC3546__))
static MSTATUS
processPACOpaque(SSLSocket* pSSLSock, ubyte* pPacOpaque, ubyte4 pacOpaqueLen)
{
    MSTATUS status;

    if (!SSL_sslSettings()->funcPtrPACOpaqueCallback)
        return ERR_EAP_FAST_NO_PACOPAQUECALLBACK;

    if (OK > (status = (MSTATUS)SSL_sslSettings()->funcPtrPACOpaqueCallback(
            SSL_findConnectionInstance(pSSLSock), pPacOpaque, pacOpaqueLen, pSSLSock->pacKey)))
    {
        return status;
    }

    /* do the rest only if we are not resuming already */
    if ( E_NoSessionResume == pSSLSock->sessionResume)
    {
        /* the cipher is fixed */
        if ( OK > ( status = SSL_SOCK_setCipherById( pSSLSock, 0x0005)))
        {
            return status;
        }

        pSSLSock->sessionResume = E_SessionEAPFASTResume;
    }

    return OK;
}
#endif


/*------------------------------------------------------------------*/

#ifdef __ENABLE_RFC3546__
static MSTATUS
processClientHelloExtension(SSLSocket* pSSLSock, ubyte2 extensionType,
                                ubyte2 extensionSize, ubyte* pExtension)
{
    MSTATUS status = OK;

    switch (extensionType)
    {
#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
        case tlsExt_renegotiated_connection:
        {
            /* due to problems using "hello extensions" with SSLv3, we don't support SSLv3 rehandshakes. */
            ubyte4  renegotiatedConnLength = *pExtension;
            ubyte4  expectedRenegotiationLength = 0;
            sbyte4  result;
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
            ubyte4  verifyDataSize = (SSL3_MINORVERSION == pSSLSock->sslMinorVersion) ? SSL_VERIFY_DATA : TLS_VERIFYDATASIZE;
#else
            ubyte4 verifyDataSize = TLS_VERIFYDATASIZE;
#endif
            pSSLSock->isRehandshakeExtPresent = TRUE;

            if (0 < pSSLSock->handshakeCount)
            {
                expectedRenegotiationLength = verifyDataSize;
            }

            if (expectedRenegotiationLength != renegotiatedConnLength)
            {
                status = ERR_SSL_SERVER_RENEGOTIATE_LENGTH;
                goto exit;
            }

            if (0 < expectedRenegotiationLength)
            {
                if (OK > (status = MOC_MEMCMP(1 + pExtension, pSSLSock->client_verify_data, verifyDataSize, &result)))
                    goto exit;

                if (0 != result)
                {
                    status = ERR_SSL_SERVER_RENEGOTIATE_CLIENT_VERIFY;
                    goto exit;
                }
            }

            if (0 == pSSLSock->handshakeCount)
                pSSLSock->isRehandshakeAllowed = TRUE;
        }
        break;
#endif /* ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__))) */

        case tlsExt_server_name:
        {
            sbyte4 serverNameListLength;

            if ( extensionSize < 2)
            {
                /* buffer overrun attack? */
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }

            serverNameListLength = getShortValue(pExtension);
            if ( serverNameListLength + 2 > extensionSize)
            {
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }

            if (OK > (status = SSLSOCK_sanityServerNameList(pExtension+2, serverNameListLength)))
            {
                if (ERR_SSL_EXTENSION_DUPLICATE_NAMETYPE_SNI == status)
                {
                    /* Non Fatal Error; Ignore SNI Extension moving forward*/
                    serverNameListLength = 0;
                } else
                    goto exit;

            }

            if (NULL != pSSLSock->serverNameList)
                FREE(pSSLSock->serverNameList);

            if (serverNameListLength)
            {
                if (NULL == (pSSLSock->serverNameList = MALLOC(serverNameListLength)))
                {
                    status = ERR_MEM_ALLOC_FAIL;
                    goto exit;
                }

                MOC_MEMCPY(pSSLSock->serverNameList, pExtension+2, serverNameListLength);
                pSSLSock->serverNameListLength = serverNameListLength;
            }
        }
        break;

        case tlsExt_status_request:
        {
#ifdef __ENABLE_TLSEXT_RFC6066__
            if (OK > (status = SSLSOCK_sanityCertificateStatusRequestExtension(pSSLSock,
                        pExtension, extensionSize)))
            {
                goto exit;
            }

            pSSLSock->certStatusReqExt = TRUE;
#endif
        }
        break;

        case tlsExt_supportedSignatureAlgorithms:
        {
            /* verify we are at 1.2 or above */
            if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
                (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
            {
                sbyte4 signatureAlgoListLength;
                if ( extensionSize < 2)
                {
                    /* buffer overrun attack? */
                    status = ERR_SSL_EXTENSION_LENGTH;
                    goto exit;
                }

                signatureAlgoListLength = getShortValue(pExtension);
                if ( signatureAlgoListLength + 2 > extensionSize)
                {
                    status = ERR_SSL_EXTENSION_LENGTH;
                    goto exit;
                }
                /* TODO: should we point directly into the clientHello buffer */
                pSSLSock->signatureAlgoList = pExtension+2;
                pSSLSock->signatureAlgoListLength = signatureAlgoListLength;
            }
        }
        break;

#if defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)
        case tlsExt_supportedEllipticCurves:
        {
            sbyte4 numCurves, i;

            if ( extensionSize < 2) /* minimal size */
            {
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }
            numCurves = getShortValue( pExtension);
            if (numCurves + 2 > extensionSize)
            {
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }
            pExtension += sizeof(ubyte2);
            /* need to verify that the client supports all the
            needed curves --- ECDH but also ECDSA keys used for signing
            this info is stored in the ecCurves member of pSSLSock */
            pSSLSock->roleSpecificInfo.server.clientECCurves = 0;
            for ( i = 0; i < numCurves; i += 2)
            {
                ubyte2 curve = getShortValue(pExtension);
                pExtension += sizeof(ubyte2);

                if ( curve < 32)
                {
                    pSSLSock->roleSpecificInfo.server.clientECCurves |= ( 1 << curve);
                }
            }

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
            /* mask the curves with the ones we allow for the connection */
            pSSLSock->roleSpecificInfo.server.clientECCurves &= pSSLSock->eccCurves;
#endif
            pSSLSock->roleSpecificInfo.server.clientECCurves &= SUPPORTED_CURVES_FLAGS;

            /* that's it. Even if the clientECCurves is 0, meaning there's no common
            curve, this is OK since we might pick a non EC-based cipher */
        }
        break;

        case tlsExt_ECPointFormat:
        /* must verify we understand it */
        {
            sbyte4 i, numFormats;

            if ( extensionSize < 2) /* minimal size */
            {
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }
            numFormats = *pExtension++;
            if ( numFormats + 1 != extensionSize)
            {
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }
            for ( i = 0; i < numFormats; ++i)
            {
                if ( *pExtension > tlsExtECPointFormat_ansiX962_compressed_char2)
                {
                    status = ERR_SSL_EXTENSION_UNKNOWN_FORMAT;
                    goto exit;
                }
            }
        }
        break;
#endif /* __ENABLE_MOCANA_SSL_ECDH_SUPPORT__  ||
        __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__  ||
        __ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__*/

        case tlsExt_ticket:
        {
            sbyte4 ticketLength = 0;

            if (1 == extensionSize)
            {
                /* buffer overrun attack? */
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }

            if (0 != extensionSize)
            {
                ticketLength = getShortValue(pExtension);

                if (ticketLength + 2 > extensionSize)
                {
                    status = ERR_SSL_EXTENSION_LENGTH;
                    goto exit;
                }
            }

#if defined(__ENABLE_MOCANA_EAP_FAST__)
            pSSLSock->roleSpecificInfo.server.sendSessionTicket = FALSE; /* don't send a ticket extension back */
#else
            pSSLSock->roleSpecificInfo.server.sendSessionTicket = TRUE; /* client requested use of a ticket */
#endif
            /* 2 options: either the ticket is non empty -> try to see if we use it */
            /* or it is empty -> request for a new ticket */
            if ( ticketLength)
            {
#if defined(__ENABLE_MOCANA_EAP_FAST__)
                status = processPACOpaque( pSSLSock, pExtension+2, ticketLength);
#endif
                if ( OK > status )
                    goto exit;
            }
        }
        break;

#if defined(__ENABLE_MOCANA_DTLS_SERVER__) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
        case dtlsExt_use_srtp:
        {
            sbyte4 srtpProfileListLength;
            ubyte4 srtpMkiLength;
            sbyte4 selectedProfile;

            if (!pSSLSock->isDTLS)
                goto exit;

            if ( extensionSize < 2)
            {
                /* buffer overrun attack? */
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }

            srtpProfileListLength = getShortValue(pExtension);
            if ( srtpProfileListLength + 1 > extensionSize)
            {
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }
            pExtension += 2;
            extensionSize -= 2;

            if (E_NoSessionResume == pSSLSock->sessionResume)
            {
                /* server pick a profile */
                selectedProfile = SSLSOCK_selectSrtpProfile(pSSLSock, 0, pExtension, srtpProfileListLength>>1);

                if (selectedProfile < 0)
                {
                    status = ERR_DTLS_SRTP_NO_PROFILE_MATCH;
                    goto exit;
                }

                pSSLSock->pHandshakeSrtpProfile = gSrtpProfiles + selectedProfile;
            }

            pExtension += srtpProfileListLength;
            extensionSize -= srtpProfileListLength;

            /* srtp_mki */
            if (extensionSize < 1)
            {
                /* buffer overrun attack? */
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }
            srtpMkiLength = (ubyte4)(*pExtension);

            if ((srtpMkiLength > 256) || (extensionSize < srtpMkiLength + 1))
            {
                /* TODO: probably need to change to another error */
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }
            if (srtpMkiLength > 0)
            {
                /* reuse buffer if they have the same length */
                if ((NULL != pSSLSock->srtpMki) && (srtpMkiLength != *(pSSLSock->srtpMki)))
                {
                    FREE(pSSLSock->srtpMki);
                    pSSLSock->srtpMki = NULL;
                }

                if (NULL == pSSLSock->srtpMki)
                    pSSLSock->srtpMki = MALLOC(srtpMkiLength + 1);

                MOC_MEMCPY(pSSLSock->srtpMki, pExtension, srtpMkiLength + 1);
            }
            pSSLSock->useSrtp = TRUE;

        }
        break;
#endif

        case tlsExt_innerApplication:
        {
            if (2 != extensionSize )
            {
                /* buffer overrun attack? */
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }

#if defined(__ENABLE_MOCANA_INNER_APP__)
            pSSLSock->receivedInnerAppValue = getShortValue(pExtension);
            pSSLSock->receivedInnerApp      = TRUE;
#endif
        }
        break;

        default:
            break;
    }

    /* callback handler for protected extension types */
    if ((64 > extensionType) && (NULL != SSL_sslSettings()->funcPtrExtensionRequestCallback))
        status = (MSTATUS)SSL_sslSettings()->funcPtrExtensionRequestCallback(
            SSL_findConnectionInstance(pSSLSock), extensionType, pExtension, extensionSize);

exit:
    return status;
}

#endif /* __ENABLE_RFC3546__ */

/*------------------------------------------------------------------*/

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
static sbyte4
findRenegotiationScsvV2( ubyte* clientCipherSuite, sbyte4 clientCipherSuiteLen )
{
    sbyte4 index;

    /* see if SCSV exist */
    for (index = 0; index < clientCipherSuiteLen; index += 3)
    {
        if ((0 == clientCipherSuite[index]) &&
            ( (((ubyte2)clientCipherSuite[index+1] << 8) | (ubyte2)clientCipherSuite[index+2]) ==
                0x00FF))
        {
            DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"TLS_EMPTY_RENEGOTIATION_INFO_SCSV");
            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
            return 1;
        }
    }

    return 0;
}
#endif


/*------------------------------------------------------------------*/

static sbyte4
findFallbackScsvV2( ubyte* clientCipherSuite, sbyte4 clientCipherSuiteLen )
{
    sbyte4 index;

    /* see if TLS_FALLBACK_SCSV exist */
    for (index = 0; index < clientCipherSuiteLen; index += 3)
    {
        if ((0 == clientCipherSuite[index]) &&
            ( (((ubyte2)clientCipherSuite[index+1] << 8) | (ubyte2)clientCipherSuite[index+2]) ==
                0x5600))
        {
            DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"TLS_FALLBACK_SCSV");
            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
            return 1;
        }
    }

    return 0;
}

/*------------------------------------------------------------------*/

static sbyte4
SSLSOCK_selectCipherSuiteV2( const SSLSocket* pSSLSock, sbyte4 start,
                            ubyte* clientCipherSuite, sbyte4 clientCipherSuiteLen)
{
    sbyte4 i;

    /* see if we support one of the cipher suites */
    for (i = start; i < (sbyte4)NUM_CIPHER_SUITES; i++)
    {
        sbyte4 index;

        if (gCipherSuites[i].supported)
        {
#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
            if ((!pSSLSock->isCipherTableInit) ||
                (pSSLSock->isCipherEnabled[i]) )
#endif
            {
                for (index = 0; index < clientCipherSuiteLen; index += 3)
                {
                    if ((0 == clientCipherSuite[index]) &&
                        ( (((ubyte2)clientCipherSuite[index+1] << 8) | (ubyte2)clientCipherSuite[index+2]) ==
                            gCipherSuites[i].cipherSuiteId))
                    {
                        /* matches one of our cipher suites and is not using EC crypto */
                        DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"SSLSOCK_selectCipherSuiteV2: CipherSuite selected = 0x");
                        DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, clientCipherSuite[index+1]);
                        DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, clientCipherSuite[index+2]);
                        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
                        return i;
                    }
                }
            }
        }
    }

    return -1;
}


/*------------------------------------------------------------------*/

static MSTATUS
processClientHello2(SSLSocket* pSSLSock)
{
    ubyte* rest;
    sbyte4 selCipherIndex = -1;

    /* already read one byte get the rest of the header */
    SSLClientHelloV2* pClientHello = &(((SSLSharedInBuffer *)(pSSLSock->pSharedInBuffer))->clientHello);
    MSTATUS status;

    /* verifications */
    if ((pClientHello->record.msgType      != 1)  ||
        (pClientHello->record.majorVersion < SSL3_MAJORVERSION)  ||      /* DO NOT CHECK MINOR VERSION 0 FOR SSL 1 FOR TLS */
        (pClientHello->record.challengeLen < 16)  ||
        (pClientHello->record.challengeLen > 32))
    {
        status = ERR_SSL_PROTOCOL_PROCESS_CLIENT_HELLO;
        goto exit;
    }

 	/* get the rest of it */
    rest =  ((ubyte *)pClientHello) + sizeof(SSLClientHelloV2Fixed);

	if (findFallbackScsvV2(rest, pClientHello->record.cipherSuiteLen))
    {
        /* this TLS_FALLBACK_SCSV means client tried to renegotiate with lower SSL version */
    	ubyte highestSSLVerSupported = ((pSSLSock->runtimeFlags & SSL_FLAG_VERSION_SET)?
                                        pSSLSock->advertisedMinorVersion : MAX_SSL_MINORVERSION);

        if (highestSSLVerSupported > pClientHello->record.minorVersion)
        {
            /* higher SSL version is supported by server as well as client hence error */
            status = ERR_SSL_SERVER_INAPPROPRIATE_FALLBACK_SCSV;
            goto exit;
        }
    }

    if (pClientHello->record.majorVersion == SSL3_MAJORVERSION)
    {
        /* note the version we received -- used for RSA later on */
        pSSLSock->clientHelloMinorVersion = pClientHello->record.minorVersion;
#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
        /* for a rehandshake, check if the version matches the last one */
        if (0 < pSSLSock->handshakeCount)
        {
            if (pSSLSock->sslMinorVersion != pClientHello->record.minorVersion)
            {
                status = ERR_SSL_PROTOCOL_VERSION;
                goto exit;
            }
        }
#endif

        /* if a version is specified via SSL_ioctl and it doesn't match */
        if (pSSLSock->runtimeFlags & SSL_FLAG_VERSION_SET)
        {
            pSSLSock->sslMinorVersion = (ubyte)(pSSLSock->advertisedMinorVersion);

			if ((pSSLSock->advertisedMinorVersion > pClientHello->record.minorVersion) ||
                (pSSLSock->advertisedMinorVersion == 0 && pClientHello->record.minorVersion > 0))
            {
                status = ERR_SSL_PROTOCOL_VERSION;
                goto exit;
            }
        }
        else
        {
            pSSLSock->sslMinorVersion = (ubyte)(pClientHello->record.minorVersion > MAX_SSL_MINORVERSION? MAX_SSL_MINORVERSION : pClientHello->record.minorVersion);
        }
    }
    else
    {
        /* returning an error here. Will provision for higher versions later when we actually support them */
        status = ERR_SSL_PROTOCOL_VERSION;
        goto exit;
    }

#ifdef __ENABLE_ALL_DEBUGGING__
    {
    	DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte *)"SSL version = ");
    	DEBUG_INT(DEBUG_SSL_TRANSPORT, pSSLSock->sslMinorVersion);
		DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte *)"");
        sbyte4 i;

        DEBUG_PRINT(DEBUG_SSL_TRANSPORT,(sbyte *)"RecordLength = ");
        DEBUG_INT(DEBUG_SSL_TRANSPORT, pClientHello->record.recordLen);
        DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte *)", Components = ");
        DEBUG_INT(DEBUG_SSL_TRANSPORT, pClientHello->record.cipherSuiteLen + pClientHello->record.sessionLen + pClientHello->record.challengeLen);
        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte *)".");

        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte *)"Client Hello Ciphers:");

        for ( i = 0; i < pClientHello->record.cipherSuiteLen;  i+=3)
        {
            if (0 == rest[i])
            {
                DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte *)"CipherSuite = 0x");
                DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, rest[i+1]);
                DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, rest[i+2]);
                DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte *)"");
            }
        }
    }
#endif /* __ENABLE_ALL_DEBUGGING__ */

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if (findRenegotiationScsvV2(rest, pClientHello->record.cipherSuiteLen))
    {
        if (0 < pSSLSock->handshakeCount)
        {
            /* SCSV should not be present during renegotitation */
            status = ERR_SSL_SERVER_RENEGOTIATE_ILLEGAL_SCSV;
            goto exit;
        }

        pSSLSock->isRehandshakeAllowed = TRUE;
    }
#endif


    pSSLSock->pHandshakeCipherSuite = NULL;
    selCipherIndex = SSLSOCK_selectCipherSuiteV2(pSSLSock, 0, rest,
                                                pClientHello->record.cipherSuiteLen);
    if (selCipherIndex >= 0 )
    {
        /* found one we like -- set the member pCipherSuite ....*/
        pSSLSock->pHandshakeCipherSuite = gCipherSuites + selCipherIndex;
        /* ... before calling SSL_setCerverCert so that the SSL
        layer knows which certificate to use for the cipher */
        if (OK > (status = SSL_setServerCert(pSSLSock)))
            goto exit;
        /* client hello version 2 does not support extensions
        assumes the client can deal with any elliptic curves we will use */
    }

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if (0 < pSSLSock->handshakeCount)
    {
        if (pSSLSock->isRehandshakeAllowed)
        {
            /* if client supports secure renegotiation,
            assume it will not use version 2 hello for renegotiation */
            status = ERR_SSL_SERVER_RENEGOTIATE_NOT_ALLOWED;
            goto exit;
        }

        /* allow legacy (insecure) renegotiation only if session flag is set */
        if ( !pSSLSock->isRehandshakeAllowed &&
             !(pSSLSock->runtimeFlags & SSL_FLAG_ALLOW_INSECURE_REHANDSHAKE) )
        {
            status = ERR_SSL_SERVER_RENEGOTIATE_NOT_ALLOWED;
            goto exit;
        }
    }
#endif

    if (NULL != pSSLSock->pHandshakeCipherSuite)
    {
        /* initialize hash pool and hash context */
        if (OK > (status = SSL_SOCK_initHashPool(pSSLSock)))
            goto exit;

        /* we do not deal with session id here: assumes a client will use a
        SSL version 3.0 hello to resume a session */
        pSSLSock->sessionResume = E_NoSessionResume;
        /* get the challenge and save it; challenge can be 16 to 32 chars incl.*/
        /* pad with zero ON THE LEFT if less than 32 */
        MOC_MEMSET(START_RANDOM(pSSLSock), 0, SSL_RANDOMSIZE - pClientHello->record.challengeLen);
        MOC_MEMCPY(START_RANDOM(pSSLSock) + SSL_RANDOMSIZE - pClientHello->record.challengeLen,
               rest + pClientHello->record.cipherSuiteLen + pClientHello->record.sessionLen,
               pClientHello->record.challengeLen);
        MOC_MEMCPY(pSSLSock->pClientRandHello, START_RANDOM(pSSLSock), SSL_RANDOMSIZE);      /*!!!!*/
        /* add to handshake hashes once and only once */
        addToHandshakeHash(pSSLSock, pClientHello->array + 2, pClientHello->record.recordLen);
        status = OK;
    }
    else
    {
        status = ERR_SSL_NO_CIPHER_MATCH;
    }

exit:
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"processClientHello2() returns status = ", status);

    return status;
}


/*------------------------------------------------------------------*/

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
static sbyte4
findRenegotiationScsvV3( ubyte* clientCipherSuite, sbyte4 clientCipherSuiteLen )
{
    sbyte4 index;

    /* see if SCSV exist */
    for (index = 0; index < clientCipherSuiteLen; index +=2)
    {
        if ( (((ubyte2)clientCipherSuite[index] << 8) | (ubyte2)clientCipherSuite[index+1]) ==
                0x00FF )
        {
            DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"TLS_EMPTY_RENEGOTIATION_INFO_SCSV");
            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
            return 1;
        }
    }

    return 0;
}
#endif


/*------------------------------------------------------------------*/

static sbyte4
findFallbackScsvV3( ubyte* clientCipherSuite, sbyte4 clientCipherSuiteLen )
{
    sbyte4 index;

    /* see if TLS_FALLBACK_SCSV exist */
    for (index = 0; index < clientCipherSuiteLen; index += 2)
    {
        if ((((ubyte2)clientCipherSuite[index] << 8) | (ubyte2)clientCipherSuite[index+1]) ==
                0x5600 )
        {
            DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"TLS_FALLBACK_SCSV");
            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
            return 1;
        }
    }

    return 0;
}


/*---------------------------------------------------------------------------*/

static sbyte4
SSLSOCK_selectCipherSuiteV3( const SSLSocket* pSSLSock, sbyte4 start,
                            ubyte* clientCipherSuite, sbyte4 clientCipherSuiteLen)

{
    sbyte4 i;

    for (i = start; i < (sbyte4)NUM_CIPHER_SUITES; i++)
    {
        sbyte4  index;

        if (gCipherSuites[i].supported)
        {
#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
            if ((!pSSLSock->isCipherTableInit) ||
                (pSSLSock->isCipherEnabled[i]) )
#endif
            {
                for (index = 0; index < clientCipherSuiteLen; index +=2)
                {
                    if ( (((ubyte2)clientCipherSuite[index] << 8) | (ubyte2)clientCipherSuite[index+1]) ==
                        gCipherSuites[i].cipherSuiteId )
                    {
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
                        if (pSSLSock->isDTLS)
                        {
                            /* DTLS does not support stream ciphers */
                            if (isCipherIdExcludedForDTLS(gCipherSuites[i].cipherSuiteId))
                            {
                                continue;
                            }

                            /* cipher supported for this version?. Convert DTLS version to SSL
                             version by using 256 - dtlsVersion */
                            if (256 - pSSLSock->sslMinorVersion < gCipherSuites[i].minimumSSLVersion)
                            {
                                continue;
                            }
                        }
                        else
#endif

                        if (pSSLSock->sslMinorVersion < gCipherSuites[i].minimumSSLVersion)
                        {
                            continue;
                        }

                        DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"SSLSOCK_selectCipherSuiteV3: CipherSuite selected = 0x");
                        DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, clientCipherSuite[index]);
                        DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, clientCipherSuite[index+1]);
                        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");

                        return i;
                    }
                }
            }
        }
    }

    return -1;
}


/*---------------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_DTLS_SERVER__) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
static sbyte4
SSLSOCK_selectSrtpProfile( const SSLSocket* pSSLSock, sbyte4 start,
                            ubyte* clientSrtpProfiles, sbyte4 clientSrtpProfileLen)

{
    sbyte4 i;

    for (i = start; i < (sbyte4)NUM_SRTP_PROFILES; i++)
    {
        sbyte4  index;

        if (gSrtpProfiles[i].supported)
        {
#ifdef __ENABLE_MOCANA_SRTP_PROFILES_SELECT__
            if ((!pSSLSock->isSrtpProfileTableInit) ||
                (pSSLSock->isSrtpProfileEnabled[i]) )
#endif
            {
                for (index = 0; index < clientSrtpProfileLen; index +=2)
                {
                    if ( (((ubyte2)clientSrtpProfiles[index] << 8) | (ubyte2)clientSrtpProfiles[index+1]) ==
                            gSrtpProfiles[i].profileId )
                    {
                        DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"SSLSOCK_selectSrtpProtectionProfile: profile selected = 0x");
                        DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, clientSrtpProfiles[index]);
                        DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, clientSrtpProfiles[index+1]);
                        DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");

                        return i;
                    }
                }
            }
        }
    }

    return -1;
}
#endif

/*------------------------------------------------------------------*/

static MSTATUS
processClientHello3(SSLSocket* pSSLSock)
{
    ubyte*              rest;
    ubyte               sessionIdLen;
    ubyte               compressionMethodsLen;
    ubyte*              pSHSH;
    sbyte4              recLen; /* length of handshake record */
    ubyte*              pClientHelloVer;
    ubyte4              MAJORVERSION = 0;
    ubyte4              MINORVERSION = 1;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status = ERR_SSL_PROTOCOL_PROCESS_CLIENT_HELLO;
    sbyte4              cacheIndex = 0;

    pSSLSock->pHandshakeCipherSuite = NULL;

    /* verifications */
    pSHSH = (ubyte*)pSSLSock->pReceiveBuffer;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        recLen = getMediumValue(((DTLSHandshakeHeader *)pSHSH)->handshakeSize);
        if ((((DTLSHandshakeHeader *)pSHSH)->handshakeType != SSL_CLIENT_HELLO)  ||
            (recLen + sizeofHandshakeHeader != pSSLSock->recordSize))
        {
            goto exit;
        }
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        recLen = getMediumValue(((SSLHandshakeHeader *)pSHSH)->handshakeSize);
        if ((((SSLHandshakeHeader *)pSHSH)->handshakeType != SSL_CLIENT_HELLO)  ||
            (recLen + sizeofHandshakeHeader != pSSLSock->recordSize))
        {
            goto exit;
        }
    }

    pClientHelloVer = pSHSH + sizeofHandshakeHeader;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        /* verify versions */
        if (pClientHelloVer[MAJORVERSION] != DTLS1_MAJORVERSION)  /* do not check minor version here */
            goto exit;

        pSSLSock->clientHelloMinorVersion = pClientHelloVer[MINORVERSION];

        if (pSSLSock->runtimeFlags & SSL_FLAG_VERSION_SET)
        {
            pSSLSock->sslMinorVersion = pSSLSock->advertisedMinorVersion;
			if (pSSLSock->advertisedMinorVersion != pClientHelloVer[MINORVERSION])
            {
                status = ERR_SSL_PROTOCOL_VERSION;
                goto exit;
            }
        }  else
        {
            if (MIN_DTLS_MINORVERSION < pClientHelloVer[MINORVERSION])
            {
                status = ERR_SSL_PROTOCOL_VERSION;
                goto exit;
            }
            else
            {
                pSSLSock->sslMinorVersion = (ubyte)(pClientHelloVer[MINORVERSION] < MAX_DTLS_MINORVERSION ?
                                                MAX_DTLS_MINORVERSION : pClientHelloVer[MINORVERSION]);
            }
        }
    }
    else
#endif
    {
        /* verify versions */
        if ((pClientHelloVer[MAJORVERSION] < SSL3_MAJORVERSION))  /* do not check minor version here */
            goto exit;

        /* note the version we received -- used for RSA later on */
        pSSLSock->clientHelloMinorVersion = pClientHelloVer[MINORVERSION];

        if (pClientHelloVer[MAJORVERSION] == SSL3_MAJORVERSION)
        {
#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
            /* for a rehandshake, check if the version matches the last one */
            if (0 < pSSLSock->handshakeCount)
            {
                if (pSSLSock->sslMinorVersion != pClientHelloVer[MINORVERSION])
                {
                    status = ERR_SSL_PROTOCOL_VERSION;
                    goto exit;
                }
            }
#endif
            /* if version is set via SSL_ioctl, it has to match the one advertised by client */
            if (pSSLSock->runtimeFlags & SSL_FLAG_VERSION_SET)
            {
				/*Need to set this to determine correct Alert to send*/
				pSSLSock->sslMinorVersion = pSSLSock->advertisedMinorVersion;
				if (pSSLSock->advertisedMinorVersion > pClientHelloVer[MINORVERSION])
                {
                    status = ERR_SSL_PROTOCOL_VERSION;
                    goto exit;
                }
            }
            else
            {
                if (MIN_SSL_MINORVERSION > pClientHelloVer[MINORVERSION])
                {
                    status = ERR_SSL_PROTOCOL_VERSION;
                    goto exit;
                }
                else
                {
                    pSSLSock->sslMinorVersion = (ubyte)(pClientHelloVer[MINORVERSION] > MAX_SSL_MINORVERSION ?
                                                    MAX_SSL_MINORVERSION : pClientHelloVer[MINORVERSION]);
                }
            }
        }
        else
        {
            /* returning an error here. Will provision for higher versions later when we actually support them */
            status = ERR_SSL_PROTOCOL_VERSION;
            goto exit;
        }
    }

    /* copy the next 32 bit as client random */
    rest = pSHSH + sizeofHandshakeHeader + 2;
    MOC_MEMCPY(pSSLSock->pClientRandHello, rest, SSL_RANDOMSIZE);
    MOC_MEMCPY(START_RANDOM(pSSLSock), rest, SSL_RANDOMSIZE);
    rest += SSL_RANDOMSIZE;

    /* id len */
    sessionIdLen = *rest++;

    if (sessionIdLen > 32)  /* 0..32 */
        goto exit;

    pSSLSock->sessionResume = E_NoSessionResume;
    if (sizeof(SESSIONID) == sessionIdLen) /* might be one of ours */
    {
        /* look in our cache */
        SESSIONID sessionId;
        /* get sessionId */
        MOC_MEMCPY( (ubyte*) &sessionId, rest, sizeof(SESSIONID));
        /* cacheIndex is session modulo SESSION_CACHE_SIZE */
        cacheIndex = sessionId % SESSION_CACHE_SIZE;

        if (OK > (status = RTOS_mutexWait(gSslSessionCacheMutex)))
            goto exit;

        /* is there a match? */
        if (sessionId == gSessionCache[cacheIndex].m_sessionId)
        {
            if (SSL_SESSION_RESUME_TIMEOUT > RTOS_deltaMS(&gSessionCache[cacheIndex].startTime, NULL))
            {
                pSSLSock->sessionResume = E_SessionIDResume;
                if (pSSLSock->sslMinorVersion != gSessionCache[cacheIndex].m_minorVersion)
                {
                    status = ERR_SSL_BAD_HEADER_VERSION;
                    RTOS_mutexRelease(gSslSessionCacheMutex);
                    goto exit;
                }
                /* don't set the cipher suite there blindly, first
                 verify that the client hello matches the one in the session cache
                pSSLSock->pHandshakeCipherSuite = gSessionCache[cacheIndex].m_pCipherSuite; */
                pSSLSock->roleSpecificInfo.server.sessionId = sessionId;
                MOC_MEMCPY(pSSLSock->pSecretAndRand,
                           gSessionCache[cacheIndex].m_masterSecret,
                           SSL_MASTERSECRETSIZE);
#if ( defined(__ENABLE_MOCANA_DTLS_SERVER__) && defined(__ENABLE_MOCANA_DTLS_SRTP__))
                if (pSSLSock->isDTLS)
                {
                    pSSLSock->pHandshakeSrtpProfile = gSessionCache[cacheIndex].m_pSrtpProfile;
                }
#endif
                if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
                    (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
                {
                    pSSLSock->signatureAlgo = gSessionCache[cacheIndex].m_signatureAlgo;
                }

				pSSLSock->roleSpecificInfo.server.clientECCurves = gSessionCache[cacheIndex].m_clientECCurves;
            }
        }

        if (OK > (status = RTOS_mutexRelease(gSslSessionCacheMutex)))
            goto exit;

        if ((E_SessionIDResume != pSSLSock->sessionResume) && (sessionId == (1 + gNextSessionId)))
            gNextSessionId++;
    }

    /* in all cases, jump over the session id*/
    rest += sessionIdLen;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        pSSLSock->nextRecvSeq++;
        /* verify cookie is done already (before initialize SSLSock) */
        rest += (*rest) + 1;
    }
#endif

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    pSSLSock->isRehandshakeExtPresent = FALSE;
#endif

    status = ERR_SSL_PROTOCOL_PROCESS_CLIENT_HELLO;

    /* determine the cipher suite from the client hello in all cases */
    {
        ubyte*  clientCipherSuite;
        sbyte4  cipherSuiteLen;
        sbyte4  selCipherIndex;

        /* check to see if we can fetch the next two bytes for cipherSuiteLen */
        if ( rest + 2 - (pSHSH + sizeofHandshakeHeader) >= recLen)
            goto exit;

        cipherSuiteLen = getShortValue(rest);

        /* check cipher suite length */
        if (2 + cipherSuiteLen + rest - (pSHSH + sizeofHandshakeHeader) >= recLen)
            goto exit;

        rest += 2;

        clientCipherSuite = rest;
        rest += cipherSuiteLen;

#ifdef __ENABLE_ALL_DEBUGGING__
        {
        	DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte *)"SSL version = ");
    		DEBUG_INT(DEBUG_SSL_TRANSPORT, pSSLSock->sslMinorVersion);
    		DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte *)"");
            sbyte4 i;

            DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte *)"Client Hello Ciphers (v3):");

            for (i = 0; i < cipherSuiteLen;  i+=2)
            {
                DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte *)"CipherSuite = 0x");
                DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, clientCipherSuite[i]);
                DEBUG_HEXBYTE(DEBUG_SSL_TRANSPORT, clientCipherSuite[i+1]);
                DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte *)"");
            }
        }
#endif /* __ENABLE_ALL_DEBUGGING__ */

        if (findFallbackScsvV3(clientCipherSuite, cipherSuiteLen))
        {
            /* this TLS_FALLBACK_SCSV means client tried to renegotiate with lower SSL version */
            ubyte highestSSLVerSupported = -1;
            if(pSSLSock->isDTLS)
            {
                highestSSLVerSupported = ((pSSLSock->runtimeFlags & SSL_FLAG_VERSION_SET)?
                                          pSSLSock->advertisedMinorVersion : DTLS12_MINORVERSION);
                if (highestSSLVerSupported < pClientHelloVer[MINORVERSION])
                {
                    /* higher SSL version is supported by server as well as client hence error */
                    status = ERR_SSL_SERVER_INAPPROPRIATE_FALLBACK_SCSV;
                    goto exit;
                }
            }
            else
            {
                highestSSLVerSupported = ((pSSLSock->runtimeFlags & SSL_FLAG_VERSION_SET)?
                                          pSSLSock->advertisedMinorVersion : TLS12_MINORVERSION);
                if (highestSSLVerSupported > pClientHelloVer[MINORVERSION])
                {
                    /* higher SSL version is supported by server as well as client hence error */
                    status = ERR_SSL_SERVER_INAPPROPRIATE_FALLBACK_SCSV;
                    goto exit;
                }
            }
        }

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
        if (findRenegotiationScsvV3(clientCipherSuite, cipherSuiteLen))
        {
            if (0 < pSSLSock->handshakeCount)
            {
                /* SCSV should not be present during renegotiation */
                status = ERR_SSL_SERVER_RENEGOTIATE_ILLEGAL_SCSV;
                goto exit;
            }

            pSSLSock->isRehandshakeAllowed = TRUE;
        }
#endif

        pSSLSock->pHandshakeCipherSuite = NULL;
        selCipherIndex = SSLSOCK_selectCipherSuiteV3( pSSLSock, 0, clientCipherSuite, cipherSuiteLen);

        /* skip past compression methods */
        compressionMethodsLen = *rest;
        rest += (1 + compressionMethodsLen);

        /* set the clientECCurves to 0xFFFF -> the client can deal with any curves if
        there is no ECCurve Hello Extension */
		if (E_SessionIDResume != pSSLSock->sessionResume)
        {
			pSSLSock->roleSpecificInfo.server.clientECCurves = 0xFFFFFFFF;
        }

        if (selCipherIndex >= 0)
        {
#ifdef __ENABLE_RFC3546__
            /* extension is only supported in TLS
               but minimal support is added in SSL 3.0 for secure renegotiation */
            /* if we successfully found a cipher suite, and the version is TLS 1.0+ proceed */
            if ( rest + 2 - (pSHSH+sizeofHandshakeHeader) < recLen)
            {
                /* check the extension length */
                ubyte4 extensionsLength = getShortValue(rest);
                rest += 2;
                if ( rest + extensionsLength - (pSHSH+sizeofHandshakeHeader) <= recLen)
                {
                    if (OK > (status = processHelloExtensions(pSSLSock, rest, extensionsLength)))
                        goto exit;
                }
            }
#endif /* __ENABLE_RFC3546__ */

#ifdef __ENABLE_MOCANA_CCM_8__
            if ((0xc0ae == gCipherSuites[selCipherIndex].cipherSuiteId) &&
                (0xFFFFFFFF == pSSLSock->roleSpecificInfo.server.clientECCurves))
            {
                /* The ellipticCurves extension was not received */
                status = ERR_SSL_EXTENSION_ELLIPTIC_CURVE_NOT_RECEIVED;
                goto exit;
            }
#endif

            /* now that we have parsed the extensions call the
            function to get a certificate repeatedly until sure it can be used*/
            while (selCipherIndex >= 0 )
            {
                /* found one we like -- set the member pCipherSuite ....*/
                pSSLSock->pHandshakeCipherSuite = gCipherSuites + selCipherIndex;

                /* ... before calling SSL_setCerverCert so that the SSL
                layer knows which certificate to use for the cipher */
                status = SSL_setServerCert(pSSLSock);
                if (0 <= status)
                {
                    /* cipher is good to use */
                    break;
                }

                /* otherwise loop and try again */
                DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"Incompatible EC curves returned by SSL_setServerCert: ");
                DEBUG_HEXINT(DEBUG_SSL_TRANSPORT, pSSLSock->roleSpecificInfo.server.clientECCurves);
                DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)"");
                pSSLSock->pHandshakeCipherSuite = NULL;
                selCipherIndex = SSLSOCK_selectCipherSuiteV3( pSSLSock, selCipherIndex + 1,
                                                               clientCipherSuite, cipherSuiteLen);
            }
        }

        /* fferino@mocana.com: current code: if we are resuming a session, makes sure the selected handshake suite
         is the same as before */
        if ((pSSLSock->sessionResume == E_SessionIDResume) &&
            (pSSLSock->pHandshakeCipherSuite != gSessionCache[cacheIndex].m_pCipherSuite))
        {
            status = ERR_SSL_PROTOCOL;
            goto exit;
        }
    }
#if 0
    else
    {
        /* some TLS extensions need processing for a resumed session */
        sbyte4  cipherSuiteLen;

        /* check to see if we can fetch the next two bytes for cipherSuiteLen */
        if ( rest + 2 - (pSHSH + sizeofHandshakeHeader) >= recLen)
            goto exit;

        cipherSuiteLen = getShortValue(rest);

        /* check cipher suite length */
        if (2 + cipherSuiteLen + rest - (pSHSH + sizeofHandshakeHeader) >= recLen)
            goto exit;

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
        if (findRenegotiationScsvV3(rest + 2, cipherSuiteLen))
        {
            if (0 < pSSLSock->handshakeCount)
            {
                /* SCSV should not be present during renegotitation */
                status = ERR_SSL_SERVER_RENEGOTIATE_ILLEGAL_SCSV;
                goto exit;
            }

            pSSLSock->isRehandshakeAllowed = TRUE;
        }
#endif

        rest += 2 + cipherSuiteLen;

        /* skip past compression methods */
        compressionMethodsLen = *rest;
        rest += (1 + compressionMethodsLen);

#ifdef __ENABLE_RFC3546__
        /* extension is only supported in TLS
           but minimal support is added in SSL 3.0 for secure renegotiation */
        if ( rest + 2 - (pSHSH+sizeofHandshakeHeader) < recLen)
        {
            /* check the extension length */
            ubyte4 extensionsLength = getShortValue(rest);
            rest += 2;
            if ( rest + extensionsLength - (pSHSH+sizeofHandshakeHeader) <= recLen)
            {
                if (OK > (status = processHelloExtensions(pSSLSock, rest, extensionsLength)))
                    goto exit;
            }
        }
#endif /* __ENABLE_RFC3546__ */
    }
#endif

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if (0 < pSSLSock->handshakeCount)
    {
        if ( (pSSLSock->isRehandshakeAllowed && !pSSLSock->isRehandshakeExtPresent) ||
             (!pSSLSock->isRehandshakeAllowed && pSSLSock->isRehandshakeExtPresent) )
        {
            status = ERR_SSL_SERVER_RENEGOTIATE_ILLEGAL_EXTENSION;
            goto exit;
        }

        /* allow legacy (insecure) renegotiation only if session flag is set */
        if ( (!pSSLSock->isRehandshakeAllowed && !pSSLSock->isRehandshakeExtPresent) &&
             !(pSSLSock->runtimeFlags & SSL_FLAG_ALLOW_INSECURE_REHANDSHAKE) )
        {
            status = ERR_SSL_SERVER_RENEGOTIATE_NOT_ALLOWED;
            goto exit;
        }
    }
#endif

    if (pSSLSock->pHandshakeCipherSuite)
    {
#if defined(__ENABLE_MOCANA_EAP_FAST__)
        /* it must be RC4 SHA1 */
        if ( E_SessionEAPFASTResume == pSSLSock->sessionResume &&
            pSSLSock->pHandshakeCipherSuite->cipherSuiteId != 0x05)
        {
            status = ERR_EAP_FAST_WRONG_CIPHER;
            goto exit;
        }
#endif
        /* initialize hash pool and hash context */
        if (OK > (status = SSL_SOCK_initHashPool(pSSLSock)))
            goto exit;

        /* add to handshakeHash once and only once */
        addToHandshakeHash(pSSLSock, (ubyte *)pSSLSock->pReceiveBuffer, pSSLSock->recordSize);
        status = OK;
    }
    else
    {
        status = ERR_SSL_NO_CIPHER_MATCH;
    }

#if defined(__ENABLE_MOCANA_DTLS_SERVER__) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
    if (pSSLSock->useSrtp && NULL == pSSLSock->pHandshakeSrtpProfile)
    {
        /* TODO: change error code */
        return ERR_DTLS_SRTP_NO_PROFILE_MATCH;
    }
#endif

exit:
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"processClientHello3() returns status = ", status);

    return status;

} /* processClientHello3 */


/*------------------------------------------------------------------*/

static MSTATUS
sslHelloStateMachine(SSLSocket* pSSLSock)
{
    MSTATUS status = OK;

    switch (pSSLSock->protocol)
    {
        case SSLV2_HELLO_CLIENT:
            status = processClientHello2(pSSLSock);
            break;

        case SSL_HANDSHAKE:
            status = processClientHello3(pSSLSock);
            break;

        default:
            status = ERR_SSL_PROTOCOL_BAD_STATE;
            break;
    }

    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"sslHelloStateMachine() returns status = ", status);

#ifdef __ENABLE_ALL_DEBUGGING__
    DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte*)"Resume state = ");
    DEBUG_INT(DEBUG_SSL_TRANSPORT, pSSLSock->sessionResume);
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte*)".");
#endif

    return status;
}


/*------------------------------------------------------------------*/

static ubyte *
fillServerHello(SSLSocket* pSSLSock, ubyte* pHSRec)
{
    ubyte* pVarPart;
    ubyte* pExtensionsLength;
    ubyte2 totalExtensionsLength = 0;

    (void)pExtensionsLength;
    (void)totalExtensionsLength;

    /* pointer to variable part */
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        pVarPart = (ubyte *)(((DTLSHandshakeHeader*)pHSRec) + 1);
        *pVarPart++ = DTLS1_MAJORVERSION; /* major version */
    } else
#endif
    {
        pVarPart = (ubyte *)(((SSLHandshakeHeader*)pHSRec) + 1);
        *pVarPart++ = SSL3_MAJORVERSION; /* major version */
    }

    *pVarPart++ = pSSLSock->sslMinorVersion; /* minor version */

    /* place the server random value in the record */
    MOC_MEMCPY(pVarPart,
            pSSLSock->pSecretAndRand + SSL_RSAPRESECRETSIZE + SSL_RANDOMSIZE,
            SSL_RANDOMSIZE);

    pVarPart += SSL_RANDOMSIZE;
    *pVarPart++ = sizeof(SESSIONID); /* session id len */

    /* bottleneck : generate session id here if not resume session */
    if ( E_SessionIDResume != pSSLSock->sessionResume)
    {
        /*increment gNextSessionId and make sure it is not zero */
        ++gNextSessionId;
        if ( 0 == gNextSessionId)
        {
            ++gNextSessionId;
        }
        pSSLSock->roleSpecificInfo.server.sessionId = gNextSessionId;
    }

    /* fill session id part */
    MOC_MEMCPY( pVarPart, (ubyte*) &pSSLSock->roleSpecificInfo.server.sessionId, sizeof(SESSIONID));
    pVarPart += sizeof(SESSIONID);
    *pVarPart++ = (ubyte)(pSSLSock->pHandshakeCipherSuite->cipherSuiteId >> 8); /* cipher suite byte 1 */
    *pVarPart++ = (ubyte)(pSSLSock->pHandshakeCipherSuite->cipherSuiteId); /* cipher suite byte 2 */
    *pVarPart++ = 0; /* compression */

#ifdef __ENABLE_RFC3546__
    pExtensionsLength = pVarPart;
    pVarPart += 2;

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if (pSSLSock->isRehandshakeAllowed)
    {
        ubyte4  renegotiatedConnLength = 0;

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
        ubyte4  verifyDataSize = (SSL3_MINORVERSION == pSSLSock->sslMinorVersion) ? SSL_VERIFY_DATA : TLS_VERIFYDATASIZE;
#else
        ubyte4   verifyDataSize = TLS_VERIFYDATASIZE;
#endif
        if (0 < pSSLSock->handshakeCount)
        {
            renegotiatedConnLength = (2 * verifyDataSize);
        }

        /* type(2) + extLen(2) + renegotiated_connection<0..255> */
        totalExtensionsLength += 2 + 2 + 1 + renegotiatedConnLength;

        /* write extension type */
        setShortValue(pVarPart, (ubyte2)tlsExt_renegotiated_connection);
        pVarPart += sizeof(ubyte2);

        /* write extension length */
        setShortValue(pVarPart, (ubyte2)(1 + renegotiatedConnLength));
        pVarPart += sizeof(ubyte2);

        /* write renegotiated_connection<0..255> length */
        *pVarPart = renegotiatedConnLength;
        pVarPart += 1;

        if (0 < renegotiatedConnLength)
        {
            /* write renegotiated_connection string */
            MOC_MEMCPY(pVarPart, pSSLSock->client_verify_data, verifyDataSize);
            MOC_MEMCPY(verifyDataSize + pVarPart, pSSLSock->server_verify_data, verifyDataSize);
            pVarPart += renegotiatedConnLength;
        }
    }
#endif /* ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__))) */

    if (0 < pSSLSock->sslMinorVersion)
    {
        if ((SSL_FLAG_ACCEPT_SERVER_NAME_LIST & pSSLSock->runtimeFlags) &&
               pSSLSock->serverNameListLength )
        {
            totalExtensionsLength += 4;

            setShortValue( pVarPart, tlsExt_server_name);/* type */
            pVarPart+=2;
            setShortValue( pVarPart, 0); /* size */
            pVarPart+=2;
        }

        if (pSSLSock->certStatusReqExt)
        {
            totalExtensionsLength += 4;

            setShortValue( pVarPart, tlsExt_status_request);/* type */
            pVarPart+=2;
            setShortValue( pVarPart, 0); /* size */
            pVarPart+=2;
        }
#ifdef __ENABLE_MOCANA_INNER_APP__
        if (pSSLSock->roleSpecificInfo.server.innerApp && pSSLSock->receivedInnerApp)
        {
            totalExtensionsLength += 6;

            setShortValue( pVarPart, tlsExt_innerApplication);/* type */
            pVarPart+=2;
            setShortValue( pVarPart, 2); /* size */
            pVarPart+=2;
            setShortValue( pVarPart, (ubyte2) pSSLSock->roleSpecificInfo.server.innerAppValue); /* Shoudl be Based upon Session Resumption Also */
            pVarPart+=2;
        }
#endif

#if (defined(__ENABLE_MOCANA_DTLS_SRTP__))
        if (pSSLSock->isDTLS && pSSLSock->useSrtp)
        {
            ubyte4 useSrtpExtLen;
            /* type(2) + extLen(2) + profileLen(2) + profile(2) + srtpMkiLen(1) + srtpMki */
            useSrtpExtLen = 2 + 2 + 2 + 2 + 1 + ((NULL != pSSLSock->srtpMki) ? *(pSSLSock->srtpMki) : 0);
            totalExtensionsLength += useSrtpExtLen;

            setShortValue( pVarPart, dtlsExt_use_srtp);/* type */
            pVarPart += 2;
            setShortValue( pVarPart, useSrtpExtLen - 2 - 2); /* size */
            pVarPart += 2;
            /* set profile size */
            setShortValue( pVarPart, 2); /* size */
            pVarPart += 2;
            /* set the server chosen srtp protection profile */
            setShortValue( pVarPart, pSSLSock->pHandshakeSrtpProfile->profileId);
            pVarPart += 2;
            if (NULL != pSSLSock->srtpMki)
            {
                MOC_MEMCPY(pVarPart, pSSLSock->srtpMki, 1 + *(pSSLSock->srtpMki));
                pVarPart += 1 + *(pSSLSock->srtpMki);
            }
            else
            {
                *pVarPart = 0;
                pVarPart += 1;
            }
        }
#endif
    }

    /* set total extensions length now */
    if ( totalExtensionsLength)
    {
        setShortValue( pExtensionsLength, totalExtensionsLength);
    }
#endif /* __ENABLE_RFC3546__ */

    return pVarPart;
}


/*------------------------------------------------------------------*/

static ubyte *
fillCertificate(SSLSocket* pSSLSock, ubyte* pHSRec)
{
    sbyte4 i;
    ubyte* pVarPart;
    ubyte2 hsSize;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        pVarPart = (ubyte *)(((DTLSHandshakeHeader*)pHSRec) + 1);
        hsSize = getMediumValue(((DTLSHandshakeHeader*)pHSRec)->handshakeSize);
    } else
#endif
    {
        pVarPart = (ubyte *)(((SSLHandshakeHeader*)pHSRec) + 1);
        hsSize = getMediumValue(((SSLHandshakeHeader*)pHSRec)->handshakeSize);
    }

    setMediumValue(pVarPart, (ubyte2)(hsSize - SSL_MEDIUMSIZE)); /* certificate chain length */
    pVarPart += SSL_MEDIUMSIZE;

    /* for each certificate, store value in medium and then certificate     */
    for (i = 0; i < pSSLSock->roleSpecificInfo.server.numCertificates; ++i)
    {
        ubyte2 certLen = pSSLSock->roleSpecificInfo.server.certificates[i].length;

        setMediumValue(pVarPart, certLen);
        pVarPart += SSL_MEDIUMSIZE;

        MOC_MEMCPY(pVarPart,
                pSSLSock->roleSpecificInfo.server.certificates[i].data,
                certLen);

        pVarPart += certLen;
    }
    return pVarPart;
}


/*------------------------------------------------------------------*/

#ifdef __ENABLE_TLSEXT_RFC6066__

static ubyte *
fillCertificateStatus(SSLSocket* pSSLSock, ubyte* pHSRec, ubyte* pOcspResponse, ubyte4 responseLen)
{
    ubyte* pVarPart;
    ubyte2 hsSize;

    pVarPart = (ubyte *)(((SSLHandshakeHeader*)pHSRec) + 1);
    hsSize = getMediumValue(((SSLHandshakeHeader*)pHSRec)->handshakeSize);


    /* Fill in Certificate status */
    *pVarPart = 1; /* Certificate status type */
    pVarPart ++;

    setMediumValue(pVarPart, responseLen);
    pVarPart += SSL_MEDIUMSIZE;

    MOC_MEMCPY(pVarPart, pOcspResponse, responseLen);
    pVarPart += responseLen;

    return pVarPart;
}

#endif

/*------------------------------------------------------------------*/

#if ( defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) )
static MSTATUS
signServerKeyExchangeECC(SSLSocket* pSSLSock, ubyte* pData, ubyte4 dataLen,
                         ubyte4* pMsgSize)
{
    ubyte*          pHashResult = NULL;
    SHA1_CTX*       pShaContext = NULL;
    BulkCtx         pHashCtx = NULL;
    hashSuite       *pHashSuite = NULL;
    ubyte4          hashLen;
    ubyte*          pDest;
    ECCKey*         pECCKey;
    PrimeFieldPtr   pPF = 0;
    sbyte4          elementLen;
    PFEPtr          r, s;
    ubyte*          pStorageBuffer = 0;
    ubyte*          pR;
    ubyte*          pS;
    DER_ITEMPTR     pSignature = 0;
    ubyte4          remainingBufferLen;
    MSTATUS         status;
    intBoolean      isTLSorDTLS12 = FALSE;

    if ( akt_ecc != pSSLSock->roleSpecificInfo.server.privateKey.type)
        return ERR_BAD_KEY_TYPE;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult))))
        goto exit;

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        isTLSorDTLS12 = TRUE;
    }

    if (!isTLSorDTLS12)
    {
        hashLen = SHA1_RESULT_SIZE;
        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->shaPool, (void **)(&pShaContext))))
            goto exit;

        if (OK > (status = SHA1_initDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext)))
            goto exit;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pSSLSock->pClientRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pSSLSock->pServerRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pData, dataLen)))
            goto exit;

        if (OK > (status = SHA1_finalDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pHashResult)))
            goto exit;
    }
    else
    {
        /* For TLS1.2, the hash algo comes from the sigAlgo ubyte2 before the signature */
        ubyte2  sigAlgo = pSSLSock->signatureAlgo;
        ubyte4 i;
        for (i = 0; i < NUM_SSL_SUPPORTED_HASH_ALGORITHMS; i++)
        {
            if (gSupportedHashAlgorithms[i].hashType == ((sigAlgo >> 8) & 0xff))
            {
                pHashSuite = &gSupportedHashAlgorithms[i];
                break;
            }
        }

        if (!pHashSuite)
        {
            status = ERR_SSL_UNSUPPORTED_ALGORITHM;
            goto exit;
        }

        hashLen = pHashSuite->algo->digestSize;

        if (OK > (status = pHashSuite->algo->allocFunc(MOC_HASH(pSSLSock->hwAccelCookie) &pHashCtx )))
            goto exit;

        /* compute the hash of the data */
        if (OK > (status = pHashSuite->algo->initFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx)))
            goto exit;

        if (OK > (status = pHashSuite->algo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pSSLSock->pClientRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = pHashSuite->algo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pSSLSock->pServerRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = pHashSuite->algo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pData, dataLen)))
            goto exit;

        if (OK > (status = pHashSuite->algo->finalFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pHashResult )))
            goto exit;
    }

    pECCKey = pSSLSock->roleSpecificInfo.server.privateKey.key.pECC;

    pPF = EC_getUnderlyingField( pECCKey->pCurve);

    if (*pMsgSize < dataLen)
    {
        status = ERR_BUFFER_OVERFLOW;
        goto exit;
    }

    /* for TLS1.2 and above, add sigalgo */
    if (isTLSorDTLS12)
    {
        setShortValue(pData + dataLen, pSSLSock->signatureAlgo);
    }

    /* store signature */

    if (OK > ( status = PRIMEFIELD_newElement( pPF, &r)))
        goto exit;

    if (OK > (status = PRIMEFIELD_newElement( pPF, &s)))
        goto exit;

    if (OK > ( status =  ECDSA_sign( pECCKey->pCurve, pECCKey->k, pSSLSock->rngFun,
                                        pSSLSock->rngFunArg, pHashResult, hashLen,
                                        r, s)))
    {
        goto exit;
    }

    /* allocate buffer for the elements */
    if ( OK > (status = PRIMEFIELD_getElementByteStringLen( pPF, &elementLen)))
        goto exit;

    /* add 2 more bytes for the possible zero padding */
    pStorageBuffer = MALLOC( 2 + 2 * elementLen);
    if ( !pStorageBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* zero padding */
    pR = pStorageBuffer;
    *pR = 0x00; /* leading 0 */
    pS = pStorageBuffer + 1 + elementLen;
    *pS = 0x00; /* leading 0 */
    /* write to buffer */
    if ( OK > (status = PRIMEFIELD_writeByteString( pPF, r, pR+1, elementLen)))
        goto exit;
    if ( OK > ( status = PRIMEFIELD_writeByteString( pPF, s, pS+1, elementLen)))
        goto exit;

    /* create the ASN.1 sequence */
    if ( OK > ( status = DER_AddSequence(NULL, &pSignature)))
        goto exit;
    if ( OK > ( status = DER_AddInteger( pSignature, elementLen+1, pR, NULL)))
        goto exit;
    if ( OK > ( status = DER_AddInteger( pSignature, elementLen+1, pS, NULL)))
        goto exit;

    /* serialize to the destination buffer */
    /* TLS1.2 has a signatureAndHashAlgorithm ubyte2 before signature */
    pDest = pData + dataLen+ (isTLSorDTLS12? 2 : 0);
    remainingBufferLen = *pMsgSize - dataLen - 2 - (isTLSorDTLS12? 2 : 0);
    if (OK > ( status = DER_SerializeInto( pSignature, pDest + 2, &remainingBufferLen)))
        goto exit;

    /* set the short size before the signature */
    setShortValue( pDest, (ubyte2) remainingBufferLen);
    /* adjust size */
    *pMsgSize = dataLen + 2 + remainingBufferLen + (isTLSorDTLS12? 2 : 0);

exit:
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult));

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        if (pShaContext)
            MEM_POOL_putPoolObject(&pSSLSock->shaPool, (void **)(&pShaContext));
    } else
    {
        if (pHashCtx)
            pHashSuite->algo->freeFunc(MOC_HASH(pSSLSock->hwAccelCookie) &pHashCtx);
    }
    PRIMEFIELD_deleteElement( pPF, &r);
    PRIMEFIELD_deleteElement( pPF, &s);

    if ( pSignature)
    {
        TREE_DeleteTreeItem( (TreeItem*) pSignature);
    }

    if ( pStorageBuffer)
    {
        FREE(pStorageBuffer);
    }

    return status;
}
#endif /* defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) */

/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)))
static MSTATUS
signServerKeyExchangeRSA( SSLSocket* pSSLSock, ubyte* pData, ubyte4 dataLen)
{
    ubyte*      pHashResult = NULL;
    ubyte4      hashLen = 0;
    sbyte4      lenRsaKey;
    RSAKey*     pRSAKey;
    ubyte*      pDest;
    vlong*      pVlongQueue = NULL;
    MSTATUS     status;
    intBoolean  isTLSorDTLS12 = FALSE;
    MD5_CTX*    pMd5Context = NULL;
    SHA1_CTX*   pShaContext = NULL;

    if ( akt_rsa != pSSLSock->roleSpecificInfo.server.privateKey.type)
    {
        status = ERR_BAD_KEY_TYPE;
        goto exit;
    }

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult))))
        goto exit;

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        isTLSorDTLS12 = TRUE;
    }

    if (isTLSorDTLS12)
    {
        if (OK > (status = calculateTLS12KeyExchangeRSASignature(pSSLSock, pData, dataLen,
                                                            pSSLSock->signatureAlgo, pHashResult,
                                                            &hashLen)))
        {
            goto exit;
        }
    }
    else
    {
        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Context))))
            goto exit;

        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->shaPool, (void **)(&pShaContext))))
            goto exit;

        if (OK > (status = MD5Init_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context)))
            goto exit;

        if (OK > (status = MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context, pSSLSock->pClientRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context, pSSLSock->pServerRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context, pData, dataLen)))
            goto exit;

        if (OK > (status = MD5Final_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context, pHashResult)))
            goto exit;

        if (OK > (status = SHA1_initDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext)))
            goto exit;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pSSLSock->pClientRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pSSLSock->pServerRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pData, dataLen)))
            goto exit;

        if (OK > (status = SHA1_finalDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pHashResult + MD5_DIGESTSIZE)))
            goto exit;

        hashLen = MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE;
    }

    pRSAKey = pSSLSock->roleSpecificInfo.server.privateKey.key.pRSA;

    if (OK> ( status = RSA_getCipherTextLength( pRSAKey, &lenRsaKey)))
        goto exit;

    /* store signature */
    /* TLS1.2 has a signatureAndHashAlgorithm ubyte2 before signature */
    pDest = pData + dataLen+ (isTLSorDTLS12? 2 : 0);

    setShortValue(pDest, (ubyte2)lenRsaKey);

    status = RSA_signMessage(MOC_RSA(pSSLSock->hwAccelCookie) pRSAKey,
                             pHashResult, hashLen,
                             2 + pDest, &pVlongQueue);

exit:

    MEM_POOL_putPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Context));
    MEM_POOL_putPoolObject(&pSSLSock->shaPool, (void **)(&pShaContext));
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult));
    VLONG_freeVlongQueue( &pVlongQueue);

    return status;
}
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__))
static MSTATUS
fillServerKeyExchange(SSLSocket* pSSLSock, ubyte* pHSH, ubyte *pPskHint, ubyte4 pskHintLength)
{
#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
    sbyte4              lenP;
    sbyte4              lenG;
    sbyte4              lenYs;
#endif
    ubyte*              p_storeData;
    ubyte*              pStartData;
    MSTATUS             status = OK;
#ifndef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
    MOC_UNUSED(pPskHint);
    MOC_UNUSED(pskHintLength);
#endif

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        pStartData = p_storeData = (ubyte *)(pHSH + sizeof(DTLSHandshakeHeader));
    } else
#endif
    {
        pStartData = p_storeData = (ubyte *)(pHSH + sizeof(SSLHandshakeHeader));
    }

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
    /* calc lengths */
    lenP  = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_p)) / 8;
    lenG  = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_g)) / 8;
    lenYs = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_f)) / 8;
#endif

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
    /* store PSK hint */
   if ((SSL_PSK_BIT & pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags))
    {
        /* store psk hint length */
        setShortValue(p_storeData, (ubyte2)pskHintLength);

        /* dup psk hint */
        MOC_MEMCPY(2 + p_storeData, pPskHint, pskHintLength);

        /* update position */
        p_storeData += 2 + pskHintLength;
    }
#endif

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
    /* make sure DH is the key exchange algorithm */
    if (0 == (SSL_KEYEX_DH_BIT & pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags))
        goto exit;

    /* store p */
    setShortValue(p_storeData, (ubyte2)lenP);

    if (OK > (status = VLONG_fixedByteStringFromVlong(pSSLSock->pDHcontext->dh_p, 2 + p_storeData, lenP)))
        goto exit;

    p_storeData += 2 + lenP;

    /* store g */
    setShortValue(p_storeData, (ubyte2)lenG);

    if (OK > (status = VLONG_fixedByteStringFromVlong(pSSLSock->pDHcontext->dh_g, 2 + p_storeData, lenG)))
        goto exit;

    p_storeData += 2 + lenG;

    /* store Ys */
    setShortValue(p_storeData, (ubyte2)lenYs);

    if (OK > (status = VLONG_fixedByteStringFromVlong(pSSLSock->pDHcontext->dh_f, 2 + p_storeData, lenYs)))
        goto exit;

    p_storeData += 2 + lenYs;

#ifdef __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__
    if (SSL_DH_ANON == pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags)
        goto exit;
#endif

    if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_RSA_BIT)
    {
#if ((defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__)) && (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__)))
        if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
            (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
        {
            /* add signatureAndHashAlgorithm ubyte2 */
            setShortValue(p_storeData, (ubyte2)pSSLSock->signatureAlgo);
        }
        /* create data to be signed */
        /* pSecret = (0 == pSSLSock->advertisedMinorVersion) ? START_RANDOM(pSSLSock) : */
        if (OK > ( status = signServerKeyExchangeRSA( pSSLSock, pStartData, (ubyte4)(p_storeData - pStartData))))
        {
            goto exit;
        }
#else
        status = ERR_RSA_DISABLED;
#endif /* ((defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__)) && (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__))) */
    }
#endif /* (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)) */

exit:
    return status;

} /* fillServerKeyExchange */
#endif /* (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)) */


/*------------------------------------------------------------------*/

#if (   defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
        defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
static MSTATUS
fillServerEcdheKeyExchange(SSLSocket* pSSLSock, ubyte* pHSH,
                           ubyte *pPskHint, ubyte4 pskHintLength)
{
    ubyte*  pMsg;
    sbyte4  ephemeralKeyLen;
    ECCKey* pECCKey;
    MSTATUS status;
    ubyte4  msgSize;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        pMsg = (ubyte *)(pHSH + sizeof(DTLSHandshakeHeader));
        msgSize = getMediumValue(((DTLSHandshakeHeader*)pHSH)->handshakeSize);
    } else
#endif
    {
        pMsg = (ubyte *)(pHSH + sizeof(SSLHandshakeHeader));
        msgSize = getMediumValue(((SSLHandshakeHeader*)pHSH)->handshakeSize);
    }

    if ( akt_ecc != pSSLSock->ecdheKey.type)
    {
        status = ERR_BAD_KEY_TYPE;
        goto exit;
    }

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
    /* store PSK hint */
    if ((SSL_PSK_BIT & pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags))
    {
        /* store psk hint length */
        setShortValue(pMsg, (ubyte2)pskHintLength);

        /* dup psk hint */
        MOC_MEMCPY(2 + pMsg, pPskHint, pskHintLength);

        /* update position */
        pMsg += 2 + pskHintLength;
    }
#endif

    pECCKey = pSSLSock->ecdheKey.key.pECC;

    /* fill the message */
    pMsg[0] =  tlsECCurveType_named_curve;
    /* NamedCurve */
    setShortValue( pMsg+1, SSL_getNamedCurveOfCurveId( CRYPTO_getECCurveId( &pSSLSock->ecdheKey)));
    /* ECPoint */
    if (OK > ( status = EC_getPointByteStringLen( pECCKey->pCurve, &ephemeralKeyLen)))
        goto exit;
    /* write byte length */
    pMsg[3] = (ubyte) ephemeralKeyLen;
    /* write point (uncompressed format) */
    if ( OK > ( status = EC_writePointToBuffer( pECCKey->pCurve, pECCKey->Qx, pECCKey->Qy,
                                                pMsg + 4, ephemeralKeyLen)))
    {
        goto exit;
    }

#if defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)
    if (SSL_AUTH_ANON_BIT & pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags)
        goto exit;
#endif

#if defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
         setShortValue(pMsg + 4 + ephemeralKeyLen, pSSLSock->signatureAlgo ); /* ubyte2 SignatureAndHashAlgorithm */
    }

    /* signature */
    switch ( pSSLSock->roleSpecificInfo.server.privateKey.type)
    {
        case akt_rsa:
            if (OK > (status = signServerKeyExchangeRSA(pSSLSock, pMsg, 4 + ephemeralKeyLen)))
            {
                goto exit;
            }
            break;

        case akt_ecc:
            if (OK > (status = signServerKeyExchangeECC(pSSLSock, pMsg, 4 + ephemeralKeyLen, &msgSize)))
            {
                goto exit;
            }
            /* msgSize might have changed -- business with DER encoding of INTEGER --  */
            /* communicate it by setting the medium in the HandshakeHeader -- not very elegant... */
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
            if (pSSLSock->isDTLS)
            {
                setMediumValue(((DTLSHandshakeHeader*)pHSH)->handshakeSize, (ubyte2) msgSize);
            } else
#endif
            {
                setMediumValue(((SSLHandshakeHeader*)pHSH)->handshakeSize, (ubyte2) msgSize);
            }
            break;

        default:
            status = ERR_BAD_KEY_TYPE;
            goto exit;
            break;
    }
#endif

exit:

    return status;

} /* fillServerEcdheKeyExchange */
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__))
static MSTATUS
fillServerPskKeyExchange(SSLSocket* pSSLSock, ubyte* pHSH, ubyte *pPskHint, ubyte4 pskHintLength)
{
    ubyte*              p_storeData;
    MSTATUS             status = OK;
    MOC_UNUSED(pSSLSock);

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        p_storeData = (ubyte *)(pHSH + sizeof(DTLSHandshakeHeader));
    } else
#endif
    {
        p_storeData = (ubyte *)(pHSH + sizeof(SSLHandshakeHeader));
    }

    /* store PSK hint */
    if ((0 < pskHintLength) && (NULL != pPskHint))
    {
        /* store psk hint length */
        setShortValue(p_storeData, (ubyte2)pskHintLength);

        /* dup psk hint */
        MOC_MEMCPY(2 + p_storeData, pPskHint, pskHintLength);
    }

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)) */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
static MSTATUS
fillCertificateRequest(SSLSocket* pSSLSock, ubyte* pHSH)
{
    ubyte*  p_storeData;
    MSTATUS status = OK;
    MOC_UNUSED(pSSLSock);

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        p_storeData = (ubyte *)(pHSH + sizeof(DTLSHandshakeHeader));
    } else
#endif
    {
        p_storeData = (ubyte *)(pHSH + sizeof(SSLHandshakeHeader));
    }

    *p_storeData++ = NUM_CLIENT_CERT_TYPES;      /* number of client certificate types accepted */
    *p_storeData++ = tlsClientCertType_rsa_sign;
#ifdef __ENABLE_MOCANA_ECC__
    *p_storeData++ = tlsClientCertType_ecdsa_sign;
    *p_storeData++ = tlsClientCertType_rsa_fixed_ecdh;
    *p_storeData++ = tlsClientCertType_ecdsa_fixed_ecdh;
#endif

    /* TLS1.2 introduced a supported signature algorithms field */
    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        ubyte4 i;

        setShortValue(p_storeData, NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS*2);
        p_storeData += 2;
        for ( i = 0; i < NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS; ++i)
        {
            setShortValue( p_storeData, gSupportedSignatureAlgorithms[i]);
            p_storeData += sizeof(ubyte2);
        }
    }

    /* number of certificate authorities */
    *p_storeData++ = 0x00;
    *p_storeData++ = 0x00;

    return status;

} /* fillCertificateRequest */
#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ */


/*------------------------------------------------------------------*/

/*******************************************************************************
*      SSL_SERVER_sendServerHello
* send serverhello if sessionResume = E_SessionIDResume or sessionResume = E_SessionEAPFASTResume
* send serverhello, newsessionticket if sessionResume = E_SessionTicketResume and we renew session tickets
* send serverhello, certificate and serverhellodone if sessionResume = E_NoSessionResume
*/
static MSTATUS
SSL_SERVER_sendServerHello(SSLSocket* pSSLSock)
{
    intBoolean          isRehandshake = (pSSLSock->pActiveOwnCipherSuite) ? TRUE : FALSE;
#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) ||  \
    defined(__ENABLE_MOCANA_SSEL_ECDH_ANON_SUPPORT)
    ubyte               pskHint[SSL_PSK_SERVER_IDENTITY_LENGTH];
    ubyte4              pskHintLength = 0;
#endif
    sbyte4              i;
    sbyte4              bufIndex     = 0;
    sbyte4              numBufs      = 0;
    ubyte4              totalLen     = 0;
    ubyte4              fragmentLen  = 0;
    ubyte*              pHSH;
    ubyte*              pSRH;
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    ubyte4              sslFlags     = pSSLSock->runtimeFlags;
#endif
    ubyte4              numBytesSent = 0;
    intBoolean          isServerKeyExchangePresent = FALSE;
    ubyte4              totalExtensionsLength = 0;
    ubyte4              sizeofRecordHeader;
    ubyte4              sizeofHandshakeHeader;
    ubyte4              sizeofHandshakeRecord;
    MSTATUS             status       = OK;
    intBoolean          isTLSorDTLS12 = FALSE;
#if defined (__ENABLE_TLSEXT_RFC6066__) && defined (__ENABLE_MOCANA_OCSP_CLIENT__)
    ubyte*              pOcspResponse = NULL;
    ubyte4              responseLen   = 0;
#endif


    (void)isTLSorDTLS12;
    pSSLSock->buffers[0].pHeader = NULL;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        sizeofRecordHeader = sizeof(DTLSRecordHeader);
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        sizeofHandshakeRecord = sizeof(DTLSHandshakeRecord);
    } else
#endif
    {
        sizeofRecordHeader = sizeof(SSLRecordHeader);
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        sizeofHandshakeRecord = sizeof(SSLHandshakeRecord);
    }

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        isTLSorDTLS12 = TRUE;
    }

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
    if (SSL_PSK_BIT & pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags)
        if (NULL != SSL_sslSettings()->funcPtrGetHintPSK)
            if (OK > (status = (MSTATUS)(SSL_sslSettings()->funcPtrGetHintPSK(SSL_findConnectionInstance(pSSLSock), pskHint, &pskHintLength))))
                goto exit;
#endif

    /* generate server random */
    pSSLSock->rngFun(pSSLSock->rngFunArg, SSL_RANDOMSIZE,
                        pSSLSock->pSecretAndRand + SSL_RSAPRESECRETSIZE + SSL_RANDOMSIZE);

    MOC_MEMCPY(pSSLSock->pServerRandHello, pSSLSock->pSecretAndRand + SSL_RSAPRESECRETSIZE + SSL_RANDOMSIZE, SSL_RANDOMSIZE);

#ifdef __ENABLE_RFC3546__

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if (pSSLSock->isRehandshakeAllowed)
    {
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
        ubyte4  verifyDataSize = (0 == pSSLSock->sslMinorVersion) ? SSL_VERIFY_DATA : TLS_VERIFYDATASIZE;
#else
        ubyte4  verifyDataSize = TLS_VERIFYDATASIZE;
#endif

        totalExtensionsLength += (2 + 2 + 1); /* extension type + extension length + renegotiated_connection length */

        if (0 < pSSLSock->handshakeCount)
        {
            /* on rehandshakes, use verify data string from the previous handshake */
            totalExtensionsLength += (2 * verifyDataSize);
        }
    }
#endif /* ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__))) */

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if (SSL3_MINORVERSION < pSSLSock->sslMinorVersion)
#endif
    {
        if ((SSL_FLAG_ACCEPT_SERVER_NAME_LIST & pSSLSock->runtimeFlags) &&
               pSSLSock->serverNameListLength )
        {
            /* Check if servermame can be recognized */
            if (OK > (status = validateServerName(pSSLSock)))
            {
                pSSLSock->serverNameListLength = 0;
                if (pSSLSock->serverNameList)
                {
                    FREE (pSSLSock->serverNameList);
                    pSSLSock->serverNameList = NULL;
                }

                goto exit;
            } else {
                totalExtensionsLength += 4;
            }
        }

        if (pSSLSock->certStatusReqExt)
        {
#if defined(__ENABLE_MOCANA_OCSP_CLIENT__)
            if (OK > (status =  SSL_OCSP_getOcspResponse(pSSLSock, &pOcspResponse, &responseLen)))
                goto exit;

            /* Check for the response length */
            if ((0 < responseLen) && (NULL != pOcspResponse))
            {
                /* we have an ocsp response to send back */
                totalExtensionsLength += 4;
            } else {
                pSSLSock->certStatusReqExt = FALSE;
            }
#endif
        }
#ifdef __ENABLE_MOCANA_INNER_APP__
        if (pSSLSock->roleSpecificInfo.server.innerApp && pSSLSock->receivedInnerApp)
        {
            totalExtensionsLength += 6;
        }
#endif

#if (defined(__ENABLE_MOCANA_DTLS_SRTP__))
        if (pSSLSock->isDTLS && pSSLSock->useSrtp)
        {
            /* type(2) + extLen(2) + profileLen(2) + profile(2) + srtpMkiLen(1) + srtpMki */
            totalExtensionsLength += 2 + 2 + 2 + 2 + 1 + ((NULL != pSSLSock->srtpMki) ? *(pSSLSock->srtpMki) : 0);
        }
#endif
    }

    if ( totalExtensionsLength)
    {
        totalExtensionsLength += 2; /* for its own size */
    }
#endif /* __ENABLE_RFC3546__ */

    /* compute the size of all three, four or five handshakes messages */
    /* SSL and TLS p.80 */
    /* server hello */
    pSSLSock->buffers[bufIndex].length = (ubyte)(sizeofRecordHeader + sizeofHandshakeHeader + 2 +
                                         SSL_RANDOMSIZE + 1 + sizeof(SESSIONID) + 2 + 1 + totalExtensionsLength);
    bufIndex++;
    numBufs++;

    pSSLSock->buffers[1].length = pSSLSock->buffers[2].length = pSSLSock->buffers[3].length = pSSLSock->buffers[4].length = 0;

    if (E_NoSessionResume == pSSLSock->sessionResume)
    {
        /* SSL and TLS p.81 */
        if (0 == (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ANON_BIT))
        {
            /* certificate */
            pSSLSock->buffers[bufIndex].length = (ubyte2)(sizeofRecordHeader + sizeofHandshakeHeader + SSL_MEDIUMSIZE * (1 + pSSLSock->roleSpecificInfo.server.numCertificates));

            /* now add length of certificates themselves */
            for (i = 0; i < pSSLSock->roleSpecificInfo.server.numCertificates; ++i)
            {
                pSSLSock->buffers[bufIndex].length = (ubyte2)(pSSLSock->buffers[bufIndex].length + pSSLSock->roleSpecificInfo.server.certificates[i].length);
            }

            bufIndex++;
            numBufs++;
        }

#ifdef __ENABLE_MOCANA_OCSP_CLIENT__
        if (pSSLSock->certStatusReqExt)
        {
            /* Add certificate status right after certificate if response is acquired */
            pSSLSock->buffers[bufIndex].length = (ubyte2)(sizeofRecordHeader + sizeofHandshakeHeader + 1 + SSL_MEDIUMSIZE + responseLen);
            bufIndex ++;
            numBufs++;
        }
#endif

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
        /* for PSK ciphers, we need to send a ServerKeyExchange if there's a hint or
         the algo requires a ServerKeyExchange message: DHE or ECDHE */
      	if ((pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_PSK_BIT) &&
            (pskHintLength > 0 ||
             (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_DH_BIT) ||
             (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_ECDHE_BIT) ))
        {
            pSSLSock->buffers[bufIndex].length = (ubyte2)(pSSLSock->buffers[bufIndex].length + 2 + pskHintLength);
            isServerKeyExchangePresent = TRUE;
        }
#endif

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)))
        if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_DH_BIT)
        {
            sbyte4  lenP;
            sbyte4  lenG;
            sbyte4  lenYs;
            sbyte4  lenRsaKey = 0;

            /* server key exchange for DHE support */
            isServerKeyExchangePresent = TRUE;

            if (NULL != pSSLSock->pDHcontext)
                DH_freeDhContext(&pSSLSock->pDHcontext, NULL);

            if (OK > (status = DH_allocateServer(MOC_DH(pSSLSock->hwAccelCookie) pSSLSock->rngFunArg,
                                                 &(pSSLSock->pDHcontext),
                                                 SSL_DEFAULT_DH_GROUP)))
            {
                goto exit;
            }

            lenP  = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_p)) / 8;
            lenG  = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_g)) / 8;
            lenYs = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_f)) / 8;

#if defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__)
            if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_RSA_BIT)
            {
                AsymmetricKey* pPrivateKey;

                pPrivateKey = &pSSLSock->roleSpecificInfo.server.privateKey;
                if ( akt_rsa != pPrivateKey->type)
                {
                    status = ERR_SSL_INVALID_KEY_TYPE;
                    goto exit;
                }
                lenRsaKey = 2 + ((7 + VLONG_bitLength(RSA_N(pPrivateKey->key.pRSA))) / 8);
            }
#endif
            pSSLSock->buffers[bufIndex].length = (ubyte2)(pSSLSock->buffers[bufIndex].length + (6 + lenP + lenG + lenYs + lenRsaKey));
            if (isTLSorDTLS12 &&
                !(pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ANON_BIT))
            {
                pSSLSock->buffers[bufIndex].length += 2; /* ubyte2 SignatureAndHashAlgorithm */
            }
        }
#endif /* (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))) */

#if (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  || \
     defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
        if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_ECDHE_BIT)
        {
            ubyte4 msgLen;
            sbyte4 tmpLen, i;
            PEllipticCurvePtr pCurve = 0;
            ECCKey *pECCKey;
            enum tlsExtNamedCurves chosenCurve = 0;

            /* server key exchange for DHE support */
            isServerKeyExchangePresent = TRUE;

            /* allocate the key using the information in clientECCurves */
            if (SSL_sslSettings()->funcPtrChooseECCCurve)
            {
                ubyte4 listLength = 0;
                enum tlsExtNamedCurves possibleCurves[NUM_ECC_PCURVES];

                for (i = 0; i < NUM_SSL_SUPPORTED_ELLIPTIC_CURVES; ++i)
                {
                    if ( ( 1 << (gSupportedEllipticCurves[i])) &
                        (pSSLSock->roleSpecificInfo.server.clientECCurves))
                    {
                        possibleCurves[listLength++] = gSupportedEllipticCurves[i];
                    }
                }

                if (OK > (status = (MSTATUS)
                          (SSL_sslSettings()->funcPtrChooseECCCurve(
                                        SSL_findConnectionInstance(pSSLSock),
                                        pSSLSock->pHandshakeCipherSuite->cipherSuiteId,
                                        possibleCurves, listLength,
                                        &chosenCurve))))
                {
                    goto exit;
                }
            }
            else
            {
                for (i = 0; i < NUM_SSL_SUPPORTED_ELLIPTIC_CURVES; ++i)
                {
                    if ( ( 1 << (gSupportedEllipticCurves[i])) &
                            (pSSLSock->roleSpecificInfo.server.clientECCurves))
                    {
                        chosenCurve = gSupportedEllipticCurves[i];
                        break;
                    }
                }
            }

            pCurve = SSL_SOCK_getCurveFromName(chosenCurve);

            if ( !pCurve)
            {
                status = ERR_SSL_UNSUPPORTED_CURVE;
                goto exit;
            }

            /* allocate the key here */
            if (OK > (status = CRYPTO_createECCKey( &pSSLSock->ecdheKey, pCurve, NULL)))
                goto exit;

            pECCKey = pSSLSock->ecdheKey.key.pECC;

            /* generate it */
            if ( OK > ( status = EC_generateKeyPair( pCurve, pSSLSock->rngFun, pSSLSock->rngFunArg,
                                                    pECCKey->k, pECCKey->Qx, pECCKey->Qy)))
            {
                goto exit;
            }

            msgLen = 4; /* named curve type (1) + named curve (2) + point length (1) */

            if (OK > ( status = EC_getPointByteStringLen( pCurve, &tmpLen)))
                goto exit;

            msgLen += (ubyte4) tmpLen;

#if defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)
            if ( pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & (SSL_AUTH_RSA_BIT | SSL_AUTH_ECDSA_BIT))
            {
                /* add signature length */
                switch ( pSSLSock->roleSpecificInfo.server.privateKey.type)
                {
                    case akt_rsa:
                        if (OK > (status = RSA_getCipherTextLength( pSSLSock->roleSpecificInfo.server.privateKey.key.pRSA,
                                                                    &tmpLen)))
                        {
                            goto exit;
                        }
                        msgLen += (ubyte4) (tmpLen + 2);
                        break;

                    case akt_ecc:
                        /* the ECDSA signature is more complex -- needs its own function */
                        if ( OK > ( status = SSL_SOCK_getECCSignatureLength( pSSLSock->roleSpecificInfo.server.privateKey.key.pECC,
                                                                                &tmpLen)))
                        {
                            goto exit;
                        }
                        msgLen += (ubyte4) (tmpLen + 2);
                        break;

                    default:
                        status = ERR_BAD_KEY_TYPE;
                        goto exit;
                        break;
                }
                if (isTLSorDTLS12 &&
                    !(pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ANON_BIT))
                {
                    msgLen += 2; /* ubyte2 SignatureAndHashAlgorithm */
                }
            }
#endif
            pSSLSock->buffers[bufIndex].length = (ubyte2)(pSSLSock->buffers[bufIndex].length + msgLen);
        }
#endif /* defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) ||
        defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) */

        if (TRUE == isServerKeyExchangePresent)
        {
            pSSLSock->buffers[bufIndex].length = (ubyte2)(pSSLSock->buffers[bufIndex].length + sizeofRecordHeader + sizeofHandshakeHeader);
            bufIndex++;
            numBufs++;
        }

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
        if ((SSL_NO_MUTUAL_AUTH_BIT != (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_NO_MUTUAL_AUTH_BIT)) &&
            (SSL_FLAG_NO_MUTUAL_AUTH_REQUEST != (sslFlags & SSL_FLAG_NO_MUTUAL_AUTH_REQUEST)) )
        {

            /* calc certificate request length */
            pSSLSock->buffers[bufIndex].length = (ubyte2)(sizeofRecordHeader + sizeofHandshakeHeader +
                                                 (isTLSorDTLS12? 2 + NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS*2 :0) +
                                                 3 + NUM_CLIENT_CERT_TYPES);
            bufIndex++;
            numBufs++;
        }
#endif

        /* SSL and TLS p.85 */
        /* server hello done */
        pSSLSock->buffers[bufIndex].length = sizeofRecordHeader + sizeofHandshakeHeader;
        numBufs++;
    }

    /* allocate enough space for all records */
    for (bufIndex = 0; bufIndex < numBufs; bufIndex++)
        totalLen +=  pSSLSock->buffers[bufIndex].length;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        fragmentLen = calculateExtraFragmentHeader(pSSLSock, numBufs);
    }
#endif

    if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, totalLen+fragmentLen, TRUE, (void **)&(pSSLSock->buffers[0].pHeader))))
        goto exit;

    /* initialize other pointers */
    for (bufIndex = 0; bufIndex < numBufs; bufIndex++)
    {
        if (bufIndex < numBufs - 1)
        {
            pSSLSock->buffers[bufIndex + 1].pHeader = pSSLSock->buffers[bufIndex].pHeader + pSSLSock->buffers[bufIndex].length;
        }
        pSSLSock->buffers[bufIndex].data = pSSLSock->buffers[bufIndex].pHeader + sizeofRecordHeader;
    }
/*
    pSSLSock->buffers[1].pHeader = pSSLSock->buffers[0].pHeader + pSSLSock->buffers[0].length;
    pSSLSock->buffers[2].pHeader = pSSLSock->buffers[1].pHeader + pSSLSock->buffers[1].length;
    pSSLSock->buffers[3].pHeader = pSSLSock->buffers[2].pHeader + pSSLSock->buffers[2].length;
    pSSLSock->buffers[4].pHeader = pSSLSock->buffers[3].pHeader + pSSLSock->buffers[3].length;

    pSSLSock->buffers[0].data = pSSLSock->buffers[0].pHeader + sizeofRecordHeader;
    pSSLSock->buffers[1].data = pSSLSock->buffers[1].pHeader + sizeofRecordHeader;
    pSSLSock->buffers[2].data = pSSLSock->buffers[2].pHeader + sizeofRecordHeader;
    pSSLSock->buffers[3].data = pSSLSock->buffers[3].pHeader + sizeofRecordHeader;
    pSSLSock->buffers[4].data = pSSLSock->buffers[4].pHeader + sizeofRecordHeader;
*/
    bufIndex = 0;

    /* server hello ******************/
    pSRH = pSSLSock->buffers[bufIndex].pHeader;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        DTLS_SET_RECORD_HEADER_EXT(pSRH,pSSLSock,SSL_HANDSHAKE,pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
        pHSH = pSSLSock->buffers[bufIndex].data;
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader*)pHSH), pSSLSock->nextSendSeq++, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
        setMediumValue(((DTLSHandshakeHeader *)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
        ((DTLSHandshakeHeader *)pHSH)->handshakeType = SSL_SERVER_HELLO;
    } else
#endif
    {
        SSL_SET_RECORD_HEADER(pSRH, SSL_HANDSHAKE, pSSLSock->sslMinorVersion, pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
        pHSH = pSSLSock->buffers[bufIndex].data;
        setMediumValue(((SSLHandshakeHeader *)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
        ((SSLHandshakeHeader *)pHSH)->handshakeType = SSL_SERVER_HELLO;
    }

    fillServerHello(pSSLSock, pHSH);

    bufIndex++;

    if (E_NoSessionResume == pSSLSock->sessionResume)
    {
        if (0 == (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ANON_BIT))
        {
            /* certificate *******************/
            pSRH = pSSLSock->buffers[bufIndex].pHeader;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
            if (pSSLSock->isDTLS)
            {
                DTLS_SET_RECORD_HEADER_EXT(pSRH,pSSLSock,SSL_HANDSHAKE,pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
                pHSH = pSSLSock->buffers[bufIndex].data;
                DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader*)pHSH), pSSLSock->nextSendSeq++, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
                setMediumValue(((DTLSHandshakeHeader*)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
                ((DTLSHandshakeHeader*)pHSH)->handshakeType = SSL_CERTIFICATE;
            } else
#endif
            {
            SSL_SET_RECORD_HEADER(pSRH, SSL_HANDSHAKE, pSSLSock->sslMinorVersion, pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
            pHSH = pSSLSock->buffers[bufIndex].data;
            setMediumValue(((SSLHandshakeHeader*)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
            ((SSLHandshakeHeader*)pHSH)->handshakeType = SSL_CERTIFICATE;
            }

            fillCertificate(pSSLSock, pHSH);

            bufIndex++;
        }

#if defined(__ENABLE_RFC3546__) && defined(__ENABLE_TLSEXT_RFC6066__)
        if (pSSLSock->certStatusReqExt)
        {
            /* Certificate Status: Need to fill in certificate status response here */
            pSRH = pSSLSock->buffers[bufIndex].pHeader;
            SSL_SET_RECORD_HEADER(pSRH, SSL_HANDSHAKE, pSSLSock->sslMinorVersion, pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
            pHSH = pSSLSock->buffers[bufIndex].data;
            setMediumValue(((SSLHandshakeHeader*)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
            ((SSLHandshakeHeader*)pHSH)->handshakeType = SSL_CERTIFCATE_STATUS;

            fillCertificateStatus(pSSLSock, pHSH, pOcspResponse, responseLen);

            bufIndex++;
        }
#endif

#if defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__)          ||  \
        defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)  ||  \
        defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)      ||  \
        defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)    ||  \
        defined(__ENABLE_MOCANA_SSEL_ECDH_ANON_SUPPORT)
        /* server key exchange */
        if ((TRUE == isServerKeyExchangePresent) && (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->FillServerKEX))
        {
#ifdef __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__
            ubyte4 newBufferLen;
#endif

            pSRH = pSSLSock->buffers[bufIndex].pHeader;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
            if (pSSLSock->isDTLS)
            {
                DTLS_SET_RECORD_HEADER_EXT(pSRH,pSSLSock,SSL_HANDSHAKE,pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
                pHSH = pSSLSock->buffers[bufIndex].data;
                DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pHSH), pSSLSock->nextSendSeq++, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
                /* set the buffer lengths before calling the routine */
                setMediumValue(((DTLSHandshakeHeader*)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
                ((DTLSHandshakeHeader*)pHSH)->handshakeType = SSL_SERVER_KEY_EXCHANGE;
            } else
#endif
            {
                SSL_SET_RECORD_HEADER(pSRH, SSL_HANDSHAKE, pSSLSock->sslMinorVersion, pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
                pHSH = pSSLSock->buffers[bufIndex].data;
                /* set the buffer lengths before calling the routine */
                setMediumValue(((SSLHandshakeHeader*)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
                ((SSLHandshakeHeader*)pHSH)->handshakeType = SSL_SERVER_KEY_EXCHANGE;
            }

            if (OK > (status = pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->FillServerKEX(pSSLSock, pHSH, pskHint, pskHintLength)))
                goto exit;

#if defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)
            /* the routine might have modified the buffer length  -- this is because of
            the unfortunate ECDSA DER encoding */
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
            if (pSSLSock->isDTLS)
            {
                newBufferLen = getMediumValue( ((DTLSHandshakeHeader*)pHSH)->handshakeSize);
                setMediumValue(((DTLSHandshakeHeader*)pHSH)->fragLength, newBufferLen);
            } else
#endif
            {
                newBufferLen = getMediumValue( ((SSLHandshakeHeader*)pHSH)->handshakeSize);
            }

            if ( newBufferLen > pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord )
            {
                /* the FillServerKEX increased the buffer size! something is wrong here! */
                status = ERR_INTERNAL_ERROR;
                goto exit;
            }
            else if ( newBufferLen < pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord)
            {
                /* adjust buffer length */
                pSSLSock->buffers[bufIndex].length = (ubyte2) (sizeofHandshakeRecord + newBufferLen);

                /* adjust SSLRecord size */
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
                if (pSSLSock->isDTLS)
                {
                    setShortValue( ((DTLSRecordHeader*) pSRH)->recordLength, pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
                } else
#endif
                {
                    setShortValue( ((SSLRecordHeader*) pSRH)->recordLength, pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
                }

                /* need to adjust all the downstream buffers */
                for ( i = bufIndex; i < 4; ++i)
                {
                    pSSLSock->buffers[i+1].pHeader = pSSLSock->buffers[i].pHeader + pSSLSock->buffers[i].length;
                    pSSLSock->buffers[i+1].data = pSSLSock->buffers[i+1].pHeader + sizeofRecordHeader;
                }
                /* recompute totalLen too! */
                totalLen =  pSSLSock->buffers[0].length + pSSLSock->buffers[1].length + pSSLSock->buffers[2].length +
                            pSSLSock->buffers[3].length + pSSLSock->buffers[4].length;
            }
#endif /* __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__ */

            bufIndex++;
        }
#endif

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
        /* (to client) certificate request */
        if ((SSL_NO_MUTUAL_AUTH_BIT != (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_NO_MUTUAL_AUTH_BIT)) &&
            (SSL_FLAG_NO_MUTUAL_AUTH_REQUEST != (sslFlags & SSL_FLAG_NO_MUTUAL_AUTH_REQUEST)) )
        {
            pSSLSock->isMutualAuthNegotiated = TRUE;

            pSRH = pSSLSock->buffers[bufIndex].pHeader;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
            if (pSSLSock->isDTLS)
            {
                DTLS_SET_RECORD_HEADER(pSRH, SSL_HANDSHAKE, pSSLSock->sslMinorVersion, pSSLSock->ownSeqnumHigh, pSSLSock->ownSeqnum, pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);

                if (0 == (++pSSLSock->ownSeqnum))
                    pSSLSock->ownSeqnumHigh = (pSSLSock->ownSeqnumHigh & 0xffff0000) | ((pSSLSock->ownSeqnumHigh + 1) & 0xffff);

                pHSH = pSSLSock->buffers[bufIndex].data;
                DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pHSH), pSSLSock->nextSendSeq++, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
                setMediumValue(((DTLSHandshakeHeader *)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
                ((DTLSHandshakeHeader *)pHSH)->handshakeType = SSL_CERTIFICATE_REQUEST;
            }
            else
#endif
            {
                SSL_SET_RECORD_HEADER(pSRH, SSL_HANDSHAKE, pSSLSock->sslMinorVersion, pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
                pHSH = pSSLSock->buffers[bufIndex].data;
                setMediumValue(((SSLHandshakeHeader *)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
                ((SSLHandshakeHeader *)pHSH)->handshakeType = SSL_CERTIFICATE_REQUEST;
            }

            if (OK > (status = fillCertificateRequest(pSSLSock, pHSH)))
                goto exit;

            bufIndex++;
        }
#endif

        /* server hello done *************/
        pSRH = pSSLSock->buffers[bufIndex].pHeader;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
        if (pSSLSock->isDTLS)
        {
            DTLS_SET_RECORD_HEADER_EXT(pSRH,pSSLSock,SSL_HANDSHAKE,pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
            pHSH = pSSLSock->buffers[bufIndex].data;
            DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader*)pHSH), pSSLSock->nextSendSeq++, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
            setMediumValue(((DTLSHandshakeHeader *)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
            ((DTLSHandshakeHeader *)pHSH)->handshakeType = SSL_SERVER_HELLO_DONE;
        } else
#endif
        {
            SSL_SET_RECORD_HEADER(pSRH, SSL_HANDSHAKE, pSSLSock->sslMinorVersion, pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
            pHSH = pSSLSock->buffers[bufIndex].data;
            setMediumValue(((SSLHandshakeHeader *)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofHandshakeRecord));
            ((SSLHandshakeHeader *)pHSH)->handshakeType = SSL_SERVER_HELLO_DONE;
        }

    }

    /* add to the hashes */
    for (bufIndex = 0; bufIndex < numBufs; bufIndex++)
        addToHandshakeHash(pSSLSock, pSSLSock->buffers[bufIndex].data, (pSSLSock->buffers[bufIndex].length - sizeofRecordHeader));

    /* send the handshakes records to SSL record layer *************/
    if (TRUE != isRehandshake)
    {
        if (SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)
        {
            numBytesSent =  0;
            status = (MSTATUS)totalLen;
        }
        else
        {
#ifndef __MOCANA_IPSTACK__
        if (OK > (status = TCP_WRITE(pSSLSock->tcpSock, (sbyte *)(pSSLSock->buffers[0].pHeader), totalLen, &numBytesSent)))
#else
        if (OK > (status = MOC_TCP_WRITE(pSSLSock->tcpSock, (sbyte *)(pSSLSock->buffers[0].pHeader), totalLen, &numBytesSent)))
#endif
                  goto exit;
        }

        if (numBytesSent != totalLen)
        {
            pSSLSock->pOutputBufferBase  = pSSLSock->buffers[0].pHeader;
            pSSLSock->pOutputBuffer      = numBytesSent + pSSLSock->buffers[0].pHeader;
            pSSLSock->outputBufferSize   = totalLen+fragmentLen;
            pSSLSock->numBytesToSend     = totalLen+fragmentLen - numBytesSent;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
            if (pSSLSock->isDTLS)
            {
                /* fragment the records to meet PMTU */
                pSSLSock->bufIndex = 0;
                pSSLSock->numBuffers = numBufs;
                if (OK > (status = fragmentHandshakeMessages(pSSLSock, fragmentLen)))
                    goto exit;

                releaseRetransmissionBuffer(pSSLSock);
                addDataToRetransmissionBuffer(pSSLSock, SSL_HANDSHAKE, (const sbyte*)pSSLSock->pOutputBuffer, pSSLSock->numBytesToSend);
            }
#endif

            pSSLSock->buffers[0].pHeader = NULL;
        }
    }
    else
    {
        pSSLSock->bufIndex = 0;
        pSSLSock->numBuffers = numBufs;

        status = SSLSOCK_sendEncryptedHandshakeBuffer(pSSLSock);
    }

exit:
    if ((TRUE != isRehandshake) || (OK > status))
    {
        /* free the buffers */
        if (pSSLSock->buffers[0].pHeader)
        {
            CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&(pSSLSock->buffers[0].pHeader));
        }
    }

    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSL_SERVER_sendServerHello() returns status = ", status);

    return status;

} /* SSL_SERVER_sendServerHello */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__))
static MSTATUS
processClientRsaKeyExchangeCore(SSLSocket* pSSLSock,
                             ubyte* pEncryptedPremasterSecret, ubyte2 mesgLen,
                             ubyte* pPlainText,
                             vlong **ppVlongQueue)
{
    /* the client key exchange contains the encrypted premaster secret */
    ubyte4  realPlainTextLen;
    sbyte4  cipherTextLen;
    RSAKey* pRSAKey;
    ubyte   R[SSL_RSAPRESECRETSIZE]; /* Bleichenbacher attack */
    MSTATUS status;

    if ( akt_rsa != pSSLSock->roleSpecificInfo.server.privateKey.type)
    {
        status = ERR_SSL_INVALID_KEY_TYPE;
    }

    pRSAKey = pSSLSock->roleSpecificInfo.server.privateKey.key.pRSA;

    /* check that the actual record length matches the advertised record length */
    status = RSA_getCipherTextLength(pRSAKey, &cipherTextLen);

    if (status < OK)
        goto exit;

    if (MOCANA_MAX_MODULUS_SIZE < cipherTextLen)
    {
        status = ERR_SSL_CLIENT_KEY_SIZE_TOO_LONG;
        goto exit;
    }

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if (SSL3_MINORVERSION == pSSLSock->sslMinorVersion)
    {
        if ( cipherTextLen != mesgLen)
        {
            status = ERR_SSL_INVALID_MSG_SIZE;
            goto exit;
        }
    }
    else
#endif
    {
        if (cipherTextLen != mesgLen)
        {
            if ((2 + cipherTextLen) != mesgLen)
            {
                status = ERR_SSL_INVALID_MSG_SIZE;
                goto exit;
            }

            pEncryptedPremasterSecret += 2; /* length */
        }
    }

    /* TLS 1.2 Bleichenbacher/Klima attacks: always generate the random data */
    R[0] = (pSSLSock->isDTLS) ? DTLS1_MAJORVERSION : SSL3_MAJORVERSION;
    R[1] = pSSLSock->clientHelloMinorVersion;
    pSSLSock->rngFun(pSSLSock->rngFunArg, SSL_RSAPRESECRETSIZE - 2, R + 2);

    status = RSA_decrypt(MOC_RSA(pSSLSock->hwAccelCookie) pRSAKey,
                         pEncryptedPremasterSecret, pPlainText, &realPlainTextLen, pSSLSock->rngFun,
                         pSSLSock->rngFunArg, ppVlongQueue);

    /* algorithm described in TLS 1.2*/
    if ((OK > status) ||
        (realPlainTextLen != SSL_RSAPRESECRETSIZE) )
    {
        MOC_MEMCPY( pPlainText, R, SSL_RSAPRESECRETSIZE);
        status = OK;
    }
    else if ( pPlainText[0] != R[0] || pPlainText[1] != R[1])
    {
        pPlainText[0] = R[0];
        pPlainText[1] = R[1];
        status = OK;
    }

exit:
    return status;

} /* processClientRsaKeyExchangeCore */
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__))
static MSTATUS
processClientRsaKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage,
                         ubyte2 recLen, vlong **ppVlongQueue)
{
    /* the client key exchange contains the encrypted premaster secret */
    ubyte*      pPlainText = NULL;
    MSTATUS     status;

    if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, recLen, TRUE, (void **)&(pPlainText))))
        goto exit;

    if (OK > (status = processClientRsaKeyExchangeCore(pSSLSock, pMessage, recLen, pPlainText, ppVlongQueue)))
        goto exit;

    /* all checks passed -> generate key materials */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, pPlainText, SSL_RSAPRESECRETSIZE);

exit:
    if (pPlainText)
        CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&(pPlainText));

    return status;

} /* processClientRsaKeyExchange */
#endif


/*------------------------------------------------------------------*/
#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )

static MSTATUS
processClientEccKeyExchangeCore(ECCKey* pECCKey,
                                ubyte* pMessage, ubyte2 recLen,
                                ubyte** ppSharedSecret,
                                sbyte4 *pSharedSecretLen)
{
    /* the client key exchange contains the ECDH shared secret */
    MSTATUS             status;
    PrimeFieldPtr       pPF;
    PFEPtr              pQx = 0;
    PFEPtr              pQy = 0;
    ubyte2              keyLen;

    pPF = EC_getUnderlyingField( pECCKey->pCurve);

    /* extract the generated public key from the message */
    /* redundant byte length */
    keyLen = *pMessage++;
    if ( keyLen != recLen - 1)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    if ( OK > ( status = EC_byteStringToPoint( pECCKey->pCurve, pMessage, keyLen, &pQx, &pQy)))
        goto exit;

    /* generate the shared secret by multiplying the public key by our private key*/
    if ( OK > ( status = ECDH_generateSharedSecretAux( pECCKey->pCurve, pQx, pQy, pECCKey->k,
                                                      ppSharedSecret, pSharedSecretLen,
                                                      1))) /* X coordinate only */
    {
        goto exit;
    }

exit:

    PRIMEFIELD_deleteElement( pPF, &pQx);
    PRIMEFIELD_deleteElement( pPF, &pQy);

    return status;

} /* processClientEccKeyExchangeCore */


/*----------------------------------------------------------------------------*/

static MSTATUS
processClientEccKeyExchange(SSLSocket* pSSLSock, ECCKey* pECCKey,
                            ubyte* pMessage, ubyte2 recLen)
{
    /* the client key exchange contains the ECDH shared secret */
    MSTATUS             status;
    ubyte*              sharedSecret = 0;
    sbyte4              sharedSecretLen;

    if (OK > ( status = processClientEccKeyExchangeCore(pECCKey,
                                                        pMessage, recLen,
                                                        &sharedSecret,
                                                        &sharedSecretLen)))
    {
        goto exit;
    }

    /* generate key material */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, sharedSecret, sharedSecretLen);

exit:

    if ( sharedSecret)
    {
        FREE(sharedSecret);
    }

    return status;

} /* processClientEccKeyExchange */
#endif /* defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) ||
        defined( __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  ||
        defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)*/


/*------------------------------------------------------------------*/
#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__))

static MSTATUS
processClientEcdhKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage,
                         ubyte2 recLen, vlong **ppVlongQueue)
{
    /* the client key exchange contains the ECDH shared secret */
    if (akt_ecc != pSSLSock->roleSpecificInfo.server.privateKey.type)
    {
        return ERR_SSL_INVALID_KEY_TYPE;
    }

    return processClientEccKeyExchange( pSSLSock,
                pSSLSock->roleSpecificInfo.server.privateKey.key.pECC,
                pMessage, recLen);

} /* processClientEcdhKeyExchange */
#endif /* defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) */


/*------------------------------------------------------------------*/

#if (defined( __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) ||   \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)    )

static MSTATUS
processClientEcdheKeyExchange(SSLSocket* pSSLSock, ubyte *pMessage,
                              ubyte2 recLen, vlong **ppVlongQueue)
{
    if (akt_ecc != pSSLSock->ecdheKey.type)
    {
        return ERR_BAD_KEY_TYPE;
    }

    return processClientEccKeyExchange( pSSLSock,
                                        pSSLSock->ecdheKey.key.pECC,
                                        pMessage, recLen);
}

#endif /* defined( __ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)  ||
            defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)*/



/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__))
static MSTATUS
processClientEcdhePskKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage,
                                 ubyte2 recLen, vlong **ppVlongQueue)
{
    /* the client key exchange contains the encrypted premaster secret */
    ubyte*      pKeyMaterial        = 0;
    ubyte*      sharedSecret        = 0;
    sbyte4      sharedSecretLen;
    ubyte2      identityLengthPSK;
    ubyte*      pIdentityPSK;
    ubyte4      pskLength           = 0;
    MSTATUS     status;


    /* extract PSK identity */
    identityLengthPSK = getShortValue(pMessage);
    pIdentityPSK      = 2 + pMessage;

    if ((2 + identityLengthPSK) > recLen)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    /* skip past PSK identity */
    pMessage += (2 + identityLengthPSK);
    recLen    = (ubyte2)(recLen - (2 + identityLengthPSK));

    if (akt_ecc != pSSLSock->ecdheKey.type)
    {
        return ERR_BAD_KEY_TYPE;
    }

    /* compute premaster secret */
    if (OK > (status = processClientEccKeyExchangeCore(pSSLSock->ecdheKey.key.pECC,
                                                       pMessage, recLen,
                                                       &sharedSecret,
                                                       &sharedSecretLen)))
    {
        goto exit;
    }
    
    /* allocate buffer for key material */
    if (NULL == (pKeyMaterial = MALLOC(2 + sharedSecretLen + 2 + SSL_PSK_MAX_LENGTH)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* set length for secret */
    setShortValue(pKeyMaterial, sharedSecretLen);
    MOC_MEMCPY(pKeyMaterial+2, sharedSecret, sharedSecretLen);

    /* look up PSK */
    if (NULL != SSL_sslSettings()->funcPtrLookupPSK)
    {
        status = (MSTATUS)SSL_sslSettings()->funcPtrLookupPSK(SSL_findConnectionInstance(pSSLSock),
                                                              pIdentityPSK, identityLengthPSK,
                                                              pKeyMaterial+4+sharedSecretLen,
                                                              &pskLength);
    }

    if (SSL_PSK_MAX_LENGTH < pskLength)
    {
        status = ERR_BUFFER_OVERFLOW;
        goto exit;
    }

    /* set length for psk */
    setShortValue(pKeyMaterial+2+sharedSecretLen, (ubyte2)(pskLength));

    /* all checks passed -> generate key materials */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, pKeyMaterial,
                                          2 + sharedSecretLen + 2 + pskLength);

exit:
    if (NULL != pKeyMaterial)
        FREE(pKeyMaterial);

    if (NULL != sharedSecret)
        FREE(sharedSecret);

    return status;

} /* processClientEcdhePskKeyExchange */

#endif /* (defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__))
static MSTATUS
processClientRsaPskKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage,
                               ubyte2 recLen, vlong **ppVlongQueue)
{
    /* the client key exchange contains the encrypted premaster secret */
    ubyte*      pKeyMaterial = NULL;
    ubyte*      pKeyMaterialTemp;
    ubyte2      identityLengthPSK;
    ubyte*      pIdentityPSK;
    ubyte4      pskLength = 0;
    sbyte4      cipherTextLen;
    MSTATUS     status;

    if (OK > (status = RSA_getCipherTextLength(pSSLSock->roleSpecificInfo.server.privateKey.key.pRSA, &cipherTextLen)))
        goto exit;

    /* allocate buffer for key material */
    if (NULL == (pKeyMaterialTemp = pKeyMaterial = MALLOC((2048 / 8) + 2 + SSL_PSK_MAX_LENGTH)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* extract PSK identity */
    identityLengthPSK = getShortValue(pMessage);
    pIdentityPSK      = 2 + pMessage;

    if ((2 + identityLengthPSK) > recLen)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    /* skip past PSK identity */
    pMessage += (2 + identityLengthPSK);
    recLen    = (ubyte2)(recLen - (2 + identityLengthPSK));

    /* set length for secret */
    *pKeyMaterialTemp = 0;
    pKeyMaterialTemp++;
    *pKeyMaterialTemp = (ubyte)SSL_RSAPRESECRETSIZE;
    pKeyMaterialTemp++;

    /* decrypt premaster secret */
    if (OK > (status = processClientRsaKeyExchangeCore(pSSLSock, pMessage, recLen, pKeyMaterialTemp, ppVlongQueue)))
        goto exit;

    pKeyMaterialTemp += SSL_RSAPRESECRETSIZE;

    /* look up PSK */
    if (NULL != SSL_sslSettings()->funcPtrLookupPSK)
        status = (MSTATUS)SSL_sslSettings()->funcPtrLookupPSK(SSL_findConnectionInstance(pSSLSock), pIdentityPSK, identityLengthPSK, 2 + pKeyMaterialTemp, &pskLength);

    if (SSL_PSK_MAX_LENGTH < pskLength)
    {
        status = ERR_BUFFER_OVERFLOW;
        goto exit;
    }

    /* set length for psk */
    setShortValue(pKeyMaterialTemp, (ubyte2)(pskLength));    pKeyMaterialTemp += 2;

    /* all checks passed -> generate key materials */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, pKeyMaterial, 2 + SSL_RSAPRESECRETSIZE + 2 + pskLength);

exit:
    if (NULL != pKeyMaterial)
        FREE(pKeyMaterial);

    return status;

} /* processClientRsaPskKeyExchange */
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
static MSTATUS
processClientDiffieHellmanCore(SSLSocket* pSSLSock,
                               ubyte* pMessage, ubyte2 recLen,
                               ubyte** ppRetK, ubyte4 *pRetLenK,
                               vlong **ppVlongQueue)
{
    /* the client key exchange contains the encrypted premaster secret */
    ubyte*  pE = pMessage;
    ubyte4  lenE;
    ubyte*  pK = NULL;
    ubyte4  lenK;
    MSTATUS status;

    /* get length of Yc */
    lenE = getShortValue(pE);
    pE += 2;

    /* validate record length against length of Yc */
    if ((lenE + 2) != recLen)
    {
        status = ERR_BAD_CLIENT_E;
        goto exit;
    }

    /* extract Yc, store in e */
    if (OK > (status = VLONG_vlongFromByteString(pE, lenE, &(pSSLSock->pDHcontext->dh_e), ppVlongQueue)))
        goto exit;

    /* calculate shared secret */
    if (OK > (status = DH_computeKeyExchange(MOC_DH(pSSLSock->hwAccelCookie) pSSLSock->pDHcontext, ppVlongQueue)))
        goto exit;

    /* allocate buffer for shared secret */
    lenK  = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_k)) / 8;

    if (NULL == (pK = MALLOC(lenK)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* convert shared secret (k) into something digestible */
    if (OK > (status = VLONG_fixedByteStringFromVlong(pSSLSock->pDHcontext->dh_k, pK, lenK)))
        goto exit;

    *ppRetK   = pK;     pK = NULL;
    *pRetLenK = lenK;

exit:
    if (NULL != pK)
        FREE(pK);

    return status;

} /* processClientDiffieHellmanCore */

#endif /* (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
static MSTATUS
processClientDiffieHellmanKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage,
                                      ubyte2 recLen, vlong **ppVlongQueue)
{
    /* the client key exchange contains the encrypted premaster secret */
    ubyte*      pK = NULL;
    ubyte4      lenK = 0;
    MSTATUS     status;

    if (OK > (status = processClientDiffieHellmanCore(pSSLSock, pMessage, recLen, &pK, &lenK, ppVlongQueue)))
        goto exit;

    /* all checks passed -> generate key materials */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, pK, lenK);

exit:
    if (NULL != pK)
        FREE(pK);

    return status;

} /* processClientDiffieHellmanKeyExchange */

#endif /* (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__))
static MSTATUS
processClientDiffieHellmanPskKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage,
                                         ubyte2 recLen, vlong **ppVlongQueue)
{
    /* the client key exchange contains the encrypted premaster secret */
    ubyte*      pKeyMaterial        = NULL;
    ubyte*      pK                  = NULL;
    ubyte4      lenK;
    ubyte2      identityLengthPSK;
    ubyte*      pIdentityPSK;
    ubyte4      pskLength           = 0;
    MSTATUS     status;

    /* extract PSK identity */
    identityLengthPSK = getShortValue(pMessage);
    pIdentityPSK      = 2 + pMessage;

    if ((2 + identityLengthPSK) > recLen)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    /* skip past PSK identity */
    pMessage += (2 + identityLengthPSK);
    recLen    = (ubyte2)(recLen - (2 + identityLengthPSK));

    /* compute premaster secret */
    if (OK > (status = processClientDiffieHellmanCore(pSSLSock, pMessage,
                                                      recLen, &pK, &lenK,
                                                      ppVlongQueue)))
    {
        goto exit;
    }

    /* allocate buffer for key material */
    if (NULL == (pKeyMaterial = MALLOC(2 + lenK + 2 + SSL_PSK_MAX_LENGTH)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* set length for secret */
    setShortValue(pKeyMaterial, (ubyte2) lenK);
    /* dup pK */
    MOC_MEMCPY(pKeyMaterial+2, pK, lenK);

    /* look up PSK */
    if (NULL != SSL_sslSettings()->funcPtrLookupPSK)
    {
        status = (MSTATUS)SSL_sslSettings()->funcPtrLookupPSK(SSL_findConnectionInstance(pSSLSock),
                                                              pIdentityPSK,
                                                              identityLengthPSK,
                                                              pKeyMaterial + lenK + 4,
                                                              &pskLength);
    }
    if (SSL_PSK_MAX_LENGTH < pskLength)
    {
        status = ERR_BUFFER_OVERFLOW;
        goto exit;
    }

    /* set length for psk */
    setShortValue(pKeyMaterial+2+lenK, (ubyte2)(pskLength));
    /* all checks passed -> generate key materials */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, pKeyMaterial,
                                          2 + lenK + 2 + pskLength);

exit:
    if (NULL != pKeyMaterial)
        FREE(pKeyMaterial);

    if (NULL != pK)
        FREE(pK);

    return status;

} /* processClientDiffieHellmanPskKeyExchange */

#endif /* (defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__))
static MSTATUS
processClientPskKeyExchange(SSLSocket* pSSLSock, ubyte* pMessage,
                            ubyte2 recLen, vlong **ppVlongQueue)
{
    /* the client key exchange contains the encrypted premaster secret */
    ubyte*      pKeyMaterial        = NULL;
    ubyte*      pKeyMaterialTemp;
    ubyte2      identityLengthPSK;
    ubyte*      pIdentityPSK;
    ubyte4      pskLength           = 0;
    MSTATUS     status;
    MOC_UNUSED(ppVlongQueue);

    /* extract PSK identity */
    identityLengthPSK = getShortValue(pMessage);
    pIdentityPSK      = 2 + pMessage;

    if ((2 + identityLengthPSK) > recLen)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    /* skip past PSK identity */
    pMessage += (2 + identityLengthPSK);
    recLen    = (ubyte2)(recLen - (2 + identityLengthPSK));

    /* allocate buffer for key material */
    if (NULL == (pKeyMaterialTemp = pKeyMaterial = MALLOC(2 + SSL_PSK_MAX_LENGTH + 2 + SSL_PSK_MAX_LENGTH)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* look up PSK */
    if (NULL != SSL_sslSettings()->funcPtrLookupPSK)
        status = (MSTATUS)SSL_sslSettings()->funcPtrLookupPSK(SSL_findConnectionInstance(pSSLSock), pIdentityPSK, identityLengthPSK, 2 + pKeyMaterialTemp, &pskLength);

    if (SSL_PSK_MAX_LENGTH < pskLength)
    {
        status = ERR_BUFFER_OVERFLOW;
        goto exit;
    }

    /* set length for secret */
    setShortValue(pKeyMaterialTemp, (ubyte2)(pskLength));    pKeyMaterialTemp += 2;

    /* move psk */
    MOC_MEMCPY(2 + pskLength + 2 + pKeyMaterial, 2 + pKeyMaterial, pskLength);

    /* clear "other" key material */
    MOC_MEMSET(2 + pKeyMaterial, 0x00, pskLength);
    pKeyMaterialTemp += pskLength;

    /* set length for psk */
    setShortValue(pKeyMaterialTemp, (ubyte2)(pskLength));    pKeyMaterialTemp += 2;

    /* all checks passed -> generate key materials */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, pKeyMaterial, 2 + pskLength + 2 + pskLength);

exit:
    if (NULL != pKeyMaterial)
        FREE(pKeyMaterial);

    return status;

} /* processClientPskKeyExchange */

#endif /* (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)) */

/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__)
static MSTATUS
processClientCertificateVerifyRSA(const ubyte* pHashResult, ubyte4 hashLen,
                                  const ubyte* hashOID, SSLSocket *pSSLSock,
                                  ubyte* pSignature, ubyte4 signatureLen,
                                  vlong **ppVlongQueue)
{
    MSTATUS status;
    ubyte4  lenRsaN;
    ubyte*  pDecrypt = NULL;
    ubyte4  decryptLen;
    sbyte4  compareResult;
    ASN1_ITEMPTR pRootItem = 0;

    lenRsaN = ((7 + VLONG_bitLength(RSA_N(pSSLSock->mutualAuthKey.key.pRSA))) / 8);

    if (lenRsaN < signatureLen)
    {
        status = ERR_SSL_INVALID_CERT_VERIFY_MSG_SIZE;
        goto exit;
    }

    /* verify signature matches */
    if (NULL == (pDecrypt = MALLOC(lenRsaN)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    if (OK > (status = RSA_verifySignature(MOC_RSA(pSSLSock->hwAccelCookie)
                                                pSSLSock->mutualAuthKey.key.pRSA, pSignature,
                                                pDecrypt, &decryptLen, ppVlongQueue)))
    {
        goto exit;
    }

    if (hashOID) /* TLS 1.2 DER struct -> parse and verify the OID and hash matches */
    {
        MemFile mf;
        CStream cs;
        ASN1_ITEMPTR pOctetString;
        const ubyte* digest;

        WalkerStep digestInfoWalkInstructions[] =
        {
            { GoFirstChild, 0, 0},          /* 0: DigestInfo */
            { VerifyType, SEQUENCE, 0 },    /* 1: */
            { GoFirstChild, 0, 0},          /* 2: AlgorithmIdentifier */
            { VerifyType, SEQUENCE, 0 },    /* 3: */
            { GoFirstChild, 0,0 },          /* 4: OID */
            { VerifyOID, 0, 0 },            /* 5: */
            { GoParent, 0, 0 },             /* 6: AlgorithmIdentifier */
            { GoNextSibling, 0, 0},         /* 7: digest */
            { VerifyType, OCTETSTRING, 0},  /* 8: */
            { Complete, 0, 0}
        };

        /* place the hashOID in the WalkerStep s */
#ifdef __UCOS_DIRECT_RTOS__
	digestInfoWalkInstructions[5].extra2 = (ubyte*)hashOID;
#else
        digestInfoWalkInstructions[5].extra2 = (ubyte*)hashOID;
#endif /* __UCOS_DIRECT_RTOS__ */

        status = ERR_SSL_INVALID_SIGNATURE;

        MF_attach( &mf, decryptLen, pDecrypt);
        CS_AttachMemFile( &cs, &mf);

        if (OK > ASN1_Parse( cs, &pRootItem))
            goto exit;

        if (OK > ASN1_WalkTree( pRootItem, cs,
                                digestInfoWalkInstructions,
                                &pOctetString))
        {
            goto exit;
        }

        if (pOctetString->length != hashLen)
            goto exit;

        digest = CS_memaccess( cs, pOctetString->dataOffset, pOctetString->length);
        if (!digest)
            goto exit;

        MOC_MEMCMP( digest, pHashResult, hashLen, &compareResult);
        status = (0 == compareResult) ? OK : ERR_SSL_INVALID_SIGNATURE;
        CS_stopaccess( cs, digest);
    }
    else
    {
        if (hashLen != decryptLen)
            goto exit;

        MOC_MEMCMP(pDecrypt, pHashResult, hashLen, &compareResult);
        status = (0 == compareResult) ? OK : ERR_SSL_INVALID_SIGNATURE;
    }

exit:

    FREE(pDecrypt);
    TREE_DeleteTreeItem((TreeItem*) pRootItem);

    return status;
} /* processClientCertificateVerifyRSA */

#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ */




/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined(__ENABLE_MOCANA_ECC__)
static MSTATUS
processClientCertificateVerifyECC(const ubyte* pHashResult, ubyte4 hashLen,
                                  SSLSocket *pSSLSock,
                                  ubyte* pSignature, ubyte4 signatureLen,
                                  vlong **ppVlongQueue)
{
    MSTATUS status;
    MemFile mf;
    CStream cs;
    ASN1_ITEMPTR pRootItem = 0;

    MF_attach( &mf, signatureLen, pSignature);
    CS_AttachMemFile( &cs, &mf);

    if (OK > ( status = ASN1_Parse( cs, &pRootItem)))
        goto exit;

    /* key type should have been verified before calling this function*/
    if (OK > ( status = CERT_verifyECDSASignature( ASN1_FIRST_CHILD( pRootItem), cs,
                                                    pSSLSock->mutualAuthKey.key.pECC,
                                                    hashLen, pHashResult)))
    {
        goto exit;
    }

exit:

    TREE_DeleteTreeItem( (TreeItem*) pRootItem);

    return status;
} /* processClientCertificateVerifyECC */

#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ && __ENABLE_MOCANA_ECC__ */

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
static MSTATUS
processClientCertificateVerify(SSLSocket *pSSLSock, ubyte* pSHSH, ubyte2 recLen, vlong **ppVlongQueue)
{
    ubyte*  pParams;
    ubyte2  lenParams;
    ubyte4  sizeofHandshakeHeader;
    MSTATUS status;
    ubyte2  signatureAlgo=0;
    ubyte   *pHashResult = 0;
    ubyte4  hashLen = 0;
    ubyte   dsaOffset = 0; /* offset in pHashResult to use for ECDSA and DSA keys */
    const   ubyte* hashOID = 0; /* for TLS 1.2 */

    if (2 > recLen)
    {
        status = ERR_SSL_INVALID_CERT_VERIFY_MSG_SIZE;
        goto exit;
    }

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
    }
    else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
    }

    /* start of message */
    pParams = (ubyte *)(pSHSH + sizeofHandshakeHeader);

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        /* extract signature/hash algo */
        signatureAlgo = getShortValue(pParams);
        pParams += 2;
    }

    /* extract length of signature */
    lenParams = getShortValue(pParams);
    pParams += 2;

    if (((signatureAlgo != 0? 2: 0) + 2 + lenParams) != recLen)
    {
        status = ERR_SSL_INVALID_CERT_VERIFY_MSG_SIZE;
        goto exit;
    }

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult))))
        goto exit;

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        if (OK > (status = calculateTLS12CertificateVerifyHash(signatureAlgo, pSSLSock,
            pHashResult, &hashLen, &hashOID)))
        {
            goto exit;
        }
    }
    else
    {
        hashLen = MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE;
        dsaOffset = MD5_DIGESTSIZE; /* offset in pHashResult to use for ECDSA and DSA keys */
        if (OK > (status = calculateSSLTLSHashes(pSSLSock, 0, pHashResult, pSSLSock->sslMinorVersion)))
            goto exit;
    }


    switch ( pSSLSock->mutualAuthKey.type)
    {
    case akt_rsa:
        if ( OK > (status = processClientCertificateVerifyRSA( pHashResult, hashLen, hashOID,
                                                                pSSLSock, pParams, lenParams, ppVlongQueue)))
        {
            goto exit;
        }
        break;

#ifdef __ENABLE_MOCANA_ECC__
    case akt_ecc:
        if ( OK > (status = processClientCertificateVerifyECC( pHashResult + dsaOffset, hashLen - dsaOffset,
                                                                pSSLSock, pParams, lenParams, ppVlongQueue)))
        {
            goto exit;
        }
        break;
#endif

    default:
        status = ERR_SSL_INVALID_KEY_TYPE;
        goto exit;
        break;
    }

exit:

    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult));

    CRYPTO_uninitAsymmetricKey( &pSSLSock->mutualAuthKey, ppVlongQueue);

    return status;

} /* processClientCertificateVerify */

#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ */


/*------------------------------------------------------------------*/

static MSTATUS
handleServerHandshakeMessages(SSLSocket* pSSLSock)
{
    ubyte2  recordLen;
    sbyte*  pMsg        = pSSLSock->pReceiveBuffer;
    vlong*  pVlongQueue = NULL;
    ubyte4  sizeofHandshakeHeader;
    MSTATUS status      = OK;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
    }

    recordLen = (ubyte2)(pSSLSock->recordSize);

    while ((status >= OK) && (recordLen > 0))
    {
        ubyte2 handshakeRecLen;
        sbyte4 handshakeType;
        ubyte* pSHSH = (ubyte*)pMsg;

        if (sizeofHandshakeHeader > recordLen)
        {
            status = ERR_SSL_PROTOCOL_BAD_LENGTH;
            break;
        }

        /* advance pointers */
        pMsg += sizeofHandshakeHeader;
        recordLen -= sizeofHandshakeHeader;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
        if (pSSLSock->isDTLS)
        {
            handshakeRecLen = getMediumValue(((DTLSHandshakeHeader*)pSHSH)->handshakeSize);
            handshakeType = ((DTLSHandshakeHeader*)pSHSH)->handshakeType;
        } else
#endif
        {
            handshakeRecLen = getMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize);
            handshakeType = ((SSLHandshakeHeader*)pSHSH)->handshakeType;
        }

        /* check length */
        if ((ubyte2)handshakeRecLen > (ubyte2)recordLen)
        {
            status = ERR_SSL_PROTOCOL_BAD_LENGTH;
            break;
        }

        status = ERR_SSL_PROTOCOL_SERVER;

        switch (handshakeType)
        {
            case SSL_CERTIFICATE:
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
                if ((SSL_BEGIN == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) && (pSSLSock->isMutualAuthNegotiated))
                {
                    intBoolean isCertRequired = ((pSSLSock->runtimeFlags & SSL_FLAG_REQUIRE_MUTUAL_AUTH) ? TRUE : FALSE);

                    status = processCertificate(pSSLSock, pSHSH, handshakeRecLen, isCertRequired);

                    if (OK > status)
                        break;

                    addToHandshakeHash(pSSLSock,
                                       (ubyte *)pSHSH,
                                       handshakeRecLen + sizeofHandshakeHeader);

                    if (akt_undefined == pSSLSock->mutualAuthKey.type)
                    {
                        pSSLSock->isMutualAuthNegotiated = FALSE;
                    }
                }
                else
#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ */
                {
                    status = ERR_SSL_MUTUAL_AUTHENTICATION_NOT_REQUESTED;
                }
                break;

            case SSL_CLIENT_KEY_EXCHANGE:
                if (NULL == pSSLSock->pHandshakeCipherSuite)
                    break;

                if ((SSL3_MINORVERSION == pSSLSock->sslMinorVersion) &&
                    (SSL_BEGIN == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) &&
                    (pSSLSock->isMutualAuthNegotiated))
                {
                    pSSLSock->isMutualAuthNegotiated = FALSE;
                }

                /* mutual authentication required trumps all other combinations */
                if ((SSL_FLAG_REQUIRE_MUTUAL_AUTH == (pSSLSock->runtimeFlags & SSL_FLAG_REQUIRE_MUTUAL_AUTH)) &&
                    (SSL_NO_MUTUAL_AUTH_BIT != (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_NO_MUTUAL_AUTH_BIT)))
                {
                    pSSLSock->isMutualAuthNegotiated = TRUE;
                }

                if (((SSL_BEGIN       == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) && (!pSSLSock->isMutualAuthNegotiated)) ||
                     (SSL_CERTIFICATE == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) )
                {
                    ubyte*  pMessage = pSHSH + sizeofHandshakeHeader;
                    ubyte2  mesgLen  = handshakeRecLen;

                    if (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->ProcessClientKEX)
                    {
                        /* process kex & auth */
                        status = pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->ProcessClientKEX(pSSLSock, pMessage, mesgLen, &pVlongQueue);

                        if (OK > status)
                            break;
                    }

                    addToHandshakeHash(pSSLSock,
                                        (ubyte *)pSHSH,
                                        handshakeRecLen + sizeofHandshakeHeader);
                }
                else
                {
                    status = ERR_SSL_MUTUAL_AUTHENTICATION_REQUEST_IGNORED;
                }

                break;

            case SSL_CLIENT_CERTIFICATE_VERIFY:
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
                if ((SSL_CLIENT_KEY_EXCHANGE == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) && (pSSLSock->isMutualAuthNegotiated))
                {
                    status = processClientCertificateVerify(pSSLSock, pSHSH, handshakeRecLen, &pVlongQueue);

                    if (OK > status)
                        break;

                    addToHandshakeHash(pSSLSock,
                                       (ubyte *)pSHSH,
                                       handshakeRecLen + sizeofHandshakeHeader);

                    SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = SSL_CLIENT_CERTIFICATE_VERIFY;
                }
                else
#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ */
                {
                    status = ERR_SSL_MUTUAL_AUTHENTICATION_NOT_REQUESTED;
                }

                break;

            case SSL_FINISHED:
                if (SSL_EXPECTING_FINISHED == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock))
                {
                    status = processFinished(pSSLSock, pSHSH, handshakeRecLen);

                    if (OK > status)
                        break;

                    addToHandshakeHash(pSSLSock,
                                        (ubyte *)pSHSH,
                                        handshakeRecLen + sizeofHandshakeHeader);

                    SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = SSL_BEGIN;
                }
                else
                {
                    status = ERR_SSL_MUTUAL_AUTHENTICATION_REQUEST_IGNORED;
                }

                break;

            default:
                status = ERR_SSL_PROTOCOL_BAD_STATE;
                break;
        }

        /* if OK, advance state */
        if (status >= OK)
        {
            /* before advancing the state, we check if the routines called
             did not advance it already to a further state (client resuming session
             for example treats a SERVERHELLO as SERVERHELLODONE) */
            if (SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) < handshakeType)
            {
                SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = handshakeType;
            }

            if (recordLen < handshakeRecLen)
            {
                status = ERR_SSL_PROTOCOL_BAD_LENGTH;
                break;
            }

            /* advance pointers */
            pMsg += handshakeRecLen;
            recordLen = (ubyte2)(recordLen - handshakeRecLen);
        }
    }

    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"handleServerHandshakeMessages() returns status = ", status);

    VLONG_freeVlongQueue(&pVlongQueue);

    return status;

} /* handleServerHandshakeMessages */

/*------------------------------------------------------------------*/

static MSTATUS
SSL_SOCK_serverHandshakeSSL(SSLSocket* pSSLSock, intBoolean isWriter)
{
#if (defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__))
    sbyte4  doRehandshake       = FALSE;
#ifdef __ENABLE_MOCANA_SSL_NEW_HANDSHAKE__
    sbyte4  doSessionResumption = FALSE;
#endif
#endif
    MSTATUS status = OK;
    sbyte4  available = 0;

    if(!pSSLSock)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* handle any data pending on a send */
    if ((NULL != pSSLSock->pOutputBuffer) &&
        (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)) )
    {
        goto exit;
    }

    switch (SSL_HANDSHAKE_STATE(pSSLSock))
    {
#if (defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__))
        case kSslOpenState:                                         /* rehandshake support */
        {
            if (TRUE == isWriter)
                break;

            SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = SSL_BEGIN;

            doRehandshake = TRUE;

#ifdef __ENABLE_MOCANA_SSL_NEW_HANDSHAKE__
            if (NULL != SSL_sslSettings()->funcPtrNewHandshakeCallback)
                if (OK > (status = (MSTATUS)SSL_sslSettings()->funcPtrNewHandshakeCallback(SSL_findConnectionInstance(pSSLSock), &doRehandshake, &doSessionResumption)))
                    goto exit;
#endif

            if (FALSE == doRehandshake)
            {
                /* ignore rehandshake message */
                SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = SSL_BEGIN;
                SSL_HANDSHAKE_STATE(pSSLSock) = kSslOpenState;
                break;
            }

           pSSLSock->isMutualAuthNegotiated = FALSE;

            /* FALL-THROUGH */
        }
#endif /* __ENABLE_MOCANA_SSL_REHANDSHAKE__ */

        case kSslReceiveHelloInitState:
        {
            if (TRUE == isWriter)
                break;

            if (0 == pSSLSock->recordSize)                          /* do we have data to process? */
                break;

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveHelloState;

            if (OK > (status = sslHelloStateMachine(pSSLSock)))     /* receive here */
                goto exit;

            pSSLSock->recordSize = 0;

            if (OK > (status = SSL_SERVER_sendServerHello(pSSLSock)))
                goto exit;

            if (E_NoSessionResume != pSSLSock->sessionResume)
            {
#if defined( __ENABLE_MOCANA_EAP_FAST__)
                if ( E_SessionEAPFASTResume == pSSLSock->sessionResume)
                {
                    /* need to generate master secret from PAC Key now
                    that we have the client hello, server hello and PAC key */
                    if (OK > (status = SSL_SOCK_generateEAPFASTMasterSecret(pSSLSock)))
                        goto exit;
                }
#endif
                SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveHelloState1;
            }
            else
            {
                /* session resumed */
                SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntil;
                break;
            }

            /* check if data is pending, if so break */
            if ((NULL != pSSLSock->pOutputBuffer) && (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
                break;

            /* FALL-THROUGH */;
        }

        case kSslReceiveHelloState1:
        {
            /* generate the key material here (presecret is unused and NULL) */
            if (OK > (status = SSL_SOCK_generateKeyMaterial(pSSLSock, NULL, 0)))
                goto exit;

            if (OK > (status = sendChangeCipherSpec(pSSLSock)))
                goto exit;

            if (OK > (status = SSL_SOCK_setServerKeyMaterial(pSSLSock)))
                goto exit;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
            pSSLSock->retransCipherInfo.pOldCipherSuite = pSSLSock->pActiveOwnCipherSuite;
#endif
            pSSLSock->pActiveOwnCipherSuite = pSSLSock->pHandshakeCipherSuite;

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveHelloState2;

            /* check if data is pending, if so break */
            if ((NULL != pSSLSock->pOutputBuffer) && (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
                break;

            /* FALL-THROUGH */;
        }

        case kSslReceiveHelloState2:
        {
            if (OK > (status = sendFinished(pSSLSock)))
                goto exit;

            /* handshake state is now like we received the CLIENT_KEY_EXCHANGE already*/
            SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)  = SSL_CLIENT_KEY_EXCHANGE;
            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntil;

            /* FALL-THROUGH */
        }

        case kSslReceiveUntil:
        {
            if (TRUE == isWriter)
                break;

            if (OK > (status = handleServerHandshakeMessages(pSSLSock)))   /* receive here */
                goto exit;

            status = (MSTATUS)(pSSLSock->numBytesToSend);

            if (SSL_FINISHED != SSL_REMOTE_HANDSHAKE_STATE(pSSLSock))
                break;

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntil1;

            /* FALL-THROUGH */
        }

        case kSslReceiveUntil1:
        {
            /* send ChangeCipherSpec and Finished only if session was not
            resumed; they were sent before otherwise */
            if (E_NoSessionResume != pSSLSock->sessionResume)
            {
                SSL_OPEN_STATE(pSSLSock) = kSslSecureSessionEstablished;
                SSL_HANDSHAKE_STATE(pSSLSock) = kSslOpenState;

                if (OK > (status = SSL_INTERNAL_setConnectionState(SSL_findConnectionInstance(pSSLSock), CONNECT_OPEN)))
                    goto exit;

                if (OK > (status = SSLSOCK_doOpenUpcalls(pSSLSock)))
                    goto exit;

                break;
            }

            if (OK > (status = sendChangeCipherSpec(pSSLSock)))
                goto exit;

            if (OK > (status = SSL_SOCK_setServerKeyMaterial(pSSLSock)))
                goto exit;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
            pSSLSock->retransCipherInfo.pOldCipherSuite = pSSLSock->pActiveOwnCipherSuite;
#endif
            pSSLSock->pActiveOwnCipherSuite = pSSLSock->pHandshakeCipherSuite;

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntil2;

            /* check if data is pending, if so break */
            if ((NULL != pSSLSock->pOutputBuffer) && (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
                break;

            /* FALL-THROUGH */
        }

        case kSslReceiveUntil2:
        {
            /* send ChangeCipherSpec and Finished only if session was not
            resumed; they were sent before otherwise */
            if (OK > (status = sendFinished(pSSLSock)))
                goto exit;
            /* For EAP */
            available = status;

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntil3;

            /* check if data is pending, if so break */
            if ((NULL != pSSLSock->pOutputBuffer) && (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
                break;

            /* FALL-THROUGH */
        }

        case kSslReceiveUntil3:
        {
            SSL_OPEN_STATE(pSSLSock) = kSslSecureSessionEstablished;
            SSL_HANDSHAKE_STATE(pSSLSock) = kSslOpenState;

            if (OK > (status = SSL_INTERNAL_setConnectionState(SSL_findConnectionInstance(pSSLSock), CONNECT_OPEN)))
                goto exit;

            status = SSLSOCK_doOpenUpcalls(pSSLSock);

            break;
        }

        default:
        {
            status = ERR_SSL_BAD_STATE;
            break;
        }
    }

exit:
    if (OK > status)
    {
        if ((pSSLSock) && (pSSLSock->sessionResume == E_SessionIDResume))
           SSLSOCK_clearServerSessionCache(pSSLSock);

        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"SSL_SOCK_serverHandshake() returns status = ", status);
    }

    if (available)
        status = (MSTATUS)available;

    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_serverHandshake(SSLSocket* pSSLSock, intBoolean isWriter)
{
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        return  SSL_SOCK_serverHandshakeDTLS(pSSLSock, isWriter);
    } else
#endif
    {
        return  SSL_SOCK_serverHandshakeSSL(pSSLSock, isWriter);
    }
}

/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_initServerEngine(RNGFun rngFun, void* rngFunArg)
{
    sbyte4  cacheIndex;
    MSTATUS status = OK;
	rngFun(rngFunArg, sizeof(SESSIONID), (ubyte*)&gNextSessionId);
    /* scramble initial session secrets */
    for (cacheIndex = 0; cacheIndex < SESSION_CACHE_SIZE; cacheIndex++)
    {
        sbyte4 index;

        /* make sure they are not reused -- this can happen if the same app
            is bringing down and up the TLS stack repeatedly */
        gSessionCache[cacheIndex].m_sessionId = 0;
        if (OK > (status = rngFun(rngFunArg, SSL_MASTERSECRETSIZE, gSessionCache[cacheIndex].m_masterSecret)))
            goto exit;

        for (index = 0; index < SSL_MASTERSECRETSIZE; index++)
        {
            gSessionCache[cacheIndex].m_masterSecret[index] ^= 0x5c;
            gSessionCache[cacheIndex].m_masterSecret[index] += 0x36;
        }
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_initSocketExtraServer(SSLSocket* pSSLSock)
{
    resetCipher(pSSLSock, TRUE, TRUE);

    SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveHelloInitState;
#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        DTLS_TIMER_STATE(pSSLSock) = kDtlsWaiting;
    }
#endif
    SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = SSL_BEGIN;
    pSSLSock->server = 1;

#ifdef __ENABLE_RFC3456__
    pSSLSock->roleSpecificInfo.server.useSessionTicket = 0;
#endif

    return OK;
}


/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_setServerCert( SSLSocket* pSSLSock, sbyte4 numCertificates,
                            SizedBuffer* certificates,
                            AsymmetricKey* privateKey,
                            ubyte4 certECCurves)
{
    MSTATUS status = OK;
    ubyte*  signatureAlgoList = NULL;
    ubyte4  signatureAlgoListLength = 0;
    ubyte*  pLeafCert = NULL;
    ubyte4  leafCertLen = 0;
    ubyte4  certSignatureAlgo = 0;

#ifdef __ENABLE_RFC3546__
    signatureAlgoList       = pSSLSock->signatureAlgoList;
    signatureAlgoListLength = pSSLSock->signatureAlgoListLength;
#endif

    /* This function can be called several times -- make sure to clean up */
    if (pSSLSock->roleSpecificInfo.server.isDynamicAlloc)
    {
        CRYPTO_uninitAsymmetricKey(&pSSLSock->roleSpecificInfo.server.privateKey, NULL);
    }

    pSSLSock->roleSpecificInfo.server.isDynamicAlloc = FALSE;

    if (OK > (status = CRYPTO_initAsymmetricKey(&pSSLSock->roleSpecificInfo.server.privateKey)))
        return status;

    if (privateKey && (akt_undefined != privateKey->type) && (NULL != certificates->data))
    {
        CRYPTO_copyAsymmetricKey(&pSSLSock->roleSpecificInfo.server.privateKey, privateKey);
        pSSLSock->roleSpecificInfo.server.isDynamicAlloc = TRUE;

        pSSLSock->roleSpecificInfo.server.certificates = certificates;
        pSSLSock->roleSpecificInfo.server.numCertificates = numCertificates;
        pSSLSock->roleSpecificInfo.server.certECCurves = certECCurves;

        /* save leaf certificate for later use */
        leafCertLen = pSSLSock->roleSpecificInfo.server.certificates[0].length;
        pLeafCert = pSSLSock->roleSpecificInfo.server.certificates[0].data;

        if ( (NULL != pSSLSock->pHandshakeCipherSuite) &&
             (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo) )
        {
            /* verify the certificate we got can be used by the client (EC curves overlap) */
            /* if the pCipherSuite uses any ECC then certECCurves must be ! 0 and
               certECCurves must be in clientECCurves */
            if ( pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags &
                 (SSL_KEYEX_ECDH_BIT | SSL_KEYEX_ECDHE_BIT | SSL_AUTH_ECDSA_BIT) )
            {
                if ( (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & (SSL_KEYEX_ECDHE_BIT)) &&
                     (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & (SSL_AUTH_RSA_BIT)) )
                {
                    /* if using an ephemeral key with an RSA certificate, check if the client supports at
                       least one of our curves*/
                    if (0 == (pSSLSock->roleSpecificInfo.server.clientECCurves & SUPPORTED_CURVES_FLAGS))
                    {
                        status = ERR_SSL_UNSUPPORTED_CURVE;
                        goto exit;
                    }

                    /* must use RSA key */
                    if ( akt_rsa != pSSLSock->roleSpecificInfo.server.privateKey.type)
                    {
                        status = ERR_SSL_UNSUPPORTED_ALGORITHM;
                        goto exit;
                    }
                }
                else /* ECDH or ECDSA certificate -> client can used certificate curves */
                {
                    if (0 == (pSSLSock->roleSpecificInfo.server.certECCurves & pSSLSock->roleSpecificInfo.server.clientECCurves))
                    {
                        status = ERR_SSL_UNSUPPORTED_CURVE;
                        goto exit;
                    }

                    /* must use ECC key */
                    if ( akt_ecc != pSSLSock->roleSpecificInfo.server.privateKey.type)
                    {
                        status = ERR_SSL_UNSUPPORTED_ALGORITHM;
                        goto exit;
                    }

#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
                    if ( (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & (SSL_KEYEX_ECDH_BIT)) &&
                         (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & (SSL_AUTH_RSA_BIT)) )
                    {
                        /* get leaf certificate signature algorithm */
                        if (OK > (status = getCertSigAlgo(pLeafCert, leafCertLen, &certSignatureAlgo)))
                            goto exit;

                        /* RFC 4492
                         * ECDH_RSA                Certificate MUST contain an
                         *                         ECDH-capable public key.  It
                         *                         MUST be signed with RSA.
                         */
                        if (TLS_RSA != (certSignatureAlgo & 0xff))
                        {
                            status = ERR_SSL_UNSUPPORTED_ALGORITHM;
                            goto exit;
                        }
                    }
#endif
                }
            }
            else if ( pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags &
                      (SSL_KEYEX_RSA_BIT | SSL_AUTH_RSA_BIT) )
            {
                /* must use RSA key */
                if ( akt_rsa != pSSLSock->roleSpecificInfo.server.privateKey.type)
                {
                    status = ERR_SSL_UNSUPPORTED_ALGORITHM;
                    goto exit;
                }
            }
        }
    }
#ifdef __ENABLE_SSL_DYNAMIC_CERTIFICATE__
    else if ((NULL != pSSLSock->pCertStore) &&
             (NULL != pSSLSock->pHandshakeCipherSuite) &&
             (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo) &&
             (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & (SSL_AUTH_RSA_BIT | SSL_AUTH_ECDSA_BIT)))
    {
        ubyte4 pubKeyType;
        ubyte4 certStoreAlgoFlags = 0;
        AsymmetricKey *pPrivateKey;

        /* initialize in case of failure */
        pSSLSock->roleSpecificInfo.server.certificates = NULL;
        pSSLSock->roleSpecificInfo.server.numCertificates = 0;

        /* get public keyType */
        if ((pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ECDSA_BIT) ||
            (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_ECDH_BIT))
        {
            pubKeyType = akt_ecc;
        }
        else
        {
            pubKeyType = akt_rsa;
        }

        /* convert client supported eccurves, sig algos into CERT_STORE_ALGO_FLAGS */
        convertToCertStoreFlags(pSSLSock, (pSSLSock->roleSpecificInfo.server.clientECCurves & SUPPORTED_CURVES_FLAGS),
            signatureAlgoList, signatureAlgoListLength, &certStoreAlgoFlags);

        status = CERT_STORE_findIdentityCertChainFirst( pSSLSock->pCertStore,
                                                        pubKeyType, certStoreAlgoFlags,
                                                        &pPrivateKey,
                                                        &pSSLSock->roleSpecificInfo.server.certificates,
                                                        (ubyte4*) &pSSLSock->roleSpecificInfo.server.numCertificates,
                                                        NULL );

        if (0 >= pSSLSock->roleSpecificInfo.server.numCertificates)
        {
            status = ERR_SSL_UNSUPPORTED_ALGORITHM; /* no usable cert found */
            goto exit;
        }

        /* save leaf certificate for later use */
        leafCertLen = pSSLSock->roleSpecificInfo.server.certificates[0].length;
        pLeafCert = pSSLSock->roleSpecificInfo.server.certificates[0].data;

        CRYPTO_copyAsymmetricKey(&pSSLSock->roleSpecificInfo.server.privateKey, pPrivateKey);

        pSSLSock->roleSpecificInfo.server.isDynamicAlloc = TRUE;
    }
#endif

    if (((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
         (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION)) &&
         (NULL != pSSLSock->pHandshakeCipherSuite) &&
         (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo) &&
         (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & (SSL_AUTH_RSA_BIT | SSL_AUTH_ECDSA_BIT)) )
    {
#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
        /* get leaf certificate signature algorithm */
        if ( (0 == certSignatureAlgo) &&
             (OK > (status = getCertSigAlgo(pLeafCert, leafCertLen, &certSignatureAlgo))) )
        {
            goto exit;
        }
#endif

        status = ERR_SSL_UNSUPPORTED_ALGORITHM;

		/* If this is a resumed session, check the signature algorithm retrieved from cache */
		if (pSSLSock->signatureAlgo == certSignatureAlgo)
		{
			status = OK;
		}
        else if (0 < signatureAlgoListLength)
        {
			/* pick a hash/sig pair from signature Algo extension */
            ubyte4 i;
            ubyte2 clientSigAlgo;

            for (i = 0; i < signatureAlgoListLength; i += 2)
            {
                clientSigAlgo = signatureAlgoList[i] << 8 | signatureAlgoList[i+1];

                if (clientSigAlgo == certSignatureAlgo)
                {
                    status = OK;
                    break;
                }
            }
        }
        else
        {
            /*
             * If the client does not send the signature_algorithms extension,
             * assume TLS_SHA1 as the default hash algorithm.
             *
             * For detail, see RFC 5246 section 7.4.1.4.1.
             */
            if ( pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ECDSA_BIT )
            {
                if ((TLS_SHA1 << 8 | TLS_ECDSA) == certSignatureAlgo)
                {
                    status = OK;
                }
            }
            else
            {
                if ((TLS_SHA1 << 8 | TLS_RSA) == certSignatureAlgo)
                {
                    status = OK;
                }
            }
        }

        if (OK == status)
            pSSLSock->signatureAlgo = certSignatureAlgo;
    }

exit:
    return status;
}


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_RFC3546__))
extern MSTATUS
SSL_SOCK_setCipherById( SSLSocket* pSSLSock, ubyte2 cipherId)
{
    sbyte4 cipherIndex;

    if ( !pSSLSock)
        return ERR_NULL_POINTER;

    /* retrieve the cipher suite */
    cipherIndex = SSL_SOCK_getCipherTableIndex(pSSLSock, cipherId);

    if (cipherIndex < 0)
        return ERR_FALSE; /* not a supported cipher */

    pSSLSock->pHandshakeCipherSuite = gCipherSuites + cipherIndex;
    return OK;
}
#endif /* (defined(__ENABLE_RFC3546__)) */


/*------------------------------------------------------------------*/

#if (defined( __ENABLE_MOCANA_SSL_REHANDSHAKE__))
extern MSTATUS
SSL_SOCK_sendServerHelloRequest(SSLSocket* pSSLSock)
{
    ubyte*              pHelloRequest = NULL;
    ubyte*              pSHSH;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status = OK;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pHelloRequest))))
        goto exit;

    pSHSH = pHelloRequest;

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_HELLO_REQUEST;
        setMediumValue(((DTLSHandshakeHeader*)pSHSH)->handshakeSize, 0);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader*)pSHSH), 0, 0);
        MOC_MEMCPY(pSSLSock->HSHBytes, pSHSH, sizeofHandshakeHeader);
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_HELLO_REQUEST;
        setMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize, 0);
    }

#ifdef __ENABLE_MOCANA_DTLS_SERVER__
    if (pSSLSock->isDTLS)
    {
        releaseRetransmissionBuffer(pSSLSock);
        resetRetransmissionSessionInfo(pSSLSock);
    }
#endif

    status = sendData(pSSLSock, SSL_HANDSHAKE, (sbyte *)pHelloRequest, sizeofHandshakeHeader, TRUE);

exit:
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pHelloRequest));

    return status;
}
#endif /* __ENABLE_MOCANA_SSL_REHANDSHAKE__ */
#endif
