/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2016 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : r_flash_rx65n.h
* Description  : This is a private header file used internally by the FLASH API module. It should not be modified or
*                included by the user in their application.
***********************************************************************************************************************/
/***********************************************************************************************************************
* History : DD.MM.YYYY Version Description
*         : 03.02.2016 1.00    First Release
*         : 14.07.2016 2.00    Modified for BSPless flash.
***********************************************************************************************************************/

#ifndef RX65N_FLASH_PRIVATE_HEADER_FILE
#define RX65N_FLASH_PRIVATE_HEADER_FILE

#include "r_flash_rx_config.h"
#include "r_mcu_config.h"

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/
/* Tests to make sure the user configured the code correctly. */

#if ((FLASH_CFG_CODE_FLASH_BGO == 1) && (MCU_CFG_PART_MEMORY_SIZE == 0xF))
    #error "Code Flash BGO is only supported for MCUs with over 2 MB of Code Flash Memory"
#endif


#if (MCU_CFG_PART_MEMORY_SIZE == 0x4)       // 512 Kb
     #define FLASH_NUM_BLOCKS_CF (16+6)
#elif (MCU_CFG_PART_MEMORY_SIZE == 0x7 )    // 768 Kb
     #define FLASH_NUM_BLOCKS_CF (24+6)
#elif (MCU_CFG_PART_MEMORY_SIZE == 0x9 )    // 1 Mb
     #define FLASH_NUM_BLOCKS_CF (32+6)
#endif

/* no data flash */

#define FLASH_CF_MIN_PGM_SIZE       (128)
#define FLASH_CF_SMALL_BLOCK_SIZE   (8192)
#define FLASH_CF_MEDIUM_BLOCK_SIZE  (32768)
//#define FLASH_CF_LOWEST_VALID_BLOCK (FLASH_CF_BLOCK_INVALID + 1)

