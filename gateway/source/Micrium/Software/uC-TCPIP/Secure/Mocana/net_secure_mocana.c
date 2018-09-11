/*
*********************************************************************************************************
*                                             uC/TCP-IP
*                                      The Embedded TCP/IP Suite
*
*                         (c) Copyright 2004-2015; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/TCP-IP is provided in source form to registered licensees ONLY.  It is
*                  illegal to distribute this source code to any third party unless you receive
*                  written permission by an authorized Micrium representative.  Knowledge of
*                  the source code may NOT be used to develop a similar product.
*
*                  Please help us continue to provide the Embedded community with the finest
*                  software available.  Your honesty is greatly appreciated.
*
*                  You can find our product's user manual, API reference, release notes and
*                  more information at: https://doc.micrium.com
*
*                  You can contact us at: http://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                     NETWORK SECURITY PORT LAYER
*
*                                            Mocana nanoSSL
*
* Filename      : net_secure_mocana.c
* Version       : V3.04.00
* Programmer(s) : SL
*********************************************************************************************************
* Note(s)       : (1) Assumes the following versions (or more recent) of software modules are included in
*                     the project build :
*
*                     (a) Mocana nanoSSL V5.5
*                     (b) uC/Clk V3.09
*
*                     See also 'net.h  Note #1'.
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    NET_SECURE_MODULE
#include  "net_secure_mocana.h"
#include  "../../Source/net_cfg_net.h"
#include  "../../Source/net_sock.h"
#include  <lib_str.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#define  NET_SECURE_MOCANA_MODULE
#ifdef   NET_SECURE_MODULE_EN


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/


typedef  struct  net_secure_server_desc {
    certStorePtr     CertStorePtr;
    certDescriptor   CertDesc;
} NET_SECURE_SERVER_DESC;


typedef  struct  net_secure_client_desc {
    CPU_CHAR                          *CommonNamePtr;
    NET_SOCK_SECURE_UNTRUSTED_REASON   UntrustedReason;
    NET_SOCK_SECURE_TRUST_FNCT         TrustCallBackFnctPtr;
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    certStorePtr                       CertStorePtr;
    certDescriptor                     CertDesc;
    CPU_INT08U                        *KeyPtr;
#endif
} NET_SECURE_CLIENT_DESC;


typedef  struct  net_secure_session {
    sbyte4                 ConnInstance;
    NET_SOCK_SECURE_TYPE   Type;
    void                  *DescPtr;
} NET_SECURE_SESSION;


                                                                /* ---------------- NET SECURE POOLS ------------------ */
typedef  struct  net_secure_mem_pools {
    MEM_DYN_POOL                SessionPool;
    MEM_DYN_POOL                ServerDescPool;
    MEM_DYN_POOL                ClientDescPool;
} NET_SECURE_MEM_POOLS;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  certDescriptor               CaCertDesc;
static  CPU_INT08U                   CaBuf[NET_SECURE_CFG_MAX_CA_CERT_LEN];

static  NET_SECURE_MEM_POOLS         NetSecure_Pools;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

           void                      NetSecure_MocanaFnctLog            (       sbyte4                          module,
                                                                                sbyte4                          severity,
                                                                                sbyte                          *msg);

static     sbyte4                    NetSecure_CertificateStoreLookup   (sbyte4                    connectionInstance,
                                                                         certDistinguishedName    *pLookupCertDN,
                                                                         certDescriptor           *pReturnCert);

static     sbyte4                    NetSecure_CertificateStoreVerify   (sbyte4                    connectionInstance,
                                                                         ubyte                    *pCertificate,
                                                                         ubyte4                    certificateLength,
                                                                         sbyte4                    isSelfSigned);

