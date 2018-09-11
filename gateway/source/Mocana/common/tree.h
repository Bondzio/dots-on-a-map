/* Version: mss_v6_3 */
/*
 * tree.h
 *
 * Mocana ASN1 Tree Abstraction Layer
 *
 * Copyright Mocana Corp 2004-2007. All Rights Reserved.
 * Proprietary and Confidential Material.
 *
 */

#ifndef __TREE_H__
#define __TREE_H__

#ifdef __cplusplus
extern "C" {
#endif
    
struct TreeItem;
/* this is a destructor function that will be called
when the tree item is deleted */
typedef void (*TreeItemDtor)(struct TreeItem*);

typedef struct TreeItem
{
    struct TreeItem* m_pParent;
    struct TreeItem* m_pFirstChild;
    struct TreeItem* m_pNextSibling;
    TreeItemDtor            m_dtorFun;
} TreeItem;


/*------------------------------------------------------------------*/

MOC_EXTERN TreeItem*    TREE_MakeNewTreeItem(sbyte4 size);
MOC_EXTERN MSTATUS      TREE_AppendChild( TreeItem* pParent, TreeItem* pChild);
MOC_EXTERN MSTATUS      TREE_DeleteTreeItem( TreeItem* pTreeItem);
MOC_EXTERN MSTATUS      TREE_DeleteChildren( TreeItem* pTreeItem);
MOC_EXTERN TreeItem*    TREE_GetParent( TreeItem* pTreeItem);
MOC_EXTERN TreeItem*    TREE_GetFirstChild( TreeItem* pTreeItem);
MOC_EXTERN TreeItem*    TREE_GetNextSibling(TreeItem* pTreeItem);
MOC_EXTERN sbyte4       TREE_GetTreeItemLevel(TreeItem* pTreeItem);

/* the VisitTreeFunc should return FALSE to stop the visit */
typedef sbyte4 (*VisitTreeFunc)(TreeItem* treeItem, void* arg);

/* VisitTree returns the tree item on which the visit was stopped if the visit was interrupted by the VisitTreeFunc */
MOC_EXTERN TreeItem*    TREE_VisitTree(TreeItem* start, VisitTreeFunc visitTreeFunc, void* arg);

#ifdef __cplusplus
}
#endif    
    
#endif
