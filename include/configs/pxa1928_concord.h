/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_PXA1928_H
#define __CONFIG_PXA1928_H

#define CONFIG_ARM64
#define CONFIG_REMAKE_ELF
/*
 * Version number information
 */
#define CONFIG_IDENT_STRING	"\nMarvell-PXA1928 Platform"

/*
 * High Level Configuration Options
 */
#define CONFIG_PXA1928		1
#define CONFIG_SYS_TEXT_BASE            0x9000000

#define CONFIG_POWER			1
#define CONFIG_POWER_I2C		1
#define CONFIG_POWER_88PM860		1
#define CONFIG_POWER_88PM830		1
#define CONFIG_PXA1928_POWER		1

#define CONFIG_ARMV8
#define CONFIG_SYS_CACHELINE_SIZE	64
#define CONFIG_SYS_DCACHE_OFF

/* Generic Timer Definitions for arm timer */
#define COUNTER_FREQUENCY              (0x340000)     /* 3.25MHz */
#define SECONDARY_CPU_MAILBOX           0x01210000

#define CPU_RELEASE_ADDR		0xffffffff /* should not be touched */

#define CONFIG_CHECK_DISCRETE

#define CONFIG_SMP

#define CONFIG_SYS_SDRAM_BASE           0
#define CONFIG_SRAM_BASE                0xd1020000

#define CONFIG_SYS_INIT_SP_ADDR         (CONFIG_SRAM_BASE + 0x1000)
#define CONFIG_SYS_RELOC_END            0x09700000

#define CONFIG_NR_DRAM_BANKS_MAX	2

#define CONFIG_TZ_HYPERVISOR
#ifdef CONFIG_TZ_HYPERVISOR
#define CONFIG_TZ_HYPERVISOR_SIZE       (0x01000000)
#else
#define CONFIG_TZ_HYPERVISOR_SIZE       0
#endif

/*
 * Commands configuration
 */
#define CONFIG_SYS_NO_FLASH		/* Declare no flash (NOR/SPI) */
#include <config_cmd_default.h>
#define CONFIG_CMD_GPIO
#define CONFIG_CMD_MFP
#define CONFIG_CMD_MIPS
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MMC
#define CONFIG_FS_EXT4
#define CONFIG_CMD_FS_GENERIC
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS

#define CONFIG_MRVL_USB_PHY 1
#define CONFIG_MRVL_USB_PHY_28LP 1
#define CONFIG_CI_UDC
#define CONFIG_USB_ETHER
#define CONFIG_USB_ETH_CDC
#define CONFIG_CMD_FASTBOOT
#define CONFIG_CMD_BOOTI

#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_PXA
#define CONFIG_USB_GADGET
#define CONFIG_USB_GADGET_VBUS_DRAW     0
#define CONFIG_USBDOWNLOAD_GADGET
#define CONFIG_G_DNL_VENDOR_NUM         0x0451
#define CONFIG_G_DNL_PRODUCT_NUM        0xd022
#define CONFIG_G_DNL_MANUFACTURER       "Marvell"
#define CONFIG_ANDROID_BOOT_IMAGE
#define CONFIG_USB_FASTBOOT_BUF_ADDR    0x10000000
#define CONFIG_USB_FASTBOOT_BUF_SIZE    0x20000000
#define CONFIG_FASTBOOT_FLASH
#define CONFIG_FASTBOOT_FLASH_MMC_DEV	0
/*
 * mv-common.h should be defined after CMD configs since it used them
 * to enable certain macros
 */
#include "mv-common.h"

#undef CONFIG_ARCH_MISC_INIT

/*
 * Boot setting
 */
#define CONFIG_ZERO_BOOTDELAY_CHECK
#define CONFIG_SHOW_BOOT_PROGRESS
#define CONFIG_MRVL_BOOT    1
#define CONFIG_MISC_INIT_R	1

#define CONFIG_BOOTARGS                 \
		"initrd=0x03000000,10m rw androidboot.console=ttyS0" \
		" console=ttyS0,115200 panic_debug uart_dma" \
		" crashkernel=4k@0x8140000 androidboot.lcd=1080_50 user_debug=31" \
		" earlyprintk=uart8250-32bit,0xd4018000"

#define CONFIG_BOOTCOMMAND	"mrvlboot"
#define CONFIG_MMC_BOOT_DEV     "mmc dev 0 0"
#define RAMDISK_LOADADDR        (CONFIG_TZ_HYPERVISOR_SIZE + 0x02000000)
#define BOOTIMG_EMMC_ADDR       0x01000000
#define RECOVERYIMG_EMMC_ADDR   0x00500000
/* Kernel size is set to 4MB for legacy non-boot.img format */
#define KERNEL_SIZE             0x00400000
#define RAMDISK_SIZE            0x00400000
#define RECOVERY_KERNEL_LOADADDR        (CONFIG_TZ_HYPERVISOR_SIZE + 0x01080000)
#define RECOVERY_RAMDISK_LOADADDR       (CONFIG_TZ_HYPERVISOR_SIZE + 0x02A00000)
#define MRVL_BOOT               1
#define CONFIG_OF_LIBFDT        1
#ifdef CONFIG_OF_LIBFDT
#define DTB_LOADADDR            (CONFIG_TZ_HYPERVISOR_SIZE + 0x000e0000)
#define DTB_EMMC_ADDR           (BOOTIMG_EMMC_ADDR + 0xF00000)
#define RECOVERY_DTB_EMMC_ADDR  (RECOVERYIMG_EMMC_ADDR + 0x900000)
#define DTB_SIZE                0x00040000
#endif
/*
 * Environment variables configurations
 */
#define CONFIG_ENV_IS_IN_MMC   1       /* save env in MMV */
#define CONFIG_SYS_MMC_ENV_DEV 0       /* save env in eMMC */
#define CONFIG_CMD_SAVEENV
/* The begin addr where to save u-boot.bin in the eMMC/Nand */
#define CONFIG_UBOOT_PA_START   0xF00000
/* The total size can be used for uboot and env in the eMMC/Nand */
#define CONFIG_UBOOT_PA_SIZE    0x100000
#define CONFIG_UBOOT_PA_END             (CONFIG_UBOOT_PA_START + CONFIG_UBOOT_PA_SIZE)
#define CONFIG_ENV_SIZE		0x8000		/* env size set to 32KB */
#define CONFIG_ENV_OFFSET               (CONFIG_UBOOT_PA_END - CONFIG_ENV_SIZE)
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2      "> "

#define CONFIG_REVISION_TAG	1
/* Marvell PXAV3 MMC Controller Configuration */
#define CONFIG_SDHCI_PXAV3	1
#define CONFIG_EEPROM_34XX02    1

#define CONFIG_DDR_HW_DFC

#define CONFIG_SUPPORT_RAW_INITRD
#define CONFIG_PXA1928_DFC
#define CONFIG_PXA1928_LPM
#define CONFIG_PXA1928_COMM_D2
#endif	/* __CONFIG_PXA1928_H */