#define FLASH_RAM_END_ADDRESS       (0x0003FFFF)

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/
typedef enum _flash_block_address
{
    FLASH_CF_BLOCK_END     = 0xFFFFFFFF,    /*   End of Code Flash Area       */
    FLASH_CF_BLOCK_0       = 0xFFFFE000,    /*   8KB: 0xFFFFE000 - 0xFFFFFFFF */
    FLASH_CF_BLOCK_1       = 0xFFFFC000,    /*   8KB: 0xFFFFC000 - 0xFFFFDFFF */
    FLASH_CF_BLOCK_2       = 0xFFFFA000,    /*   8KB: 0xFFFFA000 - 0xFFFFBFFF */
    FLASH_CF_BLOCK_3       = 0xFFFF8000,    /*   8KB: 0xFFFF8000 - 0xFFFF9FFF */
    FLASH_CF_BLOCK_4       = 0xFFFF6000,    /*   8KB: 0xFFFF6000 - 0xFFFF7FFF */
    FLASH_CF_BLOCK_5       = 0xFFFF4000,    /*   8KB: 0xFFFF4000 - 0xFFFF5FFF */
    FLASH_CF_BLOCK_6       = 0xFFFF2000,    /*   8KB: 0xFFFF2000 - 0xFFFF3FFF */
    FLASH_CF_BLOCK_7       = 0xFFFF0000,    /*   8KB: 0xFFFF0000 - 0xFFFF1FFF */
    FLASH_CF_BLOCK_8       = 0xFFFE8000,    /*  32KB: 0xFFFE8000 - 0xFFFEFFFF */
    FLASH_CF_BLOCK_9       = 0xFFFE0000,    /*  32KB: 0xFFFE0000 - 0xFFFE7FFF */
    FLASH_CF_BLOCK_10      = 0xFFFD8000,    /*  32KB: 0xFFFD8000 - 0xFFFDFFFF */
    FLASH_CF_BLOCK_11      = 0xFFFD0000,    /*  32KB: 0xFFFD0000 - 0xFFFD7FFF */
    FLASH_CF_BLOCK_12      = 0xFFFC8000,    /*  32KB: 0xFFFC8000 - 0xFFFCFFFF */
    FLASH_CF_BLOCK_13      = 0xFFFC0000,    /*  32KB: 0xFFFC0000 - 0xFFFC7FFF */
    FLASH_CF_BLOCK_14      = 0xFFFB8000,    /*  32KB: 0xFFFB8000 - 0xFFFBFFFF */
    FLASH_CF_BLOCK_15      = 0xFFFB0000,    /*  32KB: 0xFFFB0000 - 0xFFFB7FFF */
    FLASH_CF_BLOCK_16      = 0xFFFA8000,    /*  32KB: 0xFFFA8000 - 0xFFFAFFFF */
    FLASH_CF_BLOCK_17      = 0xFFFA0000,    /*  32KB: 0xFFFA0000 - 0xFFFA7FFF */
    FLASH_CF_BLOCK_18      = 0xFFF98000,    /*  32KB: 0xFFF98000 - 0xFFF9FFFF */
    FLASH_CF_BLOCK_19      = 0xFFF90000,    /*  32KB: 0xFFF90000 - 0xFFF97FFF */
    FLASH_CF_BLOCK_20      = 0xFFF88000,    /*  32KB: 0xFFF88000 - 0xFFF8FFFF */
    FLASH_CF_BLOCK_21      = 0xFFF80000,    /*  32KB: 0xFFF80000 - 0xFFF87FFF */
#if   MCU_CFG_PART_MEMORY_SIZE == 0x04  /*   '4' parts 512 Kb ROM */
    FLASH_CF_BLOCK_INVALID = (FLASH_CF_BLOCK_21 - 1),
#else
    FLASH_CF_BLOCK_22      = 0xFFF78000,    /*  32KB: 0xFFF78000 - 0xFFF7FFFF */
    FLASH_CF_BLOCK_23      = 0xFFF70000,    /*  32KB: 0xFFF70000 - 0xFFF77FFF */
    FLASH_CF_BLOCK_24      = 0xFFF68000,    /*  32KB: 0xFFF68000 - 0xFFF6FFFF */
    FLASH_CF_BLOCK_25      = 0xFFF60000,    /*  32KB: 0xFFF60000 - 0xFFF67FFF */
    FLASH_CF_BLOCK_26      = 0xFFF58000,    /*  32KB: 0xFFF58000 - 0xFFF5FFFF */
    FLASH_CF_BLOCK_27      = 0xFFF50000,    /*  32KB: 0xFFF50000 - 0xFFF57FFF */
    FLASH_CF_BLOCK_28      = 0xFFF48000,    /*  32KB: 0xFFF48000 - 0xFFF4FFFF */
    FLASH_CF_BLOCK_29      = 0xFFF40000,    /*  32KB: 0xFFF40000 - 0xFFF47FFF */
#if   MCU_CFG_PART_MEMORY_SIZE == 0x07  /*   '7' parts 768 Kb ROM */
    FLASH_CF_BLOCK_INVALID = (FLASH_CF_BLOCK_29 - 1),
#else
    FLASH_CF_BLOCK_30      = 0xFFF38000,    /*  32KB: 0xFFF38000 - 0xFFF3FFFF */
    FLASH_CF_BLOCK_31      = 0xFFF30000,    /*  32KB: 0xFFF30000 - 0xFFF37FFF */
    FLASH_CF_BLOCK_32      = 0xFFF28000,    /*  32KB: 0xFFF28000 - 0xFFF2FFFF */
    FLASH_CF_BLOCK_33      = 0xFFF20000,    /*  32KB: 0xFFF20000 - 0xFFF27FFF */
    FLASH_CF_BLOCK_34      = 0xFFF18000,    /*  32KB: 0xFFF18000 - 0xFFF1FFFF */
    FLASH_CF_BLOCK_35      = 0xFFF10000,    /*  32KB: 0xFFF10000 - 0xFFF17FFF */
    FLASH_CF_BLOCK_36      = 0xFFF08000,    /*  32KB: 0xFFF08000 - 0xFFF0FFFF */
    FLASH_CF_BLOCK_37      = 0xFFF00000,    /*  32KB: 0xFFF00000 - 0xFFF07FFF */
    FLASH_CF_BLOCK_INVALID = (FLASH_CF_BLOCK_37 - 1),    /* end 1 Mb parts */
#endif
#endif
    FLASH_CF_LOWEST_VALID_BLOCK = (FLASH_CF_BLOCK_INVALID + 1)
} flash_block_address_t;


