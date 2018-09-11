/* Version: mss_v6_3 */
/*
 * oiddefs.h
 *
 * OID Definitions
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __OIDDEFS_HEADER__
#define __OIDDEFS_HEADER__

/* all OIDS are length prefixed, i.e. the first byte is the length of the rest */

/* OID */
MOC_EXTERN const ubyte pkcs1_OID[]; /* 1.2.840.113549.1.1 */
#define PKCS1_OID_LEN   (8)
#define MAX_SIG_OID_LEN (1 + PKCS1_OID_LEN)

#if 0
enum {
    rsaEncryption = 1,
    md2withRSAEncryption = 2,
    md4withRSAEncryption = 3,
    md5withRSAEncryption = 4,
    sha1withRSAEncryption = 5,
    sha256withRSAEncryption = 11,
    sha384withRSAEncryption = 12,
    sha512withRSAEncryption = 13,
    sha224withRSAEncryption = 14
};
#endif

MOC_EXTERN const ubyte rsaEncryption_OID[];             /* 1.2.840.113549.1.1.1  (repeat for convenience)*/
MOC_EXTERN const ubyte md5withRSAEncryption_OID[];      /* 1.2.840.113549.1.1.4  (repeat for convenience)*/
MOC_EXTERN const ubyte sha1withRSAEncryption_OID[];      /* 1.2.840.113549.1.1.5  (repeat for convenience)*/
MOC_EXTERN const ubyte sha256withRSAEncryption_OID[];      /* 1.2.840.113549.1.1.11  (repeat for convenience)*/
MOC_EXTERN const ubyte sha384withRSAEncryption_OID[];      /* 1.2.840.113549.1.1.12 (repeat for convenience)*/
MOC_EXTERN const ubyte sha512withRSAEncryption_OID[];      /* 1.2.840.113549.1.1.13 (repeat for convenience)*/
MOC_EXTERN const ubyte sha224withRSAEncryption_OID[];      /* 1.2.840.113549.1.1.14 (repeat for convenience)*/

MOC_EXTERN const ubyte rsaDSI_OID[];            /* 1 2 840 113549 2 */
/* suffixes of rsaDSI_OID */
enum {
    md2Digest = 2,
    md4Digest = 4,
    md5Digest = 5,
    hmacSHA1Digest = 7
};

MOC_EXTERN const ubyte md5_OID[];  /*1 2 840 113549 2 5 (repeat for convenience) */

MOC_EXTERN const ubyte commonName_OID[];				/* 2.5.4.3 */
MOC_EXTERN const ubyte serialNumber_OID[];				/* 2.5.4.5 */
MOC_EXTERN const ubyte countryName_OID[];               /* 2.5.4.6 */
MOC_EXTERN const ubyte localityName_OID[];              /* 2 5 4 7 */
MOC_EXTERN const ubyte stateOrProvinceName_OID[];       /* 2 5 4 8 */
MOC_EXTERN const ubyte organizationName_OID[];          /* 2 5 4 10 */
MOC_EXTERN const ubyte organizationalUnitName_OID[];    /* 2 5 4 11 */

MOC_EXTERN const ubyte domainComponent_OID[]; /* 0 9 2342 19200300 100 1 25 from rfc2247 */

MOC_EXTERN const ubyte userID_OID[]; /* 0.9.2342.19200300.100.1.1 from RFC 4519, 1274 */

MOC_EXTERN const ubyte subjectKeyIdentifier_OID[]; /* 2.5.29.14 */
MOC_EXTERN const ubyte keyUsage_OID[];          /* 2.5.29.15 */
MOC_EXTERN const ubyte subjectAltName_OID[]; /* 2.5.29.17 */
MOC_EXTERN const ubyte issuerAltName_OID[]; /* 2.5.29.18 */
MOC_EXTERN const ubyte basicConstraints_OID[];  /* 2.5.29.19 */
MOC_EXTERN const ubyte crlNumber_OID[];         /* 2.5.29.20 */
MOC_EXTERN const ubyte crlReason_OID[];         /* 2.5.29.21 */
MOC_EXTERN const ubyte invalidityDate_OID[];    /* 2.5.29.24 */
MOC_EXTERN const ubyte nameConstraints_OID[];   /* 2.5.29.30 */
MOC_EXTERN const ubyte crl_OID[];               /* 2.5.29.31 cRLDistributionPoints */
MOC_EXTERN const ubyte certificatePolicies_OID[]; /* 2.5.29.32 */
MOC_EXTERN const ubyte authorityKeyIdentifier_OID[]; /* 2.5.29.35 */
MOC_EXTERN const ubyte extendedKeyUsage_OID[]; /* 2.5.29.37 */
MOC_EXTERN const ubyte userPrincipalName_OID[]; /* 1.3.6.1.4.1.311.20.2.3 */

MOC_EXTERN const ubyte sha1_OID[];                  /* 1 3 14 3 2 26 */
MOC_EXTERN const ubyte sha1withRsaSignature_OID[];  /* 1 3 14 3 2 29 */

