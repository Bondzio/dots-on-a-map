/* Version: mss_v6_3 */
/*
 * OIDDefs.c
 *
 * OID Definitions
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#include "../common/moptions.h"
#include "../common/mtypes.h"

#include "../asn1/oiddefs.h"

/* from pkcs-1v2-1.asn on the RSA web site
-- ============================
--   Basic object identifiers
-- ============================

-- The DER encoding of this in hexadecimal is:
-- (0x)06 08
--        2A 86 48 86 F7 0D 01 01
--
pkcs-1    OBJECT IDENTIFIER ::= {
    iso(1) member-body(2) us(840) rsadsi(113549) pkcs(1) 1
}

--
-- When rsaEncryption is used in an AlgorithmIdentifier the parameters
-- MUST be present and MUST be NULL.
--
rsaEncryption    OBJECT IDENTIFIER ::= { pkcs-1 1 }

--
-- When id-RSAES-OAEP is used in an AlgorithmIdentifier the parameters MUST
-- be present and MUST be RSAES-OAEP-params.
--
id-RSAES-OAEP    OBJECT IDENTIFIER ::= { pkcs-1 7 }

--
-- When id-pSpecified is used in an AlgorithmIdentifier the parameters MUST
-- be an OCTET STRING.
--
id-pSpecified    OBJECT IDENTIFIER ::= { pkcs-1 9 }

--
-- When id-RSASSA-PSS is used in an AlgorithmIdentifier the parameters MUST
-- be present and MUST be RSASSA-PSS-params.
--
id-RSASSA-PSS    OBJECT IDENTIFIER ::= { pkcs-1 10 }

--
-- When the following OIDs are used in an AlgorithmIdentifier the parameters
-- MUST be present and MUST be NULL.
--
md2WithRSAEncryption       OBJECT IDENTIFIER ::= { pkcs-1 2 }
md5WithRSAEncryption       OBJECT IDENTIFIER ::= { pkcs-1 4 }
sha1WithRSAEncryption      OBJECT IDENTIFIER ::= { pkcs-1 5 }
sha256WithRSAEncryption    OBJECT IDENTIFIER ::= { pkcs-1 11 }
sha384WithRSAEncryption    OBJECT IDENTIFIER ::= { pkcs-1 12 }
sha512WithRSAEncryption    OBJECT IDENTIFIER ::= { pkcs-1 13 }
sha224WithRSAEncryption    OBJECT IDENTIFIER ::= { pkcs-1 14 }

  <snip>

-- ===================
--   Main structures
-- ===================

RSAPublicKey ::= SEQUENCE {
    modulus           INTEGER,  -- n
    publicExponent    INTEGER   -- e
}

*/
/* The OIDs for PKCS 1 always start with the same bytes
    the final byte indicates the subtype */
/* pkcs-1 (1 2 840 113549 1 1) */
/*rsaEncryption */

/*
    1 = rsaEncryption
        { 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01}
        (1 2 840 113549 1 1 1)
    2 = md2withRSAEncryption
        { 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x02}
        (1 2 840 113549 1 1 2)
    3 = md4withRSAEncryption
        { 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x03}
        (1 2 840 113549 1 1 3)
    4 = md5withRSAEncryption
        { 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x04}
        (1 2 840 113549 1 1 4)
    5 = sha1withRSAEncryption
        { 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05}
        (1 2 840 113549 1 1 5)
    11 = sha256WithRSAEncryption
    12 = sha384WithRSAEncryption
    13 = sha512WithRSAEncryption
    14 = sha224WithRSAEncryption
*/
/**************** OID MUST BE LENGTH PREFIXED **************************/

MOC_EXTERN_DATA_DEF const ubyte pkcs1_OID[] = { 8, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01};

MOC_EXTERN_DATA_DEF const ubyte rsaEncryption_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01};

MOC_EXTERN_DATA_DEF const ubyte md5withRSAEncryption_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x04};

MOC_EXTERN_DATA_DEF const ubyte sha1withRSAEncryption_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x05};

MOC_EXTERN_DATA_DEF const ubyte sha256withRSAEncryption_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B};

MOC_EXTERN_DATA_DEF const ubyte sha384withRSAEncryption_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0C};

