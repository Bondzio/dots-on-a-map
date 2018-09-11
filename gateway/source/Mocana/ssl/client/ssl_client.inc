/*
 * ssl_client.inc
 *
 * SSL Client Specific Functionality
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */
#ifndef __SSL_CLIENT_INC__
#define __SSL_CLIENT_INC__

static MSTATUS SSL_SOCK_clientHandshakeSSL(SSLSocket* pSSLSock, intBoolean isWriter);
static MSTATUS SSL_CLIENT_sendClientResponseBlock(SSLSocket* pSSLSock);
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
static ubyte4 calculateExtraFragmentHeader(SSLSocket *pSSLSock, ubyte4 numBufs);
static MSTATUS fragmentHandshakeMessages(SSLSocket *pSSLSock, ubyte4 fragmentLen);
#include "../../dtls/client/dtls_client.inc"
#endif
#ifdef __ENABLE_TLSEXT_RFC6066__
extern MSTATUS SSL_OCSP_validateOcspResponse(SSLSocket * pSSLSock,ubyte * pResponse,ubyte4 responseLen);
//static MSTATUS processCertificateStatus(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen);
#endif

/*------------------------------------------------------------------*/

static ubyte2
SSL_CLIENT_numCipherSuites(SSLSocket* pSSLSock, ubyte4* flags)
{
    ubyte2 count = 0;
    ubyte2 i;
    ubyte4 algoFlags = 0;
#ifndef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
    MOC_UNUSED(pSSLSock);
#endif

    for (i = 0; i < NUM_CIPHER_SUITES; i++)
    {
        if (gCipherSuites[i].supported)
        {
#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
            if ((TRUE != pSSLSock->isCipherTableInit) ||
                (TRUE == pSSLSock->isCipherEnabled[i]) )
#endif
            {
#ifdef __ENABLE_MOCANA_CCM_8__
                /* Make sure this cipher is advertised only for TLS 1.2 and above */
                /* FIXME: ff 05/20/15 there's a function that does that already */
                if ((0xC0AE == gCipherSuites[i].cipherSuiteId) &&
                    (pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
                    continue;
#endif
                algoFlags |= gCipherSuites[i].pKeyExAuthAlgo->flags;
                count++;
            }
        }
    }

    *flags = algoFlags;

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if ((SSL3_MINORVERSION == pSSLSock->sslMinorVersion) &&
        (0 == pSSLSock->handshakeCount) )
    {
        /* TLS_EMPTY_RENEGOTIATION_INFO_SCSV */
        /* send only during initial SSL 3.0 hello */
        count++;
    }
#endif /* ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__))) */
#endif /* MIN_SSL_MINORVERSION <= SSL3_MINORVERSION */
	if(pSSLSock->runtimeFlags & SSL_FLAG_SCSV_FALLBACK_VERSION_SET)
	{
        /* TLS_FALLBACK_SCSV */
        /* send only if client is trying to connect with fallback version */
        count++;
	}
    return count;
}

/*------------------------------------------------------------------*/
#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) && defined(__ENABLE_MOCANA_DTLS_SRTP__))

static ubyte2
numSrtpProfiles(SSLSocket* pSSLSock)
{
    ubyte2 count = 0;
    ubyte2 i;
#ifndef __ENABLE_MOCANA_SRTP_PROFILES_SELECT__
    MOC_UNUSED(pSSLSock);
#endif

    for (i = 0; i < NUM_SRTP_PROFILES; i++)
    {
        if (gSrtpProfiles[i].supported)
        {
#ifdef __ENABLE_MOCANA_SRTP_PROFILES_SELECT__
            if ((TRUE != pSSLSock->isSrtpProfileTableInit) ||
                (TRUE == pSSLSock->isSrtpProfileEnabled[i]) )
#endif
            {
                count++;
            }
        }
    }

    return count;
}
#endif

/*------------------------------------------------------------------*/

/*********************************************************************************
*  SSL_CLIENT_sendClientHello
*   see page 77 of SSL and TLS essentials
*/
/* DTLS notes: because server don't keep state for unverified (cookie) client hello.
 * client hello can not fragment
 */
static MSTATUS
SSL_CLIENT_sendClientHello(SSLSocket* pSSLSock)
{
    intBoolean          isRehandshake = (pSSLSock->pActiveOwnCipherSuite) ? TRUE : FALSE;
    ubyte*              pSRH;
    ubyte*              pHSH;
    ubyte*              pTemp;
    sbyte4              i;
    ubyte4              numBytesSent = 0;
    ubyte4              extensionsLength = 0;
    ubyte4              fragmentLen = 0;
    ubyte4              sizeofRecordHeader;
    ubyte4              sizeofHandshakeHeader;
    ubyte4              sizeofHandshakeRecord;
    ubyte4              cipherFlags, eccCurves;
    ubyte2              numCiphers, numECCurves;
    MSTATUS             status = OK;

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
    eccCurves = pSSLSock->eccCurves;
#else
    eccCurves = SUPPORTED_CURVES_FLAGS;
#endif

    numECCurves = (ubyte2) MOC_BITCOUNT( eccCurves);
    (void)numECCurves;

#if defined(__ENABLE_MOCANA_DTLS_CLIENT__)
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

    pSSLSock->buffers[0].pHeader = NULL;

    /* minor version initialization: if MinorVer is set by SSL_ioctl, use it; else default to max supported */
    pSSLSock->sslMinorVersion = ((pSSLSock->runtimeFlags & SSL_FLAG_VERSION_SET)?
                                 pSSLSock->advertisedMinorVersion : MAX_SSL_MINORVERSION);

    /* generate client random in its usual place */
    /* don't re-generate random when received server HelloVerifyRequest */
    if (kSslReceiveHelloInitState == SSL_HANDSHAKE_STATE(pSSLSock))
    {
        pSSLSock->rngFun(pSSLSock->rngFunArg, SSL_RANDOMSIZE, START_RANDOM(pSSLSock));
    }

    numCiphers = SSL_CLIENT_numCipherSuites( pSSLSock, &cipherFlags);

#if (defined(__ENABLE_RFC3546__) || defined(__ENABLE_TLSEXT_RFC6066__))

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    /* to interoperate with old server, SSL 3.0 should only
       send renegotiation extension during rehandshake */
    if ( SSL3_MINORVERSION < pSSLSock->sslMinorVersion ||
        (SSL3_MINORVERSION == pSSLSock->sslMinorVersion && pSSLSock->isRehandshakeAllowed) )
    {
        extensionsLength += (2 + 2 + 1); /* extension type + extension length + renegotiated_connection length */

        if (0 < pSSLSock->handshakeCount)
        {
#if MIN_SSL_MINORVERSION <= SSLV3_MINOR_VERSION
            ubyte4  verifyDataSize = (0 == pSSLSock->sslMinorVersion) ? SSL_VERIFY_DATA : TLS_VERIFYDATASIZE;
            /* on rehandshakes, use verify data string from the previous handshake */
            extensionsLength += verifyDataSize;
#else
            extensionsLength += TLS_VERIFYDATASIZE;
#endif
        }
    }
#endif /* ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__))) */


#if MIN_SSL_MINORVERSION <= SSLV3_MINOR_VERSION
    if ( SSL3_MINORVERSION < pSSLSock->sslMinorVersion)
#endif
    {
        /* server name extension */
        if (pSSLSock->serverNameListLength)
        {
            /* TLS 1.0+ feature */
            extensionsLength += 4; /* extension type + extension length*/
            extensionsLength += 2; /* server name list length */
            extensionsLength += pSSLSock->serverNameListLength; /* see SSL_setServerNameList */
        }

        if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
            (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
        {
            /* supported signature algorithms extension */
            extensionsLength += 4; /* extension type + extension length */
            extensionsLength += 2;
            extensionsLength += NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS * 2;
        }

#ifdef __ENABLE_TLSEXT_RFC6066__
        if (pSSLSock->certStatusReqExt)
        {
            /* TLS 1.0+ feature */
                extensionsLength += 4; /* extension type + extension length*/
                extensionsLength += pSSLSock->roleSpecificInfo.client.certStatusReqExtLen;
        }
#endif

#ifdef __ENABLE_MOCANA_EAP_FAST__
        /* only if we have a ticket extension */
        if ( pSSLSock->roleSpecificInfo.client.ticket &&
            pSSLSock->roleSpecificInfo.client.ticketLength)
        {
            extensionsLength += 2; /* extension type */
#ifndef __DISABLE_EAP_FAST_EXTENSION_LENGTH__
            extensionsLength += 2; /* extension length */
#endif
            extensionsLength += 2; /* ticket length */

            extensionsLength += pSSLSock->roleSpecificInfo.client.ticketLength;
        }
#endif

#ifdef __ENABLE_MOCANA_INNER_APP__
        /* only if we have a ticket extension */
        if ( pSSLSock->roleSpecificInfo.client.innerApp )
        {
            extensionsLength += 2; /* extension type */
            extensionsLength += 2; /* App length */
            extensionsLength += 2; /* App  Value */
        }
#endif

#if (defined(__ENABLE_MOCANA_ECC__))
        /* add ECC related extensions only if the cipher suites include EC based ones */
        if ( cipherFlags & (SSL_KEYEX_ECDH_BIT | SSL_KEYEX_ECDHE_BIT))
        {
            /* supported elliptic curve extension */
            extensionsLength += 4; /* extension type + extension length */
            extensionsLength += 2; /* size of curve list */
            extensionsLength += numECCurves * 2;
            /* supported point compression -- we don't -> always the same data */
            extensionsLength += 6; /* everything */
        }
#endif

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) && defined(__ENABLE_MOCANA_DTLS_SRTP__))
        /* use_srtp extension */
        if (pSSLSock->isDTLS && pSSLSock->useSrtp)
        {
            ubyte4 profileCount = numSrtpProfiles(pSSLSock);

            /* dtls_srtp feature */
            extensionsLength += 4; /* extension type + extension length*/
            extensionsLength += 2; /* SRTP protection profile list length */
            extensionsLength += 2*profileCount;
            extensionsLength += 1 + ((NULL != pSSLSock->srtpMki) ? *(pSSLSock->srtpMki) : 0); /* srtp_mki is variable length */
        }
#endif
    }

    if ( extensionsLength) /* do we have any extensions? */
    {
        extensionsLength += 2; /* length of extensions */
    }

#endif /* (defined(__ENABLE_RFC3546__) || defined(__ENABLE_TLSEXT_RFC6066__)) */

    /* compute size of buffers */
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        pSSLSock->buffers[0].length = (ubyte2)(sizeofRecordHeader + sizeofHandshakeHeader + 2 + SSL_RANDOMSIZE +
                                      1 + pSSLSock->roleSpecificInfo.client.sessionIdLen +
                                      1 + pSSLSock->helloCookieLen +
                                      2 + (2 * numCiphers) + 2 + extensionsLength);
    } else
#endif
    {
        pSSLSock->buffers[0].length = (ubyte2)(sizeofRecordHeader + sizeofHandshakeHeader + 2 + SSL_RANDOMSIZE +
                                      1 + pSSLSock->roleSpecificInfo.client.sessionIdLen +
                                      2 + (2 * numCiphers) + 2 + extensionsLength);
    }

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        fragmentLen = calculateExtraFragmentHeader(pSSLSock, 1);
    }
#endif

    if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie, fragmentLen + pSSLSock->buffers[0].length, TRUE, (void **)&(pSSLSock->buffers[0].pHeader))))
        goto exit;

    pSSLSock->buffers[0].data = pSSLSock->buffers[0].pHeader + sizeofRecordHeader;

    /* fill buffers */
    pSRH = pSSLSock->buffers[0].pHeader;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        DTLS_SET_RECORD_HEADER_EXT(pSRH,pSSLSock,SSL_HANDSHAKE,pSSLSock->buffers[0].length - sizeofRecordHeader);
        pHSH = pSSLSock->buffers[0].data;
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pHSH), pSSLSock->nextSendSeq++, (ubyte2)(pSSLSock->buffers[0].length - sizeofHandshakeRecord));
        setMediumValue(((DTLSHandshakeHeader *)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[0].length - sizeofHandshakeRecord));
        ((DTLSHandshakeHeader *)pHSH)->handshakeType = SSL_CLIENT_HELLO;
    } else
#endif
    {
        SSL_SET_RECORD_HEADER(pSRH,SSL_HANDSHAKE,
                              (pSSLSock->sslMinorVersion == TLS12_MINORVERSION) ? 1 : pSSLSock->sslMinorVersion,
                              pSSLSock->buffers[0].length - sizeofRecordHeader);
        pHSH = pSSLSock->buffers[0].data;
        setMediumValue(((SSLHandshakeHeader *)pHSH)->handshakeSize, (ubyte2)(pSSLSock->buffers[0].length - sizeofHandshakeRecord));
        ((SSLHandshakeHeader *)pHSH)->handshakeType = SSL_CLIENT_HELLO;
    }

    pTemp = (ubyte *)(pHSH + sizeofHandshakeHeader);
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        *pTemp++ = DTLS1_MAJORVERSION;
    } else
#endif
    {
        *pTemp++ = SSL3_MAJORVERSION;
    }
    *pTemp++ = pSSLSock->clientHelloMinorVersion = pSSLSock->sslMinorVersion;/* note the version we sent -- used for RSA later on */

    /* client random */
    MOC_MEMCPY(pTemp, START_RANDOM(pSSLSock), SSL_RANDOMSIZE);
    MOC_MEMCPY(pSSLSock->pClientRandHello, START_RANDOM(pSSLSock), SSL_RANDOMSIZE);
    pTemp += SSL_RANDOMSIZE;

    /* session id */
    *pTemp++ = pSSLSock->roleSpecificInfo.client.sessionIdLen;
    if (pSSLSock->roleSpecificInfo.client.sessionIdLen > 0)
    {
        MOC_MEMCPY(pTemp, pSSLSock->roleSpecificInfo.client.sessionId, pSSLSock->roleSpecificInfo.client.sessionIdLen);
        pTemp += pSSLSock->roleSpecificInfo.client.sessionIdLen;
    }

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    /* DTLS cookie */
    if (pSSLSock->isDTLS)
    {
        *pTemp++ = pSSLSock->helloCookieLen;
        if (pSSLSock->helloCookieLen > 0)
        {
            MOC_MEMCPY(pTemp, pSSLSock->helloCookie, pSSLSock->helloCookieLen);
            pTemp += pSSLSock->helloCookieLen;
        }
    }
#endif

    /* cipher suite */
    setShortValue(pTemp, (ubyte2)(2 * numCiphers));
    pTemp += sizeof(ubyte2);

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if ((SSL3_MINORVERSION == pSSLSock->sslMinorVersion) &&
        (0 == pSSLSock->handshakeCount) )
    {
        /* TLS_EMPTY_RENEGOTIATION_INFO_SCSV {0x00, 0xFF} */
        /* send only during initial SSL 3.0 hello */
        /* DO NOT add SCSV to gCipherSuites;
         * This SCSV is not a true cipher suite (it does not correspond to any
         * valid set of algorithms) and cannot be negotiated.
         */
        *pTemp++ = (ubyte)0x00;
        *pTemp++ = (ubyte)0xFF;
    }