MOC_EXTERN const ubyte sha2_OID[];                  /* 2 16 840 1 101 3 4 2 */
/* suffixes of sha2_OID */
enum {
    sha256Digest = 1,
    sha384Digest = 2,
    sha512Digest = 3,
    sha224Digest = 4
};
/* repeat for convenience */
MOC_EXTERN const ubyte sha224_OID[];                  /* 2 16 840 1 101 3 4 2 4 */
MOC_EXTERN const ubyte sha256_OID[];                  /* 2 16 840 1 101 3 4 2 1 */
MOC_EXTERN const ubyte sha384_OID[];                  /* 2 16 840 1 101 3 4 2 2 */
MOC_EXTERN const ubyte sha512_OID[];                  /* 2 16 840 1 101 3 4 2 3 */

MOC_EXTERN const ubyte desCBC_OID[];        /* 1 3 14 3 2 7 */

MOC_EXTERN const ubyte aes128CBC_OID[];     /* 2.16.840.1.101.3.4.1.2 */
MOC_EXTERN const ubyte aes192CBC_OID[];     /* 2.16.840.1.101.3.4.1.22 */
MOC_EXTERN const ubyte aes256CBC_OID[];     /* 2.16.840.1.101.3.4.1.42 */

MOC_EXTERN const ubyte dsa_OID[];           /* 1 2 840 10040 4 1*/
MOC_EXTERN const ubyte dsaWithSHA1_OID[];   /* 1 2 840 10040 4 3*/

MOC_EXTERN const ubyte dsaWithSHA2_OID[];   /* 2 16 840 1 101 3 4 3 */

MOC_EXTERN const ubyte rsaEncryptionAlgoRoot_OID[]; /* 1.2.840.113549.3 */
/* subtypes */
MOC_EXTERN const ubyte rc2CBC_OID[];        /* 1.2.840.113549.3.2 */
MOC_EXTERN const ubyte rc4_OID[];           /* 1.2.840.113549.3.4 */
MOC_EXTERN const ubyte desEDE3CBC_OID[];    /* 1.2.840.113549.3.7 */

/* PKCS 7 */
MOC_EXTERN const ubyte pkcs7_root_OID[]; /* 1.2.840.113549.1.7 */
MOC_EXTERN const ubyte pkcs7_data_OID[]; /* 1.2.840.113549.1.7.1 */
MOC_EXTERN const ubyte pkcs7_signedData_OID[]; /* 1.2.840.113549.1.7.2 */
MOC_EXTERN const ubyte pkcs7_envelopedData_OID[]; /* 1.2.840.113549.1.7.3 */
MOC_EXTERN const ubyte pkcs7_signedAndEnvelopedData_OID[]; /* 1.2.840.113549.1.7.4 */
MOC_EXTERN const ubyte pkcs7_digestedData_OID[]; /* 1.2.840.113549.1.7.5 */
MOC_EXTERN const ubyte pkcs7_encryptedData_OID[]; /* 1.2.840.113549.1.7.6 */

/* PKCS 9 */
MOC_EXTERN const ubyte pkcs9_emailAddress_OID[]; /*1 2 840 113549 1 9 1*/
MOC_EXTERN const ubyte pkcs9_unstructuredName_OID[]; /*1 2 840 113549 1 9 2*/
MOC_EXTERN const ubyte pkcs9_contentType_OID[]; /*1 2 840 113549 1 9 3*/
MOC_EXTERN const ubyte pkcs9_messageDigest_OID[]; /*1 2 840 113549 1 9 4*/
MOC_EXTERN const ubyte pkcs9_signingTime_OID[]; /*1 2 840 113549 1 9 5*/
MOC_EXTERN const ubyte pkcs9_challengePassword_OID[]; /* 1.2.840.113549.1.9.7 */
MOC_EXTERN const ubyte pkcs9_extensionRequest_OID[]; /* 1.2.840.113549.1.9.14 */
MOC_EXTERN const ubyte pkcs9_friendlyName_OID[];  /* 1.2.840.113549.1.9.20 */
MOC_EXTERN const ubyte pkcs9_localKeyId_OID[];    /* 1.2.840.113549.1.9.21 */

/* Other X509 Objects */
MOC_EXTERN const ubyte x509_description_OID[];    /* 2.5.4.13 */

#if (defined(__ENABLE_MOCANA_CMS__))
/* S/MIME */
MOC_EXTERN const ubyte smime_capabilities_OID[]; /* 1.2.840.113549.1.9.15 */
/* S/MIME content types */
MOC_EXTERN const ubyte smime_receipt_OID[]; /* 1.2.840.113549.1.9.16.1.1 */
/* S/MIME Authenticated Attributes */
MOC_EXTERN const ubyte smime_receiptRequest_OID[]; /* 1.2.840.113549.1.9.16.2.1 */
MOC_EXTERN const ubyte smime_msgSigDigest_OID[];   /* 1.2.840.113549.1.9.16.2.5 */
#endif