MOC_EXTERN_DATA_DEF const ubyte sha512withRSAEncryption_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0D};

MOC_EXTERN_DATA_DEF const ubyte sha224withRSAEncryption_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0E};

/*
# RSADSI digest algorithms

OID = 06 07 2A 86 48 86 F7 0D 02
Description = digestAlgorithm

OID = 06 08 2A 86 48 86 F7 0D 02 02
Comment = RSADSI digestAlgorithm
Description = md2 (1 2 840 113549 2 2)

OID = 06 08 2A 86 48 86 F7 0D 02 04
Comment = RSADSI digestAlgorithm
Description = md4 (1 2 840 113549 2 4)

OID = 06 08 2A 86 48 86 F7 0D 02 05
Comment = RSADSI digestAlgorithm
Description = md5 (1 2 840 113549 2 5)

OID = 06 08 2A 86 48 86 F7 0D 02 07
Comment = RSADSI digestAlgorithm
Description = hmacWithSHA1 (1 2 840 113549 2 7)
*/

MOC_EXTERN_DATA_DEF const ubyte rsaDSI_OID[] =  { 7, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02};


/*OID = 06 08 2A 86 48 86 F7 0D 02 05
Comment = RSADSI digestAlgorithm
Description = md5 (1 2 840 113549 2 5) */
/* CONVENIENT, repeat of above information */
MOC_EXTERN_DATA_DEF const ubyte md5_OID[] = { 8, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05};

/* OID = 06 05 2B 0E 03 02 1A
Comment = OIW
Description = sha1 (1 3 14 3 2 26) */
MOC_EXTERN_DATA_DEF const ubyte sha1_OID[] = { 5, 0x2B, 0x0E, 0x03, 0x02, 0x1A }; /* 1 3 14 3 2 26 */

/* OID = 06 05 2B 0E 03 02 1D
Comment = OIW (from 1989)
Description = sha1withRsaSignature (1 3 14 3 2 29) */
MOC_EXTERN_DATA_DEF const ubyte sha1withRsaSignature_OID[] = { 5, 0x2B, 0x0E, 0x03, 0x02, 0x1d }; /* 1 3 14 3 2 29 */

/* the other SHA hash algos, aka SHA2
OID = 06 09 60 86 48 01 65 03 04 02 01
Comment = NIST Algorithm
Description = sha2-256 (2 16 840 1 101 3 4 2 1)

OID = 06 09 60 86 48 01 65 03 04 02 02
Comment = NIST Algorithm
Description = sha2-384 (2 16 840 1 101 3 4 2 2)

OID = 06 09 60 86 48 01 65 03 04 02 03
Comment = NIST Algorithm
Description = sha2-512 (2 16 840 1 101 3 4 2 3)

OID = 06 09 60 86 48 01 65 03 04 02 04
Comment = NIST Algorithm
Description = sha2-224 (2 16 840 1 101 3 4 2 4)
*/

MOC_EXTERN_DATA_DEF const ubyte sha2_OID[]   = {8, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02}; /* 2 16 840 1 101 3 4 2 */
MOC_EXTERN_DATA_DEF const ubyte sha224_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04}; /* 2 16 840 1 101 3 4 2 4 */
MOC_EXTERN_DATA_DEF const ubyte sha256_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01}; /* 2 16 840 1 101 3 4 2 1 */
MOC_EXTERN_DATA_DEF const ubyte sha384_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02}; /* 2 16 840 1 101 3 4 2 2 */
MOC_EXTERN_DATA_DEF const ubyte sha512_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03}; /* 2 16 840 1 101 3 4 2 3 */

/*
OID = 06 05 2B 0E 03 02 06
Description = desECB (1 3 14 3 2 6)

OID = 06 05 2B 0E 03 02 07
Description = desCBC (1 3 14 3 2 7)

OID = 06 05 2B 0E 03 02 08
Description = desOFB (1 3 14 3 2 8)

OID = 06 05 2B 0E 03 02 09
Description = desCFB (1 3 14 3 2 9)
*/

MOC_EXTERN_DATA_DEF const ubyte desCBC_OID[] = { 5, 0x2B, 0x0E, 0x03, 0x02, 0x07 };    /* 1 3 14 3 2 7 */

