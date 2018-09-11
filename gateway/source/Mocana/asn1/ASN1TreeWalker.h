/* Version: mss_v6_3 */
/*
 * ASN1TreeWalker.h
 *
 * ASN1 Parse Tree Walker
 *
 * Copyright Mocana Corp 2005-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __ASN1TREEWALKER_HEADER__
#define __ASN1TREEWALKER_HEADER__


/*------------------------------------------------------------------*/

/* Walker instructions codes */

typedef enum
{
    Complete,           /* always use this to indicate end of instructions */
    GoFirstChild,       /* unused, unused */
    GoNextSibling,      /* unused, unused */
    GoParent,           /* unused, unused */
    GoToTag,            /* tag, unused */
    VerifyType,         /* type, unused */
    VerifyTag,          /* tag, unused */
    VerifyOID,          /* unused, &oid */
    VerifyInteger,      /* number, unused */
    GoChildWithTag,     /* tag, unused */
    GoChildWithOID,     /* unused, &oid */
    GoNthChild,         /* child #, unused */
    GoFirstChildBER     /* unused, unused */
} E_WalkerInstructions;

typedef struct WalkerStep
{
    E_WalkerInstructions    instruction;
    sbyte4                  extra1; /* tag or type or length or number */
    ubyte*                  extra2; /* oid (first byte is length)*/
} WalkerStep;

/* exported routines */

MOC_EXTERN MSTATUS ASN1_WalkTree( ASN1_ITEM* pStart,
                             CStream s,
                             const WalkerStep* pSteps,
                             ASN1_ITEM** pFound);


#endif  /*#ifndef __ASN1TREEWALKER_HEADER__ */
