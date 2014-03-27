/*
 * boot.c mrvl boot support
 *
 * (C) Copyright 2013
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Xiaofan Tian <tianxf@marvell.com>
 *
 * SPDX-License-Identifier:	 GPL-2.0+
 */

#include <common.h>
#include <libfdt.h>
#ifdef CONFIG_OBM_PARAM_ADDR
#include "obm2osl.h"
#endif
#include <mv_recovery.h>
#include "cmdline.h"
#include <android_image.h>

static unsigned char load_op;
struct recovery_reg_funcs *p_recovery_reg_funcs;
int recovery_key_detect_default(void)
{
	return 0;
}
int recovery_key_detect(void)
	__attribute__((weak, alias("recovery_key_detect_default")));
int recovery_default(int primary_valid, int recovery_valid,
		int magic_key, struct recovery_reg_funcs *funcs)
{
	return 0;
}
int recovery(int primary_valid, int recovery_valid,
		int magic_key, struct recovery_reg_funcs *funcs)
	__attribute__((weak, alias("recovery_default")));

#ifdef CONFIG_OF_LIBFDT
void handle_dtb_default(struct fdt_header *devtree)
{
}

void handle_dtb(struct fdt_header *devtree)
	__attribute__((weak, alias("handle_dtb_default")));

unsigned int dtb_offset_default(void)
{
	return 0;
}

unsigned int dtb_offset(void)
	__attribute__((weak, alias("dtb_offset_default")));
#endif

#define MV_KERNEL_LOADED	(1<<0)
#define MV_RAMDISK_LOADED	(1<<1)
#define MV_DTB_LOADED		(1<<2)
#define MV_DTB_FLASH		(1<<3) /* flash dtb to flash */

void image_load_notify(unsigned long load_addr)
{
	switch (load_addr) {
	case CONFIG_LOADADDR:
	case RECOVERY_KERNEL_LOADADDR:
		load_op |= MV_KERNEL_LOADED;
		break;
	case RAMDISK_LOADADDR:
	case RECOVERY_RAMDISK_LOADADDR:
		load_op |= MV_RAMDISK_LOADED;
		break;
#ifdef CONFIG_OF_LIBFDT
	case DTB_LOADADDR:
		load_op |= MV_DTB_LOADED;
		break;
#endif
	}
}

void image_flash_notify(unsigned long load_addr)
{
#ifdef CONFIG_OF_LIBFDT
	char cmd[128];
	unsigned long dtb_load_addr, dtb_emmc_addr;
	/* Load DTB */
	dtb_load_addr = load_addr;
	dtb_emmc_addr = DTB_EMMC_ADDR;
	dtb_emmc_addr += dtb_offset();
	/*load_op bit[3] flash dtb to mmc */
	if (load_op & MV_DTB_FLASH) {
		printf("write dtb to mmc 0x%lx\n", dtb_emmc_addr / 512);
		sprintf(cmd, "%s; mmc write %lx %lx %x", CONFIG_MMC_BOOT_DEV,
			dtb_load_addr, dtb_emmc_addr / 512, DTB_SIZE / 512);
		run_command(cmd, 0);
	}
#endif
}

int get_recovery_flag(void)
{
	int primary_valid = 1, recovery_valid = 1;
	int magic_key;
#ifdef CONFIG_OBM_PARAM_ADDR
	struct OBM2OSL *params = NULL;
#endif
	magic_key = recovery_key_detect();

#ifdef CONFIG_OBM_PARAM_ADDR
	params = (struct OBM2OSL *)(uintptr_t)(*(u32 *)CONFIG_OBM_PARAM_ADDR);

	if (!params || params->signature != OBM2OSL_IDENTIFIER) {
		printf("WARING: no obm parameters !!!\n");
		params = NULL;
	} else {
		magic_key |= (params->booting_mode == RECOVERY_MODE);

		primary_valid = params->primary.validation_status;
		recovery_valid = params->recovery.validation_status;
	}
#endif
	return recovery(primary_valid, recovery_valid,
			magic_key, p_recovery_reg_funcs);
}

#ifdef CONFIG_CMD_MMC
int do_mrvlboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char cmd[128];
	char cmdline[COMMAND_LINE_SIZE];
	char *bootargs;
	unsigned int kernel_offsize = 0, ramdisk_offsize = KERNEL_SIZE;
	unsigned int kernel_size = KERNEL_SIZE, ramdisk_size = RAMDISK_SIZE;
	unsigned long base_addr, kernel_load_addr, ramdisk_load_addr;
	int recovery_flag = 0;
#ifdef CONFIG_OF_LIBFDT
	unsigned long dtb_load_addr, dtb_emmc_addr;
	struct fdt_header *devtree;