/* AES oids */
MOC_EXTERN_DATA_DEF const ubyte aes128CBC_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x02};     /* 2.16.840.1.101.3.4.1.2 */
MOC_EXTERN_DATA_DEF const ubyte aes192CBC_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x16};     /* 2.16.840.1.101.3.4.1.22 */
MOC_EXTERN_DATA_DEF const ubyte aes256CBC_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x2a};     /* 2.16.840.1.101.3.4.1.42 */


/* X.509 Certificate Extensions */
MOC_EXTERN_DATA_DEF const ubyte subjectKeyIdentifier_OID[] = { 3, 0x55, 0x1D, 0x0E}; /* 2.5.29.14 */
MOC_EXTERN_DATA_DEF const ubyte keyUsage_OID[] = { 3, 0x55, 0x1D, 0x0F};  /* 2.5.29.15 */
MOC_EXTERN_DATA_DEF const ubyte subjectAltName_OID[] = { 3, 0x55, 0x1D, 0x11};  /* 2.5.29.17 */
MOC_EXTERN_DATA_DEF const ubyte issuerAltName_OID[] = { 3, 0x55, 0x1D, 0x12};  /* 2.5.29.18 */
MOC_EXTERN_DATA_DEF const ubyte basicConstraints_OID[] = { 3, 0x55, 0x1D, 0x13}; /* 2.5.29.19 */
MOC_EXTERN_DATA_DEF const ubyte crlNumber_OID[] = { 3, 0x55, 0x1D, 0x14};         /* 2.5.29.20 */
MOC_EXTERN_DATA_DEF const ubyte crlReason_OID[] = { 3, 0x55, 0x1D, 0x15};         /* 2.5.29.21 */
MOC_EXTERN_DATA_DEF const ubyte invalidityDate_OID[] = { 3, 0x55, 0x1D, 0x18};    /* 2.5.29.24 */
MOC_EXTERN_DATA_DEF const ubyte nameConstraints_OID[] = { 3, 0x55, 0x1D, 0x1E};  /* 2.5.29.30 */
MOC_EXTERN_DATA_DEF const ubyte crl_OID[] = { 3, 0x55, 0x1D, 0x1F }; /* 2.5.29.31 */
MOC_EXTERN_DATA_DEF const ubyte certificatePolicies_OID[] = { 3, 0x55, 0x1D, 0x20 }; /* 2.5.29.32 */
MOC_EXTERN_DATA_DEF const ubyte authorityKeyIdentifier_OID[] = { 3, 0x55, 0x1D, 0x23 }; /* 2.5.29.35 */
MOC_EXTERN_DATA_DEF const ubyte extendedKeyUsage_OID[] = { 3, 0x55, 0x1D, 0x25 }; /* 2.5.29.37 */
MOC_EXTERN_DATA_DEF const ubyte userPrincipalName_OID[] = { 10, 0x2b, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x14, 0x02, 0x03 }; /* 1.3.6.1.4.1.311.20.2.3 */

MOC_EXTERN_DATA_DEF const ubyte dsa_OID[] =
    { 7, 0x2A, 0x86, 0x48, 0xCE, 0x38, 0x04, 0x01 }; /* 1 2 840 10040 4 1*/

MOC_EXTERN_DATA_DEF const ubyte dsaWithSHA1_OID[] =
    { 7, 0x2A, 0x86, 0x48, 0xCE, 0x38, 0x04, 0x03 }; /* 1 2 840 10040 4 3*/

MOC_EXTERN_DATA_DEF const ubyte dsaWithSHA2_OID[] =
    { 8, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x03 };/* 2 16 840 1 101 3 4 3 */

/* X.520 */
MOC_EXTERN_DATA_DEF const ubyte commonName_OID[] =              { 3, 0x55, 0x04, 0x03 };    /* 2.5.4.3 */
MOC_EXTERN_DATA_DEF const ubyte serialNumber_OID[] =			{ 3, 0x55, 0x04, 0x05 };	/* 2.5.4.5 */
MOC_EXTERN_DATA_DEF const ubyte countryName_OID[] =             { 3, 0x55, 0x04, 0x06 };    /* 2.5.4.6 */
MOC_EXTERN_DATA_DEF const ubyte localityName_OID[] =            { 3, 0x55, 0x04, 0x07 };    /* 2 5 4 7 */
MOC_EXTERN_DATA_DEF const ubyte stateOrProvinceName_OID[] =     { 3, 0x55, 0x04, 0x08 };    /* 2 5 4 8 */
MOC_EXTERN_DATA_DEF const ubyte organizationName_OID[] =        { 3, 0x55, 0x04, 0x0A };    /* 2 5 4 10 */
MOC_EXTERN_DATA_DEF const ubyte organizationalUnitName_OID[] =  { 3, 0x55, 0x04, 0x0B };    /* 2 5 4 11 */