#endif /* ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__))) */
#endif /*  MIN_SSL_MINORVERSION <= SSL3_MINORVERSION */

	if(pSSLSock->runtimeFlags & SSL_FLAG_SCSV_FALLBACK_VERSION_SET)
	{
		/* TLS_FALLBACK_SCSV {0x56, 0x00} */
        /* send only when application is trying to re-negotiate with fall back version */
        /* DO NOT add SCSV to gCipherSuites;
         * This SCSV is not a true cipher suite (it does not correspond to any
         * valid set of algorithms) and cannot be negotiated.
         */
        *pTemp++ = (ubyte)0x56;
        *pTemp++ = (ubyte)0x00;
	}

    for (i = 0; i < (sbyte4)NUM_CIPHER_SUITES; ++i)
    {
        if (gCipherSuites[i].supported)
        {
#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
            if ((TRUE != pSSLSock->isCipherTableInit) ||
                (TRUE == pSSLSock->isCipherEnabled[i]) )
#endif
            {
#ifdef __ENABLE_MOCANA_CCM_8__
                /* FIXME: ff 05/20/15 there's a function to do that */
                /* Make sure this cipher is advertised only for TLS 1.2 and above */
                if ((0xC0AE == gCipherSuites[i].cipherSuiteId) &&
                    (pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
                    continue;
#endif
                *pTemp++ = (ubyte) (gCipherSuites[i].cipherSuiteId >> 8);
                *pTemp++ = (ubyte) (gCipherSuites[i].cipherSuiteId);
            }
        }
    }

    *pTemp++ = 1;
    *pTemp++ = 0; /* no compression  (cf SSL & TLS essentials p. 79) */

#ifdef __ENABLE_RFC3546__
    if (extensionsLength)
    {
        /* write extensions length -- subtract 2 from the extensions length */
        setShortValue(pTemp, (ubyte2)(extensionsLength-2));
        pTemp += sizeof(ubyte2);

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
        /* to interoperate with old server, SSL 3.0 should only
           send renegotiation extension during rehandshake */
        if ( SSL3_MINORVERSION < pSSLSock->sslMinorVersion ||
            (SSL3_MINORVERSION == pSSLSock->sslMinorVersion && pSSLSock->isRehandshakeAllowed) )
        {
            ubyte4  renegotiatedConnLength = 0;
#if MIN_SSL_MINORVERSION <= SSLV3_VERSION
            ubyte4  verifyDataSize = (SSL3_MINORVERSION == pSSLSock->sslMinorVersion) ?
                                    SSL_VERIFY_DATA : TLS_VERIFYDATASIZE;
#else
            ubyte4  verifyDataSize = TLS_VERIFYDATASIZE;
#endif
            if (0 < pSSLSock->handshakeCount)
            {
                /* TLS 1.0, 1.1 and 1.2 */
                renegotiatedConnLength = verifyDataSize;
            }

            /* write extension type */
            setShortValue(pTemp, (ubyte2)tlsExt_renegotiated_connection);
            pTemp += sizeof(ubyte2);

            /* write extension length */
            setShortValue(pTemp, (ubyte2)(1 + renegotiatedConnLength));
            pTemp += sizeof(ubyte2);

            /* write renegotiated_connection<0..255> length */
            *pTemp = renegotiatedConnLength;
            pTemp += 1;

            if (0 < renegotiatedConnLength)
            {
                /* write renegotiated_connection string */
                MOC_MEMCPY(pTemp, pSSLSock->client_verify_data, verifyDataSize);
                pTemp += renegotiatedConnLength;
            }
        }
#endif /* ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__))) */

#if MIN_SSL_MINORVERSION <= SSLV3_VERSION
        if (SSL3_MINORVERSION < pSSLSock->sslMinorVersion)
#endif
        {
            /* server name extension? */
            if (pSSLSock->serverNameListLength)
            {
                /* write extension type */
                setShortValue(pTemp, (ubyte2)tlsExt_server_name);
                pTemp += sizeof(ubyte2);

                /* write extension length */
                setShortValue(pTemp, (ubyte2) (2 + pSSLSock->serverNameListLength));
                pTemp += sizeof(ubyte2);

                /* write serverNameListLength */
                setShortValue(pTemp, (ubyte2)pSSLSock->serverNameListLength);
                pTemp += sizeof(ubyte2);

                /* write extension specific data */
                MOC_MEMCPY(pTemp, pSSLSock->serverNameList, pSSLSock->serverNameListLength);
                pTemp += pSSLSock->serverNameListLength;
            }

#ifdef __ENABLE_TLSEXT_RFC6066__
            if (pSSLSock->certStatusReqExt)
            {
                /* write extension type */
                setShortValue(pTemp, (ubyte2)tlsExt_status_request);
                pTemp += sizeof(ubyte2);

                /* write extension length */
                setShortValue(pTemp, (ubyte2)(pSSLSock->roleSpecificInfo.client.certStatusReqExtLen));
                pTemp += sizeof(ubyte2);

                /* write extension specific data */
                MOC_MEMCPY(pTemp, pSSLSock->roleSpecificInfo.client.certStatusReqExtData,
                           pSSLSock->roleSpecificInfo.client.certStatusReqExtLen);
                pTemp += pSSLSock->roleSpecificInfo.client.certStatusReqExtLen;
            }
#endif

            /* TLS1.2: signature algorithms extension */
            if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
                (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
            {
                /* write extension type */
                setShortValue(pTemp, (ubyte2)tlsExt_supportedSignatureAlgorithms);
                pTemp += sizeof(ubyte2);

                /* write extension length */
                setShortValue(pTemp, (ubyte2) (2 + NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS*2));
                pTemp += sizeof(ubyte2);

                /* write supportedSignatureAlgorithmListLength */
                setShortValue(pTemp, (ubyte2)NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS*2);
                pTemp += sizeof(ubyte2);

                /* write extension specific data */
                for ( i = 0; i < NUM_SSL_SUPPORTED_SIGNATURE_ALGORITHMS; ++i)
                {
                    setShortValue( pTemp, gSupportedSignatureAlgorithms[i]);
                    pTemp += sizeof(ubyte2);
                }
            }
#ifdef __ENABLE_MOCANA_EAP_FAST__
            /* ticket extension ? */
            /* only if we have a ticket extension */
            if ( pSSLSock->roleSpecificInfo.client.ticket &&
                pSSLSock->roleSpecificInfo.client.ticketLength)
            {
                /* write extension type */
                setShortValue(pTemp, (ubyte2)tlsExt_ticket);
                pTemp += sizeof(ubyte2);

#ifndef __DISABLE_EAP_FAST_EXTENSION_LENGTH__
                /* write extension length */
                setShortValue(pTemp, (ubyte2) (2 + pSSLSock->roleSpecificInfo.client.ticketLength));
                pTemp += sizeof(ubyte2);
#endif

                /* write ticket size */
                setShortValue(pTemp, (ubyte2) pSSLSock->roleSpecificInfo.client.ticketLength);
                pTemp += sizeof(ubyte2);

                if ( pSSLSock->roleSpecificInfo.client.ticketLength)
                {
                    /* write extension specific data */
                    MOC_MEMCPY(pTemp, pSSLSock->roleSpecificInfo.client.ticket,
                                pSSLSock->roleSpecificInfo.client.ticketLength);
                    pTemp += pSSLSock->roleSpecificInfo.client.ticketLength;
                }
            }
#endif /* __ENABLE_MOCANA_EAP_FAST__ */
#ifdef __ENABLE_MOCANA_INNER_APP__
            /* only if we have a inner application extension */
            if (pSSLSock->roleSpecificInfo.client.innerApp)
            {
                /* write extension type */
                setShortValue(pTemp, (ubyte2)tlsExt_innerApplication);
                pTemp += sizeof(ubyte2);

                /* write innerApp size */
                setShortValue(pTemp, (ubyte2) sizeof(ubyte2));
                pTemp += sizeof(ubyte2);

                /* write innerApp Value */
                setShortValue(pTemp, (ubyte2) pSSLSock->roleSpecificInfo.client.innerAppValue);
                pTemp += sizeof(ubyte2);
            }
#endif
#if defined(__ENABLE_MOCANA_ECC__)

            if ( cipherFlags & (SSL_KEYEX_ECDH_BIT | SSL_KEYEX_ECDHE_BIT))
            {
                setShortValue( pTemp, (ubyte2) tlsExt_supportedEllipticCurves);
                pTemp += sizeof(ubyte2);

                /* extension length */
                setShortValue( pTemp, numECCurves * 2 + 2);
                pTemp += sizeof(ubyte2);

                /* size of curves list */
                setShortValue( pTemp, numECCurves * 2);
                pTemp += sizeof(ubyte2);

                /* curves in order of preference filtered with eccCurves */
                for ( i = 0; i < NUM_SSL_SUPPORTED_ELLIPTIC_CURVES; ++i)
                {
                    ubyte4 curve = gSupportedEllipticCurves[i];

                    if ( ( 1 << curve) & eccCurves )
                    {
                        setShortValue( pTemp, (ubyte2) curve);
                        pTemp += sizeof(ubyte2);
                    }
                }

                /* supported point compression -- we don't */
                setShortValue( pTemp, (ubyte2) tlsExt_ECPointFormat);
                pTemp += 2;
                setShortValue( pTemp, (ubyte2) 2);
                pTemp += 2;
                *pTemp++ = 1;
                *pTemp++ = tlsExtECPointFormat_uncompressed;
            }

#endif /* __ENABLE_MOCANA_ECC__ */

#if (defined(__ENABLE_MOCANA_DTLS_CLIENT__) && defined(__ENABLE_MOCANA_DTLS_SRTP__))
            /* use_srtp extension? */
            if (pSSLSock->isDTLS && pSSLSock->useSrtp)
            {
                /* TODO: do this once */
                ubyte4 profileCount = numSrtpProfiles(pSSLSock);

                /* write extension type */
                setShortValue(pTemp, (ubyte2)dtlsExt_use_srtp);
                pTemp += sizeof(ubyte2);

                /* write extension length */
                setShortValue(pTemp, (ubyte2) (2 + 2*profileCount + 1 + ((NULL != pSSLSock->srtpMki) ? *(pSSLSock->srtpMki) : 0)) );
                pTemp += sizeof(ubyte2);

                /* write SRTPProtectionProfileListLength */
                setShortValue(pTemp, (ubyte2)(2*profileCount));
                pTemp += sizeof(ubyte2);

                for (i = 0; i < (sbyte4)NUM_SRTP_PROFILES; ++i)
                {
                    if (gSrtpProfiles[i].supported)
                    {
#ifdef __ENABLE_MOCANA_SRTP_PROFILES_SELECT__
                        if ((TRUE != pSSLSock->isSrtpProfileTableInit) ||
                            (TRUE == pSSLSock->isSrtpProfileEnabled[i]) )
#endif
                        {
                            *pTemp++ = (ubyte) (gSrtpProfiles[i].profileId >> 8);
                            *pTemp++ = (ubyte) (gSrtpProfiles[i].profileId);
                        }
                    }
                }

                /* write srtp_mki content if any */
                if (NULL != pSSLSock->srtpMki)
                {
                    MOC_MEMCPY(pTemp, pSSLSock->srtpMki, 1 + *(pSSLSock->srtpMki));
                    pTemp += 1 + *(pSSLSock->srtpMki);
                }
                else
                {
                    *pTemp = 0;
                    pTemp += 1;
                }
            }

#endif
        }
    }
#endif /* __ENABLE_RFC3546__ */

    /* add to the hashes */
    /* addToHandshakeHash(pSSLSock, pSSLSock->buffers[0].data, (pSSLSock->buffers[0].length - sizeofRecordHeader)); */
    /* save client hello for hash calculation later: after processed server hello and know the version and cipher */
    if (pSSLSock->roleSpecificInfo.client.helloBuffer)
    {
        FREE(pSSLSock->roleSpecificInfo.client.helloBuffer);
    }

    pSSLSock->roleSpecificInfo.client.helloBuffer = MALLOC( pSSLSock->buffers[0].length - sizeofRecordHeader);
    if (NULL == pSSLSock->roleSpecificInfo.client.helloBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    MOC_MEMCPY(pSSLSock->roleSpecificInfo.client.helloBuffer, pSSLSock->buffers[0].data, (pSSLSock->buffers[0].length - sizeofRecordHeader));
    pSSLSock->roleSpecificInfo.client.helloBufferLen = (pSSLSock->buffers[0].length - sizeofRecordHeader);
    if (TRUE != isRehandshake)
    {
        if (SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)
        {
            numBytesSent  = 0;
            status = pSSLSock->buffers[0].length;
        }
        else
        {
#ifndef __MOCANA_IPSTACK__
        if (OK > (status = TCP_WRITE(pSSLSock->tcpSock, (sbyte *)(pSSLSock->buffers[0].pHeader), pSSLSock->buffers[0].length, &numBytesSent)))
#else
        if (OK > (status = MOC_TCP_WRITE(pSSLSock->tcpSock, (sbyte *)(pSSLSock->buffers[0].pHeader), pSSLSock->buffers[0].length, &numBytesSent)))
#endif
        goto exit;
        }

        if (numBytesSent != pSSLSock->buffers[0].length)
        {
            pSSLSock->pOutputBufferBase = pSSLSock->buffers[0].pHeader;
            pSSLSock->pOutputBuffer     = numBytesSent + pSSLSock->buffers[0].pHeader;
            pSSLSock->outputBufferSize  = pSSLSock->buffers[0].length + fragmentLen;
            pSSLSock->numBytesToSend    = pSSLSock->buffers[0].length + fragmentLen - numBytesSent;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
            if (pSSLSock->isDTLS)
            {
                /* fragment the records to meet PMTU */
                pSSLSock->bufIndex = 0;
                pSSLSock->numBuffers = 1;

                if (OK > (status = fragmentHandshakeMessages(pSSLSock, fragmentLen)))
                    goto exit;

                releaseRetransmissionBuffer(pSSLSock);
                addDataToRetransmissionBuffer(pSSLSock, SSL_HANDSHAKE, (const sbyte*)pSSLSock->pOutputBuffer, pSSLSock->numBytesToSend);
            }
#endif

            pSSLSock->buffers[0].pHeader              = NULL;
        }
    }
    else
    {
        pSSLSock->bufIndex = 0;
        pSSLSock->numBuffers = 1;

        status = SSLSOCK_sendEncryptedHandshakeBuffer(pSSLSock);
    }
	/* used to remove warnings*/
	MOC_UNUSED(numECCurves);
exit:
    if ((TRUE != isRehandshake) || (OK > status))
    {
        /* free the buffers */
        if (pSSLSock->buffers[0].pHeader)
        {
            CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&(pSSLSock->buffers[0].pHeader));
        }
    }

    return status;

} /* SSL_CLIENT_sendClientHello */


/*------------------------------------------------------------------*/

static MSTATUS
fillClientCertificate(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length)
{
    ubyte*              pSHSH;
    ubyte*              pWriteCert;
    SizedBuffer*        certificates = pSSLSock->roleSpecificInfo.client.sslMutualAuthCerts;
    ubyte4              numCert = pSSLSock->roleSpecificInfo.client.numMutualAuthCert;
    ubyte4              sizeofHandshakeHeader;

    /* set the handshake part */
    pSHSH = pBuffer;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader*)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(length - sizeofHandshakeHeader));
        setMediumValue(((DTLSHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_CERTIFICATE;
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        setMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_CERTIFICATE;
    }

    pWriteCert = (ubyte*)(pSHSH + sizeofHandshakeHeader);

    /* store cert chain length */
    setMediumValue(pWriteCert, (ubyte2) (length - sizeofHandshakeHeader - SSL_MEDIUMSIZE));
    pWriteCert += SSL_MEDIUMSIZE;

    if (0 != numCert)
    {
        ubyte4 i;
        ubyte* pVarPart = pWriteCert;
        /* for each certificate, store value in medium and then certificate     */
        for (i = 0; i < numCert; ++i)
        {
            ubyte2 certLen = certificates[i].length;

            setMediumValue(pVarPart, certLen);
            pVarPart += SSL_MEDIUMSIZE;

            MOC_MEMCPY(pVarPart, certificates[i].data, certLen);

            pVarPart += certLen;
        }
    }
    else
    {
        /* we have no key data, we're unable to perform a certificateVerify */
        pSSLSock->isMutualAuthNegotiated = FALSE;
    }

    /* add to the hashes */
    addToHandshakeHash(pSSLSock, pBuffer, length);

    return OK;

} /* fillClientCertificate */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__))
static MSTATUS
fillClientEncryptedRsaPresecret(SSLSocket* pSSLSock, ubyte *pPresecret,
                                ubyte *pEncryptedPresecret, vlong **ppVlongQueue)
{
    sbyte4              cipherTextLen;
    MSTATUS             status;
    RSAKey*             pRSAKey;

    /* generate presecret */
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        pPresecret[0] = DTLS1_MAJORVERSION;
    } else
#endif
    {
        pPresecret[0] = SSL3_MAJORVERSION;
    }

    /* this needs to match the one we sent in client hello */
    pPresecret[1] = pSSLSock->clientHelloMinorVersion;

    /* generate rest of presecret */
    pSSLSock->rngFun(pSSLSock->rngFunArg, SSL_RSAPRESECRETSIZE - 2, pPresecret + 2);

    if ( akt_rsa != pSSLSock->roleSpecificInfo.client.publicKey.type)
    {
        status = ERR_SSL_INVALID_KEY_TYPE;
        goto exit;
    }

    pRSAKey = pSSLSock->roleSpecificInfo.client.publicKey.key.pRSA;

    /* allocate buffer big enough for the handshake record */
    if (OK > (status = RSA_getCipherTextLength(pRSAKey, &cipherTextLen)))
        goto exit;

#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
    if (SSL3_MINORVERSION < pSSLSock->sslMinorVersion)
#endif
    {
        setShortValue(pEncryptedPresecret, (ubyte2)cipherTextLen);
        pEncryptedPresecret += 2;
    }

    /* encrypt presecret with certificate key into the buffer */
    if (OK > (status = RSA_encrypt(MOC_RSA(pSSLSock->hwAccelCookie) pRSAKey,
                                   pPresecret, SSL_RSAPRESECRETSIZE, pEncryptedPresecret,
                                   pSSLSock->rngFun, pSSLSock->rngFunArg, ppVlongQueue)))
    {
        goto exit;
    }