typedef struct _rom_block_sizes
{
    uint32_t num_blocks;            // number of blocks at this size
    uint32_t block_size;            // Size of each block
}rom_block_sizes_t;


typedef struct _rom_block_info
{
    uint32_t start_addr;            // starting address for this block section
    uint32_t end_addr;              // ending (up to and including this) address
    uint16_t block_number;          // the rom block number for this address queried
    uint32_t thisblock_stAddr;      // the starting address for the above block #
    uint32_t block_size;            // Size of this block
} rom_block_info_t;


#define NUM_BLOCK_TABLE_ENTRIES 3
static rom_block_sizes_t g_flash_RomBlockSizes[NUM_BLOCK_TABLE_ENTRIES] =
{
    8,  8192,   // 8 blocks of 8K
    30, 32768,  // 30 blocks of 32K
    0,  0
};

#pragma bitfields=reversed
typedef union
{
    unsigned long LONG;
    struct {
        unsigned long BTFLG:1;
        unsigned long :3;
        unsigned long FAWE:12;
        unsigned long FSPR:1;
        unsigned long :3;
        unsigned long FAWS:12;
    } BIT;
} fawreg_t;
#pragma bitfields=default


/*  According to HW Manual the Max Programming Time for 128 bytes (ROM)
    is 13.2ms.  This is with a FCLK of 4 MHz. The calculation below
    calculates the number of ICLK ticks needed for the timeout delay.
    The 31.2ms number is adjusted linearly depending on the FCLK frequency.
*/
#define WAIT_MAX_ROM_WRITE \
        ((int32_t)(13200 * (60.0/(MCU_CFG_FCLK_HZ/1000000)))*(MCU_CFG_ICLK_HZ/1000000))

/*  According to HW Manual the max timeout value when using the peripheral
    clock notification command is 60us. This is with a FCLK of 50MHz. The
    calculation below calculates the number of ICLK ticks needed for the
    timeout delay. The 60us number is adjusted linearly depending on
    the FCLK frequency.
*/
// TODO: GI?
#define WAIT_MAX_NOTIFY_FCU_CLOCK \
        ((int32_t)(60 * (50.0/(MCU_CFG_FCLK_HZ/1000000)))*(MCU_CFG_ICLK_HZ/1000000))

/*  According to HW Manual the Max Erasure Time for a 32KB block is
    around 1040ms.  This is with a FCLK of 4 MHz. The calculation below
    calculates the number of ICLK ticks needed for the timeout delay.
    The 1040ms number is adjusted linearly depending on the FCLK frequency.
*/
#define WAIT_MAX_ERASE_CF_32K \
        ((int32_t)(1040000 * (60.0/(MCU_CFG_FCLK_HZ/1000000)))*(MCU_CFG_ICLK_HZ/1000000))

/*  According to HW Manual the Max Erasure Time for a 8KB block is
    around 260ms.  This is with a FCLK of 4 MHz. The calculation below
    calculates the number of ICLK ticks needed for the timeout delay.
    The 260ms number is adjusted linearly depending on the FCLK frequency.
*/
#define WAIT_MAX_ERASE_CF_8K \
        ((int32_t)(260000 * (60.0/(MCU_CFG_FCLK_HZ/1000000)))*(MCU_CFG_ICLK_HZ/1000000))


/******************************************************************************
Error checking
******************************************************************************/
// frequency range checking moved to flash_api_open()

#endif/* RX65N_FLASH_PRIVATE_HEADER_FILE */