/* rfc2247 */
MOC_EXTERN_DATA_DEF const ubyte domainComponent_OID[] = {10, 0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x19};

/* rfc1274, rfc4519 */
MOC_EXTERN_DATA_DEF const ubyte userID_OID[] = {10, 0x09, 0x92, 0x26, 0x89, 0x93, 0xf2, 0x2c, 0x64, 0x01, 0x01}; /* 0.9.2342.19200300.100.1.1 */

/* PKCS 7*/
MOC_EXTERN_DATA_DEF const ubyte pkcs7_root_OID[] =
    { 8, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07}; /* 1.2.840.113549.1.7 */
MOC_EXTERN_DATA_DEF const ubyte pkcs7_data_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x01 }; /* 1.2.840.113549.1.7.1 */
MOC_EXTERN_DATA_DEF const ubyte pkcs7_signedData_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x02 }; /* 1.2.840.113549.1.7.2 */
MOC_EXTERN_DATA_DEF const ubyte pkcs7_envelopedData_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x03}; /* 1.2.840.113549.1.7.3 */
MOC_EXTERN_DATA_DEF const ubyte pkcs7_signedAndEnvelopedData_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x04}; /* 1.2.840.113549.1.7.4 */
MOC_EXTERN_DATA_DEF const ubyte pkcs7_digestedData_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x05}; /* 1.2.840.113549.1.7.5 */
MOC_EXTERN_DATA_DEF const ubyte pkcs7_encryptedData_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x07, 0x06}; /* 1.2.840.113549.1.7.6 */

MOC_EXTERN_DATA_DEF const ubyte rsaEncryptionAlgoRoot_OID[] =
    { 7, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x03};         /* 1.2.840.113549.3 */
MOC_EXTERN_DATA_DEF const ubyte rc2CBC_OID[] =
    { 8, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x03, 0x02};   /* 1.2.840.113549.3.2 */
MOC_EXTERN_DATA_DEF const ubyte rc4_OID[] =
    { 8, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x03, 0x04};   /* 1.2.840.113549.3.4 */
MOC_EXTERN_DATA_DEF const ubyte desEDE3CBC_OID[] =
    { 8, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x03, 0x07};   /* 1.2.840.113549.3.7 */

/* PKCS9 */
MOC_EXTERN_DATA_DEF const ubyte pkcs9_emailAddress_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x01}; /*1 2 840 113549 1 9 1*/
MOC_EXTERN_DATA_DEF const ubyte pkcs9_unstructuredName_OID[] =
	{ 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x02}; /*1 2 840 113549 1 9 2*/
MOC_EXTERN_DATA_DEF const ubyte pkcs9_contentType_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x03}; /*1 2 840 113549 1 9 3*/
MOC_EXTERN_DATA_DEF const ubyte pkcs9_messageDigest_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x04}; /*1 2 840 113549 1 9 4*/
MOC_EXTERN_DATA_DEF const ubyte pkcs9_signingTime_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x05}; /*1 2 840 113549 1 9 5*/
MOC_EXTERN_DATA_DEF const ubyte pkcs9_challengePassword_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x07}; /*1 2 840 113549 1 9 7*/
MOC_EXTERN_DATA_DEF const ubyte pkcs9_extensionRequest_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x0E}; /*1 2 840 113549 1 9 14*/
MOC_EXTERN_DATA_DEF const ubyte pkcs9_friendlyName_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x14}; /*1 2 840 113549 1 9 20*/
MOC_EXTERN_DATA_DEF const ubyte pkcs9_localKeyId_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x15}; /*1 2 840 113549 1 9 21*/