exit:
    return status;

} /* fillClientEncryptedRsaPresecret */
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__))
static MSTATUS
fillClientRsaKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue)
{
    ubyte*              pSHSH;
    ubyte               presecret[SSL_RSAPRESECRETSIZE];
    sbyte4              cipherTextLen;
    ubyte*              encryptedPresecret;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status;
    RSAKey*             pRSAKey;

    if ( akt_rsa != pSSLSock->roleSpecificInfo.client.publicKey.type)
    {
        status = ERR_SSL_INVALID_KEY_TYPE;
        goto exit;
    }

    pRSAKey = pSSLSock->roleSpecificInfo.client.publicKey.key.pRSA;
    /* allocate buffer big enough for the handshake record */
    if (OK > (status = RSA_getCipherTextLength(pRSAKey, &cipherTextLen)))
        goto exit;

    /* set the handshake part */
    pSHSH = pBuffer;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(length - sizeofHandshakeHeader));
        setMediumValue( ((DTLSHandshakeHeader *)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((DTLSHandshakeHeader *)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        setMediumValue( ((SSLHandshakeHeader *)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((SSLHandshakeHeader *)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    }
    encryptedPresecret = (ubyte *)(pSHSH + sizeofHandshakeHeader);

    if (OK > (status = fillClientEncryptedRsaPresecret(pSSLSock, presecret,
                                    encryptedPresecret, ppVlongQueue)))
        goto exit;

    /* add to the hashes */
    addToHandshakeHash(pSSLSock, pBuffer, length);

    /* we can generate the key material now */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, presecret, SSL_RSAPRESECRETSIZE);

exit:
    return status;

} /* fillClientRsaKeyExchange */
#endif


/*---------------------------------------------------------------------------*/

#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )

static MSTATUS fillClientEccKeyExchange(SSLSocket* pSSLSock, ECCKey* pECCKey,
                                        ubyte *pBuffer, ubyte2 length)
{
    ubyte*              pSHSH;
    MSTATUS             status;
    ubyte*              sharedSecret = 0;
    ubyte*              pPayload;
    sbyte4              sharedSecretLen;
    PrimeFieldPtr       pPF;
    PFEPtr              k = 0, Qx = 0, Qy = 0;
    ubyte4              sizeofHandshakeHeader;

    pPF = EC_getUnderlyingField(pECCKey->pCurve);

    /* set the handshake part */
    pSHSH = pBuffer;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(length - sizeofHandshakeHeader));
        setMediumValue( ((DTLSHandshakeHeader *)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((DTLSHandshakeHeader *)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        setMediumValue( ((SSLHandshakeHeader *)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((SSLHandshakeHeader *)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    }

    /* generate a new ECDH key pair on the curve and send the public key
        The presecret is the shared secret generated by the ECDH operation */

    if (OK > (status = PRIMEFIELD_newElement( pPF, &k)) ||
        OK > (status = PRIMEFIELD_newElement( pPF, &Qx)) ||
        OK > (status = PRIMEFIELD_newElement( pPF, &Qy)))
    {
        goto exit;
    }

    if (OK > (status = EC_generateKeyPair( pECCKey->pCurve, pSSLSock->rngFun,
                                            pSSLSock->rngFunArg, k, Qx, Qy)))
    {
        goto exit;
    }

    pPayload = (ubyte*)(pSHSH+sizeofHandshakeHeader);
    /* add the redundant byte for length (ECPoint ) */
    *pPayload++ = (ubyte) (length - 1 - sizeofHandshakeHeader);
    if (OK > (status = EC_writePointToBuffer( pECCKey->pCurve, Qx, Qy, pPayload,
                                                length - sizeofHandshakeHeader)))
    {
        goto exit;
    }

    /* add to the hashes */
    addToHandshakeHash(pSSLSock, pBuffer, length);

    /* we can generate the key material now */
    /* derive the presecret */
    if ( OK > ( status = ECDH_generateSharedSecretAux( pECCKey->pCurve,
                                                        pECCKey->Qx,
                                                        pECCKey->Qy,
                                                        k,
                                                        &sharedSecret,
                                                        &sharedSecretLen,
                                                        1))) /* X coordinate only */
    {
        goto exit;
    }

    status = SSL_SOCK_generateKeyMaterial(pSSLSock, sharedSecret, sharedSecretLen);

exit:

    PRIMEFIELD_deleteElement( pPF, &k);
    PRIMEFIELD_deleteElement( pPF, &Qx);
    PRIMEFIELD_deleteElement( pPF, &Qy);

    if ( sharedSecret)
    {
        FREE( sharedSecret);
    }

    return status;

} /* fillClientEccKeyExchange */
#endif /* (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__)
            || defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) ||
            defined(__ENABLE_MOCANA_SSL_ECDH_ANON SUPPORT__) ) */


/*---------------------------------------------------------------------------*/

#if (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__))

static MSTATUS fillClientEcdhKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer,
                                        ubyte2 length, vlong **ppVlongQueue)
{

    if ( akt_ecc != pSSLSock->roleSpecificInfo.client.publicKey.type)
    {
        return ERR_SSL_INVALID_KEY_TYPE;
    }

    return fillClientEccKeyExchange(pSSLSock,
                pSSLSock->roleSpecificInfo.client.publicKey.key.pECC,
                pBuffer,
                length);

} /* fillClientEcdhKeyExchange */
#endif /* (defined( __ENABLE_MOCANA_SSL_ECDH_SUPPORT__)) */


/*---------------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__))

static MSTATUS fillClientEcdheKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer,
                                        ubyte2 length, vlong **ppVlongQueue)
{
    if ( akt_ecc != pSSLSock->ecdheKey.type)
    {
        return ERR_SSL_INVALID_KEY_TYPE;
    }
    return fillClientEccKeyExchange(pSSLSock,
                pSSLSock->ecdheKey.key.pECC,
                pBuffer,
                length);

} /* fillClientEcdheKeyExchange */
#endif /* (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) \\
        defined(__ENABLE_MOCANA_SSL_ECDH_ANON SUPPORT__)) */


/*---------------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
static MSTATUS
fillClientPskKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue)
{
    ubyte*              pSHSH;
    ubyte               presecret[2 + SSL_PSK_MAX_LENGTH + 2 + SSL_PSK_MAX_LENGTH];
    ubyte*              pPsk;
    sbyte4              pskLength;
    ubyte*              pPskIdentity;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status;
    MOC_UNUSED(ppVlongQueue);

    /* set the handshake part */
    pSHSH = pBuffer;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(length - sizeofHandshakeHeader));
        setMediumValue( ((DTLSHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        setMediumValue( ((SSLHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    }

    /* fill in the identity */
    pPskIdentity = (ubyte *)(pSHSH + sizeofHandshakeHeader);
    setShortValue(pPskIdentity, (ubyte2)(pSSLSock->roleSpecificInfo.client.pskIdentityLength));
    MOC_MEMCPY(2 + pPskIdentity, pSSLSock->roleSpecificInfo.client.pskIdentity, pSSLSock->roleSpecificInfo.client.pskIdentityLength);

    /* add to the hashes */
    addToHandshakeHash(pSSLSock, pBuffer, length);

    /* build shared secret */
    pPsk      = presecret;
    pskLength = pSSLSock->roleSpecificInfo.client.pskLength;

    setShortValue(pPsk, (ubyte2)(pskLength));    pPsk += 2;
    MOC_MEMSET   (pPsk, 0x00, pskLength);        pPsk += pskLength;
    setShortValue(pPsk, (ubyte2)(pskLength));    pPsk += 2;
    MOC_MEMCPY   (pPsk, pSSLSock->roleSpecificInfo.client.psk, pskLength);

    /* we can generate the key material now */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, presecret, 2 + pskLength + 2 + pskLength);

    return status;

} /* fillClientPskKeyExchange */
#endif


/*------------------------------------------------------------------*/

#if ((defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__)) && (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)))
static MSTATUS
fillClientRsaPskKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue)
{
    ubyte*              pSHSH;
    ubyte               presecret[2 + SSL_RSAPRESECRETSIZE + 2 + SSL_PSK_MAX_LENGTH];
    ubyte*              pPsk;
    sbyte4              pskLength;
    ubyte*              pPskIdentity;
    ubyte4              pskIdentityLength;
    ubyte*              encryptedPresecret;
    sbyte4              cipherTextLen;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status;

    /* allocate buffer big enough for the handshake record */
    if (OK > (status = RSA_getCipherTextLength(pSSLSock->roleSpecificInfo.client.publicKey.key.pRSA, &cipherTextLen)))
        goto exit;

    /* set the handshake part */
    pSHSH = pBuffer;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(length - sizeofHandshakeHeader));
        setMediumValue( ((DTLSHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        setMediumValue( ((SSLHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    }

    /* fill in the identity */
    pPskIdentity = (ubyte *)(pSHSH + sizeofHandshakeHeader);
    pskIdentityLength = pSSLSock->roleSpecificInfo.client.pskIdentityLength;
    setShortValue(pPskIdentity, (ubyte2)(pskIdentityLength));
    MOC_MEMCPY(2 + pPskIdentity, pSSLSock->roleSpecificInfo.client.pskIdentity, pskIdentityLength);

    encryptedPresecret = 2 + pPskIdentity + pskIdentityLength;

    /* fill in encrypted presecret and presecret */
    if (OK > (status = fillClientEncryptedRsaPresecret(pSSLSock, 2 + presecret, encryptedPresecret, ppVlongQueue)))
        goto exit;

    /* set length of RSA pre-secret */
    presecret[0] = 0;
    presecret[1] = (ubyte)SSL_RSAPRESECRETSIZE;

    /* fill in psk into shared secret */
    pPsk      = &(presecret[2 + SSL_RSAPRESECRETSIZE]);
    pskLength = pSSLSock->roleSpecificInfo.client.pskLength;

    setShortValue(pPsk, (ubyte2)(pskLength));    pPsk += 2;
    MOC_MEMCPY   (pPsk, pSSLSock->roleSpecificInfo.client.psk, pskLength);

    /* add to the hashes */
    addToHandshakeHash(pSSLSock, pBuffer, length);

    /* we can generate the key material now */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, presecret, 2 + SSL_RSAPRESECRETSIZE + 2 + pskLength);

exit:
    return status;

} /* fillClientPskKeyExchange */
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
static MSTATUS
fillClientDiffieHellmanKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue)
{
    ubyte*              pSHSH;
    ubyte*              pYc;
    ubyte4              lenYc;
    ubyte*              pK = NULL;
    ubyte4              lenK;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status;
    MOC_UNUSED(ppVlongQueue);

    /* get length of Yc */
    lenYc = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_f)) / 8;

    /* set the handshake part */
    pSHSH = pBuffer;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(length - sizeofHandshakeHeader));
        setMediumValue(((DTLSHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;

    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        setMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    }

    pYc = (ubyte *)(pSHSH + sizeofHandshakeHeader);

    /* store Yc */
    setShortValue(pYc, (ubyte2)lenYc);

    if (OK > (status = VLONG_fixedByteStringFromVlong(pSSLSock->pDHcontext->dh_f, 2 + pYc, lenYc)))
        goto exit;

    /* add to the hashes */
    addToHandshakeHash(pSSLSock, pBuffer, length);

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

    /* we can generate the key material now */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, pK, lenK);

exit:
    if (NULL != pK)
        FREE(pK);

    return status;

} /* fillClientDiffieHellmanKeyExchange */
#endif /* (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
static MSTATUS
fillClientDiffieHellmanPskKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue)
{
    ubyte*              pSHSH;
    ubyte*              pYc;
    ubyte4              lenYc;
    ubyte4              lenK;
    ubyte*              pSecret = NULL;
    sbyte4              pskLength;
    ubyte*              pPskIdentity;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status;
    MOC_UNUSED(ppVlongQueue);

    /* get length of Yc */
    lenYc = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_f)) / 8;

    /* set the handshake part */
    pSHSH = pBuffer;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(length - sizeofHandshakeHeader));
        setMediumValue(((DTLSHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;

    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        setMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    }

    /* fill in the identity */
    pPskIdentity = (ubyte *)(pSHSH + sizeofHandshakeHeader);
    setShortValue(pPskIdentity, (ubyte2)(pSSLSock->roleSpecificInfo.client.pskIdentityLength));
    MOC_MEMCPY(2 + pPskIdentity, pSSLSock->roleSpecificInfo.client.pskIdentity, pSSLSock->roleSpecificInfo.client.pskIdentityLength);

    pYc = (ubyte *)(pPskIdentity + 2 + pSSLSock->roleSpecificInfo.client.pskIdentityLength);

    /* store Yc */
    setShortValue(pYc, (ubyte2)lenYc);

    if (OK > (status = VLONG_fixedByteStringFromVlong(pSSLSock->pDHcontext->dh_f, 2 + pYc, lenYc)))
        goto exit;

    /* add to the hashes */
    addToHandshakeHash(pSSLSock, pBuffer, length);

    /* fill in psk into shared secret */
    pskLength = pSSLSock->roleSpecificInfo.client.pskLength;

    /* allocate buffer for shared secret plus psk */
    lenK  = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_k)) / 8;

    if (NULL == (pSecret = MALLOC(2 + lenK + 2 + pskLength)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* length of k */
    setShortValue(pSecret, (ubyte2)(lenK));
    /* convert shared secret (k) into something digestible */
    if (OK > (status = VLONG_fixedByteStringFromVlong(pSSLSock->pDHcontext->dh_k,
                                                      pSecret + 2, lenK)))
    {
        goto exit;
    }

    /* fill in psk into shared secret */
    setShortValue(pSecret + 2 + lenK, (ubyte2)(pskLength));
    MOC_MEMCPY(pSecret + 2 + lenK + 2, pSSLSock->roleSpecificInfo.client.psk, pskLength);

    /* we can generate the key material now */
    status = SSL_SOCK_generateKeyMaterial(pSSLSock, pSecret, 2 + lenK + 2 + pskLength);

exit:
    if (NULL != pSecret)
        FREE(pSecret);

    return status;

} /* fillClientDiffieHellmanPskKeyExchange */
#endif /* (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)) */



/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__))
static MSTATUS
fillClientEcdhePskKeyExchange(SSLSocket* pSSLSock, ubyte *pBuffer, ubyte2 length, vlong **ppVlongQueue)
{
    ubyte*              pSHSH;
    MSTATUS             status;
    ubyte*              sharedSecret = 0;
    ubyte*              preSecret = 0;
    ubyte*              pTemp;
    sbyte4              sharedSecretLen;
    PrimeFieldPtr       pPF;
    PFEPtr              k = 0, Qx = 0, Qy = 0;
    ubyte4              sizeofHandshakeHeader;
    ECCKey*             pECCKey;
    sbyte4              pskLength;
    sbyte4              pskIdentityLength;


    if ( akt_ecc != pSSLSock->ecdheKey.type)
    {
        return ERR_SSL_INVALID_KEY_TYPE;
    }

    pECCKey = pSSLSock->ecdheKey.key.pECC;

    pPF = EC_getUnderlyingField(pECCKey->pCurve);

    /* set the handshake part */
    pSHSH = pBuffer;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(length - sizeofHandshakeHeader));
        setMediumValue(((DTLSHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;

    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        setMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_KEY_EXCHANGE;
    }

    /* fill in the identity */
    pskIdentityLength = pSSLSock->roleSpecificInfo.client.pskIdentityLength;

    pTemp = (ubyte *)(pSHSH + sizeofHandshakeHeader);
    setShortValue(pTemp, (ubyte2)(pSSLSock->roleSpecificInfo.client.pskIdentityLength));
    MOC_MEMCPY(pTemp+2, pSSLSock->roleSpecificInfo.client.pskIdentity, pskIdentityLength);

    /* generate a new ECDH key pair on the curve and send the public key
     The presecret is the shared secret generated by the ECDH operation */

    if (OK > (status = PRIMEFIELD_newElement( pPF, &k)) ||
        OK > (status = PRIMEFIELD_newElement( pPF, &Qx)) ||
        OK > (status = PRIMEFIELD_newElement( pPF, &Qy)))
    {
        goto exit;
    }

    if (OK > (status = EC_generateKeyPair( pECCKey->pCurve, pSSLSock->rngFun,
                                          pSSLSock->rngFunArg, k, Qx, Qy)))
    {
        goto exit;
    }

    pTemp += 2 + pskIdentityLength;
    /* add the redundant byte for length (ECPoint ) */
    *pTemp = (ubyte) (length - (1 + 2 + pskIdentityLength + sizeofHandshakeHeader));
    if (OK > (status = EC_writePointToBuffer( pECCKey->pCurve, Qx, Qy, pTemp + 1,
                                             *pTemp)))
    {
        goto exit;
    }

    /* add to the hashes */
    addToHandshakeHash(pSSLSock, pBuffer, length);

    /* we can generate the key material now */
    /* derive the presecret */
    if ( OK > ( status = ECDH_generateSharedSecretAux( pECCKey->pCurve,
                                                      pECCKey->Qx,
                                                      pECCKey->Qy,
                                                      k,
                                                      &sharedSecret,
                                                      &sharedSecretLen,
                                                      1))) /* X coordinate only */
    {
        goto exit;
    }

    pskLength = pSSLSock->roleSpecificInfo.client.pskLength;

    /* allocate a buffer big enough for the presecret */
    if (NULL == (preSecret = MALLOC( 2 + sharedSecretLen + 2 + pskLength)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    setShortValue(preSecret, sharedSecretLen);
    MOC_MEMCPY(preSecret + 2, sharedSecret, sharedSecretLen);
    setShortValue(preSecret + 2 + sharedSecretLen, pskLength);
    MOC_MEMCPY(preSecret + 2 + sharedSecretLen + 2,
               pSSLSock->roleSpecificInfo.client.psk, pskLength);

    status = SSL_SOCK_generateKeyMaterial(pSSLSock, preSecret,
                                          2 + sharedSecretLen + 2 + pskLength);

exit:

    PRIMEFIELD_deleteElement( pPF, &k);
    PRIMEFIELD_deleteElement( pPF, &Qx);
    PRIMEFIELD_deleteElement( pPF, &Qy);

    if ( sharedSecret)
    {
        FREE( sharedSecret);
    }

    if ( preSecret)
    {
        FREE( preSecret);
    }

    return status;
} /* fillClientEcdhePskKeyExchange */
#endif /* (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)) */


/*--------------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined (__ENABLE_MOCANA_ECC__)

static MSTATUS
SSL_CLIENT_GenerateECDSASignature(ECCKey* pECCKey,
                                  RNGFun rngFun, void* rngArg,
                                  const ubyte* hash, ubyte4 hashLen,
                                  ubyte* pSignature, ubyte4* pSignatureLen)
{
    DER_ITEMPTR pTempSeq = 0;
    PFEPtr sig_r = 0, sig_s = 0;
    PrimeFieldPtr pPF;
    ubyte* pSignatureBuffer = 0;
    sbyte4 elementLen;
    ubyte* pRBuffer;
    ubyte* pSBuffer;
    MSTATUS status;

    pPF = EC_getUnderlyingField( pECCKey->pCurve);

    if (OK > ( status = PRIMEFIELD_newElement( pPF, &sig_r)))
        goto exit;
    if (OK > ( status = PRIMEFIELD_newElement( pPF, &sig_s)))
        goto exit;

    if (OK > ( status = ECDSA_sign( pECCKey->pCurve,
                            pECCKey->k,
                            rngFun, rngArg,
                            hash, hashLen,
                            sig_r, sig_s)))
    {
        goto exit;
    }
    /* add the signature */
    /* allocate buffer for sig_r and sig_s with leading zeroes */
    if ( OK > ( status = PRIMEFIELD_getElementByteStringLen( pPF, &elementLen)))
        goto exit;

    /* allocate 2 extra bytes for the possible zero padding */
    pSignatureBuffer = MALLOC( 2 + 2 * elementLen);
    if (! pSignatureBuffer)
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    pRBuffer = pSignatureBuffer;
    *pRBuffer = 0x00; /* leading 0 */
    pSBuffer = pSignatureBuffer + 1 + elementLen;
    *pSBuffer = 0x00; /* leading 0 */
    /* write R */
    if ( OK > ( status = PRIMEFIELD_writeByteString( pPF, sig_r, pRBuffer+1, elementLen)))
        goto exit;

    /* write S */
    if ( OK > ( status = PRIMEFIELD_writeByteString( pPF, sig_s, pSBuffer+1, elementLen)))
        goto exit;

    /* create a sequence with the two integer -> signature */
    if (OK > ( status = DER_AddSequence( NULL, &pTempSeq)))
        goto exit;

    if (OK > ( status = DER_AddInteger( pTempSeq, elementLen + 1, pRBuffer, NULL)))
        goto exit;

    if (OK > ( status = DER_AddInteger( pTempSeq, elementLen + 1, pSBuffer, NULL)))
        goto exit;

    /* serialize the sequence */
    if (OK > ( status = DER_SerializeInto( pTempSeq, pSignature, pSignatureLen)))
        goto exit;

exit:

    if (pTempSeq)
    {
        TREE_DeleteTreeItem( (TreeItem*) pTempSeq);
    }

    FREE(pSignatureBuffer);

    PRIMEFIELD_deleteElement( pPF, &sig_r);
    PRIMEFIELD_deleteElement( pPF, &sig_s);

    return status;
}

#endif /* defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined (__ENABLE_MOCANA_ECC__) */


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
/* if a hashOID is specified, then this is TLS 1.2 and we need to
build a DER structure that is the signature input */
static MSTATUS
fillCertificateVerifyRSA(ubyte4 mutualAuthSignAlgo, SSLSocket* pSSLSock,
                         ubyte *pBuffer, ubyte2 length,
                         const ubyte* pHash, ubyte4 hashLen,
                         const ubyte* hashOID, vlong **ppVlongQueue)
{
    ubyte4              lenRsaN;
    ubyte*              pSHSH;
    ubyte*              pParams;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status;

    /* TLS 1.2 extra memory */
    DER_ITEMPTR         pDigestInfo = 0;
    ubyte*              derBuffer = 0;
    ubyte4              derBufferLen;

    /* set the handshake part */
    pSHSH = pBuffer;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(length - sizeofHandshakeHeader));
        setMediumValue(((DTLSHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_CERTIFICATE_VERIFY;

    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        setMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(length - sizeofHandshakeHeader));
        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_CERTIFICATE_VERIFY;
    }

    pParams = (ubyte*)(pSHSH + sizeofHandshakeHeader);

    /* store signature/hash algo */
    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        setShortValue(pParams, (ubyte2) mutualAuthSignAlgo); pParams += 2;
    }

    lenRsaN = ((7 + VLONG_bitLength(RSA_N(pSSLSock->mutualAuthKey.key.pRSA))) / 8);
    /* store signature length */
    setShortValue(pParams, (ubyte2)lenRsaN);    pParams += 2;

    /* if a hashOID is specified, this is TLS 1.2 and we need to build and sign a DER structure */
    if (hashOID)
    {
        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&derBuffer))))
            goto exit;

        if (OK > ( status = DER_AddSequence( NULL, &pDigestInfo)))
            goto exit;

        /* add the whole algorithm identifier sequence */
        if (OK > ( status = DER_StoreAlgoOID( pDigestInfo, hashOID, 1)))
            goto exit;

        /* add the digest */
        if (OK > ( status = DER_AddItem( pDigestInfo, OCTETSTRING, hashLen,
                                            pHash,  NULL)))
        {
            goto exit;
        }

        derBufferLen = SSL_BIGGER_TEMP_BUF_SIZE;
        if (OK > ( status = DER_SerializeInto( pDigestInfo, derBuffer, &derBufferLen)))
            goto exit;

        /* the DER is the thing to sign */
        pHash = derBuffer;
        hashLen = derBufferLen;
    }

    /* if this is a private key, we can do the signature ourselves */
    if (pSSLSock->mutualAuthKey.key.pRSA->keyType != RsaPublicKey)
    {
        if (OK > (status = RSA_signMessage(MOC_RSA(pSSLSock->hwAccelCookie) pSSLSock->mutualAuthKey.key.pRSA,
                                           pHash, hashLen, pParams, ppVlongQueue)))
        {
            goto exit;
        }
    }
    else /* a public key -> the private key was not provided so use the callback  */
    {
        if (!SSL_sslSettings()->funcPtrMutualAuthCertificateVerify)
        {
            status = ERR_SSL_UNABLE_TO_SIGN_CERTIFICATE_VERIFY;
            goto exit;
        }

        if (OK > (status = SSL_sslSettings()->funcPtrMutualAuthCertificateVerify(
                    SSL_findConnectionInstance(pSSLSock), pHash,hashLen,
                    pParams, lenRsaN)))
        {
            goto exit;
        }
    }