static     certDescriptor            NetSecure_CertKeyConvert            (const  CPU_INT08U                     *p_cert,
                                                                                 CPU_SIZE_T                      cert_size,
                                                                          const  CPU_INT08U                     *p_key,
                                                                                 CPU_SIZE_T                      key_size,
                                                                                 NET_SOCK_SECURE_CERT_KEY_FMT    fmt,
                                                                                 NET_ERR                        *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                     NETWORK SECURITY FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     NetSecure_CA_CertIntall()
*
* Description : Install certificate autority's certificate.
*
* Argument(s) : p_ca_cert       Pointer to CA certificate.
*
*               ca_cert_len     Certificate lenght.
*
*               fmt             Certificate format:
*
*                                   NET_SOCK_SECURE_CERT_KEY_FMT_PEM
*                                   NET_SOCK_SECURE_CERT_KEY_FMT_DER
*
*               Pointer to variable that will receive the return error code from this function :
*
*                               NET_SECURE_ERR_NONE         Certificate   successfully installed.
*                               NET_SECURE_ERR_INSTALL      Certificate installation   failed.
*                               NET_SECURE_ERR_INVALID_FMT  Certificate has invalid format.
*
* Return(s)   : DEF_OK,
*
*               DEF_FAIL.
*
* Caller(s)   : Application.
*
*               This function is a network protocol suite application programming interface (API) function
*               & MAY be called by application function(s) [see also Note #2].
*
* Note(s)     : (1) Net_Secure_CA_CertIntall() is called by application function(s) & ... :
*
*                   (a) MUST NOT be called with the global network lock already acquired; ...
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock.
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSecure_CA_CertIntall (const  void                 *p_ca_cert,
                                             CPU_INT32U            ca_cert_len,
                                             NET_SECURE_CERT_FMT   fmt,
                                             NET_ERR              *p_err)
{
    CPU_BOOLEAN  rtn_val;
    CPU_INT32S   rc;


#if (NET_ERR_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ---------------- VALIDATE CERT LEN ----------------- */
    if (ca_cert_len > NET_SECURE_CFG_MAX_CA_CERT_LEN) {
       *p_err = NET_SECURE_ERR_INSTALL;
        rtn_val = DEF_FAIL;
        goto exit_fault;
    }
#endif

    Net_GlobalLockAcquire((void *)&NetSecure_CA_CertIntall, p_err);
    if (*p_err != NET_ERR_NONE) {
         rtn_val = DEF_FAIL;
         goto exit_fault;
    }

    Mem_Copy(CaBuf, p_ca_cert, ca_cert_len);

    rc = LAST_ERROR;

    switch (fmt) {
        case NET_SOCK_SECURE_CERT_KEY_FMT_PEM:
             rc = CA_MGMT_decodeCertificate(CaBuf, ca_cert_len, &CaCertDesc.pCertificate, &CaCertDesc.certLength);
             if (rc != OK) {
                *p_err = NET_SECURE_ERR_INSTALL;
                 rtn_val = DEF_FAIL;
                 goto exit;
             }
             break;


        case NET_SOCK_SECURE_CERT_KEY_FMT_DER:
             CaCertDesc.pCertificate = CaBuf;
             CaCertDesc.certLength   = ca_cert_len;
             break;


        case NET_SOCK_SECURE_CERT_KEY_FMT_NONE:
        default:
            *p_err = NET_ERR_FAULT_NOT_SUPPORTED;
             rtn_val = DEF_FAIL;
             goto exit;
    }

   *p_err = NET_SECURE_ERR_NONE;

exit:
    Net_GlobalLockRelease();

exit_fault:
    return (rtn_val);
}

/*
*********************************************************************************************************
*                                            NetSecure_Log()
*
* Description : log the given string.
*
* Argument(s) : p_str   Pointer to string to log.
*
* Return(s)   : none.
*
* Caller(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSecure_Log (CPU_CHAR  *p_str)
{

    SSL_TRACE_DBG(((CPU_CHAR *)msg));

}


/*
*********************************************************************************************************
*                                     NetSecure_ExtractCertDN()
*
* Description : Extract certificate distinguished name into a string.
*
* Argument(s) : p_buf       Pointer to string fill.
*
*               buf_len     Buffer lenght.
*
*               p_dn        Pointer to distinguished name.
*
* Return(s)   : DEF_OK,   all distinguished name data printed sucessfully.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application.
*
*               This function is a network protocol suite application programming interface (API) function
*               & MAY be called by application function(s) [see also Note #2].
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if 0
CPU_BOOLEAN  NetSecure_ExtractCertDN (CPU_CHAR               *p_buf,
                                      CPU_INT32U              buf_len,
                                      certDistinguishedName  *p_dn)
{
    CPU_CHAR               *p_str;
    relativeDN             *p_relative_dn;
    nameAttr               *p_name_attr;
    CPU_INT32U              dn_ctr;
    CPU_INT32U              item_ctr;
    CPU_INT32U              len;
    CPU_INT32U              rem_len;
    CPU_BOOLEAN             wr_started;


    p_str         = p_buf;
    rem_len       = buf_len;
    wr_started    = DEF_NO;

    p_relative_dn = p_dn->pDistinguishedName;

    for (dn_ctr = 0; dn_ctr< p_dn->dnCount; dn_ctr++) {
        p_name_attr = p_relative_dn->pNameAttr;
        for (item_ctr = 0; item_ctr < p_relative_dn->nameAttrCount; item_ctr++) {
            if (wr_started == DEF_YES) {
                len = DEF_MIN(rem_len, 4);
                Str_Copy_N(p_str, " - ", len);
                rem_len -= 4;
                p_str   += 3;
            }

            if (p_name_attr->type == 19) {
                len = DEF_MIN(rem_len, p_name_attr->valueLen);
                if (len == 0) {
                    return (DEF_FAIL);
                }
                Str_Copy_N(p_str, (CPU_CHAR *)p_name_attr->value, len);
                rem_len -= (p_name_attr->valueLen);
                p_str   += (p_name_attr->valueLen);
               *p_str    =  ASCII_CHAR_NULL;
                if (wr_started != DEF_YES) {
                    wr_started  = DEF_YES;
                }
            }

            p_name_attr++;
        }

        p_relative_dn++;
    }

    return (DEF_OK);
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          NetSecure_Init()
*
* Description : (1) Initialize security port :
*
*                   (a) Initialize security memory pools
*                   (b) Initialize CA descriptors
*                   (c) Initialize Mocana
*
*
* Argument(s) : p_err       Pointer to variable that will receive the return error code from this function :
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSecure_Init (NET_ERR  *p_err)
{
    CPU_INT32S  rc;
    LIB_ERR     err;


    SSL_TRACE_DBG(("%s: Start\n", __FUNCTION__));


    Mem_DynPoolCreate("SSL Session pool",
                      &NetSecure_Pools.SessionPool,
                       DEF_NULL,
                       sizeof(NET_SECURE_SESSION),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                      &err);
    if (err != LIB_MEM_ERR_NONE) {
        SSL_TRACE_DBG(("Mem_DynPoolCreate() returned an error"));
        return;
    }

    Mem_DynPoolCreate("SSL Server Descriptor pool",
                      &NetSecure_Pools.ServerDescPool,
                       DEF_NULL,
                       sizeof(NET_SECURE_SERVER_DESC),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                      &err);
    if (err != LIB_MEM_ERR_NONE) {
        SSL_TRACE_DBG(("Mem_DynPoolCreate() returned an error"));
        return;
    }


    Mem_DynPoolCreate("SSL Client Descriptor pool",
                      &NetSecure_Pools.ClientDescPool,
                       DEF_NULL,
                       sizeof(NET_SECURE_SERVER_DESC),
                       sizeof(CPU_ALIGN),
                       0u,
                       LIB_MEM_BLK_QTY_UNLIMITED,
                      &err);
    if (err != LIB_MEM_ERR_NONE) {
        SSL_TRACE_DBG(("Mem_DynPoolCreate() returned an error"));
        return;
    }



 #ifdef NET_SECURE_MODULE_EN
                                                                /* Init CA desc.                                        */
    CaCertDesc.pCertificate = NULL;
    CaCertDesc.certLength   = 0;
#endif

                                                                /* Init Mocana nanoSSL.                                 */
    rc = -1;
    rc =  MOCANA_initMocana();
    if (rc != OK) {
        SSL_TRACE_DBG(("MOCANA_initMocana() returned an error"));
       *p_err = NET_ERR_FAULT_UNKNOWN_ERR;
        return;
    }

    MOCANA_initLog(NetSecure_MocanaFnctLog);

    rc = SSL_init(NET_SECURE_CFG_MAX_NBR_SOCK_SERVER, NET_SECURE_CFG_MAX_NBR_SOCK_CLIENT);
    if (rc != OK) {
        SSL_TRACE_DBG(("%s: %s returned: %s\n", __FUNCTION__, "SSL_init", MERROR_lookUpErrorCode((MSTATUS)rc)));
       *p_err = NET_ERR_FAULT_UNKNOWN_ERR;
        return;
    }

    SSL_sslSettings()->funcPtrCertificateStoreVerify = NetSecure_CertificateStoreVerify;
    SSL_sslSettings()->funcPtrCertificateStoreLookup = NetSecure_CertificateStoreLookup;

   *p_err = NET_SECURE_ERR_NONE;

    SSL_TRACE_DBG(("%s: Normal exit\n", __FUNCTION__));
}


/*
*********************************************************************************************************
*                                       NetSecure_InitSession()
*
* Description : Initalize a new secure session.
*
* Argument(s) : p_sock      Pointer to the accepted/connected socket.
*               ------      Argument checked in NetSock_CfgSecure(),
*                                               NetSecure_SockAccept().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SECURE_ERR_NONE         Secure session     available.
*                               NET_SECURE_ERR_NOT_AVAIL    Secure session NOT available.
*
* Return(s)   : none.
*
* Caller(s)   : NetSock_CfgSecure().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSecure_InitSession (NET_SOCK  *p_sock,
                             NET_ERR   *p_err)
{
#ifdef   NET_SECURE_MODULE_EN
    NET_SECURE_SESSION  *p_blk;
    LIB_ERR              err;


    SSL_TRACE_DBG(("%s: Start\n", __FUNCTION__));
                                                                /* Get SSL session buf.                             */
    p_blk = Mem_DynPoolBlkGet(&NetSecure_Pools.SessionPool, &err);
    if (err != LIB_MEM_ERR_NONE) {
       *p_err = NET_SECURE_ERR_NOT_AVAIL;
        return;
    }


    p_blk->ConnInstance   = 0u;
    p_blk->DescPtr        = DEF_NULL;
    p_blk->Type           = NET_SOCK_SECURE_TYPE_NONE;
    p_sock->SecureSession = p_blk;


    SSL_TRACE_DBG(("%s: Normal exit\n", __FUNCTION__));

   *p_err = NET_SECURE_ERR_NONE;
#else
   *p_err = NET_ERR_FAULT_FEATURE_DIS;
#endif
}