#endif

	/*The default boot image header and recovery image header ddr address */
	const struct andr_img_hdr *ahdr =
		(struct andr_img_hdr *)(CONFIG_LOADADDR - 0x1000);

	recovery_flag = get_recovery_flag();
	/*
	 * OBM doesn't load kernel and ramdisk for nontrusted boot, but it
	 * still passes validation_status and loading_status as 1 to uboot
	 * since uboot is loaded and validated.
	 * Always load the kernel and ramdisk here to solve this problem,
	 * negative impact of a little boot time.
	 */
	base_addr = recovery_flag ? RECOVERYIMG_EMMC_ADDR : BOOTIMG_EMMC_ADDR;
	base_addr /= 512;

	/* copy the image header to parse */
	sprintf(cmd, "%s; mmc read %p %lx 0x8", CONFIG_MMC_BOOT_DEV,
		ahdr, base_addr);
	run_command(cmd, 0);
	if (!memcmp(ANDR_BOOT_MAGIC, ahdr->magic, ANDR_BOOT_MAGIC_SIZE)) {
		printf("This is Android %s image\n",
		       recovery_flag ? "recovery" : "boot");
		kernel_offsize = ahdr->page_size;
		kernel_size = ALIGN(ahdr->kernel_size, ahdr->page_size);
		ramdisk_offsize = kernel_offsize + kernel_size;
		ramdisk_size = ALIGN(ahdr->ramdisk_size, ahdr->page_size);
	}
	/* calculate mmc block number */
	kernel_offsize /= 512;
	ramdisk_offsize /= 512;
	kernel_size /= 512;
	ramdisk_size /= 512;

	/* Load kernel */
	kernel_load_addr =
		recovery_flag ? RECOVERY_KERNEL_LOADADDR : CONFIG_LOADADDR;
	if ((load_op & MV_KERNEL_LOADED) == 0) {
		printf("Load kernel to 0x%lx\n", kernel_load_addr);
		sprintf(cmd, "mmc read %lx %lx %x", kernel_load_addr,
			base_addr + kernel_offsize, kernel_size);
		run_command(cmd, 0);
	}

	/* Load ramdisk */
	ramdisk_load_addr =
		recovery_flag ? RECOVERY_RAMDISK_LOADADDR : RAMDISK_LOADADDR;
	if ((load_op & MV_RAMDISK_LOADED) == 0) {
		printf("Load ramdisk to 0x%lx\n", ramdisk_load_addr);
		sprintf(cmd, "mmc read %lx %lx %x", ramdisk_load_addr,
			base_addr + ramdisk_offsize, ramdisk_size);
		run_command(cmd, 0);
	}

	/* Modify ramdisk command line */
	bootargs = getenv("bootargs");
	strncpy(cmdline, bootargs, COMMAND_LINE_SIZE);
	remove_cmdline_param(cmdline, "initrd=");
	sprintf(cmdline + strlen(cmdline),  " initrd=0x%lx,10m rw",
		ramdisk_load_addr);
	if (recovery_flag)
		sprintf(cmdline + strlen(cmdline), " recovery=1");
	setenv("bootargs", cmdline);

#ifdef CONFIG_OF_LIBFDT
	/* Load DTB */
	dtb_load_addr = DTB_LOADADDR;
	dtb_emmc_addr = recovery_flag ? RECOVERY_DTB_EMMC_ADDR : DTB_EMMC_ADDR;
	dtb_emmc_addr += dtb_offset();
	if ((load_op & MV_DTB_LOADED) == 0) {
		printf("Load dtb to 0x%lx\n", dtb_load_addr);
		sprintf(cmd, "mmc read %lx %lx %x", dtb_load_addr,
			dtb_emmc_addr / 512, DTB_SIZE / 512);
		run_command(cmd, 0);
	}

	devtree = (struct fdt_header *)dtb_load_addr;
	if (be32_to_cpu(devtree->magic) == FDT_MAGIC) {
		handle_dtb(devtree);
		sprintf(cmd, "fdt addr 0x%lx; booti 0x%lx - 0x%lx",
			dtb_load_addr, kernel_load_addr, dtb_load_addr);
	} else
#endif
		sprintf(cmd, "booti 0x%lx", kernel_load_addr);
	run_command(cmd, 0);

	return 0;
}
#else
int do_mrvlboot(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return 0;
}
#endif

U_BOOT_CMD(
	mrvlboot, 1, 1, do_mrvlboot,
	"Do marvell specific boot",
	"Usage:\nmrvlboot"
);

#ifdef CONFIG_OF_LIBFDT
int do_dtb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char cmd[128];
	unsigned long dtb_load_addr, dtb_emmc_addr;
	int recovery_flag = 0;

	if (argc > 3)
		return cmd_usage(cmdtp);

	recovery_flag = get_recovery_flag();

	if (argc == 1) {
		/* Load DTB from emmc */
		dtb_load_addr = DTB_LOADADDR;
		dtb_emmc_addr = recovery_flag ? RECOVERY_DTB_EMMC_ADDR : DTB_EMMC_ADDR;
		dtb_emmc_addr += dtb_offset();
		printf("Load dtb to 0x%lx\n", dtb_load_addr);
		sprintf(cmd, "mmc read %lx %lx %x", dtb_load_addr,
				dtb_emmc_addr / 512, DTB_SIZE / 512);
		run_command(cmd, 0);

		load_op |= MV_DTB_LOADED;
		return 0;
	}
	/*load_op bit[3] flash dtb to mmc */
	if ((argc == 3) && !strcmp(argv[1], "b"))
		load_op |= MV_DTB_FLASH;

	sprintf(cmd, "tftpboot 0x%x %s",
		DTB_LOADADDR, argc == 3 ? argv[2] : argv[1]);
	run_command(cmd, 0);

	return 0;
}

U_BOOT_CMD(
	dtb, 3, 1, do_dtb,
	"Load dtb files through tftp or emmc",
	"Usage:\ndtb  load dtb file from emmc, then mrvlboot sequence would skip dtb load"
	"\ndtb <dtb_file> or dtb d <dtb_filename> to download dtb only"
	"\ndtb b <dtb_filename> to download dtb and flash"
);
#endif