exit:

    TREE_DeleteTreeItem( (TreeItem*) pDigestInfo);
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&derBuffer));

    return status;

} /* fillCertificateVerifyRSA */

#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ */


/*------------------------------------------------------------------*/

#if defined(__ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__) && defined(__ENABLE_MOCANA_ECC__)
static MSTATUS
fillCertificateVerifyECC(ubyte4 mutualAuthSignAlgo, SSLSocket* pSSLSock,
                         ubyte *pBuffer, ubyte2* pLength,
                         const ubyte* pHash, ubyte4 hashLen)
{
    ubyte*              pSHSH;
    ubyte*              pParams;
    ubyte4              sizeofHandshakeHeader;
    MSTATUS             status;
    ubyte4              actualLen;

    /* set the handshake part, not the length since we don't know it yet */
    pSHSH = pBuffer;
    actualLen = (ubyte4) *pLength;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
        ((DTLSHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_CERTIFICATE_VERIFY;

    }
    else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
        ((SSLHandshakeHeader*)pSHSH)->handshakeType = SSL_CLIENT_CERTIFICATE_VERIFY;
    }

    pParams = (ubyte*)(pSHSH + sizeofHandshakeHeader);
    actualLen -= sizeofHandshakeHeader;

    /* store signature/hash algo */
    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        setShortValue(pParams, (ubyte2) mutualAuthSignAlgo);
        pParams += 2;
        actualLen -= 2;
    }

    /* can't set the length yet */
    pParams += 2;  /* where to put the signature */
    actualLen -= 2;

    /* if this is a private key, we can do the signature ourselves */
    if (pSSLSock->mutualAuthKey.key.pECC->privateKey)
    {
        if (OK > (status = SSL_CLIENT_GenerateECDSASignature( pSSLSock->mutualAuthKey.key.pECC,
                                            pSSLSock->rngFun, pSSLSock->rngFunArg,
                                            pHash, hashLen, pParams, &actualLen)))
        {
            goto exit;
        }
    }
    else /* a public key -> the private key was not provided so use the callback  */
    {
        if (!SSL_sslSettings()->funcPtrMutualAuthCertificateVerify)
        {
            status = ERR_SSL_UNABLE_TO_SIGN_CERTIFICATE_VERIFY;
            goto exit;
        }

        /* callback API specify a fixed length, so the return value is
        used to indicate the length of the signature */
        if (OK > (status = SSL_sslSettings()->funcPtrMutualAuthCertificateVerify(
                    SSL_findConnectionInstance(pSSLSock), pHash, hashLen,
                    pParams, actualLen)))
        {
            goto exit;
        }
        actualLen = (ubyte4) status;
    }

    /* actualLen is the length of the signature -- update data */
    /* store signature length */
    pParams -= 2;
    setShortValue(pParams, (ubyte2)actualLen);
    actualLen += 2;

    /* store signature/hash algo */
    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        actualLen += 2;
    }

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        DTLS_SET_HANDSHAKE_HEADER_EXTRA(((DTLSHandshakeHeader *)pSHSH), pSSLSock->nextSendSeq++, (ubyte2)(actualLen));
        setMediumValue(((DTLSHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(actualLen));
    }
    else
#endif
    {
        setMediumValue(((SSLHandshakeHeader*)pSHSH)->handshakeSize, (ubyte2)(actualLen));
    }

    actualLen += sizeofHandshakeHeader;
    *pLength = (ubyte2) actualLen;

exit:

    return status;

} /* fillCertificateVerifyECC */

#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__  __ENABLE_MOCANA_ECC__ */

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
static MSTATUS
fillCertificateVerify(ubyte4 mutualAuthSignAlgo, SSLSocket* pSSLSock,
                      ubyte *pBuffer, ubyte2* pLength, vlong **ppVlongQueue)
{
    MSTATUS status;
    ubyte* pHashResult = 0;
    ubyte4 hashLen = 0, dsaOffset = 0;
    const ubyte* hashOID = 0;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult))))
        goto exit;

    if ((pSSLSock->isDTLS && pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        if (OK > (status = calculateTLS12CertificateVerifyHash(mutualAuthSignAlgo, pSSLSock,
                                                                pHashResult, &hashLen,
                                                                &hashOID)))
        {
            goto exit;
        }
    }
    else
    {
        hashLen = MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE;
        dsaOffset = MD5_DIGESTSIZE; /* offset in pHashResult to use for ECDSA and DSA keys */

        if (OK > (status = calculateSSLTLSHashes(pSSLSock, 1, pHashResult,
                                                 (enum hashTypes) pSSLSock->sslMinorVersion)))
        {
            goto exit;
        }
    }

    switch ( pSSLSock->mutualAuthKey.type)
    {
    case akt_rsa:
        status = fillCertificateVerifyRSA(mutualAuthSignAlgo, pSSLSock, pBuffer, *pLength,
                                            pHashResult, hashLen, hashOID, ppVlongQueue);
        break;

#ifdef __ENABLE_MOCANA_ECC__
    case akt_ecc:
        status = fillCertificateVerifyECC( mutualAuthSignAlgo, pSSLSock, pBuffer, pLength,
                                            pHashResult + dsaOffset, hashLen - dsaOffset);
        break;
#endif

    default:
        status = ERR_SSL_INVALID_KEY_TYPE;
        goto exit;
    }

    /* add to the hashes */
    addToHandshakeHash(pSSLSock, pBuffer, *pLength);

exit:

    CRYPTO_uninitAsymmetricKey(&pSSLSock->mutualAuthKey, NULL);

    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult));

    return status;

} /* fillCertificateVerify */

#endif /* __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__ */


/*------------------------------------------------------------------*/

static MSTATUS
SSL_CLIENT_sendClientResponseBlock(SSLSocket* pSSLSock)
{
    intBoolean  isRehandshake = (pSSLSock->pActiveOwnCipherSuite) ? TRUE : FALSE;
    ubyte*      pSRH;
    sbyte4      bufIndex     = 0;
    sbyte4      numBufs      = 0;
    ubyte4      totalLen;
    ubyte4      fragmentLen  = 0;
    ubyte4      numBytesSent = 0;
    vlong*      pVlongQueue  = NULL;
    ubyte4      pskIdentityLength = 0;
    ubyte4      sizeofRecordHeader;
    ubyte4      sizeofHandshakeRecord;
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    ASN1_ITEM*  pCertificate = 0;
#endif
    MSTATUS     status       = OK;

    if (NULL == pSSLSock->pHandshakeCipherSuite)
        return ERR_SSL_PROTOCOL;

    pSSLSock->buffers[0].pHeader = NULL;
    pSSLSock->buffers[0].length = pSSLSock->buffers[1].length = pSSLSock->buffers[2].length = pSSLSock->buffers[3].length = pSSLSock->buffers[4].length = 0;

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofRecordHeader = sizeof(DTLSRecordHeader);
        sizeofHandshakeRecord = sizeof(DTLSHandshakeRecord);
    } else
#endif
    {
        sizeofRecordHeader = sizeof(SSLRecordHeader);
        sizeofHandshakeRecord = sizeof(SSLHandshakeRecord);
    }

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    /* certificate record length */
    if (pSSLSock->isMutualAuthNegotiated)
    {
        sbyte4 i;
        /* certificate */
        pSSLSock->buffers[bufIndex].length = (ubyte2)(sizeofHandshakeRecord + SSL_MEDIUMSIZE * (1 + pSSLSock->roleSpecificInfo.client.numMutualAuthCert));

        /* now add length of certificates themselves */
        for (i = 0; i < pSSLSock->roleSpecificInfo.client.numMutualAuthCert; ++i)
        {
            pSSLSock->buffers[bufIndex].length = (ubyte2)(pSSLSock->buffers[bufIndex].length + pSSLSock->roleSpecificInfo.client.sslMutualAuthCerts[i].length);
        }

        bufIndex++;
        numBufs++;
    }
    else
#endif
    if (pSSLSock->generateEmptyCert)
    {
        /* RFC-4346 (pg 46):
         * If no suitable certificate is available, the client SHOULD send a
         * certificate message containing no certificates.  That is, the
         * certificate_list structure has a length of zero.
         */
        pSSLSock->roleSpecificInfo.client.sslMutualAuthCerts = NULL;
        pSSLSock->roleSpecificInfo.client.numMutualAuthCert = 0;

        pSSLSock->buffers[bufIndex].length = (ubyte2)(sizeofHandshakeRecord + SSL_MEDIUMSIZE);

        bufIndex++;
        numBufs++;
    }

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
    if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_PSK_BIT)
        pskIdentityLength = 2 + pSSLSock->roleSpecificInfo.client.pskIdentityLength;
#endif

    /* client key exchange length */
    if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_RSA_BIT)
    {
#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__))
        sbyte4 cipherTextLen;
        RSAKey* pRSAKey;

        if (akt_rsa != pSSLSock->roleSpecificInfo.client.publicKey.type)
        {
            status = ERR_SSL_INVALID_KEY_TYPE;
            goto exit;
        }

        pRSAKey = pSSLSock->roleSpecificInfo.client.publicKey.key.pRSA;
        /* allocate buffer big enough for the handshake record */
        if (OK > (status = RSA_getCipherTextLength(pRSAKey, &cipherTextLen)))
            goto exit;

        /* allocate the handshake record */
#if MIN_SSL_MINORVERSION <= SSL3_MINORVERSION
        pSSLSock->buffers[bufIndex].length = (ubyte2)((sizeofHandshakeRecord + cipherTextLen) +
                                                      ((SSL3_MINORVERSION < pSSLSock->sslMinorVersion) ? 2 : 0) +
                                                      pskIdentityLength);
#else
        pSSLSock->buffers[bufIndex].length = (ubyte2)((sizeofHandshakeRecord + cipherTextLen) +
                                                      2 + pskIdentityLength);
#endif

#else
        status = ERR_SSL_CONFIG;
        goto exit;
#endif
    }
#if (defined(__ENABLE_MOCANA_SSL_ECDH_SUPPORT__))
    else if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_ECDH_BIT)
    {
        sbyte4 publicKeyLen;
        ECCKey* pECCKey;

        if (akt_ecc != pSSLSock->roleSpecificInfo.client.publicKey.type)
        {
            status = ERR_SSL_INVALID_KEY_TYPE;
            goto exit;
        }

        pECCKey = pSSLSock->roleSpecificInfo.client.publicKey.key.pECC;
        if (OK > ( status = EC_getPointByteStringLen( pECCKey->pCurve, &publicKeyLen)))
            goto exit;

        /* add one byte length + length of public key + sizeof HandshakeRecord */
        pSSLSock->buffers[bufIndex].length = (ubyte2) (sizeofHandshakeRecord + 1 + publicKeyLen);
    }
#endif
#if (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_ECDH_ANON_BIT__) )
    else if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_ECDHE_BIT)
    {
        sbyte4 publicKeyLen;
        ECCKey* pECCKey;

        if (akt_ecc != pSSLSock->ecdheKey.type)
        {
            status = ERR_SSL_INVALID_KEY_TYPE;
            goto exit;
        }

        pECCKey = pSSLSock->ecdheKey.key.pECC;
        if (OK > ( status = EC_getPointByteStringLen( pECCKey->pCurve, &publicKeyLen)))
            goto exit;

        /* add one byte length + length of public key + sizeof SSLHandshakeRecord */
        pSSLSock->buffers[bufIndex].length = (ubyte2) (sizeofHandshakeRecord + 1 + publicKeyLen + pskIdentityLength);
    }
#endif
#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
    else if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_KEYEX_DH_BIT)
    {
        ubyte4              lenYc;

        if (OK > (status = DH_computeKeyExchange(MOC_DH(pSSLSock->hwAccelCookie) pSSLSock->pDHcontext, &pVlongQueue)))
            goto exit;

        lenYc = (7 + VLONG_bitLength(pSSLSock->pDHcontext->dh_f)) / 8;
        pSSLSock->buffers[bufIndex].length = (ubyte2)(sizeofHandshakeRecord + 2 + lenYc + pskIdentityLength);
    }
#endif
#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
    else if (SSL_PSK == pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags)
    {
        pSSLSock->buffers[bufIndex].length = (ubyte2)(sizeofHandshakeRecord + pskIdentityLength);
    }
#endif
    else
    {
        status = ERR_SSL_CONFIG;
        goto exit;
    }

#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
    if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_PSK_BIT)
        pskIdentityLength = 2 + pSSLSock->roleSpecificInfo.client.pskIdentityLength;
#endif

    bufIndex++;
    numBufs++;

    /* certificate verify length */
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    if (pSSLSock->isMutualAuthNegotiated)
    {
        ubyte4 lenSignature;

        /* extract key information: either the key is provided by sskMutualAuthCert or we
            will use a user callback to encrypt with an unaccessible key (on hardware) */
        if (pSSLSock->mutualAuthKey.type == akt_undefined)
        {
            /* figure out the signature length from the cert public key */
            MemFile certMemFile;
            CStream cs;
            ubyte*  pTempCert = pSSLSock->roleSpecificInfo.client.sslMutualAuthCerts[0].data;
            ubyte4  certLen = pSSLSock->roleSpecificInfo.client.sslMutualAuthCerts[0].length;

            /* parse the cert */
            MF_attach(&certMemFile, certLen, pTempCert);
            CS_AttachMemFile(&cs, &certMemFile );

            if (OK > (status = ASN1_Parse( cs, &pCertificate)))
            {
                DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() returns status = ", (sbyte4)status);
                DEBUG_ERROR(DEBUG_SSL_MESSAGES, (sbyte*)"ASN1_Parse() @ line # ", (sbyte4)__LINE__);
                goto exit;
            }
            /* get the public key */
            if (OK > ( status = CERT_setKeyFromSubjectPublicKeyInfo(
                        MOC_RSA(pSSLSock->hwAccelCookie) pCertificate, cs, &pSSLSock->mutualAuthKey)))
            {
                goto exit;
            }
        }

        switch ( pSSLSock->mutualAuthKey.type)
        {
        case akt_rsa:
            lenSignature = ((7 + VLONG_bitLength(RSA_N(pSSLSock->mutualAuthKey.key.pRSA))) / 8);
            break;

#ifdef __ENABLE_MOCANA_ECC__
        case akt_ecc:
            /* enough space for a SEQUENCE of 2 integers */
            if (OK > ( status = SSL_SOCK_getECCSignatureLength( pSSLSock->mutualAuthKey.key.pECC,
                                                               (sbyte4*) &lenSignature)))
            {
                goto exit;
            }
            break;
#endif

        default:
            status = ERR_SSL_UNSUPPORTED_ALGORITHM;
            goto exit;
        }

        pSSLSock->buffers[bufIndex].length = (ubyte2)(sizeofHandshakeRecord + 2 + lenSignature);

        if ( (pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
             (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION)  )
        {
            pSSLSock->buffers[bufIndex].length += 2;
        }
        bufIndex++;
        numBufs++;
    }
#endif

    /* allocate enough space for all records */
    totalLen =  pSSLSock->buffers[0].length + pSSLSock->buffers[1].length +
                    pSSLSock->buffers[2].length + pSSLSock->buffers[3].length +
                    pSSLSock->buffers[4].length;

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        fragmentLen = calculateExtraFragmentHeader(pSSLSock, numBufs);
    }