/* Other X509 Objects */
MOC_EXTERN_DATA_DEF const ubyte x509_description_OID[] =
    { 3, 0x55, 0x04, 0x0D };   /* 2.5.4.13 */


#if (defined(__ENABLE_MOCANA_CMS__))

MOC_EXTERN_DATA_DEF const ubyte smime_capabilities_OID[] =
    { 9, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x0F }; /* 1.2.840.113549.1.9.15 */
/* S/MIME content types */
MOC_EXTERN_DATA_DEF const ubyte smime_receipt_OID[] =
    { 11, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x10, 0x01, 0x01}; /* 1.2.840.113549.1.9.16.1.1 */
/* S/MIME Authenticated Attributes */
MOC_EXTERN_DATA_DEF const ubyte smime_receiptRequest_OID[] =
    { 11, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x10, 0x02, 0x01}; /* 1.2.840.113549.1.9.16.2.1 */
MOC_EXTERN_DATA_DEF const ubyte smime_msgSigDigest_OID[] =
    { 11, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09, 0x10, 0x02, 0x05}; /* 1.2.840.113549.1.9.16.2.5 */

#endif

/* OCSP */
MOC_EXTERN_DATA_DEF const ubyte id_pkix_ocsp_OID[]       = {8, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01};       /* 1 3 6 1 5 5 7 48 1 */
MOC_EXTERN_DATA_DEF const ubyte id_pkix_ocsp_basic_OID[] = {9, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01}; /* 1 3 6 1 5 5 7 48 1  1*/
MOC_EXTERN_DATA_DEF const ubyte id_pkix_ocsp_nonce_OID[] = {9, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x02}; /* 1 3 6 1 5 5 7 48 1  2*/
MOC_EXTERN_DATA_DEF const ubyte id_pkix_ocsp_crl_OID[]   = {9, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x03}; /* 1 3 6 1 5 5 7 48 1  3*/
MOC_EXTERN_DATA_DEF const ubyte id_pkix_ocsp_service_locator[]
                                     = {9, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x07}; /* 1 3 6 1 5 5 7 48 1  7*/
MOC_EXTERN_DATA_DEF const ubyte id_ad_ocsp[]             = {8, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01};       /* 1.3.6.1.5.5.7.48.1 */
MOC_EXTERN_DATA_DEF const ubyte id_pe_authorityInfoAcess_OID[] = {8, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01}; /* 1.3.6.1.5.5.7.1.1  */
MOC_EXTERN_DATA_DEF const ubyte id_kp_OCSPSigning_OID[]  = {8, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x09}; /* 1.3.6.1.5.5.7.3.9 */

MOC_EXTERN_DATA_DEF const ubyte id_ce_extKeyUsage_OID[]  = {3, 0x55, 0x1D, 0x25};

/* CMPv2 */

MOC_EXTERN_DATA_DEF const ubyte password_based_mac_OID[]   = {9, 0x2A, 0x86, 0x48, 0x86, 0xF6, 0x7D, 0x07, 0x42, 0x0D};
MOC_EXTERN_DATA_DEF const ubyte id_regCtrl_oldCertID_OID[] = {9, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x05, 0x01, 0x05};
MOC_EXTERN_DATA_DEF const ubyte hmac_sha1_OID[]            = {8, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x08, 0x01, 0x02};
MOC_EXTERN_DATA_DEF const ubyte id_aa_signingCertificate[] = {0x0b, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x10, 0x02, 0x0c};

/* RFC 6187 support for SSH */
MOC_EXTERN_DATA_DEF const ubyte id_kp_secureShellServer[] = {8, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x16};

/* Mocana Proprietary Certificate Extensions */
MOC_EXTERN_DATA_DEF const ubyte mocana_cert_extension_OID[]   =   /* 1.3.6.1.4.1.14421.1 */
{8, 0x2B, 0x06, 0x01, 0x04, 0x01, 0xF0, 0x55, 0x01};

MOC_EXTERN_DATA_DEF const ubyte mocana_voip_OID[]   =   /* 1.3.6.1.4.1.14421.2 */
{8, 0x2B, 0x06, 0x01, 0x04, 0x01, 0xF0, 0x55, 0x02};

MOC_EXTERN_DATA_DEF const ubyte mocana_networkLinker_OID[] =  /* 1.3.6.1.4.1.14421.3 */
{8, 0x2B, 0x06, 0x01, 0x04, 0x01, 0xF0, 0x55, 0x03};



