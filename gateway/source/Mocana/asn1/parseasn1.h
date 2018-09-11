/* Version: mss_v6_3 */
#ifndef __PARSEASN1_H__
#define __PARSEASN1_H__


#ifdef __cplusplus
extern "C" {
#endif
    
/* Tag classes */

#define CLASS_MASK      0xC0    /* Bits 8 and 7 */
#define UNIVERSAL       0x00    /* 0 = Universal (defined by ITU X.680) */
#define APPLICATION     0x40    /* 1 = Application */
#define CONTEXT         0x80    /* 2 = Context-specific */
#define PRIVATE         0xC0    /* 3 = Private */

/* Encoding type */

#define FORM_MASK       0x20    /* Bit 6 */
#define PRIMITIVE       0x00    /* 0 = primitive */
#define CONSTRUCTED     0x20    /* 1 = constructed */

/* Universal tags */

#define TAG_MASK        0x1F    /* Bits 5 - 1 */
#define EOC             0x00    /*  0: End-of-contents octets */
#define BOOLEAN         0x01    /*  1: Boolean */
#define INTEGER         0x02    /*  2: Integer */
#define BITSTRING       0x03    /*  3: Bit string */
#define OCTETSTRING     0x04    /*  4: Byte string */
#define NULLTAG         0x05    /*  5: NULL */
#define OID             0x06    /*  6: Object Identifier */
#define OBJDESCRIPTOR   0x07    /*  7: Object Descriptor */
#define EXTERNAL        0x08    /*  8: External */
#define REAL            0x09    /*  9: Real */
#define ENUMERATED      0x0A    /* 10: Enumerated */
#define EMBEDDED_PDV    0x0B    /* 11: Embedded Presentation Data Value */
#define UTF8STRING      0x0C    /* 12: UTF8 string */
#define SEQUENCE        0x10    /* 16: Sequence/sequence of */
#undef  SET
#define SET             0x11    /* 17: Set/set of */
#define NUMERICSTRING   0x12    /* 18: Numeric string */
#define PRINTABLESTRING 0x13    /* 19: Printable string (ASCII subset) */
#define T61STRING       0x14    /* 20: T61/Teletex string */
#define VIDEOTEXSTRING  0x15    /* 21: Videotex string */
#define IA5STRING       0x16    /* 22: IA5/ASCII string */
#define UTCTIME         0x17    /* 23: UTC time */
#define GENERALIZEDTIME 0x18    /* 24: Generalized time */
#define GRAPHICSTRING   0x19    /* 25: Graphic string */
#define VISIBLESTRING   0x1A    /* 26: Visible string (ASCII subset) */
#define GENERALSTRING   0x1B    /* 27: General string */
#define UNIVERSALSTRING 0x1C    /* 28: Universal string */
#define BMPSTRING       0x1E    /* 30: Basic Multilingual Plane/Unicode string */

/* Length encoding */

#define LEN_XTND  0x80          /* Indefinite or long form */
#define LEN_MASK  0x7F          /* Bits 7 - 1 */

/* Structure to hold info on an ASN.1 item */

/* BIT STRING : dataOffset points to beginning of bits not unused bits
                and length points to size of bitstring (without unused bits) */
#define ASN1_HEADER_MAX_SIZE (9)

typedef struct ASN1_ITEM {
    TreeItem    treeItem;           /* Infrastructure for tree */
    ubyte4      id;                 /* Tag class + primitive/constructed */
    ubyte4      tag;                /* Tag */
    ubyte4      length;             /* Data length */
    ubyte4      headerSize;         /* Size of tag+length */
    sbyte4      dataOffset;          /* position of data in the stream */
    union
    {
        byteBoolean m_boolVal;      /* BOOLEAN */
        ubyte4      m_intVal;       /* INTEGER, ENUMERATED */
        sbyte       m_unusedBits;   /* BIT STRING */
    } data;

    byteBoolean indefinite;         /* Item has indefinite length */
    byteBoolean encapsulates;       /* encapsulates something */
} ASN1_ITEM, *ASN1_ITEMPTR;

/* useful macros */
#define ASN1_FIRST_CHILD(a)     ((ASN1_ITEMPTR) ((a)->treeItem.m_pFirstChild))
#define ASN1_NEXT_SIBLING(a)    ((ASN1_ITEMPTR) ((a)->treeItem.m_pNextSibling))
#define ASN1_PARENT(a)          ((ASN1_ITEMPTR) ((a)->treeItem.m_pParent))

#define ASN1_CONSTRUCTED(a)     (CONSTRUCTED == ((a)->id & FORM_MASK))
#define ASN1_PRIMITIVE(a)       (PRIMITIVE == ((a)->id & FORM_MASK))

/* function to follow progress of the parsing -- called every time a new ASN.1 item
is added to the tree */
typedef void (*ProgressFun)(ASN1_ITEMPTR newAddedItem, CStream s, void* arg);

/* exported routines */
MOC_EXTERN MSTATUS ASN1_GetNthChild(ASN1_ITEM* parent, ubyte4 n, ASN1_ITEM** ppChild);
MOC_EXTERN MSTATUS ASN1_GetChildWithOID(ASN1_ITEM* parent, CStream s, const ubyte* whichOID,
                               ASN1_ITEM** ppChild);
MOC_EXTERN MSTATUS ASN1_GetChildWithTag( ASN1_ITEM* parent, ubyte4 tag, ASN1_ITEM** ppChild);
MOC_EXTERN MSTATUS ASN1_GetTag( ASN1_ITEM* pItem, ubyte4 *pTag);
MOC_EXTERN MSTATUS ASN1_GoToTag(ASN1_ITEM* parent, ubyte4 tag, ASN1_ITEM** ppTag);
MOC_EXTERN MSTATUS ASN1_VerifyOID( ASN1_ITEM* pItem, CStream s, const ubyte* whichOID);
MOC_EXTERN MSTATUS ASN1_VerifyType(ASN1_ITEM* pCurrent, ubyte4 type);
MOC_EXTERN MSTATUS ASN1_VerifyTag(ASN1_ITEM* pCurrent, ubyte4 tag);
MOC_EXTERN MSTATUS ASN1_VerifyInteger(ASN1_ITEM* pCurrent, ubyte4 n);
MOC_EXTERN MSTATUS ASN1_VerifyOIDRoot( ASN1_ITEM* pItem, CStream s, const ubyte* whichOID,
                                  ubyte* subType);
MOC_EXTERN MSTATUS ASN1_VerifyOIDStart( ASN1_ITEM* pItem, CStream s, const ubyte* whichOID);
MOC_EXTERN MSTATUS ASN1_CompareItems( ASN1_ITEM* pItem1, CStream s1, ASN1_ITEM* pItem2, CStream s2);
MOC_EXTERN MSTATUS ASN1_getBitStringBit( ASN1_ITEM* pBitString, CStream s, ubyte4 bitNum, 
                                        byteBoolean* bitVal);

MOC_EXTERN MSTATUS ASN1_Parse(CStream s, ASN1_ITEM** rootItem);
MOC_EXTERN MSTATUS ASN1_ParseEx(CStream s, ASN1_ITEM** rootItem, ProgressFun progressFun,
                            void* cbArg);

/* resumable ASN.1 parsing */
/* parser state */
typedef struct ASN1_ParseState
{
    ASN1_ITEM*              rootNode;
    ASN1_ITEM*              parentNode;
    sbyte4                  stackDepth;
    sbyte4                  filePos;
} ASN1_ParseState;

MOC_EXTERN MSTATUS ASN1_InitParseState( ASN1_ParseState* pState);

MOC_EXTERN MSTATUS ASN1_ParseASN1State(CStream as, ASN1_ParseState* pState,
                    ProgressFun progressFun, void* cbArg);

/* undocumented */
MOC_EXTERN ASN1_ITEMPTR ASN1_GetNextSiblingFromPartialParse( 
                                    const ASN1_ParseState* pState, 
                                    ASN1_ITEMPTR pSibling, CStream cs);

/* undocumented */
MOC_EXTERN ASN1_ITEMPTR ASN1_GetFirstChildFromPartialParse( 
                                    const ASN1_ParseState* pState, 
                                    ASN1_ITEMPTR pParent, CStream cs);

/* undocumented */
MOC_EXTERN ubyte4 ASN1_GetData( const ASN1_ParseState* pState, CStream cs, 
                               ubyte4 streamSize, ASN1_ITEMPTR pItem, 
                               ubyte4* pOffset, const ubyte* src, ubyte* dest);


/* verify item ( constructed) is complete */
MOC_EXTERN MSTATUS ASN1_IsItemComplete( const ASN1_ParseState* pState, 
                                       const ASN1_ITEM *item, 
                                       CStream s, intBoolean* complete);

/* API to search and retrieve based on OIDs. The whichOID parameter is a string
with the format "a.b.c.d". The last number can be set to * in which case the
remaining part of the OID will not be matched. The return value is a NULL terminated
array of the ASN1_ITEMs of type OID that match the OID parameter. The array must
be FREEed when no longer needed by the caller. An array consisting of a single
NULL value is returned if no match was found */
MOC_EXTERN MSTATUS ASN1_OIDSearch( ASN1_ITEMPTR pItem, CStream s, const sbyte* whichOID,
                                ASN1_ITEMPTR **ppResults);

    
#ifdef __cplusplus
}
#endif
        


#endif