/* ECDSA */
#if (defined(__ENABLE_MOCANA_ECC__))

MOC_EXTERN const ubyte ecPublicKey_OID[];       /* 1 2 840 10045 2 1 */
MOC_EXTERN const ubyte ecdsaWithSHA1_OID[];     /* 1 2 840 10045 4 1 */
MOC_EXTERN const ubyte ecdsaWithSHA2_OID[];     /* 1 2 840 10045 4 3 */

MOC_EXTERN const ubyte ansiX962CurvesPrime_OID[];   /* 1.2.840.10045.3.1 */

MOC_EXTERN const ubyte secp192r1_OID[];             /* 1.2.840.10045.3.1.1 */
MOC_EXTERN const ubyte secp256r1_OID[];             /* 1.2.840.10045.3.1.7 */

MOC_EXTERN const ubyte certicomCurve_OID[];    /* 1 3 132 0 */

MOC_EXTERN const ubyte secp224r1_OID[];        /* 1 3 132 0 33 */
MOC_EXTERN const ubyte secp384r1_OID[];        /* 1 3 132 0 34 */
MOC_EXTERN const ubyte secp521r1_OID[];        /* 1 3 132 0 35 */

#ifdef __ENABLE_MOCANA_PKCS7__
/* for the moment, these are only used for PKCS7/CMS */

MOC_EXTERN const ubyte dhSinglePassStdDHSha1KDF_OID[]; /* 1 3 133 16 840 63 0 2 */
MOC_EXTERN const ubyte dhSinglePassStdDHSha256KDF_OID[]; /* 1 3 132 1 11 1 */
MOC_EXTERN const ubyte dhSinglePassStdDHSha384KDF_OID[]; /* 1 3 132 1 11 2 */
MOC_EXTERN const ubyte dhSinglePassStdDHSha224KDF_OID[]; /* 1 3 132 1 11 0 */
MOC_EXTERN const ubyte dhSinglePassStdDHSha512KDF_OID[]; /* 1 3 132 1 11 3 */

MOC_EXTERN const ubyte aes128Wrap_OID[];     /* 2.16.840.1.101.3.4.1.5 */
MOC_EXTERN const ubyte aes192Wrap_OID[];     /* 2.16.840.1.101.3.4.1.25 */
MOC_EXTERN const ubyte aes256Wrap_OID[];     /* 2.16.840.1.101.3.4.1.45 */
#endif

#endif

/* OCSP */
MOC_EXTERN const ubyte id_pkix_ocsp_OID[];       /* 1 3 6 1 5 5 7 48 1 */
MOC_EXTERN const ubyte id_pkix_ocsp_basic_OID[]; /* 1 3 6 1 5 5 7 48 1  1*/
MOC_EXTERN const ubyte id_pkix_ocsp_nonce_OID[]; /* 1 3 6 1 5 5 7 48 1  2*/
MOC_EXTERN const ubyte id_pkix_ocsp_crl_OID[];   /* 1 3 6 1 5 5 7 48 1  3*/
MOC_EXTERN const ubyte id_pkix_ocsp_service_locator[];  /* 1 3 6 1 5 5 7 48 1  7*/
MOC_EXTERN const ubyte id_pe_authorityInfoAcess_OID[]; /* 1.3.6.1.5.5.7.1.1  */
MOC_EXTERN const ubyte id_ad_ocsp[]; /* 1.3.6.1.5.5.7.48.1 */
MOC_EXTERN const ubyte id_kp_OCSPSigning_OID[]; /* 1.3.6.1.5.5.7.3.9 */
MOC_EXTERN const ubyte id_ce_extKeyUsage_OID[]; /* 2.5.29.37 */

/* CMPv2 */
MOC_EXTERN const ubyte password_based_mac_OID[];
MOC_EXTERN const ubyte hmac_sha1_OID[];
MOC_EXTERN const ubyte id_aa_signingCertificate[];
MOC_EXTERN const ubyte id_regCtrl_oldCertID_OID[];

/* RFC 6187 support for SSH */
MOC_EXTERN const ubyte id_kp_secureShellServer[]; /* 1.3.6.1.5.5.7.3.22 */

/* Mocana Proprietary User Privilege Extension */
MOC_EXTERN const ubyte mocana_cert_extension_OID[]; /* 1.3.6.1.4.1.14421.1 */
MOC_EXTERN const ubyte mocana_voip_OID[];           /* 1.3.6.1.4.1.14421.2 */

/* Mocana Proprietary NetworkLinker Host/Port Extension */
MOC_EXTERN const ubyte mocana_networkLinker_OID[];  /* 1.3.6.1.4.1.14421.3 */

/* utility function */
MOC_EXTERN intBoolean EqualOID( const ubyte* oid1, const ubyte* oid2);

#endif /* __OIDDEFS_HEADER__ */