#endif

    if (OK > (status = CRYPTO_ALLOC(pSSLSock->hwAccelCookie,
                                    totalLen + fragmentLen + TLS_EAP_PAD, TRUE,
                                    (void **)&(pSSLSock->buffers[0].pHeader))))  /* EAP TEAM, why the pad? */
    {
        goto exit;
    }

    /* initialize other pointers */
    pSSLSock->buffers[1].pHeader = pSSLSock->buffers[0].pHeader + pSSLSock->buffers[0].length;
    pSSLSock->buffers[2].pHeader = pSSLSock->buffers[1].pHeader + pSSLSock->buffers[1].length;
    pSSLSock->buffers[3].pHeader = pSSLSock->buffers[2].pHeader + pSSLSock->buffers[2].length;
    pSSLSock->buffers[4].pHeader = pSSLSock->buffers[3].pHeader + pSSLSock->buffers[3].length;

    pSSLSock->buffers[0].data = pSSLSock->buffers[0].pHeader + sizeofRecordHeader;
    pSSLSock->buffers[1].data = pSSLSock->buffers[1].pHeader + sizeofRecordHeader;
    pSSLSock->buffers[2].data = pSSLSock->buffers[2].pHeader + sizeofRecordHeader;
    pSSLSock->buffers[3].data = pSSLSock->buffers[3].pHeader + sizeofRecordHeader;
    pSSLSock->buffers[4].data = pSSLSock->buffers[4].pHeader + sizeofRecordHeader;

    /* update record headers */
    for (bufIndex = 0; bufIndex < numBufs; bufIndex++)
    {
        pSRH = pSSLSock->buffers[bufIndex].pHeader;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
        if (pSSLSock->isDTLS)
        {
            DTLS_SET_RECORD_HEADER_EXT(pSRH, pSSLSock, SSL_HANDSHAKE,
                                       pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
        } else
#endif
        {
            SSL_SET_RECORD_HEADER(pSRH, SSL_HANDSHAKE, pSSLSock->sslMinorVersion,
                                  pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);
        }
    }

    bufIndex = 0;

    /* certificate */
    if ((pSSLSock->isMutualAuthNegotiated) || (pSSLSock->generateEmptyCert))
    {
        if (OK > (status = fillClientCertificate(pSSLSock, pSSLSock->buffers[bufIndex].data, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofRecordHeader))))
            goto exit;

        bufIndex++;
    }

    /* client key exchange */
    if (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->FillClientKEX)
    {
        if (OK > (status = pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->FillClientKEX(pSSLSock, pSSLSock->buffers[bufIndex].data, (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofRecordHeader), &pVlongQueue))) /* will also call generateKeyMaterial */
            goto exit;
    }
    else
    {
        status = ERR_SSL_CONFIG;
        goto exit;
    }

    bufIndex++;

    /* certificate verify */
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    if ((pSSLSock->isMutualAuthNegotiated) && (0 != pSSLSock->roleSpecificInfo.client.numMutualAuthCert))
    {
        /* unfortunately the ECC signature length cannot be computed in advance precisely since
            it's a SEQUENCE of 2 INTEGERs. And INTEGER will have a zero prefix depending on their value
            it's illegal to add a 0 octet if the first bit is 0.
            So we need to adjust the length...
        */
        ubyte2 length, initLength;

        length = initLength = (ubyte2)(pSSLSock->buffers[bufIndex].length - sizeofRecordHeader);

        if (OK > (status = fillCertificateVerify(pSSLSock->roleSpecificInfo.client.mutualAuthSignAlgo,
                                                 pSSLSock, pSSLSock->buffers[bufIndex].data,
                                                 &length, &pVlongQueue)))
        {
            goto exit;
        }

        if (length != initLength)
        {
            /* adjust pointers, length and headers ... */
            totalLen -= (initLength - length);
            pSSLSock->buffers[bufIndex].length = (ubyte2) (length + sizeofRecordHeader);

            pSRH = pSSLSock->buffers[bufIndex].pHeader;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
            if (pSSLSock->isDTLS)
            {
                DTLS_SET_RECORD_HEADER_EXT(pSRH,pSSLSock,SSL_HANDSHAKE,length);
            }
            else
#endif
            {
                SSL_SET_RECORD_HEADER(pSRH, SSL_HANDSHAKE, pSSLSock->sslMinorVersion, length);
            }
            /* it's fortunate that the certificate verify is the last message
            there's nothing else to update */
        }

        bufIndex++;
    }
#endif

    if (TRUE != isRehandshake)
    {
        /* send the handshakes records to SSL record layer *************/
        if (SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)
        {
            numBytesSent  = 0;
            status = totalLen;
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
            pSSLSock->outputBufferSize   = totalLen + fragmentLen;
            pSSLSock->numBytesToSend     = totalLen + fragmentLen - numBytesSent;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
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

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    if (pCertificate)
    {
        TREE_DeleteTreeItem( (TreeItem*) pCertificate);
    }
#endif

    if ((TRUE != isRehandshake) || (OK > status))
    {
        /* free the buffers */
        if (pSSLSock->buffers[0].pHeader)
        {
            CRYPTO_FREE(pSSLSock->hwAccelCookie, TRUE, (void **)&(pSSLSock->buffers[0].pHeader));
        }
    }

    VLONG_freeVlongQueue(&pVlongQueue);

    return status;

} /* SSL_CLIENT_sendClientResponseBlock */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_RFC3546__) || defined(__ENABLE_TLSEXT_RFC6066__))
static MSTATUS
processServerHelloExtensions(SSLSocket* pSSLSock, ubyte2 extensionType,
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
            ubyte4  verifyDataSize = (SSL3_MINORVERSION == pSSLSock->sslMinorVersion) ?
                                    SSL_VERIFY_DATA : TLS_VERIFYDATASIZE;
#else
            ubyte4  verifyDataSize = TLS_VERIFYDATASIZE;
#endif

            pSSLSock->isRehandshakeExtPresent = TRUE;

            if (0 < pSSLSock->handshakeCount)
            {
                /* TLS 1.0, 1.1 and 1.2 */
                expectedRenegotiationLength = (2 * verifyDataSize);
            }

            if (expectedRenegotiationLength != renegotiatedConnLength)
            {
                status = ERR_SSL_CLIENT_RENEGOTIATE_LENGTH;
                goto exit;
            }

            if (0 < expectedRenegotiationLength)
            {
                if (OK > (status = MOC_MEMCMP(1 + pExtension, pSSLSock->client_verify_data, verifyDataSize, &result)))
                    goto exit;

                if (0 != result)
                {
                    status = ERR_SSL_CLIENT_RENEGOTIATE_CLIENT_VERIFY;
                    goto exit;
                }

                if (OK > (status = MOC_MEMCMP(verifyDataSize + 1 + pExtension, pSSLSock->server_verify_data, verifyDataSize, &result)))
                    goto exit;

                if (0 != result)
                {
                    status = ERR_SSL_CLIENT_RENEGOTIATE_SERVER_VERIFY;
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
       /*
             * According to RFC3546/RFC 6066:
             * The "extension_data" field of this extension SHALL be empty.
             */
            if ( extensionSize != 0)
            {
                sbyte4 serverNameListLength;
                if ( extensionSize < 2)
                {
                    /* buffer overrun attack? */
                    status = ERR_SSL_EXTENSION_LENGTH;
                    goto exit;
                }

                /* if there is a name extension, it should be empty */
                serverNameListLength = getShortValue(pExtension);
                if ( serverNameListLength )
                {
                    status = ERR_SSL_EXTENSION_SERVER_NAME_LENGTH;
                    goto exit;
                }
            }
        }
        break;
#ifdef __ENABLE_TLSEXT_RFC6066__
        case tlsExt_status_request:
        {
            if (!pSSLSock->certStatusReqExt)
            {
                /* We are not expecting cert status req extension as we have
                               not send it in client hello */
                status = ERR_SSL_EXTENSION_CERTIFICATE_STATUS_UNSOLICITED;
                goto exit;
            }
                /*
                 * According to RFC6066:
                 * The "extension_data" field of this extension SHALL be empty.
                 */
            sbyte4 extDataLen = 0;
            if ( extensionSize != 0)
            {
                if ( extensionSize < 2)
                {
                    /* buffer overrun attack? */
                    status = ERR_SSL_EXTENSION_LENGTH;
                    goto exit;
                }

                if ( extDataLen )
                {
                    status = ERR_SSL_EXTENSION_CERTIFICATE_STATUS_LENGTH;
                    goto exit;
                }
            }
            pSSLSock->roleSpecificInfo.client.didRecvCertStatusExtInServHello = TRUE;
        }
        break;
#endif
        case tlsExt_ticket:
        {
            sbyte4 ticketLength;

            if (extensionSize < 2)
            {
                /* buffer overrun attack? */
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }

            /* the ticket should always be empty in ServerHello */
            ticketLength = getShortValue(pExtension);
            if ( ticketLength)
            {
                status = ERR_SSL_EXTENSION_SERVER_NAME_LENGTH;
                goto exit;
            }
            /* this extension was sent by the server
            so it indicates that a NewSessionTicket Handshake msg
            will be sent -> do we need to take note of that?*/
        }
        break;

#if defined(__ENABLE_MOCANA_DTLS_SRTP__)
        case dtlsExt_use_srtp:
        {
            sbyte4 srtpProfileLength;
            ubyte4 srtpMkiLength;
            ubyte2 serverProfileId;
            ubyte4 i;

            if ( extensionSize < 2)
            {
                /* buffer overrun attack? */
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }

            srtpProfileLength = getShortValue(pExtension);
            pExtension += 2;
            extensionSize -= 2;

            if ( srtpProfileLength + 1 > extensionSize)
            {
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }

            if (srtpProfileLength>>1 != 1) /* server should have chosen one profile */
            {
                status = ERR_SSL_EXTENSION_LENGTH; /* TODO: should change to failure to negotiate */
                goto exit;
            }

            /* record profile picked by server */
            /* cipher id */
            serverProfileId = ((ubyte2)(*pExtension++) << 8);
            serverProfileId |= *pExtension++;
            extensionSize -= 2;

            pSSLSock->pHandshakeSrtpProfile = NULL;

            for ( i = 0; i < (sbyte4)NUM_SRTP_PROFILES; ++i)
            {
                if ((gSrtpProfiles[i].supported) &&
                    (gSrtpProfiles[i].profileId == serverProfileId))
                {
#ifdef __ENABLE_MOCANA_SRTP_PROFILES_SELECT__
                    if ((TRUE != pSSLSock->isSrtpProfileTableInit) ||
                        (TRUE == pSSLSock->isSrtpProfileEnabled[i]) )
#endif
                    {
                        pSSLSock->pHandshakeSrtpProfile = gSrtpProfiles + i;
                        break;
                    }
                }
            }

            if (NULL == pSSLSock->pHandshakeSrtpProfile)
            {
                return ERR_DTLS_SRTP_NO_PROFILE_MATCH;
            }

            /* srtp_mki */
            if (extensionSize < 1)
            {
                /* buffer overrun attack? */
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }
            srtpMkiLength = (ubyte4)(*pExtension);
            pExtension += 1;
            extensionSize -= 1;

            if ((srtpMkiLength > extensionSize))
            {
                status = ERR_SSL_EXTENSION_LENGTH;
                goto exit;
            }

            status = ERR_DTLS_SRTP_EXTENSION_FAILURE;

            /* check to see it's the same as recorded one */
            if (NULL != pSSLSock->srtpMki)
            {
                sbyte4 resCmp;

                if (srtpMkiLength != (ubyte4)*(pSSLSock->srtpMki))
                    goto exit;

                MOC_MEMCMP(pSSLSock->srtpMki + 1, pExtension, srtpMkiLength, &resCmp);

                /* mki matched */
                if (0 == resCmp)
                {
                    status = OK;
                }
            }
            else
            {
                /* empty mki */
                if (0 == srtpMkiLength)
                    status = OK;
            }

            if (OK > status)
                goto exit;
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
    }

    if (NULL != SSL_sslSettings()->funcPtrExtensionApprovedCallback)
    {
        status = (MSTATUS)SSL_sslSettings()->funcPtrExtensionApprovedCallback(
                    SSL_findConnectionInstance(pSSLSock), extensionType, NULL, 0);
    }

exit:
    return status;
}
#endif /* defined(__ENABLED_RFC3546__) */


/*------------------------------------------------------------------*/

static MSTATUS
processServerHello(SSLSocket* pSSLSock, ubyte* pSHSH, ubyte2 recLen)
{
    sbyte4  i;
    ubyte   sessionIdLen;
    ubyte*  pTemp;
    ubyte   serverSSLMajorVersion;
    ubyte   serverSSLMinorVersion;
    MSTATUS status;
    ubyte2  serverCipherId;
    ubyte4  sizeofHandshakeHeader;

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
    }

    /* first bytes are the version picked by the server */
    pTemp = (ubyte*) (pSHSH+sizeofHandshakeHeader);
    serverSSLMajorVersion = *pTemp++;
    serverSSLMinorVersion = *pTemp++;

    /* validate the versions picked by server */
    if ((pSSLSock->isDTLS && !VALID_DTLS_VERSION(serverSSLMajorVersion, serverSSLMinorVersion)) ||
        (!pSSLSock->isDTLS && !VALID_SSL_VERSION(serverSSLMajorVersion, serverSSLMinorVersion)) ||
        ((pSSLSock->runtimeFlags & SSL_FLAG_VERSION_SET) && pSSLSock->advertisedMinorVersion != serverSSLMinorVersion) ||
        ((pSSLSock->isDTLS && pSSLSock->runtimeFlags & SSL_FLAG_MINIMUM_FALLBACK_VERSION_SET) && pSSLSock->minFallbackMinorVersion < serverSSLMinorVersion) ||
        ((pSSLSock->runtimeFlags & SSL_FLAG_MINIMUM_FALLBACK_VERSION_SET) && pSSLSock->minFallbackMinorVersion > serverSSLMinorVersion))

    {
        return ERR_SSL_PROTOCOL_VERSION;
    }

    pSSLSock->sslMinorVersion = serverSSLMinorVersion;

#ifdef __ENABLE_ALL_DEBUGGING__
    DEBUG_PRINT(DEBUG_SSL_TRANSPORT, (sbyte *)"SSL version = ");
    DEBUG_INT(DEBUG_SSL_TRANSPORT, pSSLSock->sslMinorVersion);
    DEBUG_PRINTNL(DEBUG_SSL_TRANSPORT, (sbyte *)"");
#endif
    /* SERVER RANDOM VALUE in its usual place */
    MOC_MEMCPY(pSSLSock->pSecretAndRand + SSL_RSAPRESECRETSIZE + SSL_RANDOMSIZE,
            pTemp,
            SSL_RANDOMSIZE);
    MOC_MEMCPY(pSSLSock->pServerRandHello, pTemp, SSL_RANDOMSIZE);
    pTemp += SSL_RANDOMSIZE;
    /* SESSION ID */
    pSSLSock->sessionResume = E_NoSessionResume;
    sessionIdLen = *pTemp++;
    if ( sessionIdLen > SSL_MAXSESSIONIDSIZE)
    {
        /* too suspect */
        return ERR_SSL_PROTOCOL_PROCESS_SERVER_HELLO;
    }

    if ((0 < sessionIdLen) && (sessionIdLen == pSSLSock->roleSpecificInfo.client.sessionIdLen))
    {
        /* could it be the same session id we requested in client hello? */
        sbyte4 memcmpRes;

        status = MOC_MEMCMP(pTemp, pSSLSock->roleSpecificInfo.client.sessionId,
                            sessionIdLen, &memcmpRes);
        if (OK > status)
            goto exit;

        if ( 0 == memcmpRes)
        {
            pSSLSock->sessionResume = E_SessionIDResume; /* another check is needed for this - done below */
        }
    }
    /* if the session id is different copy it to our buffer (otherwise already identical) */

    if ( E_NoSessionResume == pSSLSock->sessionResume)
    {
        pSSLSock->roleSpecificInfo.client.sessionIdLen = sessionIdLen;
        /* size check above */
        MOC_MEMCPY( pSSLSock->roleSpecificInfo.client.sessionId, pTemp, sessionIdLen);
    }
    pTemp += sessionIdLen;

    /* cipher id */
    serverCipherId = ((ubyte2)(*pTemp++) << 8);
    serverCipherId |= *pTemp++;

    pSSLSock->pHandshakeCipherSuite = NULL;

    for ( i = 0; i < (sbyte4)NUM_CIPHER_SUITES; ++i)
    {
        if ((gCipherSuites[i].supported) &&
            (gCipherSuites[i].cipherSuiteId == serverCipherId))
        {
#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
            if ((TRUE != pSSLSock->isCipherTableInit) ||
                (TRUE == pSSLSock->isCipherEnabled[i]) )
#endif
#if defined (__ENABLE_MOCANA_CCM_8__) && defined (__ENABLE_MOCANA_SSL_ALERTS__)
            /* Check for anomalous server selecting this cipher though it's version is < TLS 1.2 */
                /* FIXME: ff 5/120/15: there's a function to do that */
            if ((0xC0AE == gCipherSuites[i].cipherSuiteId) &&
                (pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
            {
                SSLSOCK_sendAlert(pSSLSock, 0, SSL_ALERT_ILLEGAL_PARAMETER, SSLALERTLEVEL_FATAL);
                status = ERR_SSL_UNSUPPORTED_ALGORITHM;
                goto exit;
            }
#endif
            {
                pSSLSock->pHandshakeCipherSuite = gCipherSuites + i;
                break;
            }
        }
    }

    if (0 == pSSLSock->pHandshakeCipherSuite)
    {
        return ERR_SSL_PROTOCOL_PROCESS_SERVER_HELLO;
    }

    if ( E_SessionIDResume == pSSLSock->sessionResume )
    {
        /* if we resume session, put the master secret in its place for key generation */
        MOC_MEMCPY( pSSLSock->pSecretAndRand,
                pSSLSock->roleSpecificInfo.client.pMasterSecret,
                SSL_MASTERSECRETSIZE);

        /* the handshake state is now logically  SSL_SERVER_HELLO_DONE */
        SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) =  SSL_SERVER_HELLO_DONE;
    }
#ifdef __ENABLE_MOCANA_EAP_FAST__
    else if ( pSSLSock->roleSpecificInfo.client.ticket &&
        pSSLSock->roleSpecificInfo.client.ticketLength)
    {
        /* for EAP_FAST, use the pacKey to derive the master secret if
            we don't resume the session */
        if (OK > (status = SSL_SOCK_generateEAPFASTMasterSecret( pSSLSock)))
            goto exit;

        pSSLSock->sessionResume = E_SessionEAPFASTResume;
        SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = SSL_SERVER_HELLO_DONE;
    }
#endif
    else
    {
        SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) =  SSL_SERVER_HELLO;
    }

    /* skip past compression choice */
    ++pTemp;

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    pSSLSock->isRehandshakeExtPresent = FALSE;
#endif


#if (defined( __ENABLE_RFC3546__) || defined(__ENABLE_TLSEXT_RFC6066__)) && !defined(__ENABLE_MOCANA_EAP_FAST__)
    /* extension is only supported in TLS
       but minimal support is added in SSL 3.0 for secure renegotiation */
    /* if we successfully found a cipher suite, and the version is TLS 1.0+ proceed */
    if ( pTemp + 2 - (ubyte*)(pSHSH+sizeofHandshakeHeader) < recLen)
    {
        /* check the extension length */
        ubyte4 extensionsLength = getShortValue(pTemp);
        pTemp += 2;
        if ( pTemp + extensionsLength - (ubyte*)(pSHSH+sizeofHandshakeHeader) <= recLen)
        {

            if (OK > (status = processHelloExtensions(pSSLSock, pTemp, extensionsLength)))
                goto exit;
        }
    }

#ifdef __ENABLE_TLSEXT_RFC6066__
    /* Call back in case of certificate status extension */
    if (NULL != SSL_sslSettings()->funcPtrCertStatusCallback)
    {
        intBoolean certStatus = FALSE;

        /* Check if server responds to cert_status extension */
        if (pSSLSock->certStatusReqExt &&
                pSSLSock->roleSpecificInfo.client.didRecvCertStatusExtInServHello)
        {
            certStatus = TRUE;
        }
        status = (MSTATUS)SSL_sslSettings()->funcPtrCertStatusCallback(
                    SSL_findConnectionInstance(pSSLSock), certStatus);
    }
#endif
#endif /* __ENABLE_RFC3546__ */

#if ((defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__)) && (!defined(__DISABLE_MOCANA_SSL_REHANDSHAKE_FIX__)))
    if (0 < pSSLSock->handshakeCount)
    {
        if ( (pSSLSock->isRehandshakeAllowed && !pSSLSock->isRehandshakeExtPresent) ||
             (!pSSLSock->isRehandshakeAllowed && pSSLSock->isRehandshakeExtPresent) )
        {
            status = ERR_SSL_CLIENT_RENEGOTIATE_ILLEGAL_EXTENSION;
            goto exit;
        }

        /* allow legacy (insecure) renegotiation only if session flag is set */
        if ( (!pSSLSock->isRehandshakeAllowed && !pSSLSock->isRehandshakeExtPresent) &&
             !(pSSLSock->runtimeFlags & SSL_FLAG_ALLOW_INSECURE_REHANDSHAKE) )
        {
            status = ERR_SSL_CLIENT_RENEGOTIATE_NOT_ALLOWED;
            goto exit;
        }
    }
#endif

#if defined(__ENABLE_MOCANA_DTLS_CLIENT__) && defined(__ENABLE_MOCANA_DTLS_SRTP__)
    if (pSSLSock->useSrtp && NULL == pSSLSock->pHandshakeSrtpProfile)
    {
        return ERR_DTLS_SRTP_EXTENSION_FAILURE;
    }
#endif

    if (OK > (status = SSL_SOCK_initHashPool(pSSLSock)))
        goto exit;

    status = OK;

exit:
    return status;
}