/*
*********************************************************************************************************
*                                 NetSock_CfgSecureServerCertKeyBuf()
*
* Description : Configure server secure socket's certificate and key from buffers:
*
*
* Argument(s) : p_sock         p_sock      Pointer to the server's socker to configure certificate and key.
*               ------         Argument checked in NetSock_CfgSecure(),
*                                                  NetSock_CfgSecureServerCertKeyInstall().
*
*               pbuf_cert       Pointer to the certificate         buffer to install.
*
*               buf_cert_size   Size    of the certificate         buffer to install.
*
*               pbuf_key        Pointer to the key                 buffer to install.
*
*               buf_key_size    Size    of the key                 buffer to install.
*
*               fmt             Format  of the certificate and key buffer to install.
*
*                                   NET_SECURE_INSTALL_FMT_PEM      Certificate and Key format is PEM.
*                                   NET_SECURE_INSTALL_FMT_DER      Certificate and Key format is DER.
*
*               cert_chain      Certificate point to a chain of certificate.
*
*                                   DEF_YES     Certificate points to a chain  of certificate.
*                                   DEF_NO      Certificate points to a single    certificate.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE                       Server socket's certificate and key successfully
*                                                                           installed.
*                               NET_SOCK_ERR_NULL_PTR                   Invalid pointer.
*                               NET_SOCK_ERR_SECURE_FMT                 Invalid certificate and key format.
*                               NET_SECURE_ERR_INSTALL                  Certificate and/or Key NOT successfully installed.
*
* Return(s)   : DEF_OK,   Server socket's certificate and key successfully configured.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : NetSock_CfgSecureServerCertKeyInstall().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : (1) NetSock_CfgSecureServerCertKeyBuf() is called by application function(s) & ... :
*
*                   (a) MUST be called with the global network lock already acquired; ...
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock (see 'net.h  Note #3').
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

CPU_BOOLEAN  NetSecure_SockCertKeyCfg (       NET_SOCK                       *p_sock,
                                              NET_SOCK_SECURE_TYPE            sock_type,
                                       const  CPU_INT08U                     *p_buf_cert,
                                              CPU_SIZE_T                      buf_cert_size,
                                       const  CPU_INT08U                     *p_buf_key,
                                              CPU_SIZE_T                      buf_key_size,
                                              NET_SOCK_SECURE_CERT_KEY_FMT    fmt,
                                              CPU_BOOLEAN                     cert_chain,
                                              NET_ERR                        *p_err)
{
    CPU_BOOLEAN               rtn_val = DEF_OK;
#ifdef   NET_SECURE_MODULE_EN
    CPU_INT32S                rc;
    NET_SECURE_SESSION       *p_session;
    NET_SECURE_SERVER_DESC   *p_server_desc;
    NET_SECURE_CLIENT_DESC   *p_client_desc;
    SizedBuffer               certificate;
    certStorePtr             *p_store;
    certDescriptor            cert_desc;
    LIB_ERR                   err;



    SSL_TRACE_DBG(("%s: Start\n", __FUNCTION__));

#if (NET_ERR_CFG_ARG_CHK_EXT_EN == DEF_ENABLED)                 /* ------------------- VALIDATE ARGS ------------------ */
    if (buf_cert_size > NET_SECURE_CFG_MAX_CERT_LEN) {
       *p_err = NET_SECURE_ERR_INSTALL;
        return (DEF_FAIL);
    }

    if (buf_key_size > NET_SECURE_CFG_MAX_KEY_LEN) {
       *p_err = NET_SECURE_ERR_INSTALL;
        return (DEF_FAIL);
    }
#endif


    p_session = (NET_SECURE_SESSION *)p_sock->SecureSession;
    if (p_session == DEF_NULL) {
       *p_err = NET_SOCK_ERR_NULL_PTR;
        return (DEF_FAIL);
    }


    cert_desc = NetSecure_CertKeyConvert(p_buf_cert, buf_cert_size, p_buf_key, buf_key_size, fmt, p_err);
    if (*p_err != NET_SECURE_ERR_NONE) {
        return (DEF_FAIL);
    }

    switch(sock_type) {
        case NET_SOCK_SECURE_TYPE_SERVER:
                                                                        /* Get SSL session buffer.                              */
             if (p_session->DescPtr) {
                *p_err = NET_SECURE_ERR_INSTALL;
                 return (DEF_FAIL);
             }

             p_server_desc = Mem_DynPoolBlkGet(&NetSecure_Pools.ServerDescPool, &err);
             if (err != LIB_MEM_ERR_NONE) {
                *p_err = NET_SECURE_ERR_INSTALL;
                 return (DEF_FAIL);
             }



             p_session->Type         =  NET_SOCK_SECURE_TYPE_SERVER;
             p_session->DescPtr      =  p_server_desc;
             p_store                 = &p_server_desc->CertStorePtr;
             p_server_desc->CertDesc =  cert_desc;
             break;


        case NET_SOCK_SECURE_TYPE_CLIENT:
#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__

             p_client_desc = p_session->DescPtr;
             if (p_client_desc == DEF_NULL) {
                 p_client_desc = Mem_DynPoolBlkGet(&NetSecure_Pools.ClientDescPool, &err);
                 if (err != LIB_MEM_ERR_NONE) {
                    *p_err = NET_SECURE_ERR_INSTALL;
                     return (DEF_FAIL);
                 }
             }

             p_session->Type =  NET_SOCK_SECURE_TYPE_CLIENT;
             p_store         = &p_client_desc->CertStorePtr;
             break;
#else
            *p_err = NET_ERR_FAULT_FEATURE_DIS;
             return (DEF_FAIL);
#endif

        default:
            *p_err = NET_SECURE_ERR_INSTALL;
             return (DEF_FAIL);
    }


    rc = CERT_STORE_createStore(p_store);
    if (rc != OK) {
        SSL_TRACE_DBG(("CERT_STORE_createStore() returned an error"));
       *p_err = NET_SECURE_ERR_INSTALL;
        return (DEF_FAIL);
    }

    certificate.length = cert_desc.certLength;
    certificate.data   = cert_desc.pCertificate;
    rc = CERT_STORE_addIdentityWithCertificateChain(*p_store, &certificate, 1, cert_desc.pKeyBlob, cert_desc.keyBlobLength);
    if (rc != OK) {
        SSL_TRACE_DBG(("CERT_STORE_addIdentityWithCertificateChain() returned an error"));
       *p_err = NET_SECURE_ERR_INSTALL;
        return (DEF_FAIL);
    }

   *p_err = NET_SECURE_ERR_NONE;
    SSL_TRACE_DBG(("%s: Normal exit\n", __FUNCTION__));

#else
    rtn_val = DEF_FAIL;
   *p_err   = NET_ERR_FAULT_FEATURE_DIS;
#endif
    return (rtn_val);
}