#if (defined(__ENABLE_MOCANA_ECC__))

MOC_EXTERN_DATA_DEF const ubyte ecdsaWithSHA1_OID[] =               /* 1 2 840 10045 4 1 */
{7, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x01};
MOC_EXTERN_DATA_DEF const ubyte ecPublicKey_OID[] =
{7, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01};  /* 1 2 840 10045 2 1*/

MOC_EXTERN_DATA_DEF const ubyte ecdsaWithSHA2_OID[] =               /* 1 2 840 10045 4 3 */
{7, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03};

MOC_EXTERN_DATA_DEF const ubyte ansiX962CurvesPrime_OID[] =
{ 7, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01 };     /* 1.2.840.10045.3.1 */

MOC_EXTERN_DATA_DEF const ubyte secp192r1_OID[] =
{8, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x01}; /*  1 2 840 10045 3 1 1 */
MOC_EXTERN_DATA_DEF const ubyte secp256r1_OID[] =
{8, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07}; /*  1 2 840 10045 3 1 7 */

MOC_EXTERN_DATA_DEF const ubyte certicomCurve_OID[] =
{ 4, 0x2B, 0x81, 0x04, 0x00 };              /* 1 3 132 0 */

MOC_EXTERN_DATA_DEF const ubyte secp224r1_OID[] =
{5, 0x2B, 0x81, 0x04, 0x00, 0x21 };         /* 1 3 132 0 33 */
MOC_EXTERN_DATA_DEF const ubyte secp384r1_OID[] =
{5, 0x2B, 0x81, 0x04, 0x00, 0x22 };         /* 1 3 132 0 34 */
MOC_EXTERN_DATA_DEF const ubyte secp521r1_OID[] =
{5, 0x2B, 0x81, 0x04, 0x00, 0x23 };         /* 1 3 132 0 35 */

#ifdef __ENABLE_MOCANA_PKCS7__

MOC_EXTERN_DATA_DEF const ubyte dhSinglePassStdDHSha1KDF_OID[] = { 9, 0x2b, 0x81, 0x05, 0x10, 0x86, 0x48, 0x3f, 0x00, 0x02 }; /* 1 3 133 16 840 63 0 2  */
MOC_EXTERN_DATA_DEF const ubyte dhSinglePassStdDHSha256KDF_OID[] = { 6, 0x2b, 0x81, 0x04, 0x01, 0x0b, 0x01 }; /* 1.3.132.1.11.1 */
MOC_EXTERN_DATA_DEF const ubyte dhSinglePassStdDHSha384KDF_OID[] = { 6, 0x2b, 0x81, 0x04, 0x01, 0x0b, 0x02 }; /* 1.3.132.1.11.2 */
MOC_EXTERN_DATA_DEF const ubyte dhSinglePassStdDHSha224KDF_OID[] = { 6, 0x2b, 0x81, 0x04, 0x01, 0x0b, 0x00 }; /* 1.3.132.1.11.0 */
MOC_EXTERN_DATA_DEF const ubyte dhSinglePassStdDHSha512KDF_OID[] = { 6, 0x2b, 0x81, 0x04, 0x01, 0x0b, 0x03 }; /* 1.3.132.1.11.3 */

MOC_EXTERN_DATA_DEF const ubyte aes128Wrap_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x05};   /* 2.16.840.1.101.3.4.1.5 */
MOC_EXTERN_DATA_DEF const ubyte aes192Wrap_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x19};   /* 2.16.840.1.101.3.4.1.25 */
MOC_EXTERN_DATA_DEF const ubyte aes256Wrap_OID[] = {9, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x01, 0x2d};   /* 2.16.840.1.101.3.4.1.45 */

#endif


#endif

/*----------------------------------------------------------------------*/
extern intBoolean
EqualOID( const ubyte* oid1, const ubyte* oid2)
{
    sbyte4 i;

    if ( 0 == oid1 || 0 == oid2) return 0;

    if ( *oid1 != *oid2) return 0;

    for ( i = 1; i <= *oid1; ++i)
    {
        if ( oid1[i] != oid2[i]) return 0;
    }
    return 1;
}