/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
static MSTATUS
processCertificateRequest(SSLSocket* pSSLSock, ubyte* pSHSH, ubyte2 recLen)
{
    ubyte4          lenTotal;
    ubyte2          lenCertTypes;
    ubyte2          lenSigHashAlgos = 0;
    ubyte2          lenDistinguisedNames;
    ubyte2          lenDistinguisedNamesSave;
    ubyte2          lenTempDistName;
    ubyte*          pStartCertTypes;
    ubyte*          pStartSigHashAlgos = NULL;
    ubyte*          pStartDistNames;
    ubyte*          pDistNameEntry;
    ubyte4          sizeofHandshakeHeader;
    MSTATUS         status = ERR_SSL_INVALID_CERT_REQUEST_MSG_SIZE;

    if (0 == recLen)
        goto exit;

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        sizeofHandshakeHeader = sizeof(DTLSHandshakeHeader);
    } else
#endif
    {
        sizeofHandshakeHeader = sizeof(SSLHandshakeHeader);
    }

    pDistNameEntry = (ubyte *)(pSHSH + sizeofHandshakeHeader);

    lenCertTypes = (ubyte2)(*pDistNameEntry);               pDistNameEntry += 1;

    pStartCertTypes = pDistNameEntry;

    lenTotal = (ubyte4)(1 + lenCertTypes);

    if (lenTotal > recLen)
        goto exit;

    pDistNameEntry += lenCertTypes;

    /* tls1.2 added SignatureAndHashAlgorithms after certType */
    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
    {
        lenSigHashAlgos = getShortValue(pDistNameEntry);   pDistNameEntry += 2;

        pStartSigHashAlgos = pDistNameEntry;

        pDistNameEntry += lenSigHashAlgos;
    }
    lenDistinguisedNames = getShortValue(pDistNameEntry);   pDistNameEntry += 2;

    lenTotal = lenTotal + 2;

    if ((lenDistinguisedNames + lenTotal) > recLen)
        goto exit;

    pStartDistNames          = pDistNameEntry;
    lenDistinguisedNamesSave = lenDistinguisedNames;

    /* validate array of distinguished names */
    while (lenDistinguisedNames)
    {
        lenTempDistName = getShortValue(pDistNameEntry);    pDistNameEntry += 2;

        lenTotal = lenTotal + 2 + lenTempDistName;

        if (lenTotal > recLen)
            goto exit;

        pDistNameEntry += lenTempDistName;

        if ((2 + lenTempDistName) > lenDistinguisedNames)
            goto exit;

        lenDistinguisedNames = (ubyte2)(lenDistinguisedNames - (2 + lenTempDistName));
    }

    if (NULL == pSSLSock->pCertStore && NULL != SSL_sslSettings()->funcPtrMutualAuthGetCertificate)
    {
        /* fetch certificate store if not yet assigned */
        if (OK > (status = (MSTATUS)SSL_sslSettings()->funcPtrMutualAuthGetCertificate(SSL_findConnectionInstance(pSSLSock),
                                                                                       pStartDistNames, (ubyte4)lenDistinguisedNamesSave,
                                                                                       pStartSigHashAlgos, (ubyte4)lenSigHashAlgos)) )
        {
            goto exit;
        }
    }

    /* fetch certificate for future reference */
    /* after fetching the cert we check against signature algos offered by server */
    if (NULL != pSSLSock->pCertStore)
    {
        ubyte4 pubKeyType;
        ubyte4 certStoreAlgoFlags = 0;
        ubyte4 offset = 0;
        AsymmetricKey *pPrivateKey = 0;
        ubyte4 certStoreAlgoFlagsWithSignKeyType;

        /* convert eccurve and server supported sig algos in CERT_STORE_ALGO_FLAGS */
        convertToCertStoreFlags(pSSLSock, SUPPORTED_CURVES_FLAGS,
                pStartSigHashAlgos, lenSigHashAlgos, &certStoreAlgoFlags);

        do {
            /* reinitiate certDescriptor */
            /* TODO: we should release memory of the cert and key? */

            /* convert certificateType to keyType */
            switch (pStartCertTypes[offset++])
            {
            case tlsClientCertType_rsa_sign:
                pubKeyType = akt_rsa;
                certStoreAlgoFlagsWithSignKeyType = (certStoreAlgoFlags | CERT_STORE_ALGO_FLAG_RSA);
                break;

            case tlsClientCertType_ecdsa_sign:
            case tlsClientCertType_ecdsa_fixed_ecdh:
                pubKeyType = akt_ecc;
                certStoreAlgoFlagsWithSignKeyType = (certStoreAlgoFlags | CERT_STORE_ALGO_FLAG_RSA);
                break;

            case tlsClientCertType_rsa_fixed_ecdh:
                pubKeyType = akt_ecc;
                certStoreAlgoFlagsWithSignKeyType = (certStoreAlgoFlags | CERT_STORE_ALGO_FLAG_RSA);
                break;

            default:
                continue; /* keep going */
            }

            if (OK > (status = (MSTATUS)CERT_STORE_findIdentityCertChainFirst(pSSLSock->pCertStore, pubKeyType,
                                                            certStoreAlgoFlagsWithSignKeyType,
                                                            &pPrivateKey, &pSSLSock->roleSpecificInfo.client.sslMutualAuthCerts,
                                                            (ubyte4*) &pSSLSock->roleSpecificInfo.client.numMutualAuthCert, NULL)))
            {
                continue; /* not found -- keep going */
            }

            if (0 != pSSLSock->roleSpecificInfo.client.numMutualAuthCert)
                break;    /* use the first certificate found */

        } while (offset < lenCertTypes); /* while there is the next certificateType */

        if (0 == pSSLSock->roleSpecificInfo.client.numMutualAuthCert)
        {
            /* send empty cert if no valid certificate is found */
            pSSLSock->generateEmptyCert = TRUE;
            status = OK;
            goto exit;
        }

        CRYPTO_copyAsymmetricKey(&pSSLSock->mutualAuthKey, pPrivateKey);

        if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion <= DTLS12_MINORVERSION)) ||
            (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion >= TLS12_MINORVERSION))
        {
            ubyte4  certSignatureAlgo = 0;
            ubyte2  serverSigAlgo;
            ubyte4  i;

#ifndef __DISABLE_MOCANA_CERTIFICATE_PARSING__
            /* get leaf certificate signature algorithm */
            if (OK > (status = getCertSigAlgo(pSSLSock->roleSpecificInfo.client.sslMutualAuthCerts[0].data,
                                              pSSLSock->roleSpecificInfo.client.sslMutualAuthCerts[0].length,
                                             &certSignatureAlgo)))
            {
                goto exit;
            }
#endif

            status = ERR_SSL_UNSUPPORTED_ALGORITHM;

            /* pick a hash/sig pair from signature Algo extension */
            if (0 < lenSigHashAlgos)
            {
                for (i = 0; i < lenSigHashAlgos; i += 2)
                {
                    serverSigAlgo = pStartSigHashAlgos[i] << 8 | pStartSigHashAlgos[i+1];

                    if (serverSigAlgo == certSignatureAlgo)
                    {
                        status = OK;
                        break;
                    }
                }
            }
            else
            {
                switch (pPrivateKey->type)
                {
                    case akt_rsa:
                        serverSigAlgo = TLS_RSA;
                        break;
                    case akt_dsa:
                        serverSigAlgo = TLS_DSA;
                        break;
                    case akt_ecc:
                        serverSigAlgo = TLS_ECDSA;
                        break;
                    default:
                        status = ERR_SSL_UNSUPPORTED_ALGORITHM;
                        goto exit;
                }

                serverSigAlgo = TLS_SHA1 << 8 | serverSigAlgo;

                if (serverSigAlgo == certSignatureAlgo)
                {
                    status = OK;
                }
            }

            if (OK <= status)
            {
                pSSLSock->roleSpecificInfo.client.mutualAuthSignAlgo = (ubyte2) certSignatureAlgo;
            }
            /*
            if (OK_SSL_USE_EMPTY_CERT == status)
            {
                pSSLSock->generateEmptyCert = TRUE;
                status = OK;
                goto exit;
            }
            */
        } /* TLS 1.2 */
    }
    else
    {
        pSSLSock->generateEmptyCert = TRUE;
        status = OK;
        goto exit;
    }

    pSSLSock->isMutualAuthNegotiated = TRUE;
    pSSLSock->generateEmptyCert = FALSE;
    status = OK;

exit:
    return status;

} /* processCertificateRequest */
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)))
static MSTATUS
verifyServerKeyExchangeRSASignature(SSLSocket* pSSLSock, ubyte* pMesg,
                                    ubyte2 msgLen, ubyte2 dataLen,
                                    vlong** ppVlongQueue)
{
    ubyte*      pHashResult = NULL;
    ubyte2      sigLen;
    ubyte4      cipherLen;
    sbyte4      cmpRes;
    RSAKey*     pRSAKey;
    ubyte*      pDecrypt = 0;
    MSTATUS     status;
    ubyte4      hashLen = 0;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult))))
        goto exit;

    /* verify signature length */
    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        sigLen = getShortValue( pMesg + dataLen);
        /* total msg len = 2 + sigLen + dataLen */
        if ( sigLen + 2 + dataLen != msgLen)
        {
            status = ERR_SSL_INVALID_MSG_SIZE;
            goto exit;
        }
    }
    else
    {
        /* TLS1.2 is sigAlgo+signature */
        sigLen = getShortValue( pMesg + dataLen + 2);
        /* total msg len = 2 + 2+ sigLen + dataLen */
        if ( sigLen + 2 + 2 + dataLen != msgLen)
        {
            status = ERR_SSL_INVALID_MSG_SIZE;
            goto exit;
        }
    }

    /* key type should have been verified before calling this function */
    pRSAKey = pSSLSock->roleSpecificInfo.client.publicKey.key.pRSA;

    if (OK > ( status = RSA_getCipherTextLength( pRSAKey, (sbyte4*) &cipherLen)))
        goto exit;

    if (cipherLen != sigLen)
    {
        status = ERR_SSL_INVALID_SIGNATURE;
        goto exit;
    }

    /* verify signature matches */
    if (NULL == (pDecrypt = MALLOC(cipherLen)))
    {
        status = ERR_MEM_ALLOC_FAIL;
        goto exit;
    }

    /* decrypt the signature which is at pMesg + dataLen + 2 */
    if (OK > (status = RSA_verifySignature(MOC_RSA(pSSLSock->hwAccelCookie) pRSAKey,
                                            pMesg + msgLen - sigLen, pDecrypt,
                                            (ubyte4*) &cipherLen, ppVlongQueue)))
    {
        goto exit;
    }

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        MD5_CTX*    pMd5Context = NULL;
        SHA1_CTX*   pShaContext = NULL;

        hashLen = MD5_DIGESTSIZE + SHA_HASH_RESULT_SIZE;

        if (hashLen != cipherLen)
        {
            status = ERR_SSL_INVALID_SIGNATURE;
            goto exit1;
        }

        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Context))))
            goto exit;

        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->shaPool, (void **)(&pShaContext))))
            goto exit;

        /* compute the hash of the data */
        if (OK > (status = MD5Init_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context)))
            goto exit1;

        if (OK > (status = MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context, pSSLSock->pClientRandHello, SSL_RANDOMSIZE)))
            goto exit1;

        if (OK > (status = MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context, pSSLSock->pServerRandHello, SSL_RANDOMSIZE)))
            goto exit1;

        if (OK > (status = MD5Update_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context, pMesg, dataLen)))
            goto exit1;

        if (OK > (status = MD5Final_m(MOC_HASH(pSSLSock->hwAccelCookie) pMd5Context, pHashResult)))
            goto exit1;

        if (OK > (status = SHA1_initDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext)))
            goto exit1;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pSSLSock->pClientRandHello, SSL_RANDOMSIZE)))
            goto exit1;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pSSLSock->pServerRandHello, SSL_RANDOMSIZE)))
            goto exit1;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pMesg, dataLen)))
            goto exit1;

        if (OK > (status = SHA1_finalDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pHashResult + MD5_DIGESTSIZE )))
            goto exit1;
exit1:
        MEM_POOL_putPoolObject(&pSSLSock->md5Pool, (void **)(&pMd5Context));
        MEM_POOL_putPoolObject(&pSSLSock->shaPool, (void **)(&pShaContext));

    } else
    {
        /* For TLS1.2, the hash algo comes from the sigAlgo ubyte2 before the signature */
        ubyte2  sigAlgo;

        sigAlgo = getShortValue( pMesg + dataLen);

        if (OK > (status = calculateTLS12KeyExchangeRSASignature(pSSLSock, pMesg, dataLen,
                                                            sigAlgo, pHashResult, &hashLen)))
            goto exit;
    }

    MOC_MEMCMP(pDecrypt, pHashResult, hashLen, &cmpRes);

    status = (0 == cmpRes) ? OK : ERR_SSL_INVALID_SIGNATURE;

exit:
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult));

    if (pDecrypt)
    {
        FREE( pDecrypt);
    }

    return status;
}
#endif /* (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__))) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__))
static MSTATUS
verifyServerKeyExchangeECDSASignature(SSLSocket* pSSLSock, ubyte* pMesg,
                                    ubyte2 msgLen, ubyte2 dataLen)
{
    ubyte*          pHashResult = NULL;
    SHA1_CTX*       pShaContext = NULL;
    BulkCtx         pHashCtx = NULL;
    hashSuite       *pHashSuite = NULL;
    ubyte4          hashLen;
    ubyte2          sigLen;
    ubyte4          offset;
    MemFile         mf;
    CStream         cs;
    ASN1_ITEMPTR    pRootItem = 0;
    MSTATUS         status;

    if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult))))
        goto exit;

    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        hashLen = SHA1_RESULT_SIZE;
        if (OK > (status = MEM_POOL_getPoolObject(&pSSLSock->shaPool, (void **)(&pShaContext))))
            goto exit;

        /* compute the hash of the data */
        if (OK > (status = SHA1_initDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext)))
            goto exit;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pSSLSock->pClientRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pSSLSock->pServerRandHello, SSL_RANDOMSIZE)))
            goto exit;

        if (OK > (status = SHA1_updateDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pMesg, dataLen)))
            goto exit;

        if (OK > (status = SHA1_finalDigest(MOC_HASH(pSSLSock->hwAccelCookie) pShaContext, pHashResult)))
            goto exit;

    }
    else
    {
        /* For TLS1.2, the hash algo comes from the sigAlgo ubyte2 before the signature */
        ubyte2  sigAlgo;
        ubyte4 i;
        sigAlgo = getShortValue( pMesg + dataLen);

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

        if (OK > (status = pHashSuite->algo->updateFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pMesg, dataLen)))
            goto exit;

        if (OK > (status = pHashSuite->algo->finalFunc(MOC_HASH(pSSLSock->hwAccelCookie) pHashCtx, pHashResult )))
            goto exit;

        if (OK > ( status = pHashSuite->algo->freeFunc( MOC_HASH(pSSLSock->hwAccelCookie) &pHashCtx )))
            goto exit;
    }

    /* parse the ASN.1 embedded in the message after the ECDHE params */
    /* verify signature length */
    if ((pSSLSock->isDTLS && (pSSLSock->sslMinorVersion > DTLS12_MINORVERSION)) ||
        (!pSSLSock->isDTLS && pSSLSock->sslMinorVersion < TLS12_MINORVERSION))
    {
        sigLen = getShortValue( pMesg + dataLen);
        offset = 2 + dataLen;
        /* total msg len = 2 + sigLen + dataLen */
        if ( sigLen + 2 + dataLen != msgLen)
        {
            status = ERR_SSL_INVALID_MSG_SIZE;
            goto exit;
        }
    }
    else
    {
        /* TLS1.2 has signatureAndHashAlgorigthm before the signature */
        sigLen = getShortValue( pMesg + dataLen+2);
        offset =  2 + 2 + dataLen;
        /* total msg len = 2 + sigLen + dataLen */
        if ( sigLen + 2 + 2 + dataLen != msgLen)
        {
            status = ERR_SSL_INVALID_MSG_SIZE;
            goto exit;
        }
    }
    MF_attach( &mf, sigLen, pMesg + offset);
    CS_AttachMemFile( &cs, &mf);

    if (OK > ( status = ASN1_Parse( cs, &pRootItem)))
        goto exit;

    /* key type should have been verified before calling this function*/
    if (OK > ( status = CERT_verifyECDSASignature( ASN1_FIRST_CHILD( pRootItem), cs,
                                                    pSSLSock->roleSpecificInfo.client.publicKey.key.pECC,
                                                    hashLen, pHashResult)))
    {
        goto exit;
    }