/*
*********************************************************************************************************
*                                 NetSecure_SockServerCertKeyFiles()
*
* Description : Configure server secure socket's certificate and key from buffers:
*
*
* Argument(s) : p_sock         p_sock      Pointer to the server's socker to configure certificate and key.
*               ------         Argument checked in NetSock_CfgSecure(),
*                                                  NetSock_CfgSecureServerCertKeyInstall().
*
*               pbuf_cert       Pointer to the certificate     buffer to install.
*
*               buf_cert_size   Size    of the certificate     buffer to install.
*
*               pbuf_key        Pointer to the key             buffer to install.
*
*               buf_key_size    Size    of the key             buffer to install.
*
*               fmt             Format  of the certificate and key buffer to install.
*
*                                   NET_SECURE_INSTALL_FMT_PEM      Certificate and Key format is PEM.
*                                   NET_SECURE_INSTALL_FMT_DER      Certificate and Key format is DER.
*
*               cert_chain      Certificate point to a chain of certificate.
*
*                                   DEF_YES     Certificate points to a chain  of certificate.
*                                   DEF_NO      Certificate points to a single    certificate.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE                       Server socket's certificate and key successfully
*                                                                           installed.
*                               NET_SOCK_ERR_NULL_PTR                   Invalid buffer pointer.
*                               NET_SOCK_ERR_SECURE_FMT                 Invalid certificate and key format.
*
* Return(s)   : DEF_OK,   Server socket's certificate and key successfully configured.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : none.
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : (1) NetSock_CfgSecureServerCertKeyBuf() is called by application function(s) & ... :
*
*                   (a) MUST be called with the global network lock already acquired; ...
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock (see 'net.h  Note #3').
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/

#if 0
#if ((NET_SECURE_CFG_MAX_NBR_SOCK_SERVER >  0u)         && \
     (NET_SECURE_CFG_FS_EN               == DEF_ENABLED))
CPU_BOOLEAN  NetSecure_SockServerCertKeyFiles (       NET_SOCK                       *p_sock,
                                               const  void                           *p_filename_cert,
                                               const  void                           *p_filename_key,
                                                      NET_SOCK_SECURE_CERT_KEY_FMT    fmt,
                                                      CPU_BOOLEAN                     cert_chain,
                                                      NET_ERR                        *p_err)
{
    CPU_INT32S               rc;
    NET_SECURE_SESSION      *p_session;
    NET_SECURE_SERVER_DESC  *p_server_desc;
    ubyte                   *p_buf_cert;
    ubyte                   *p_buf_key;
    CPU_INT32U               buf_cert_len;
    CPU_INT32U               buf_key_len;
    CPU_BOOLEAN              rtn_val;


    p_session = (NET_SECURE_SESSION *)p_sock->SecureSession;
    if (p_session == DEF_NULL) {
       *p_err = NET_SOCK_ERR_FAULT;
        return (DEF_FAIL);
    }

    switch(p_session->Type) {
        case NET_SECURE_SOCK_TYPE_UNKNOWN:
        case NET_SECURE_SOCK_TYPE_SERVER:
             break;


        case NET_SECURE_SOCK_TYPE_CLIENT:
        case NET_SECURE_SOCK_TYPE_ACCEPT:
        default:
            *p_err = NET_SECURE_ERR_INSTALL;
             return (DEF_FAIL);
    }

    SSL_TRACE_DBG(("%s: Start\n", __FUNCTION__));

    rc = MOCANA_readFile((sbyte const *)p_filename_cert, &p_buf_cert, &buf_cert_len);
    if (rc != OK) {
        SSL_TRACE_DBG(("%s: failed to read file %s\n",  __FUNCTION__, p_filename_cert));
        return (DEF_FAIL);
    }

    rc = MOCANA_readFile((sbyte const *)p_filename_key, &p_buf_key, &buf_key_len);
    if (rc != OK) {
        MOCANA_freeReadFile(&p_buf_cert);
        SSL_TRACE_DBG(("%s: failed to read file %s\n",  __FUNCTION__, p_filename_cert));
        return (DEF_FAIL);
    }


    rtn_val = NetSecure_SockCertKeyCfg(p_sock,
                                             p_buf_cert,
                                             buf_cert_len,
                                             p_buf_key,
                                             buf_key_len,
                                             fmt,
                                             cert_chain,
                                             p_err);


    MOCANA_freeReadFile(&p_buf_cert);
    MOCANA_freeReadFile(&p_buf_key);

    return (rtn_val);
}
#endif
#endif


/*
*********************************************************************************************************
*                                     NetSecure_ClientCommonNameSet()
*
* Description : Configure client secure socket's common name.
*
* Argument(s) : p_sock          Pointer to the client's socket to configure common name.
*               ------          Argument checked in NetSock_CfgSecure(),
*                                                   NetSock_CfgSecureClientCommonName().
*
*               p_common_name   Pointer to the common name.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE               Close notify alert successfully transmitted.
*                               NET_SECURE_ERR_NULL_PTR         Secure session pointer is NULL.
*                               NET_SECURE_ERR_NOT_AVAIL
*
* Return(s)   : DEF_OK,   Client socket's common name successfully configured.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : NetSock_CfgSecureClientCommonName().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     :  (1) NetSecure_ClientCommonNameSet() is called by application function(s) & ... :
*
*                   (a) MUST be called with the global network lock already acquired; ...
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock (see 'net.h  Note #3').
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/
#if (NET_SECURE_CFG_MAX_NBR_SOCK_CLIENT > 0)
CPU_BOOLEAN  NetSecure_ClientCommonNameSet (NET_SOCK  *p_sock,
                                            CPU_CHAR  *p_common_name,
                                            NET_ERR   *p_err)
{
#ifdef   NET_SECURE_MODULE_EN
    NET_SECURE_SESSION      *p_session;
    NET_SECURE_CLIENT_DESC  *p_desc_client;
    LIB_ERR                  err;


    p_session = (NET_SECURE_SESSION *)p_sock->SecureSession;
    if (p_session == DEF_NULL) {
       *p_err = NET_SOCK_ERR_FAULT;
        return (DEF_FAIL);
    }

    switch(p_session->Type) {
        case NET_SOCK_SECURE_TYPE_NONE:
                                                                /* Get client desc.                                     */
             p_desc_client = (NET_SECURE_CLIENT_DESC *)Mem_DynPoolBlkGet(&NetSecure_Pools.ClientDescPool, &err);
             if (err != LIB_MEM_ERR_NONE) {
                 *p_err = NET_SECURE_ERR_NOT_AVAIL;
                  return (DEF_FAIL);
             }
             p_session->DescPtr = p_desc_client;
             p_session->Type    = NET_SOCK_SECURE_TYPE_CLIENT;
             break;


        case NET_SOCK_SECURE_TYPE_CLIENT:
             if (p_session->DescPtr == DEF_NULL) {
                *p_err = NET_SECURE_ERR_INSTALL;
                 return (DEF_FAIL);
             }

             p_desc_client = (NET_SECURE_CLIENT_DESC *)p_session->DescPtr;
             break;


        case NET_SOCK_SECURE_TYPE_SERVER:
        default:
            *p_err = NET_SECURE_ERR_INSTALL;
             return (DEF_FAIL);
    }

    p_desc_client->CommonNamePtr = p_common_name;

   *p_err = NET_SECURE_ERR_NONE;

    return (DEF_OK);

#else
   *p_err = NET_ERR_FAULT_FEATURE_DIS;
    return (DEF_FAIL);
#endif
}
#endif


/*
*********************************************************************************************************
*                                     NetSecure_ClientTrustCallBackSet()
*
* Description : Configure client secure socket's trust callback function.
*
* Argument(s) : p_sock              Pointer to the client's socket to configure trust call back function.
*               ------              Argument checked in NetSock_CfgSecure(),
*                                                       NetSock_CfgSecureClientTrustCallBack().
*
*               p_callback_fnct     Pointer to the trust call back function
*               ---------------     Argument checked in NetSock_CfgSecureClientTrustCallBack(),
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE               Close notify alert successfully transmitted.
*                               NET_SECURE_ERR_NULL_PTR         Secure session pointer is NULL.
*
* Return(s)   : DEF_OK,   Client socket's trust call back function successfully configured.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : NetSock_CfgSecureClientTrustCallBack().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     :  (1) NetSecure_ClientTrustCallBackSet() is called by application function(s) & ... :
*
*                   (a) MUST be called with the global network lock already acquired; ...
*                   (b) MUST block ALL other network protocol tasks by pending on & acquiring the global
*                       network lock (see 'net.h  Note #3').
*
*                   This is required since an application's network protocol suite API function access is
*                   asynchronous to other network protocol tasks.
*********************************************************************************************************
*/
#if (NET_SECURE_CFG_MAX_NBR_SOCK_CLIENT > 0)
CPU_BOOLEAN  NetSecure_ClientTrustCallBackSet (NET_SOCK                    *p_sock,
                                               NET_SOCK_SECURE_TRUST_FNCT   p_callback_fnct,
                                               NET_ERR                     *p_err)
{
#ifdef   NET_SECURE_MODULE_EN
    NET_SECURE_SESSION      *p_session;
    NET_SECURE_CLIENT_DESC  *p_desc_client;
    LIB_ERR                  err;


    p_session = (NET_SECURE_SESSION *)p_sock->SecureSession;
    if (p_session == DEF_NULL) {
       *p_err = NET_SOCK_ERR_FAULT;
        return (DEF_FAIL);
    }

    switch(p_session->Type) {
        case NET_SOCK_SECURE_TYPE_NONE:
                                                                        /* Get SSL session buffer.                              */
             p_desc_client = (NET_SECURE_CLIENT_DESC *)Mem_DynPoolBlkGet(&NetSecure_Pools.ClientDescPool, &err);
             if (err != LIB_MEM_ERR_NONE) {
                 *p_err = NET_SECURE_ERR_NOT_AVAIL;
                  return (DEF_FAIL);
             }

             p_session->Type = NET_SOCK_SECURE_TYPE_CLIENT;
             break;


        case NET_SOCK_SECURE_TYPE_CLIENT:
             if (p_session->DescPtr == DEF_NULL) {
                *p_err = NET_SECURE_ERR_INSTALL;
                 return (DEF_FAIL);
             }

             p_desc_client = (NET_SECURE_CLIENT_DESC *)p_session->DescPtr;
             break;


        case NET_SOCK_SECURE_TYPE_SERVER:
        default:
            *p_err = NET_SECURE_ERR_INSTALL;
             return (DEF_FAIL);
    }

    p_desc_client->TrustCallBackFnctPtr = p_callback_fnct;

   *p_err = NET_SOCK_ERR_NONE;

    return (DEF_OK);

#else
   *p_err = NET_ERR_FAULT_FEATURE_DIS;
    return (DEF_FAIL);
#endif
}
#endif


