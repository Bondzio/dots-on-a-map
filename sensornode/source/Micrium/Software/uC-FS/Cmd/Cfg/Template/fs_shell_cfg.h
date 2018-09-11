/*
*********************************************************************************************************
*                                             uC/FS V4
*                                     The Embedded File System
*
*                         (c) Copyright 2008-2014; Micrium, Inc.; Weston, FL
*
*                  All rights reserved.  Protected by international copyright laws.
*
*                  uC/FS is provided in source form to registered licensees ONLY.  It is
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
*                                FS SHELL COMMANDS CONFIGURATION FILE
*
*                                              TEMPLATE
*
* Filename      : fs_shell_cfg.h
* Version       : v4.07.00
* Programmer(s) : BAN
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          INCLUDE GUARD
*********************************************************************************************************
*/

#ifndef  FS_SHELL_CFG_H
#define  FS_SHELL_CFG_H


/*
*********************************************************************************************************
*                                              FS SHELL
*
* Note(s) : (1) Defines the length of the buffer used to read/write from files during file access
*               operations.
*
*           (2) Enable/disable the FS shell command.
*********************************************************************************************************
*/

#define  FS_SHELL_CFG_BUF_LEN                            512u   /* Cfg buf len          (see Note #1).                  */

#define  FS_SHELL_CFG_CMD_CAT_EN                 DEF_ENABLED    /* En/dis fs_cat.       (see Note #2).                  */
#define  FS_SHELL_CFG_CMD_CD_EN                  DEF_ENABLED    /* En/dis fs_cd.        ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_CP_EN                  DEF_ENABLED    /* En/dis fs_cp.        ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_DATE_EN                DEF_ENABLED    /* En/dis fs_date.      ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_DF_EN                  DEF_ENABLED    /* En/dis fs_df.        ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_LS_EN                  DEF_ENABLED    /* En/dis fs_ls.        ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_MKDIR_EN               DEF_ENABLED    /* En/dis fs_mkdir.     ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_MKFS_EN                DEF_ENABLED    /* En/dis fs_mkfs.      ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_MOUNT_EN               DEF_ENABLED    /* En/dis fs_mount.     ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_MV_EN                  DEF_ENABLED    /* En/dis fs_mv.        ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_OD_EN                  DEF_ENABLED    /* En/dis fs_od.        ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_PWD_EN                 DEF_ENABLED    /* En/dis fs_pwd.       ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_RM_EN                  DEF_ENABLED    /* En/dis fs_rm.        ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_RMDIR_EN               DEF_ENABLED    /* En/dis fs_rmdir.     ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_TOUCH_EN               DEF_ENABLED    /* En/dis fs_touch.     ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_UMOUNT_EN              DEF_ENABLED    /* En/dis fs_umount.    ( "   "   " ).                  */
#define  FS_SHELL_CFG_CMD_WC_EN                  DEF_ENABLED    /* En/dis fs_wc.        ( "   "   " ).                  */


/*
*********************************************************************************************************
*                                        INCLUDE GUARD END
*********************************************************************************************************
*/

#endif   