exit:
    MEM_POOL_putPoolObject(&pSSLSock->smallPool, (void **)(&pHashResult));
    if (pSSLSock->isDTLS || pSSLSock->sslMinorVersion < TLS12_MINORVERSION)
    {
        if (pShaContext)
            MEM_POOL_putPoolObject(&pSSLSock->shaPool, (void **)(&pShaContext));
    }
    else
    {
        if (pHashCtx)
            pHashSuite->algo->freeFunc(MOC_HASH(pSSLSock->hwAccelCookie) &pHashCtx);
    }

    if ( pRootItem)
    {
        TREE_DeleteTreeItem( (TreeItem*) pRootItem);
    }

    return status;
}
#endif /* defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))
static MSTATUS
processServerKeyExchange(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen)
{
    /* Diffie-Hellman Ephemeral support */
    ubyte*      pParams = pMesg;
    ubyte*      pTemp;
    ubyte4      lenBigInt;
    ubyte4      bitLenP;
    ubyte4      lenTotal, paramLen;
    vlong*      pVlongQueue = NULL;
    MSTATUS     status;


    /* basic check for length -- previous check was incorrect --
    check only for the fields we are going to use now */
    if (mesgLen < 5) /* 2 2-byte length fields */
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    if (NULL != pSSLSock->pDHcontext)
        DH_freeDhContext(&pSSLSock->pDHcontext, &pVlongQueue);

    if (OK > (status = DH_allocateClient(MOC_DH(pSSLSock->hwAccelCookie)
                                         pSSLSock->rngFunArg,
                                         &(pSSLSock->pDHcontext),
                                         DH_GROUP_TBD)))
    {
        goto exit;
    }

    pTemp = pParams;

    /* extract length of p */
    lenBigInt = getShortValue(pTemp);
    pTemp += 2;
    lenTotal = (2 + lenBigInt);

    if (lenTotal > mesgLen)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    /* extract p */
    if (OK > (status = VLONG_vlongFromByteString(pTemp, lenBigInt,
                                                 &(pSSLSock->pDHcontext->dh_p),
                                                 &pVlongQueue)))
    {
        goto exit;
    }

    /* check that the DH key is of sufficient size */
    bitLenP = VLONG_bitLength(pSSLSock->pDHcontext->dh_p);
    if (MIN_SSL_DH_SIZE - 1 > bitLenP)
    {
        status = ERR_SSL_DH_KEY_SIZE;
        goto exit;
    }

    /* extract length of g */
    pTemp += lenBigInt;
    lenBigInt = getShortValue(pTemp);
    pTemp += 2;
    lenTotal += (2 + lenBigInt);

    if (lenTotal > mesgLen)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    /* extract g */
    if (OK > (status = VLONG_vlongFromByteString(pTemp, lenBigInt,
                                                 &(pSSLSock->pDHcontext->dh_g),
                                                 &pVlongQueue)))
    {
        goto exit;
    }

    /* extract length of Ys */
    pTemp += lenBigInt;
    lenBigInt = getShortValue(pTemp);
    pTemp += 2;
    lenTotal += (2 + lenBigInt);
    paramLen = lenTotal;

    if (lenTotal > mesgLen)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    /* extract Ys */
    if (OK > (status = VLONG_vlongFromByteString(pTemp, lenBigInt,
                                                 &(pSSLSock->pDHcontext->dh_e),
                                                 &pVlongQueue)))
    {
        goto exit;
    }

#ifdef __ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__
    if (SSL_DH_ANON == pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags)
    {
        if (lenTotal != (ubyte4)mesgLen)
        {
            /* record length should match what we found */
            /* and the signature should not be longer than the prime used */
            status = ERR_SSL_INVALID_MSG_SIZE;
        }
        goto exit;
    }
#endif

    if (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_RSA_BIT)
    {
#if (defined(__ENABLE_MOCANA_SSL_RSA_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__))
        if (akt_rsa != pSSLSock->roleSpecificInfo.client.publicKey.type)
        {
            status = ERR_SSL_INVALID_KEY_TYPE;
            goto exit;
        }

        if (OK > ( status = verifyServerKeyExchangeRSASignature( pSSLSock, pMesg, mesgLen,
                                                                (ubyte2) paramLen, &pVlongQueue)))
        {
            goto exit;
        }
#else
        status = ERR_RSA_DISABLED;
#endif
    }

exit:
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, "processServerKeyExchange() returns status = ", status);

    VLONG_freeVlongQueue(&pVlongQueue);

    return status;

} /* processServerKeyExchange */
#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) || \
    defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) )
static MSTATUS
processServerEcdheKeyExchange(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen)
{
    /* Elliptic Curve Diffie-Hellman Ephemeral support */
    MSTATUS     status;
    ubyte2      curve;

    if (mesgLen < 5)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    /* first byte should be tlsECCurveType_named_curve */
    if (pMesg[0] != tlsECCurveType_named_curve)
    {
        status = ERR_SSL_UNSUPPORTED_CURVE;
        goto exit;
    }

    /* extract the key information */
    if (pMesg[3] + 4 > mesgLen)
    {
        status = ERR_SSL_INVALID_MSG_SIZE;
        goto exit;
    }

    /* initialize the ephemeral key */
    curve = getShortValue( pMesg + 1);

#ifdef __ENABLE_MOCANA_SSL_CIPHER_SUITES_SELECT__
    /* verify that the curve is one of the authorized one */
    if ( 0 == (( 1 << curve) & pSSLSock->eccCurves) )
    {
        status = ERR_SSL_UNSUPPORTED_CURVE;
        goto exit;
    }
#endif

    if (OK > ( status = CRYPTO_setECCParameters( &pSSLSock->ecdheKey,
                                SSL_getCurveIdOfNamedCurve( curve),
                                pMesg + 4, pMesg[3], NULL, 0, NULL)))
    {
        goto exit;
    }

#if defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)
    if (SSL_AUTH_ANON_BIT & pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags)
    {
        if (pMesg[3] + 4 != mesgLen)
        {
            /* record length should consist only of the key information */
            status = ERR_SSL_INVALID_MSG_SIZE;
        }
        goto exit;
    }
#endif /* defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__) */

#if defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)
    /* verify the signature */
    switch ( pSSLSock->roleSpecificInfo.client.publicKey.type)
    {
        case akt_rsa:
            if (OK > (status = verifyServerKeyExchangeRSASignature( pSSLSock, pMesg, mesgLen,
                                                                    4 + pMesg[3], NULL)))
            {
                goto exit;
            }
            break;

        case akt_ecc:
            if (OK > ( status = verifyServerKeyExchangeECDSASignature(pSSLSock, pMesg, mesgLen,
                                                                    4 + pMesg[3])))
            {
                goto exit;
            }
            break;

        default:
            status = ERR_BAD_KEY_TYPE;
            goto exit;
            break;
    }
#endif /* defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__) */

exit:
    if (OK > status)
        DEBUG_ERROR(DEBUG_SSL_MESSAGES, "processEcdheServerKeyExchange() returns status = ", status);

    return status;

} /* processEcdheServerKeyExchange */

#endif


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__))

static MSTATUS
processServerDiffieHellmanPskKeyExchange(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen)
{
    ubyte4      pskHintLength;
    MSTATUS     status = ERR_SSL_PSK_BAD_LENGTH;

    /* basic quick check */
    if (2 > mesgLen)
        goto exit;

    /* extract identity hint */
    pskHintLength = getShortValue(pMesg);
    pMesg += 2;
    mesgLen = (ubyte2)(mesgLen - 2);

    /* another quick check */
    if (pskHintLength >= mesgLen)
        goto exit;

    status = ERR_SSL_PSK_BAD_CONFIG;

    /* fill the psk with random garbage, in case the callback does the wrong thing */
    pSSLSock->roleSpecificInfo.client.pskLength = SSL_PSK_MAX_LENGTH;
    pSSLSock->rngFun(pSSLSock->rngFunArg, SSL_PSK_MAX_LENGTH, pSSLSock->roleSpecificInfo.client.psk);

    if (NULL != SSL_sslSettings()->funcPtrChoosePSK)
    {
        status = SSL_sslSettings()->funcPtrChoosePSK(SSL_findConnectionInstance(pSSLSock), pMesg, pskHintLength,
                                                    pSSLSock->roleSpecificInfo.client.pskIdentity, &(pSSLSock->roleSpecificInfo.client.pskIdentityLength),
                                                    pSSLSock->roleSpecificInfo.client.psk, &(pSSLSock->roleSpecificInfo.client.pskLength));
    }

    /* check if the callback overran our buffer */
    if ((OK <= status) &&
        ((SSL_PSK_SERVER_IDENTITY_LENGTH < pSSLSock->roleSpecificInfo.client.pskIdentityLength) ||
         (SSL_PSK_MAX_LENGTH             < pSSLSock->roleSpecificInfo.client.pskLength)) )
    {
        /* heap space may have been corrupted by the callback handler! */
        status = ERR_SSL_PSK_CALLBACK_OVERFLOW_BUF;
    }

    if (OK > status)
        goto exit;

    /* continue processing the message */
    pMesg   += pskHintLength;
    mesgLen  = (ubyte2)(mesgLen - pskHintLength);

    status = processServerKeyExchange(pSSLSock, pMesg, mesgLen);

exit:
    return status;
}

#endif /* ((defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)) */




/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__))

static MSTATUS
processServerEcdhePskKeyExchange(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen)
{
    ubyte4      pskHintLength;
    MSTATUS     status = ERR_SSL_PSK_BAD_LENGTH;

    /* basic quick check */
    if (2 > mesgLen)
        goto exit;

    /* extract identity hint */
    pskHintLength = getShortValue(pMesg);
    pMesg += 2;
    mesgLen = (ubyte2)(mesgLen - 2);

    /* another quick check */
    if (pskHintLength >= mesgLen)
        goto exit;

    status = ERR_SSL_PSK_BAD_CONFIG;

    /* fill the psk with random garbage, in case the callback does the wrong thing */
    pSSLSock->roleSpecificInfo.client.pskLength = SSL_PSK_MAX_LENGTH;
    pSSLSock->rngFun(pSSLSock->rngFunArg, SSL_PSK_MAX_LENGTH, pSSLSock->roleSpecificInfo.client.psk);

    if (NULL != SSL_sslSettings()->funcPtrChoosePSK)
    {
        status = SSL_sslSettings()->funcPtrChoosePSK(SSL_findConnectionInstance(pSSLSock), pMesg, pskHintLength,
                                                     pSSLSock->roleSpecificInfo.client.pskIdentity, &(pSSLSock->roleSpecificInfo.client.pskIdentityLength),
                                                     pSSLSock->roleSpecificInfo.client.psk, &(pSSLSock->roleSpecificInfo.client.pskLength));
    }

    /* check if the callback overran our buffer */
    if ((OK <= status) &&
        ((SSL_PSK_SERVER_IDENTITY_LENGTH < pSSLSock->roleSpecificInfo.client.pskIdentityLength) ||
         (SSL_PSK_MAX_LENGTH             < pSSLSock->roleSpecificInfo.client.pskLength)) )
    {
        /* heap space may have been corrupted by the callback handler! */
        status = ERR_SSL_PSK_CALLBACK_OVERFLOW_BUF;
    }

    if (OK > status)
        goto exit;

    /* continue processing the message */
    pMesg   += pskHintLength;
    mesgLen  = (ubyte2)(mesgLen - pskHintLength);

    status = processServerEcdheKeyExchange(pSSLSock, pMesg, mesgLen);

exit:
    return status;
}

#endif /* ((defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) && defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__))

static MSTATUS
processServerPskKeyExchange(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen)
{
    /* for RSA_PSK and PSK ciphers suites */
    ubyte4      pskHintLength;
    MSTATUS     status = ERR_SSL_PSK_BAD_LENGTH;

    /* IOT-563: there can be no KeyExchange message if the server does
     not send any hint. in that case that function is called with
     a zero length message */

    if ( 0 == mesgLen)
    {
        pskHintLength = 0;
    }
    else
    {
        /* basic quick check */
        if (2 > mesgLen)
            goto exit;

        /* extract identity hint */
        pskHintLength = getShortValue(pMesg);
        pMesg += 2;
        mesgLen = (ubyte2)(mesgLen - 2);

        /* another quick check */
        if (pskHintLength != mesgLen)
            goto exit;
    }

    status = ERR_SSL_PSK_BAD_CONFIG;

    /* in the event the callback does something improper, we fill the psk with random garbage */
    pSSLSock->roleSpecificInfo.client.pskLength = SSL_PSK_MAX_LENGTH;
    pSSLSock->rngFun(pSSLSock->rngFunArg, SSL_PSK_MAX_LENGTH, pSSLSock->roleSpecificInfo.client.psk);

    if (NULL != SSL_sslSettings()->funcPtrChoosePSK)
    {
        status = SSL_sslSettings()->funcPtrChoosePSK(SSL_findConnectionInstance(pSSLSock), pMesg, pskHintLength,
                                                    pSSLSock->roleSpecificInfo.client.pskIdentity, &(pSSLSock->roleSpecificInfo.client.pskIdentityLength),
                                                    pSSLSock->roleSpecificInfo.client.psk, &(pSSLSock->roleSpecificInfo.client.pskLength));
    }

    /* check if the callback overran our buffer */
    if ((OK <= status) &&
        ((SSL_PSK_SERVER_IDENTITY_LENGTH < pSSLSock->roleSpecificInfo.client.pskIdentityLength) ||
         (SSL_PSK_MAX_LENGTH             < pSSLSock->roleSpecificInfo.client.pskLength)) )
    {
        /* heap space may have been corrupted by the callback handler! */
        status = ERR_SSL_PSK_CALLBACK_OVERFLOW_BUF;
    }

exit:
    return status;
}

#endif /* (defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__)) */


/*------------------------------------------------------------------*/

#if (defined(__ENABLE_TLSEXT_RFC6066__) && defined(__ENABLE_MOCANA_OCSP_CLIENT__))

MSTATUS
processCertificateStatus(SSLSocket *pSSLSock, ubyte* pMesg, ubyte2 mesgLen)
{
    MSTATUS status = ERR_SSL_EXTENSION_CERTIFICATE_STATUS_RESPONSE;

    /* Parse certificate status message
      struct {
          CertificateStatusType status_type;
          select (status_type) {
              case ocsp: OCSPResponse;
          } response;
      } CertificateStatus;
      */

    ubyte2 statusType = 0;

    statusType = (ubyte)(*pMesg);

    pMesg ++;

    if (certStatusType_ocsp == statusType)
    {
        ubyte2 responseLen = getMediumValue(pMesg);
        pMesg += 3;

        if (OK > (status = SSL_OCSP_validateOcspResponse(pSSLSock, pMesg,responseLen)))
            goto exit;

    }
exit:
    return status;
}

#endif

/*------------------------------------------------------------------*/

static MSTATUS
handleClientHandshakeMessages(SSLSocket* pSSLSock)
{
    ubyte2  recordLen;
    sbyte*  pMsg    = pSSLSock->pReceiveBuffer;
    ubyte4  sizeofHandshakeHeader;
    MSTATUS status  = OK;

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
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

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
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

        status = ERR_SSL_PROTOCOL;

        switch (handshakeType)
        {
            case SSL_HELLO_REQUEST:
                /* we ignore hello request at this layer, upper layer makes sense of it */
                status = OK;
                break;

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
            case SSL_SERVER_HELLO_VERIFY_REQUEST:
                if ((SSL_BEGIN == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) ||
                    (SSL_HELLO_REQUEST == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) ||
                    (SSL_SERVER_HELLO_VERIFY_REQUEST == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)))
                {
                    status = processServerHelloVerifyRequest(pSSLSock, (ubyte*)(pSHSH+sizeofHandshakeHeader), handshakeRecLen);
                }
                break;
#endif

            case SSL_SERVER_HELLO:
                if ((SSL_BEGIN == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) ||
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
                    (SSL_HELLO_REQUEST == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) ||
                    (SSL_SERVER_HELLO_VERIFY_REQUEST == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)))
#else
                    (SSL_HELLO_REQUEST == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)))