/*
*********************************************************************************************************
*                                        NetSecure_SockConn()
*
* Description : (1) Connect a socket to a remote host through an encryted SSL handshake :
*
*                   (a) Get & validate the SSL session of the connected socket
*                   (b) Initialize the     SSL connect.
*                   (c) Perform            SSL handshake.
*
*
* Argument(s) : p_sock      Pointer to a connected socket.
*               ------      Argument checked in NetSock_Conn().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE               Secure socket     successfully connected.
*                               NET_SECURE_ERR_HANDSHAKE        Secure socket NOT successfully connected.
*                               NET_SECURE_ERR_NULL_PTR         Secure session pointer is NULL.
*
* Return(s)   : none.
*
* Caller(s)   : NetSock_Conn().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSecure_SockConn (NET_SOCK  *p_sock,
                          NET_ERR   *p_err)
{
#ifdef   NET_SECURE_MODULE_EN
#ifdef __ENABLE_MOCANA_SSL_CLIENT__
          CPU_INT32S               rc;
          NET_SECURE_SESSION      *p_session;
          NET_SECURE_CLIENT_DESC  *p_client_desc;
    const sbyte                   *p_common_name;


                                                                /* Get & validate SSL session of the connected sock.    */
    p_session = (NET_SECURE_SESSION *)p_sock->SecureSession;
    if (p_session->Type == NET_SOCK_SECURE_TYPE_CLIENT) {
        p_client_desc = (NET_SECURE_CLIENT_DESC *)p_session->DescPtr;
    }

    if (p_client_desc != DEF_NULL) {
        p_common_name = (const sbyte *)p_client_desc->CommonNamePtr;
    } else {
        p_common_name = (const sbyte *)DEF_NULL;
    }

    p_session->Type = NET_SOCK_SECURE_TYPE_CLIENT;




                                                                /* Init SSL connect.                                    */
                                                                /* Save the whole NET_SOCK because some NetOS ...       */
                                                                /* ... functions require it.                            */
    Net_GlobalLockRelease();
    p_session->ConnInstance = SSL_connect(p_sock->ID,
                                          0,
                                          NULL,
                                          NULL,
                                          p_common_name);
    Net_GlobalLockAcquire((void *)&NetSecure_SockConn, p_err);
    if (p_session->ConnInstance < 0) {
        SSL_TRACE_DBG(("%s: %s returned: %s\n", __FUNCTION__, "SSL_Connect", MERROR_lookUpErrorCode((MSTATUS)p_session->ConnInstance )));
       *p_err = NET_SECURE_ERR_HANDSHAKE;
        goto exit;
    }

#ifdef  __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    if (p_client_desc->CertStorePtr != DEF_NULL) {
        SSL_assignCertificateStore(p_session->ConnInstance, p_client_desc->CertStorePtr);
    }
#endif


                                                            /* Perform SSL handshake.                                   */
    rc = -1;
    Net_GlobalLockRelease();
    rc = SSL_negotiateConnection(p_session->ConnInstance);
    Net_GlobalLockAcquire((void *)&NetSecure_SockConn, p_err);
    if (rc != OK) {
        SSL_TRACE_DBG(("%s: %s returned: %s\n", __FUNCTION__, "SSL_negotiateConnection", MERROR_lookUpErrorCode((MSTATUS)rc)));
       *p_err  = NET_SECURE_ERR_HANDSHAKE;
        goto exit;
    }

    SSL_TRACE_DBG(("%s: Normal exit\n", __FUNCTION__));

   *p_err = NET_SOCK_ERR_NONE;
    goto exit;

#else

    *p_err = NET_SECURE_ERR_HANDSHAKE;                          /* Mocana code not compiled with SSL client support.    */
     goto exit;
#endif
#else
    *p_err = NET_ERR_FAULT_FEATURE_DIS;
     goto exit;
#endif

exit:
    return;
}


/*
*********************************************************************************************************
*                                       NetSecure_SockAccept()
*
* Description : (1) Return a new secure socket accepted from a listen socket :
*
*                   (a) Get & validate SSL session of listening socket
*                   (b) Initialize     SSL session of accepted  socket              See Note #2
*                   (c) Initialize     SSL accept
*                   (d) Perform        SSL handshake
*
*
* Argument(s) : p_sock_listen   Pointer to a listening socket.
*               -------------   Argument validated in NetSock_Accept().
*
*               p_sock_accept   Pointer to an accepted socket.
*               -------------   Argument checked   in NetSock_Accept().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE               Secure socket      successfully accepted.
*                               NET_SECURE_ERR_HANDSHAKE        Secure socket  NOT successfully accepted.
*                               NET_SECURE_ERR_NULL_PTR         Secure session pointer is NULL.
*                               NET_SECURE_ERR_NOT_AVAIL        Secure session NOT available.
*
* Return(s)   : none.
*
* Caller(s)   : NetSock_Accept().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : (2) The SSL session of the listening socket has already been validated.  The session
*                   pointer of the accepted socket is also assumed to be valid.
*
*               (3) The listening SSL session is not initialized with the context information.  Then,
*                   the quiet shutdown option SHOULD be set to avoid trying to send encrypted data on
*                   the listening session.
*********************************************************************************************************
*/

void  NetSecure_SockAccept (NET_SOCK  *p_sock_listen,
                            NET_SOCK  *p_sock_accept,
                            NET_ERR   *p_err)

{
#ifdef   NET_SECURE_MODULE_EN
#ifdef __ENABLE_MOCANA_SSL_SERVER__
    CPU_INT32S               rc;
    NET_SECURE_SESSION      *p_session_listen;
    NET_SECURE_SESSION      *p_session_accept;
    NET_SECURE_SERVER_DESC  *p_server_desc;
    NET_ERR                  err;



                                                                /* Get & validate SSL session of listening sock.        */
    p_session_listen = (NET_SECURE_SESSION *)p_sock_listen->SecureSession;
    if (p_session_listen->Type != NET_SOCK_SECURE_TYPE_SERVER) {
       *p_err  = NET_ERR_INVALID_TYPE;
        goto exit;
    }

    p_server_desc = (NET_SECURE_SERVER_DESC *)p_session_listen->DescPtr;
    if (p_server_desc == DEF_NULL) {
       *p_err  = NET_ERR_INVALID_STATE;
        goto exit;
    }

    rc = -1;
    SSL_TRACE_DBG(("%s: Start\n", __FUNCTION__));


                                                                /* Initialize SSL session of accepted sock.             */
    NetSecure_InitSession(p_sock_accept, p_err);
    if (*p_err != NET_SECURE_ERR_NONE) {
         SSL_TRACE_DBG(("Error: NO session available\n"));
        *p_err  = NET_SECURE_ERR_NOT_AVAIL;
         goto exit;
    }


    p_session_accept       = (NET_SECURE_SESSION *)p_sock_accept->SecureSession;
    p_session_accept->Type =  NET_SOCK_SECURE_TYPE_SERVER;

                                                                /* Init SSL accept.                                     */
                                                                /* Save the whole NET_SOCK because some NetOS ...       */
                                                                /* ... functions require it.                            */
    p_session_accept->ConnInstance = SSL_acceptConnection((sbyte4)p_sock_accept->ID);
    if (p_session_accept->ConnInstance < 0) {
        SSL_TRACE_INFO(("%s: %s returned: %s\n", __FUNCTION__, "SSL_acceptConnection",
                         MERROR_lookUpErrorCode((MSTATUS)p_session_accept->ConnInstance)));
       *p_err = NET_SECURE_ERR_NOT_AVAIL;
        goto exit;
    } else {
        SSL_TRACE_DBG(("SSL_acceptConnection() accepted connection: ConnInstance = %d\n", p_session_accept->ConnInstance));
    }


    rc = SSL_assignCertificateStore(p_session_accept->ConnInstance, p_server_desc->CertStorePtr);
    if (rc != OK) {
        SSL_TRACE_INFO(("%s: %s returned: %s\n", __FUNCTION__, "SSL_assignCertificateStore",
                         MERROR_lookUpErrorCode((MSTATUS)p_session_accept->ConnInstance)));
        goto exit;
    }

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__
    rc = SSL_setSessionFlags(p_session_accept->ConnInstance, SSL_FLAG_NO_MUTUAL_AUTH_REQUEST);
    if (rc != OK) {
        SSL_TRACE_INFO(("%s: %s returned: %s\n", __FUNCTION__, "SSL_setSessionFlags",
                         MERROR_lookUpErrorCode((MSTATUS)p_session_accept->ConnInstance)));
        goto exit;
    }
#endif

    Net_GlobalLockRelease();
                                                                /* Perform SSL handshake.                               */
    rc = SSL_negotiateConnection(p_session_accept->ConnInstance);
    Net_GlobalLockAcquire((void *)&NetSecure_SockAccept, &err);
    if (rc != OK) {
        NetSecure_SockClose(p_sock_accept, p_err);
        SSL_TRACE_INFO(("%s: %s returned: %s\n", __FUNCTION__, "SSL_negotiateConnection", MERROR_lookUpErrorCode((MSTATUS)rc)));
       *p_err  = NET_SECURE_ERR_HANDSHAKE;
        goto exit;
    }


   *p_err = NET_SOCK_ERR_NONE;
    SSL_TRACE_DBG(("%s: Normal exit\n", __FUNCTION__));
    goto exit;

exit:
    return;

#else

   *p_err = NET_SECURE_ERR_NOT_AVAIL;                           /* Mocana code not compiled with SSL server support.    */

#endif
#else
   *p_err = NET_ERR_FAULT_FEATURE_DIS;
#endif
}


/*
*********************************************************************************************************
*                                    NetSecure_SockRxDataHandler()
*
* Description : Receive clear data through a secure socket :
*
*                   (a) Get & validate the SSL session of the receiving socket
*                   (b) Receive the data
*
*
* Argument(s) : p_sock          Pointer to a receive socket.
*               ------          Argument checked in NetSock_RxDataHandler().
*
*               p_data_buf      Pointer to an application data buffer that will receive the socket's
*               ----------         received data.
*
*                               Argument checked in NetSock_RxDataHandler().
*
*               data_buf_len    Size of the   application data buffer (in octets).
*               ------------    Argument checked in NetSock_RxDataHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE               Clear data successfully received.
*                               NET_SOCK_ERR_RX_Q_CLOSED        Socket receive queue closed.
*
*                               NET_SECURE_ERR_NULL_PTR         Secure session pointer is NULL.
*                               NET_ERR_RX                      Receive error.
*
* Return(s)   : Number of positive data octets received, if NO error(s).
*
*               NET_SOCK_BSD_ERR_RX,                         otherwise.
*
* Caller(s)   : NetSock_RxDataHandler().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSecure_SockRxDataHandler (NET_SOCK    *p_sock,
                                                void        *p_data_buf,
                                                CPU_INT16U   data_buf_len,
                                                NET_ERR     *p_err)
{
#ifdef   NET_SECURE_MODULE_EN
    NET_SECURE_SESSION  *p_session;
    CPU_INT32S           rxd;
    CPU_INT32S           rc;


    rxd = 0;
    rc = -1;

    SSL_TRACE_DBG(("%s: Start\n", __FUNCTION__));



    p_session = (NET_SECURE_SESSION *)p_sock->SecureSession;

    Net_GlobalLockRelease();
    rc = SSL_recv(          p_session->ConnInstance,
                            p_data_buf,
                            data_buf_len,
                 (sbyte4 *)&rxd,
                            0);
    Net_GlobalLockAcquire((void *)&NetSecure_SockRxDataHandler, p_err);
    if (OK != rc) {
        if (ERR_TCP_SOCKET_CLOSED == rc) {
           *p_err = NET_SOCK_ERR_CLOSED;
            return NET_SOCK_BSD_RTN_CODE_CONN_CLOSED;
        } else {
            SSL_TRACE_DBG(("%s: %s returned: %s\n", __FUNCTION__, "SSL_recv", MERROR_lookUpErrorCode((MSTATUS)rc)));
           *p_err = NET_ERR_RX;
            return NET_SOCK_BSD_ERR_RX;
        }
    }

   *p_err = NET_SOCK_ERR_NONE;

    SSL_TRACE_DBG(("%s: Normal exit\n", __FUNCTION__));

    return (rxd);                                               /* if successful return the number of bytes read.       */

#else
   *p_err = NET_ERR_FAULT_FEATURE_DIS;
    return (0u);
#endif
}


/*
*********************************************************************************************************
*                                    NetSecure_SockRxIsDataPending()
*
* Description : Is data pending in SSL receive queue.
*
* Argument(s) : p_sock  Pointer to a receive socket.
*               ------  Argument checked in NetSock_IsAvailRxStream().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE               Return values is valid.
*                               NET_SOCK_ERR_FAULT              Fault.
*
* Return(s)   : DEF_YES, If data is pending.
*
*               DEF_NO,  Otherwise
*
* Caller(s)   : NetSock_IsAvailRxStream().
*
* Note(s)     : none.
*********************************************************************************************************
*/
#if (NET_SOCK_CFG_SEL_EN == DEF_ENABLED)
CPU_BOOLEAN  NetSecure_SockRxIsDataPending (NET_SOCK  *p_sock,
                                            NET_ERR   *p_err)
{
    sbyte4               pending = DEF_NO;
    sbyte4               status;
    NET_SECURE_SESSION  *p_session;



    p_session = (NET_SECURE_SESSION *)p_sock->SecureSession;
    if (p_session == DEF_NULL) {
       *p_err = NET_SOCK_ERR_FAULT;
        goto exit;
    }

    status = SSL_recvPending(p_session->ConnInstance, &pending);
    if (status != OK) {
       *p_err = NET_SOCK_ERR_FAULT;
        goto exit;
    }

   *p_err = NET_SOCK_ERR_NONE;

exit:
    return ((CPU_BOOLEAN)pending);
}
#endif