#endif
                {
                    status = processServerHello(pSSLSock, pSHSH, handshakeRecLen);

                    if (OK > status)
                        break;

                    /* since we haven't yet hashed the client hello, will do it now */
                    addToHandshakeHash(pSSLSock, pSSLSock->roleSpecificInfo.client.helloBuffer, pSSLSock->roleSpecificInfo.client.helloBufferLen);
                    /* we no longer need to buffer the client hello, release memory */
                    FREE(pSSLSock->roleSpecificInfo.client.helloBuffer);
                    pSSLSock->roleSpecificInfo.client.helloBuffer = NULL;
                    pSSLSock->roleSpecificInfo.client.helloBufferLen = 0;
                    addToHandshakeHash(pSSLSock,
                                       (ubyte *)pSHSH,
                                       handshakeRecLen + sizeofHandshakeHeader);
                }
                break;

            case SSL_CERTIFICATE:
                if (NULL == pSSLSock->pHandshakeCipherSuite)
                    break;

                if ((SSL_SERVER_HELLO == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) &&
                    (0 == (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_AUTH_ANON_BIT)))
                {
                    status = processCertificate(pSSLSock, pSHSH, handshakeRecLen, TRUE);

                    if (OK > status)
                        break;

                    pSSLSock->generateEmptyCert = FALSE;
                    addToHandshakeHash(pSSLSock,
                                       (ubyte *)pSHSH,
                                       handshakeRecLen + sizeofHandshakeHeader);
                }
                break;

#if (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__)         || \
     defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)     || \
     defined(__ENABLE_MOCANA_SSL_ECDHE_SUPPORT__)       || \
     defined(__ENABLE_MOCANA_SSL_ECDH_ANON_SUPPORT__)   || \
     defined(__ENABLE_MOCANA_SSL_PSK_SUPPORT__) )
            case SSL_SERVER_KEY_EXCHANGE:
                if ((SSL_SERVER_HELLO != SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) &&
                    (SSL_CERTIFICATE  != SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)))
                {
                    /* we got here from a bad state */
                    break;
                }

                if ((NULL != pSSLSock->pHandshakeCipherSuite) &&
                    (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo) &&
                    (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->ProcessServerKEX))
                {
                    status = pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->ProcessServerKEX(pSSLSock, (ubyte *)(pSHSH + sizeofHandshakeHeader), handshakeRecLen);

                    if (OK > status)
                        break;

                    addToHandshakeHash(pSSLSock,
                                       (ubyte *)pSHSH,
                                       handshakeRecLen + sizeofHandshakeHeader);
                }
                break;
#endif /* (defined(__ENABLE_MOCANA_SSL_DHE_SUPPORT__) || defined(__ENABLE_MOCANA_SSL_DH_ANON_SUPPORT__)) */

            case SSL_CERTIFICATE_REQUEST:
                if ((SSL_CERTIFICATE         == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) ||
                    (SSL_SERVER_KEY_EXCHANGE == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) )
                {
                    status = OK;

                    pSSLSock->generateEmptyCert = TRUE;

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
                    if ((SSL_NO_MUTUAL_AUTH_BIT != (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_NO_MUTUAL_AUTH_BIT)) &&
                        (SSL_FLAG_NO_MUTUAL_AUTH_REPLY != (pSSLSock->runtimeFlags & SSL_FLAG_NO_MUTUAL_AUTH_REPLY)))
                    {
                        /* ignore certificate requests, if server is not authenticated */
                        status = processCertificateRequest(pSSLSock, pSHSH, handshakeRecLen);

                        if (OK > status)
                            break;
                    }
#endif

                    addToHandshakeHash(pSSLSock,
                                       (ubyte *)pSHSH,
                                       handshakeRecLen + sizeofHandshakeHeader);
                }
                break;

            case SSL_SERVER_HELLO_DONE:
                if ((SSL_CERTIFICATE        == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) ||
#ifdef __ENABLE_TLSEXT_RFC6066__
                    (SSL_CERTIFCATE_STATUS   == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) ||
#endif
#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
                    /* IOT-563: no message between SERVER_HELLO and SERVER_HELLO_DONE
                     if there's no hint sent by server for PSK cipher suites */
                    (SSL_SERVER_HELLO         == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) ||
#endif
                    (SSL_SERVER_KEY_EXCHANGE == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) ||
                    (SSL_CERTIFICATE_REQUEST == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) )
                {
#ifdef __ENABLE_MOCANA_SSL_PSK_SUPPORT__
                    /* IOT-563: special case for PSK suites  -- it's possible we arrive
                     here without receiving a server KeyExchange */
                    intBoolean isPSK = (NULL != pSSLSock->pHandshakeCipherSuite) &&
                        (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo) &&
                        (NULL != pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->ProcessServerKEX) &&
                        (pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->flags & SSL_PSK_BIT);

                    /* if this is a PSK cipher, makes sure the callback is called */
                    if (isPSK && 0 == pSSLSock->roleSpecificInfo.client.pskIdentityLength)
                    {
                        status = pSSLSock->pHandshakeCipherSuite->pKeyExAuthAlgo->ProcessServerKEX(pSSLSock, (ubyte *)(pSHSH + sizeofHandshakeHeader), 0);
                        if (OK > status)
                            break;
                    }

                    /* allow no server messages between server hello and server hello done
                     only if PSK */
                    if (SSL_SERVER_HELLO == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) && !isPSK)
                    {
                        status = ERR_SSL_PROTOCOL;
                        break;
                    }
#endif
                    /* easy message to process */
                    if (0 == handshakeRecLen)
                        status = OK;

                    /* fail out, if SSL_FLAG_REQUIRE_MUTUAL_AUTH is set and server did not require mutual authentication */
                    if ((pSSLSock->runtimeFlags & SSL_FLAG_REQUIRE_MUTUAL_AUTH) &&
                        (SSL_CERTIFICATE_REQUEST != SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) &&
                        (E_NoSessionResume == pSSLSock->sessionResume))
                    {
                        /* we expected the server to require mutual authentication */
                        /* as of now, SSL_CERTIFICATE_REQUEST is expected before SSL_SERVER_HELLO_DONE */
                        /* in the future, we might need to set a flag rather than inspecting SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) */
                        status = ERR_SSL_MUTUAL_AUTHENTICATION_NOT_REQUESTED;
                        break;
                    }

                    addToHandshakeHash(pSSLSock,
                                       (ubyte *)pSHSH,
                                       handshakeRecLen + sizeofHandshakeHeader);

/*                    SSL_ASYNC_STATE(pSSLSock) = SSL_ASYNC_STATE_HELLO_COMPLETED; */
                }
                break;

            case SSL_FINISHED:
                /* client or server */
                if (SSL_EXPECTING_FINISHED == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock))
                {
                    status = processFinished(pSSLSock, pSHSH, handshakeRecLen);

                    if (OK > status)
                        break;

                    addToHandshakeHash(pSSLSock,
                                        (ubyte *)pSHSH,
                                        handshakeRecLen + sizeofHandshakeHeader);

/*                    SSL_ASYNC_STATE(pSSLSock) = SSL_ASYNC_STATE_FINISHED; */
                }
                break;
#if (defined(__ENABLE_TLSEXT_RFC6066__) && defined(__ENABLE_MOCANA_OCSP_CLIENT__))
            case SSL_CERTIFCATE_STATUS:
                /* Handle certficateStatus Message here */
                if ((SSL_CERTIFICATE == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) &&
                    (pSSLSock->roleSpecificInfo.client.didRecvCertStatusExtInServHello))
                {
                    if (OK > (status = processCertificateStatus(pSSLSock, pMsg, handshakeRecLen)))
                        break;

                     addToHandshakeHash(pSSLSock,
                                       (ubyte *)pSHSH,
                                       handshakeRecLen + sizeofHandshakeHeader);
                }
                break;
#endif
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
#ifdef __ENABLE_TLSEXT_RFC6066__
            /* handle exception here as certificate status > all other handshakeType */
            else if ((SSL_CERTIFCATE_STATUS == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock)) &&
                (SSL_CERTIFICATE < handshakeType))
            {
                SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = handshakeType;
            }
#endif
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
        DEBUG_ERROR(DEBUG_SSL_TRANSPORT, (sbyte*)"handleClientHandshakeMessages() returns status = ", status);

    return status;

} /* handleClientHandshakeMessages */


/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_clientHandshake(SSLSocket* pSSLSock, intBoolean isWriter)
{
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        return SSL_SOCK_clientHandshakeDTLS(pSSLSock, isWriter);
    } else
#endif
    {
        return SSL_SOCK_clientHandshakeSSL(pSSLSock, isWriter);
    }
}

/*------------------------------------------------------------------*/

static MSTATUS
SSL_SOCK_clientHandshakeSSL(SSLSocket* pSSLSock, intBoolean isWriter)
{
    MSTATUS status = OK;

#if 1   /*!!!! this needs to happen by the caller */
    /* handle any data pending on a send */
    if ((NULL != pSSLSock->pOutputBuffer) &&
        (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
    {
        goto exit;
    }
#endif

    switch (SSL_HANDSHAKE_STATE(pSSLSock))
    {
#if (defined(__ENABLE_MOCANA_SSL_REHANDSHAKE__))
        case kSslOpenState:
        {
            sbyte4  doRehandshake       = FALSE;
            sbyte4  doSessionResumption = FALSE;

            /* rehandshake support */
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

            if (FALSE == doSessionResumption)
            {
                pSSLSock->roleSpecificInfo.client.sessionIdLen = 0;     /* prevent session resumption */
            }

            /* FALL-THROUGH */
        }
#endif

        case kSslReceiveHelloInitState:
        {
            /* MD5init_HandShake(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pMd5Ctx); */
            /* SHA1_initDigestHandShake(MOC_HASH(pSSLSock->hwAccelCookie) pSSLSock->pShaCtx); */

            status = SSL_CLIENT_sendClientHello(pSSLSock);
            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveHelloState;

            /* check if data is pending, if so break */
            if ((NULL != pSSLSock->pOutputBuffer) && (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
                break;

            /* FALL-THROUGH */
        }

        case kSslReceiveHelloState:
        {
            if (isWriter)
                break;

            if (OK > (status = handleClientHandshakeMessages(pSSLSock)))
                goto exit;

            if (SSL_SERVER_HELLO_DONE != SSL_REMOTE_HANDSHAKE_STATE(pSSLSock))
                break;

            /* if we resume sessions the path is a bit different */
            if (E_NoSessionResume != pSSLSock->sessionResume)
            {
                status = SSL_SOCK_generateKeyMaterial(pSSLSock, NULL, 0);

                SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntilResume;
                break;
            }
            else /* if (SSL_ASYNC_STATE_HELLO_COMPLETED == SSL_ASYNC_STATE(pSSLSock)) */
            {
                status = SSL_CLIENT_sendClientResponseBlock(pSSLSock);

                SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveHelloState1;
            }

            if (OK > status)
                goto exit;

            /* check if data is pending, if so break */
            if ((NULL != pSSLSock->pOutputBuffer) && (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
                break;

            /* FALL-THROUGH */
        }

        case kSslReceiveHelloState1:
        {
            if (OK > (status = sendChangeCipherSpec(pSSLSock)))
                goto exit;

            if (OK > (status = SSL_SOCK_setClientKeyMaterial(pSSLSock)))
                goto exit;

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
            pSSLSock->retransCipherInfo.pOldCipherSuite = pSSLSock->pActiveOwnCipherSuite;
#endif
            pSSLSock->pActiveOwnCipherSuite = pSSLSock->pHandshakeCipherSuite;

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveHelloState2;

            /* check if data is pending, if so break */
            if ((NULL != pSSLSock->pOutputBuffer) && (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
                break;

            /* FALL-THROUGH */
        }

        case kSslReceiveHelloState2:
        {
            status = sendFinished(pSSLSock);

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntil;

            break;
        }

        case kSslReceiveUntilResume:
        {
            if (isWriter)
                break;

            /* if we resume sessions the path is a bit different */
            if (OK > (status = handleClientHandshakeMessages(pSSLSock)))
                goto exit;

            if (SSL_FINISHED != SSL_REMOTE_HANDSHAKE_STATE(pSSLSock))
            {
                break;
            }

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntilResume1;

            /* FALL-THROUGH */
        }

        case kSslReceiveUntilResume1:
        {
            if (OK > (status = sendChangeCipherSpec(pSSLSock)))
                goto exit;

            if (OK > (status = SSL_SOCK_setClientKeyMaterial(pSSLSock)))
                goto exit;

#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
            pSSLSock->retransCipherInfo.pOldCipherSuite = pSSLSock->pActiveOwnCipherSuite;
#endif
            pSSLSock->pActiveOwnCipherSuite = pSSLSock->pHandshakeCipherSuite;

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntilResume2;

            /* check if data is pending, if so break */
            if ((NULL != pSSLSock->pOutputBuffer) && (!(SSL_FLAG_ENABLE_SEND_BUFFER & pSSLSock->runtimeFlags)))
                break;

            /* FALL-THROUGH */
        }

        case kSslReceiveUntilResume2:
        {
            status = sendFinished(pSSLSock);

            SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveUntilResume3;

            /* FALL-THROUGH */
        }

        case kSslReceiveUntilResume3:
        {
            SSL_HANDSHAKE_STATE(pSSLSock) = kSslOpenState;
            SSL_OPEN_STATE(pSSLSock) = kSslSecureSessionEstablished;

            if (OK > (status = SSL_INTERNAL_setConnectionState(SSL_findConnectionInstance(pSSLSock), CONNECT_OPEN)))
                goto exit;

            status = SSLSOCK_doOpenUpcalls(pSSLSock);

            if ( OK <= status )
                status = pSSLSock->numBytesToSend;

            break;
        }

        case kSslReceiveUntil:
        {
            if (isWriter)
                break;

            status = handleClientHandshakeMessages(pSSLSock);
            if ( OK > status )
                goto exit;

            if (SSL_FINISHED == SSL_REMOTE_HANDSHAKE_STATE(pSSLSock))
            {
                SSL_HANDSHAKE_STATE(pSSLSock) = kSslOpenState;
                SSL_OPEN_STATE(pSSLSock) = kSslSecureSessionEstablished;

                if (OK > (status = SSL_INTERNAL_setConnectionState(SSL_findConnectionInstance(pSSLSock), CONNECT_OPEN)))
                    goto exit;

                status = SSLSOCK_doOpenUpcalls(pSSLSock);
            }
            if ( OK <= status )
                status = pSSLSock->numBytesToSend;

            break;
        }

        default:
        {
            status = ERR_SSL_BAD_STATE;
            break;
        }
    }

exit:
    return status;
}

/*------------------------------------------------------------------*/

#ifdef __ENABLE_MOCANA_EAP_FAST__
extern MSTATUS
SSL_SOCK_setEAPFASTParams(SSLSocket* pSSLSock,
                          ubyte* pPacOpaque,
                          ubyte4 pacOpaqueLen,
                          ubyte  pacKey[PACKEY_SIZE])
{
    MSTATUS status = OK;

    if (!pSSLSock)
    {
        status = ERR_NULL_POINTER;
        goto exit;
    }

    /* store the PacKey */
    if (pacKey)
    {
        if (OK > (status = MOC_MEMCPY(pSSLSock->pacKey, pacKey, PACKEY_SIZE)))
            goto exit;
    }

    if (pPacOpaque)
    {
        if (!pacOpaqueLen)
        {
            status = ERR_INVALID_ARG;
            goto exit;
        }

        resetTicket(pSSLSock);

        pSSLSock->roleSpecificInfo.client.ticket = MALLOC(pacOpaqueLen);
        if (!pSSLSock->roleSpecificInfo.client.ticket)
        {
            status = ERR_MEM_ALLOC_FAIL;
            goto exit;
        }

        status = MOC_MEMCPY(pSSLSock->roleSpecificInfo.client.ticket, pPacOpaque, pacOpaqueLen);
        pSSLSock->roleSpecificInfo.client.ticketLength = pacOpaqueLen;
    }

exit:
    return status;
}
#endif /* __ENABLE_MOCANA_EAP_FAST__ */


/*------------------------------------------------------------------*/

extern MSTATUS
SSL_SOCK_initSocketExtraClient(SSLSocket* pSSLSock,
                               ubyte sessionIdLen,
                               ubyte* pSessionId,
                               ubyte* pMasterSecret,
                               const sbyte* pDNSName)
{
    MSTATUS status;

    if (!pSSLSock)
        return ERR_NULL_POINTER;

#if (!defined(__DISABLE_MOCANA_SSL_COMMON_NAME_CHECK__))
    if (!pDNSName)
        return ERR_NULL_POINTER;
#endif

    if ( sessionIdLen && (!pSessionId || !pMasterSecret))
        return ERR_INVALID_ARG;

    pSSLSock->roleSpecificInfo.client.pDNSName = pDNSName;
#ifdef __ENABLE_MOCANA_MULTIPLE_COMMON_NAMES__
    pSSLSock->roleSpecificInfo.client.pCNMatchInfos = 0;
#endif

    SSL_HANDSHAKE_STATE(pSSLSock) = kSslReceiveHelloInitState;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        pSSLSock->helloCookieLen = 0;
        DTLS_TIMER_STATE(pSSLSock) = kDtlsPreparing;
    }
#endif

    if (sessionIdLen > SSL_MAXSESSIONIDSIZE)
    {
        /* this is actually an error: a session id should never be more
        than 32 bytes long according to the SSL RFC*/
        pSSLSock->roleSpecificInfo.client.sessionIdLen = 0;
    }
    else
    {
        pSSLSock->roleSpecificInfo.client.sessionIdLen = sessionIdLen;
        if ( sessionIdLen > 0)
        {
            MOC_MEMCPY(pSSLSock->roleSpecificInfo.client.sessionId,
                    pSessionId, sessionIdLen);
            pSSLSock->roleSpecificInfo.client.pMasterSecret = pMasterSecret;
        }
    }

#ifdef __ENABLE_MOCANA_EAP_FAST__
    pSSLSock->roleSpecificInfo.client.ticket = 0;
    pSSLSock->roleSpecificInfo.client.ticketLength = 0;
#endif

    if (OK > (status = CRYPTO_initAsymmetricKey(&pSSLSock->roleSpecificInfo.client.publicKey)))
        goto exit;

    /* case kSslReceiveHelloInitState: */
    resetCipher(pSSLSock, TRUE, TRUE);
    SSL_REMOTE_HANDSHAKE_STATE(pSSLSock) = SSL_BEGIN;
#ifdef __ENABLE_MOCANA_DTLS_CLIENT__
    if (pSSLSock->isDTLS)
    {
        pSSLSock->sslMinorVersion = MAX_DTLS_MINORVERSION; /* highest DTLS version supported */
    } else
#endif
    {
        pSSLSock->sslMinorVersion = MAX_SSL_MINORVERSION; /* highest TLS version supported */
    }
exit:
    return status;
}


#endif