/*
*********************************************************************************************************
*                                    NetSecure_SockTxDataHandler()
*
* Description : Transmit clear data through a secure socket :
*
*                   (a) Get & validate the SSL session of the transmitting socket
*                   (b) Transmit the data
*
*
* Argument(s) : p_sock          Pointer to a transmit socket.
*               ------          Argument checked in NetSock_TxDataHandler().
*
*               p_data_buf      Pointer to application data to transmit.
*               ----------      Argument checked in NetSock_TxDataHandler().
*
*               data_buf_len    Length of  application data to transmit (in octets).
*               ------------    Argument checked in NetSock_TxDataHandler().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE               Clear data   successfully transmitted.
*                               NET_SECURE_ERR_NULL_PTR         Secure session pointer is NULL.
*                               NET_ERR_TX                      Transmit error.
*
* Return(s)   : Number of positive data octets transmitted, if NO error(s).
*
*               NET_SOCK_BSD_ERR_RX,                            otherwise.
*
* Caller(s)   : NetSock_TxDataHandler().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

NET_SOCK_RTN_CODE  NetSecure_SockTxDataHandler (NET_SOCK    *p_sock,
                                                void        *p_data_buf,
                                                CPU_INT16U   data_buf_len,
                                                NET_ERR     *p_err)
{
#ifdef   NET_SECURE_MODULE_EN
    NET_SECURE_SESSION  *p_session;
    sbyte4               rc = -1;

    SSL_TRACE_DBG(("%s: Start\n", __FUNCTION__));

    p_session    = (NET_SECURE_SESSION *)p_sock->SecureSession;
    Net_GlobalLockRelease();
    rc = SSL_send(p_session->ConnInstance, p_data_buf, data_buf_len);
    Net_GlobalLockAcquire((void *)&NetSecure_SockTxDataHandler, p_err);
    if (0 > rc) {
        SSL_TRACE_DBG(("%s: %s returned: %s\n", __FUNCTION__, "SSL_send", MERROR_lookUpErrorCode((MSTATUS)rc)));
        *p_err = NET_ERR_TX;
        return NET_SOCK_BSD_ERR_RX;
    }

    *p_err = NET_SOCK_ERR_NONE;

    SSL_TRACE_DBG(("%s: Normal exit\n", __FUNCTION__));

    return rc;
#else
    *p_err = NET_ERR_FAULT_FEATURE_DIS;
     return (-1);
#endif
}


/*
*********************************************************************************************************
*                                        NetSecure_SockClose()
*
* Description : (1) Close the secure socket :
*
*                   (a) Get & validate the SSL session of the socket to close
*                   (b) Transmit close notify alert to the peer
*                   (c) Free the SSL session buffer
*
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument checked in NetSock_Close().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE               Secure session successfully closed.
*                               NET_SECURE_ERR_NULL_PTR         Secure session pointer is NULL.
*
* Return(s)   : none.
*
* Caller(s)   : NetSock_CloseSockHandler(),
*               NetSecure_SockCloseNotify().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSecure_SockClose (NET_SOCK  *p_sock,
                           NET_ERR   *p_err)
{
#ifdef   NET_SECURE_MODULE_EN
    sbyte4                   status;
    NET_SECURE_SESSION      *p_session;
    NET_SECURE_SERVER_DESC  *p_server_desc;
    NET_SECURE_CLIENT_DESC  *p_client_desc;
    LIB_ERR                  err;


    SSL_TRACE_DBG(("%s: Start\n", __FUNCTION__));

    p_session = (NET_SECURE_SESSION *)p_sock->SecureSession;

    if (p_session != DEF_NULL) {
        SSL_TRACE_DBG(("SSL_closeConnection: ConnInstance = %d\n", p_session->ConnInstance));
        status = SSL_closeConnection(p_session->ConnInstance);
        if (status != OK) {
            SSL_TRACE_DBG(("SSL_closeConnection return error: %s\n", MERROR_lookUpErrorCode((MSTATUS)status)));
        }

        if (p_session->DescPtr != DEF_NULL) {
            switch (p_session->Type) {
                case NET_SOCK_SECURE_TYPE_SERVER:
                     p_server_desc = (NET_SECURE_SERVER_DESC *)p_session->DescPtr;

                     CA_MGMT_freeKeyBlob(&p_server_desc->CertDesc.pKeyBlob);
                     CA_MGMT_freeCertificate(&p_server_desc->CertDesc);
                     CERT_STORE_releaseStore(&p_server_desc->CertStorePtr);

                     Mem_DynPoolBlkFree(&NetSecure_Pools.ServerDescPool, p_server_desc, &err);
                     if (err != LIB_MEM_ERR_NONE) {
                         SSL_TRACE_DBG(("Mem_DynPoolBlkFree() returned an error:\n"));
                     }
                     break;


                case NET_SOCK_SECURE_TYPE_CLIENT:
                     p_client_desc = (NET_SECURE_CLIENT_DESC *)p_session->DescPtr;

#ifdef __ENABLE_MOCANA_SSL_MUTUAL_AUTH_SUPPORT__

                     if (p_client_desc->CertDesc.pKeyBlob != DEF_NULL) {
                         CA_MGMT_freeKeyBlob(&p_client_desc->CertDesc.pKeyBlob);
                     }

                     if (p_client_desc->CertDesc.pCertificate != DEF_NULL) {
                         CA_MGMT_freeCertificate(&p_client_desc->CertDesc);
                     }

                     if (p_client_desc->CertStorePtr != DEF_NULL) {
                         CERT_STORE_releaseStore(&p_client_desc->CertStorePtr);
                     }
#endif

                     Mem_DynPoolBlkFree(&NetSecure_Pools.ClientDescPool, p_client_desc, &err);
                     if (err != LIB_MEM_ERR_NONE) {
                         SSL_TRACE_DBG(("Mem_DynPoolBlkFree() returned an error:\n"));
                     }
                     break;


                case NET_SOCK_SECURE_TYPE_NONE:
                default:
                     break;
            }
        }

        Mem_DynPoolBlkFree(&NetSecure_Pools.SessionPool, p_sock->SecureSession, &err);
        if (err != LIB_MEM_ERR_NONE) {
            SSL_TRACE_DBG(("Mem_DynPoolBlkFree() returned an error:\n"));
        }
    }

    p_sock->SecureSession = DEF_NULL;

    SSL_TRACE_DBG(("%s: Normal exit\n", __FUNCTION__));

   *p_err = NET_SOCK_ERR_NONE;

#else
   *p_err = NET_ERR_FAULT_FEATURE_DIS;
#endif
}


/*
*********************************************************************************************************
*                                     NetSecure_SockCloseNotify()
*
* Description : Transmit the close notify alert to the peer through a SSL session.
*
* Argument(s) : p_sock      Pointer to a socket.
*               ------      Argument checked in NetSock_Close().
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                               NET_SOCK_ERR_NONE               Close notify alert successfully transmitted.
*                               NET_SECURE_ERR_NULL_PTR         Secure session pointer is NULL.
*
* Return(s)   : none.
*
* Caller(s)   : NetSock_Close(),
*               NetSecure_SockClose().
*
*               This function is an INTERNAL network protocol suite function & MUST NOT be called by
*               application function(s).
*
* Note(s)     : (1) If the server decides to close the connection, it SHOULD send a close notify
*                   alert to the connected peer prior to perform the socket close operations.
*
*               (2) (a) This function will be called twice during a socket close process but the
*                       close notify alert will only transmitted during the first call.
*
*                   (b) The error code that might me returned by 'SSL_shutdown()' is ignored because the
*                       connection can be closed by the client. In that case, the SSL session will no
*                       longer be valid and it will be impossible to send the close notify alert through
*                       that session.
*********************************************************************************************************
*/

void  NetSecure_SockCloseNotify (NET_SOCK  *p_sock,
                                 NET_ERR   *p_err)
{
    SSL_TRACE_DBG(("%s: Start\n", __FUNCTION__));

    NetSecure_SockClose(p_sock, p_err);

    SSL_TRACE_DBG(("%s: Normal exit\n", __FUNCTION__));


   *p_err = NET_SOCK_ERR_NONE;
}


/*
*********************************************************************************************************
*                                      MOCANA CERTIFICATE CALLBACK FUNCTIONS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                     NetSecure_MocanaFnctLog()
*
* Description : Mocana log call back function.
*
* Argument(s) : module      Mocana module.
*
*               severity    Error severity level.
*
*               p_msg       Error message to log.
*
* Return(s)   : none.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  NetSecure_MocanaFnctLog (sbyte4   module,
                               sbyte4   severity,
                               sbyte   *p_msg)
{
    SSL_TRACE_DBG(((CPU_CHAR *)p_msg));
}


/*
*********************************************************************************************************
*                                     NetSecure_CertificateStoreVerify()
*
* Description : Verify certificate in store.
*
* Argument(s) : connectionInstance  SSL connection instance.
*
*               p_cert              Pointer to the certificate to valid
*
*               cert_len            Certificate lenght.
*
*               isSelfSigned        Certificate is self self signed
*
* Return(s)   : OK, if certificate is trusted.
*
*               -1, otherwise.
*
* Caller(s)   : various Mocana function
*
* Note(s)     : none.
*********************************************************************************************************
*/
static  sbyte4  NetSecure_CertificateStoreVerify (sbyte4   conn_instance,
                                                  ubyte   *p_cert,
                                                  ubyte4   cert_len,
                                                  sbyte4   isSelfSigned)
{
#ifdef   NET_SECURE_MODULE_EN
    NET_SOCK                *p_sock = DEF_NULL;
    NET_SECURE_SESSION      *p_session;
    NET_SECURE_CLIENT_DESC  *p_client_desc;
    certDistinguishedName    dn;
    CPU_BOOLEAN              trusted;
    CPU_BOOLEAN              result;
    sbyte4                   sock_id;
    sbyte4                   status;


    status = SSL_getSocketId(conn_instance, &sock_id);
    if (status != OK) {
        return (status);
    }

    status = -1;

    p_sock = NetSock_GetObj(sock_id);
    if (p_sock == DEF_NULL) {
        return (status);
    }

    p_session = (NET_SECURE_SESSION *)p_sock->SecureSession;
    if (p_session->Type != NET_SOCK_SECURE_TYPE_CLIENT) {
        return (status);
    }

    if (isSelfSigned == DEF_YES) {
        p_client_desc = (NET_SECURE_CLIENT_DESC *)p_session->DescPtr;
        if (p_client_desc->TrustCallBackFnctPtr != DEF_NULL) {
            status = CA_MGMT_extractCertDistinguishedName (p_cert, cert_len, FALSE, &dn);
            if (status == OK) {
                trusted = p_client_desc->TrustCallBackFnctPtr(&dn, NET_SOCK_SECURE_SELF_SIGNED);
                if (trusted == DEF_YES) {
                    status = OK;
                }
            }
        }

    } else {
        result = Mem_Cmp(p_cert, CaCertDesc.pCertificate, cert_len);
        if ((CaCertDesc.certLength == cert_len) &&
            (result                == DEF_YES)) {
            status = OK;                                        /* we trust this cert.                                  */
        }
    }

    return (status);

#else
    return (-1);
#endif
}


/*
*********************************************************************************************************
*                                     NetSecure_CertificateStoreLookup()
*
* Description : Find CA certificate in store.
*
* Argument(s) : conn_instance           SSL connection instance.
*
*               certDistinguishedName   Received certificate distinguished name.
*
*               p_return_cert           Pointer to Certificate descriptor
*
* Return(s)   : OK.
*
* Caller(s)   : various Mocana function
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  sbyte4  NetSecure_CertificateStoreLookup(sbyte4                  conn_instance,
                                                 certDistinguishedName  *p_lookup_cert_dn,
                                                 certDescriptor         *p_return_cert)
{
    MOC_UNUSED(conn_instance);
    MOC_UNUSED(p_lookup_cert_dn);


                                                                /* For this implementation, we only recognize one ...   */
                                                                /* ... cert authority.                                  */
    p_return_cert->pCertificate = CaCertDesc.pCertificate;
    p_return_cert->certLength   = CaCertDesc.certLength;
    p_return_cert->cookie       = 0;


    return (OK);
}


/*
*********************************************************************************************************
*                                      NetSecure_CertKeyConvert()
*
* Description : (1) Convert Certificate and Key and allocate memory if needed to store converted certificate and key if needed.
*
*                   (a) DER certificate are not converted, the orignal certificate buffer is used.
*                   (b) PEM certificate are converted to DER certificatr and it is stored in an internal buffer.
*                   (c) All Keys are converted in Mocana KeyBlog and key is stored in internal buffer.
*
* Argument(s) : p_cert      Pointer to the certificate
*
*               cert_size   Certificate length
*
*               p_key       Pointer to the key
*
*               key_size    Key length
*
*               fmt         Certificate and key format
*
*               p_err       Pointer to variable that will receive the return error code from this function.
*
* Return(s)   : Certificate descriptor that contain the location of the certificate and the key.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  certDescriptor NetSecure_CertKeyConvert (const  CPU_INT08U                     *p_cert,
                                                        CPU_SIZE_T                      cert_size,
                                                 const  CPU_INT08U                     *p_key,
                                                        CPU_SIZE_T                      key_size,
                                                        NET_SOCK_SECURE_CERT_KEY_FMT    fmt,
                                                        NET_ERR                        *p_err)
{
    certDescriptor  cert_desc;
    CPU_INT32S      rc;



    Mem_Set(&cert_desc, 0, sizeof(cert_desc));


   *p_err = NET_SECURE_ERR_NONE;

    switch (fmt) {
        case NET_SOCK_SECURE_CERT_KEY_FMT_PEM:
             rc = CA_MGMT_decodeCertificate((ubyte *)p_cert, cert_size, &cert_desc.pCertificate, &cert_desc.certLength);
             if (rc != OK) {
                *p_err = NET_SECURE_ERR_INSTALL;
                 goto exit_fail;
             }

             rc = CA_MGMT_convertKeyPEM((ubyte *)p_key, key_size, &cert_desc.pKeyBlob, &cert_desc.keyBlobLength);
             if (rc != OK) {
                *p_err = NET_SECURE_ERR_INSTALL;
                 goto exit_fail;
             }
             break;


        case NET_SOCK_SECURE_CERT_KEY_FMT_DER:
             cert_desc.pCertificate = (ubyte *)p_cert;
             cert_desc.certLength   = cert_size;
             rc = CA_MGMT_convertKeyDER((ubyte *)p_key, key_size, &cert_desc.pKeyBlob, &cert_desc.keyBlobLength);
             if (rc != OK) {
                *p_err = NET_SECURE_ERR_INSTALL;
                 goto exit_fail;
             }
             break;

#if 0
        case NET_SOCK_SECURE_CERT_KEY_FMT_NATIVE:
                                                                /* We can create a NATIVE format to avoid allocating    */
                                                                /* memory                                               */
             cert_desc.pCertificate  = (ubyte *)p_cert;
             cert_desc.certLength    =  cert_size;
             cert_desc.pKeyBlob      =  p_key;
             cert_desc.keyBlobLength =  key_size;
             break;
#endif

        default:
            *p_err = NET_DEV_ERR_FAULT;
             goto exit_fail;
    }


    goto exit;

exit_fail:
    CA_MGMT_freeCertificate(&cert_desc);
    CA_MGMT_freeKeyBlob(&cert_desc.pKeyBlob);

exit:
    return (cert_desc);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* NET_SECURE_MODULE_EN */
