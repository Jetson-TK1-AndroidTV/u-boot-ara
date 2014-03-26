/*
e* (C) Copyright 2011
 * Marvell Semiconductors Ltd. <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#ifdef CONFIG_SDHCI
#include <sdhci.h>
#endif
#include <mvmfp.h>
#include <mv_recovery.h>
#include <asm/arch/mfp.h>
#include <malloc.h>
#include <eeprom_34xx02.h>
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>
#include <power/pxa1928_freq.h>
#include <asm/gpio.h>
#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif
#include "../common/cmdline.h"
#include "../common/obm2osl.h"

DECLARE_GLOBAL_DATA_PTR;

#define MACH_TYPE_PXA1928		3897
#define DVC_CONTROL_REG	0x4F
/*two dvc control register bits can support 4 control level
4(control level) * 4 (DVC level registers) can support 16level DVC*/
#define DVC_SET_ADDR1	(1 << 0)
#define DVC_SET_ADDR2	(1 << 1)
#define DVC_CTRl_LVL	4

#define PMIC_I2C_BUS 0
#define CHG_I2C_BUS 0
#define FG_I2C_BUS 0

#ifdef CONFIG_REVISION_TAG
static u8 board_rev;
static u8 board_eco[16];
static uchar board_info[64];
static uchar board_sn[33];
static u32 ddr_speed;
u32 get_board_rev(void)
{
	return (u32)board_rev;
}
#endif

unsigned int mv_profile = 0xFF;
/* Define CPU/DDR default max frequency
   CPU: 1300MHz
   DDR: 528MHz
   GC3D: 624MHz
   GC2D: 312MHz
*/
#define CPU_MAX_FREQ_DEFAULT	1300
#define DDR_MAX_FREQ_DEFAULT	528
#define GC3D_MAX_FREQ_DEFAULT	624
#define GC2D_MAX_FREQ_DEFAULT	312
/* Define CPU/DDR max frequency
   CPU: 1508MHz
   GC3D: 797MHz
   GC2D: 416MHz
*/
#define CPU_MAX_FREQ	1508
#define GC3D_MAX_FREQ	797
#define GC2D_MAX_FREQ   416

#ifdef CONFIG_CHECK_DISCRETE
static void unlock_aib_regs(void)
{
	struct pxa1928apbc_registers *apbc =
		(struct pxa1928apbc_registers *)PXA1928_APBC_BASE;

	writel(FIRST_ACCESS_KEY, &apbc->asfar);
	writel(SECOND_ACCESS_KEY, &apbc->assar);
}
#endif


/*
 * when boot kernel: UBOOT take Volume Up key (GPIO15/GPIO160) for recovery magic key
 * when power up: OBM take Volume Up key (GPIO15/GPIO160) for swdownloader
 */
static unsigned __attribute__((unused)) recovery_key = 15;
/* Take Volume Down key (GPIO17 for pop board/GPIO162 for discrete board) for fastboot */
static int fastboot_key = 17;
static int pxa1928_discrete = 0xff;
static int highperf;

static int check_discrete(void)
{
#ifdef CONFIG_CHECK_DISCRETE
	struct pxa1928aib_registers *aib =
		(struct pxa1928aib_registers *)PXA1928_AIB_BASE;

	/* unlock AIB registers */
	unlock_aib_regs();

	/* get discrete or pop info */
	pxa1928_discrete = (readl(&aib->nand) & (1 << 4)) ? 1 : 0;
#endif
	printf("PKG:   %s\n", (pxa1928_discrete == 0xff) ?
		"Discrete/PoP not checked on Zx chips" :
		(pxa1928_discrete ? "Discrete" : "PoP"));
	return 0;
}

static int chip_type = PXA1926_2L_DISCRETE;
static void check_chip_type(void)
{
	if (cpu_is_pxa1928_a0()) {
		if (1 == pxa1928_discrete)
			printf("Chip is pxa1928 A0 Discrete\n");
		else
			printf("Chip is pxa1928 A0 POP\n");
		return;
	}
	/* for pxa1928 B0 */
	chip_type = readl(PXA_CHIP_TYPE_REG);
	switch (chip_type) {
	case PXA1926_2L_DISCRETE:
		printf("Chip is PXA1926 B0 2L Discrete\n");
		break;

	case PXA1928_POP:
		printf("Chip is PXA1928 B0 PoP\n");
		break;

	case PXA1928_4L:
		printf("Chip is PXA1928 B0 4L\n");
		break;

	case PXA1928_2L:
		printf("Chip is PXA1928 B0 2L\n");
		break;

	default:
		chip_type = PXA1926_2L_DISCRETE;
		printf("Unknow chip type, set to PXA1926 B0 2L Discrete as default\n");
		break;
	}
	return;
}

int board_early_init_f(void)
{
#ifdef CONFIG_CMD_MFP
	u32 mfp_cfg[] = {
		/* UART3 */
		UART3_RXD_MMC2_DAT7_MFP33,
		UART3_TXD_MMC2_DAT6_MFP34,
		/* TWSI */
		PWR_SCL_MFP67,
		PWR_SDA_MFP68,
		TWSI6_SCL_MMC2_DAT5_MFP35,
		TWSI6_SDA_MMC2_DAT4_MFP36,

		/* eMMC */
		MMC3_DAT0_ND_IO8_MFP87,
		MMC3_DAT1_ND_IO9_MFP86,
		MMC3_DAT2_ND_IO10_MFP85,
		MMC3_DAT3_ND_IO11_MFP84,
		MMC3_DAT4_ND_IO12_MFP83,
		MMC3_DAT5_ND_IO13_MFP82,
		MMC3_DAT6_ND_IO14_MFP81,
		MMC3_DAT7_ND_IO15_MFP80,
		MMC3_CLK_SM_ADVMUX_MFP88,
		MMC3_CMD_SM_RDY_MFP89,
		MMC3_RST_ND_CLE_MFP90,
		/* SD */
		MMC1_DAT0_MFP62,
		MMC1_DAT1_MFP61,
		MMC1_DAT2_MFP60,
		MMC1_DAT3_MFP59,
		MMC1_DAT4_MFP58,
		MMC1_DAT5_MFP57,
		MMC1_DAT6_MFP56,
		MMC1_DAT7_MFP55,
		MMC1_CLK_MFP64,
		MMC1_CMD_MFP63,
		MMC1_CD_N_MFP65,
		MMC1_WP_MFP66,
		/* DVC pin */
		DVC_PIN0_MFP107,
		DVC_PIN1_MFP108,
		DVC_PIN2_MFP99,
		DVC_PIN3_MFP103,
		/*End of configureation*/
		MFP_EOC
	};
	u32 mfp_cfg_pop[] = {
		/* TWSI */
		TWSI2_SCL_MFP43,
		TWSI2_SDA_MFP44,
		TWSI3_SCL_MFP18,
		TWSI3_SDA_MFP19,
		TWSI4_SCL_MFP46,
		TWSI4_SDA_MFP45,
		TWSI5_SCL_MFP29,
		TWSI5_SDA_MFP30,
		/* Back light PWM2 */
		BACKLIGHT_PWM2_MFP51,
		BOOST_5V_EN_MFP10,
		LCD_RESET_MFP121,
		LCD_BACKLIGHT_EN_MFP6,
		/* Volume Down key for fastboot */
		GPIO17_MFP17,
		/* Volume Up key for recovery */
		GPIO15_MFP15,
		/*End of configureation*/
		MFP_EOC
	};
	u32 mfp_cfg_discrete[] = {
		GPIO136_MFP136,
		GPIO137_MFP137,
		GPIO138_MFP138,
		GPIO140_MFP140,
		GPIO141_MFP141,
		GPIO142_MFP142,
		GPIO143_MFP143,
		GPIO144_MFP144,
		/* USIM */
		IC_USB_P_DIS_MFP193,
		IC_USB_N_DIS_MFP194,
		USIM1_UCLK_DIS_MFP190,
		USIM1_UIO_DIS_MFP191,
		USIM1_NURST_DIS_MFP192,
		USIM2_UCLK_DIS_MFP195,
		USIM2_UIO_DIS_MFP196,
		USIM2_NURST_DIS_MFP197,
		/* MMC1_CD */
		MMC1_CD_ND_NCS1_MFP100,
		/* GPIO */
		GPIO65_MFP65,
		/* TWSI */
		TWSI2_SCL_MFP178,
		TWSI2_SDA_MFP179,
		TWSI3_SCL_MFP176,
		TWSI3_SDA_MFP177,
		TWSI4_SCL_MFP181,
		TWSI4_SDA_MFP180,
		TWSI5_SCL_MFP174,
		TWSI5_SDA_MFP175,
		/* Back light PWM2 */
		BACKLIGHT_PWM2_MFP186,
		BOOST_5V_EN_MFP155,
		LCD_RESET_MFP121,
		LCD_BACKLIGHT_EN_MFP151,
		/* Volume Down key for fastboot */
		GPIO162_MFP162,
		/* Volume Up key for recovery */
		GPIO160_MFP160,
		MFP_EOC
	};
	mfp_config(mfp_cfg);
	check_discrete();
	if (1 == pxa1928_discrete) {
		recovery_key = 160;
		fastboot_key = 162;
		mfp_config(mfp_cfg_discrete);
	} else {
		mfp_config(mfp_cfg_pop);
	}
#endif
	return 0;
}

int board_init(void)
{
	check_chip_type();

#ifdef CONFIG_REVISION_TAG
	/* Get board info from eeprom */
	struct eeprom_data eeprom_data;
	printf("\nBoard info from EEPROM No.2:\n");
	eeprom_data.index = 2;
	eeprom_data.i2c_num = 4;
	eeprom_get_board_category(&eeprom_data, board_info);
	if (!strcmp((char *)board_info, "PXA1928_EMCP_V30") ||
		!strcmp((char *)board_info, "PXA1926_EMCP_V30"))
		board_rev = 3;
	else if (!strcmp((char *)board_info, "PXA1928_EMCP_V40") ||
		!strcmp((char *)board_info, "PXA1926_EMCP_V40"))
		board_rev = 4;
	else
		board_rev = 2;
	printf("Board rev:%d\n", board_rev);
	if (eeprom_get_board_sn(&eeprom_data, board_sn)) {
		*board_sn = 0;
		printf("Board serial number is invalid\n");
	}
	eeprom_get_chip_name(&eeprom_data, board_info);
	eeprom_get_chip_stepping(&eeprom_data, board_info);
	eeprom_get_board_reg_date(&eeprom_data);
	eeprom_get_board_state(&eeprom_data);
	eeprom_get_user_team(&eeprom_data);
	eeprom_get_current_user(&eeprom_data);
	eeprom_get_board_eco(&eeprom_data, board_eco);
	eeprom_get_lcd_resolution(&eeprom_data, board_info);
	eeprom_get_lcd_screen_size(&eeprom_data, board_info);
	eeprom_get_ddr_type(&eeprom_data, board_info);
	eeprom_get_ddr_size_speed(&eeprom_data);
	eeprom_get_emmc_type(&eeprom_data, board_info);
	eeprom_get_emmc_size(&eeprom_data, board_info);
	eeprom_get_rf_name_ver(&eeprom_data);
	eeprom_get_rf_type(&eeprom_data, board_info);
	printf("\n");
#endif
	gd->bd->bi_arch_number = MACH_TYPE_PXA1928;
	gd->bd->bi_boot_params = CONFIG_TZ_HYPERVISOR_SIZE + 0x00100000;

#ifdef CONFIG_CMD_GPIO
	gpio_direction_input(recovery_key);
#endif

	printf("run board_init\n");
	return 0;
}

#ifdef CONFIG_GENERIC_MMC
#ifdef CONFIG_POWER_88PM860
static unsigned char pxa1928_recovery_reg_read(void)
{
	u32 data = 0;
	struct pmic *p_base;

	p_base = pmic_get(MARVELL_PMIC_BASE);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return -1;
	}
	/* Get the magic number from RTC register */
	pmic_reg_read(p_base, 0xef, &data);
	return (unsigned char)data;
}
static unsigned char pxa1928_recovery_reg_write(unsigned char value)
{
	struct pmic *p_base;

	p_base = pmic_get(MARVELL_PMIC_BASE);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return -1;
	}
	/* Set the magic number from RTC register */
	pmic_reg_write(p_base, 0xef, value);
	return 0;
}
#else
static unsigned char pxa1928_recovery_reg_read(void)
{
	return 0;
}
static unsigned char pxa1928_recovery_reg_write(unsigned char value)
{
	return 0;
}
#endif
static struct recovery_reg_funcs pxa1928_recovery_reg_funcs = {
	.recovery_reg_read = pxa1928_recovery_reg_read,
	.recovery_reg_write = pxa1928_recovery_reg_write,
};

int recovery_key_detect(void)
{
	return !gpio_get_value(recovery_key);
}
#endif

#ifdef CONFIG_OF_LIBFDT
unsigned int dtb_offset(void)
{
	unsigned int offset = pxa1928_discrete ? 1 : 0;

	return offset * DTB_SIZE;
}

void handle_dtb(struct fdt_header *devtree)
{
	char cmd[128];

	/* set dtb addr */
	sprintf(cmd, "fdt addr 0x%p", devtree);
	run_command(cmd, 0);

#ifdef CONFIG_PXA168_FB
	if (bl_chip_id == BL_KDT3102) {
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/lm3532@38", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/pm828x@10", 0);
		run_command("fdt rm /pwm-bl", 0);
	} else {
		run_command("fdt rm /r63311 bl_gpio", 0);
		if (bl_chip_id == PM828X_ID)
			run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/lm3532@38", 0);
		else
			run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/pm828x@10", 0);
	}

	if (g_panel_id == SHARP_1080P_ID) {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /otm1281", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/gt913@5d", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/s3202@20", 0);
		run_command("fdt set /soc/axi/disp/path1/pn_sclk_clksrc clksrc pll3", 0);
	} else {
		run_command("fdt rm /lg4591", 0);
		run_command("fdt rm /r63311", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/gt913@5d", 0);
		run_command("fdt rm /soc/apb@d4000000/i2c@d4033800/s3202@22", 0);
	}
#endif /* end of CONFIG_PXA168_FB */

	if (3 == board_rev || 4 == board_rev) {
		run_command("fdt set /soc/apb@d4000000/i2c@d4031000/mpu9250@69 negate_x <1>", 0);
		run_command("fdt set /soc/apb@d4000000/i2c@d4031000/mpu9250@69 negate_y <1>", 0);
		run_command("fdt set /soc/apb@d4000000/i2c@d4011000/88pm860@30/headset marvell,headset-flag <0>", 0);
		run_command("fdt set /soc/apb@d4000000/i2c@d4011000/88pm860@30/headset marvell,ground-detect <1>", 0);
	}

	/*reserve mem for emmd*/
	run_command("fdt rsvmem add 0x8140000 0x1000", 0);
	/* pass profile number */
	sprintf(cmd, "fdt set /profile marvell,profile-number <%d>\n", mv_profile);
	run_command(cmd, 0);

	/*
	 * we use 1926 pp table by default, if if sethighperf cmd is set,
	 * use pxa1928 pp instead.
	 */
	if (highperf)
		run_command("fdt set /pp_version version pxa1928", 0);

	if (cpu_is_pxa1928_a0()) {
		run_command("fdt mknode / chip_version", 0);
		run_command("fdt set /chip_version version a0", 0);
		run_command("fdt set /clock-controller/peri_clock/gc2d_clk lpm-qos <3>", 0);
		run_command("fdt rm /soc/apb@d4000000/map@c3000000/ marvell,b0_fix", 0);
		run_command("fdt set /soc/apb@d4000000/thermal@d403b000 marvell,version-flag <3>", 0);
		/* overwrite emmc rx/tx setting for A0 */
		run_command("fdt set /soc/axi@d4200000/sdh@d4217000 marvell,sdh-dtr-data"
			"<2 52000000 104000000 0 0 0 0 1 7 52000000 104000000 0 150 3 1 1>", 0);

		/*reset TWSI3 PIN for dts*/
		if (pxa1928_discrete == 1) {
			run_command("fdt set /soc/apb@d4000000/pinmux@d401e000/pinmux_twsi3_gpio_pins "
				"pinctrl-single,pins <0x00000290 0x00000004 0x00000294 0x00000004>", 0);
			run_command("fdt set /soc/apb@d4000000/pinmux@d401e000/pinmux_isp_twsi3_pins "
				"pinctrl-single,pins <0x000002d8 0x00000007 0x000002d4 0x00000007>", 0);
		} else {
			run_command("fdt set /soc/apb@d4000000/pinmux@d401e000/pinmux_twsi3_gpio_pins "
				"pinctrl-single,pins <0x00000160 0x00000004 0x00000164 0x00000004>", 0);
			run_command("fdt set /soc/apb@d4000000/pinmux@d401e000/pinmux_isp_twsi3_pins "
				"pinctrl-single,pins <0x00000194 0x00000007 0x00000198 0x00000007>", 0);
		}

		/* overwrite emmc rx/tx setting for A0 */
		run_command("fdt set /soc/axi@d4200000/sdh@d4217000 marvell,sdh-dtr-data "
			"<0 26000000 104000000 0 0 0 0 0 2 52000000 104000000 0 0 0 0 1 "
			"7 52000000 104000000 0 150 3 1 1 9 0xffffffff 104000000 0 0 0 0 0>", 0);
		/* overwrite sd card rx/tx setting for A0 */
		run_command("fdt set /soc/axi@d4200000/sdh@d4280000 marvell,sdh-dtr-data "
			"<0 26000000 104000000 0 0 0 0 0 1 26000000 104000000 0 0 0 0 1 "
			"3 52000000 104000000 0 0 0 0 1 4 52000000 104000000 0 0 0 0 1 "
			"5 52000000 104000000 0 0 0 0 1 7 52000000 104000000 0 0 0 0 1 "
			"9 0xffffffff 104000000 0 0 0 0 0>", 0);
		/* overwrite sdio rx/tx setting for A0 */
		run_command("fdt set /soc/axi@d4200000/sdh@d4280800 marvell,sdh-dtr-data "
			"<0 26000000 104000000 0 0 0 0 0 1 26000000 104000000 0 0 0 0 1 "
			"3 52000000 104000000 0 0 0 0 1 4 52000000 104000000 0 0 0 0 1 "
			"5 52000000 104000000 0 0 0 0 1 7 52000000 104000000 0 0 0 0 1 "
			"9 0xffffffff 104000000 0 0 0 0 0>", 0);
		/* disable emmc hs200 mode */
		run_command("fdt set /soc/axi@d4200000/sdh@d4217000 marvell,sdh-host-caps2-disable <0x20>", 0);
		/* disable sd card sdr104 mode */
		run_command("fdt set /soc/axi@d4200000/sdh@d4280000 marvell,sdh-host-caps-disable <0x40000>", 0);
		/* disable sdio sdr104 mode */
		run_command("fdt set /soc/axi@d4200000/sdh@d4280800 marvell,sdh-host-caps-disable <0x40000>", 0);
	} else {
		/* update dtb so as not to enable ICU for B0 stepping */
		run_command("fdt set /pxa1928_apmu_ver version bx", 0);
		run_command("fdt rm /soc/axi/wakeupgen@d4284000", 0);

		if (pxa1928_discrete == 1)
			run_command("fdt set /soc/apb@d4000000/pinmux@d401e000/pinmux_twsi3_gpio_pins "
				"pinctrl-single,pins <0x00000290 0x00000006 0x00000294 0x00000006>", 0);
		else
			run_command("fdt set /soc/apb@d4000000/pinmux@d401e000/pinmux_twsi3_gpio_pins "
				"pinctrl-single,pins <0x00000160 0x00000006 0x00000164 0x00000006>", 0);
	}

	if ((board_rev == 1) || (board_rev == 2)) {
		/*update plat_cam revision to distinguish which board*/
		run_command("fdt set /plat_cam revision <1>", 0);
		/* overwrite sd card detect pinmux (0xb8/0x7 -> 0x38/0x0) */
		run_command("fdt set /soc/apb@d4000000/pinmux@d401e000/pinmux_sdh1_pins_pullup pinctrl-single,pins"
			"<0x20 0x0 0x24 0x0 0x28 0x0 0x2c 0x0 0x30 0x0 0x38 0x0 0x3c 0x0>", 0);
	}

}
#endif

#ifndef CONFIG_GLB_SECURE_EN
#define SOC_POWER_POINT_ADDR0	0xD4292AAC
#define SOC_POWER_POINT_ADDR1	0xD4292AB0
static int svt_profile_map_table[] = {
	999, 445, 433, 421, 408, 395, 383, 371,
	358, 358, 333, 333, 309, 309, 1,
};
void show_dro(void)
{
	u32 val0, val1;
	int lvt, nlvt, nsvt, svt;
	int i;

	val0 = readl(SOC_POWER_POINT_ADDR0);
	val1 = readl(SOC_POWER_POINT_ADDR1);
	lvt = val0 & 0x7FF;
	nlvt = (val0 & 0x3FF800) >> 11;
	nsvt = ((val1 & 0x1) << 10) | ((val0 & 0xFFC00000) >> 22);
	svt = (val1 & 0xFFE) >> 1;
	printf("----show dro----\n");
	printf("LVT NUMBER: %d\n", lvt);
	printf("NLVT NUMBER: %d\n", nlvt);
	printf("NSVT NUMBER: %d\n", nsvt);
	printf("SVT NUMBER: %d\n", svt);
	printf("----------------\n");

	for (i = 1; i < 15; i++) {
		if (svt >= svt_profile_map_table[i] &&
			svt < svt_profile_map_table[i - 1])
			break;
	}
	mv_profile = i + 1;
	if (mv_profile >= 15 || mv_profile < 0)
		mv_profile = 0;
	printf("SoC Profile Number: %d\n", mv_profile);
}

int do_dro_read(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	show_dro();
	return 0;
}

U_BOOT_CMD(
	dro_read,	1,	0,	do_dro_read,
	"dro_read", ""
);
#endif

static int do_sethighperf(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char *cmdline;
	char *ep;
	unsigned int new_set;
	static unsigned int old_set;
	u32 cpu_max = CPU_MAX_FREQ_DEFAULT;
	u32 ddr_max = DDR_MAX_FREQ_DEFAULT;
	u32 gc3d_max = GC3D_MAX_FREQ_DEFAULT;
	u32 gc2d_max = GC2D_MAX_FREQ_DEFAULT;

	if (argc != 2) {
		printf("usage: sethighperf 0 or 1 to enable low or high performance\n");
		return -1;
	}
	new_set = simple_strtoul((const char *)argv[1], &ep, 10);
	if (new_set != 0 && new_set != 1) {
		printf("usage: sethighperf 0 or 1 to enable low or high performance\n");
		return -1;
	}
	if (((cpu_is_pxa1928_a0() && (1 != pxa1928_discrete)) || (chip_type != PXA1926_2L_DISCRETE)) &&
		old_set != new_set) {
		if (new_set == 1)
			highperf = 1;
		else if (new_set == 0)
			highperf = 0;

		old_set = new_set;
		cmdline = malloc(COMMAND_LINE_SIZE);
		strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
		remove_cmdline_param(cmdline, "cpu_max=");
		remove_cmdline_param(cmdline, "ddr_max=");
		remove_cmdline_param(cmdline, "gc3d_max=");
		remove_cmdline_param(cmdline, "gc2d_max=");
		if (1 == new_set) {
			cpu_max = CPU_MAX_FREQ;
			gc3d_max = GC3D_MAX_FREQ;
			gc2d_max = GC2D_MAX_FREQ;
			if (ddr_speed != 0)
				ddr_max = ddr_speed;
		} else {
			if (ddr_speed != 0 && ddr_speed < DDR_MAX_FREQ_DEFAULT)
				ddr_max = ddr_speed;
		}
		sprintf(cmdline + strlen(cmdline), " cpu_max=%u000", cpu_max);
		sprintf(cmdline + strlen(cmdline), " ddr_max=%u000", ddr_max);
		sprintf(cmdline + strlen(cmdline), " gc3d_max=%u000", gc3d_max);
		sprintf(cmdline + strlen(cmdline), " gc2d_max=%u000", gc2d_max);
		setenv("bootargs", cmdline);
		free(cmdline);
	}
	printf("sethighperf success\n");
	return 0;
}

U_BOOT_CMD(
	sethighperf, 3, 0, do_sethighperf,
	"Setting cpu, ddr frequence for high performance or low performance",
	""
);

#ifdef CONFIG_PXA1928_CONCORD_BRINGUP
static void set_limit_max_frequency(unsigned int type, u32 max)
{
	return;
}
#else
static void set_limit_max_frequency(unsigned int type, u32 max)
{
	char *cmdline;
	char *rem;
	const char *add;
	u32 tmp;
	u32 max_default;
	u32 max_preferred;

	switch (type) {
	case SKU_CPU_MAX_PREFER:
		rem = "cpu_max=";
		add = " cpu_max=%u000";
		max_default = CPU_MAX_FREQ_DEFAULT;
		break;

	case SKU_DDR_MAX_PREFER:
		rem = "ddr_max=";
		add = " ddr_max=%u000";
		max_default = DDR_MAX_FREQ_DEFAULT;
		break;

	case SKU_GC3D_MAX_PREFER:
		rem = "gc3d_max=";
		add = " gc3d_max=%u000";
		max_default = GC3D_MAX_FREQ_DEFAULT;
		break;

	case SKU_GC2D_MAX_PREFER:
		rem = "gc2d_max=";
		add = " gc2d_max=%u000";
		max_default = GC2D_MAX_FREQ_DEFAULT;
		break;

	default:
		return;
	}

	if (0 == max)
		max = max_default;

	max_preferred = get_sku_max_setting(type);
	if (0 == max_preferred)
		max_preferred = min(max_default, max);
	else {
		if (1 == pxa1928_discrete) {
			tmp = min(max_default, max);
			max_preferred = min(max_preferred, tmp);
		} else {
			max_preferred = min(max_preferred, max);
		}
	}
	cmdline = malloc(COMMAND_LINE_SIZE);
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
	remove_cmdline_param(cmdline, rem);
	sprintf(cmdline + strlen(cmdline), add, max_preferred);
	setenv("bootargs", cmdline);
	free(cmdline);
}
#endif
#if defined(CONFIG_PXA1928_DFC)
static u32 get_ddr_type(void)
{
	if (((readl(0xd0000300) >> 4) & 0xf) == 0xa) /* lpddr3 */
		if (((readl(0xd0000200) >> 16) & 0x1f) == 0xd) /* 1GB */
			return 1;
		else if (((readl(0xd0000210) >> 8) & 0xf) == 0x5)/* Hynix 2GB */
			return readl(0xD4282C98) ? 4 : 2; /* 4 for dis, 2 for pop */
		else /* Elpida 2GB */
			return 3;
	else /* lpddr2 */
		return 0;
}
#endif

#define LOW_RAM_DDR_SIZE 0x30000000

/* CONFIG_POWER_OFF_CHARGE=y */
__weak void power_off_charge(u32 *emmd_pmic, u8 pmic_i2c, u8 chg_i2c, u8 fg_i2c,
			     u32 lo_uv, u32 hi_uv) {}
int misc_init_r(void)
{
	unsigned long i, ddr_size = 0;
	char *cmdline;
#if defined(CONFIG_PXA1928_DFC)
	u32 ddr_type;
	ddr_type = get_ddr_type();
#endif
	/*
	 * handle charge in uboot, the reason we do it here is because
	 * 1) we need to wait GD_FLG_ENV_READY to setenv()
	 * 2) we need to use LCD to show uboot charging logo
	 */
	power_off_charge((u32 *)(EMMD_INDICATOR + EMMD_PMIC_OFFSET),
			 PMIC_I2C_BUS, CHG_I2C_BUS, FG_I2C_BUS, 0, 3600000);

	cmdline = malloc(COMMAND_LINE_SIZE);
	if (!cmdline) {
		printf("misc_init_r: can not allocate memory for cmdline\n");
		return -1;
	}
	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		if (gd->bd->bi_dram[i].size == 0)
			break;
		ddr_size += gd->bd->bi_dram[i].size;
	}

	/* set ddr size in bootargs */
	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
	remove_cmdline_param(cmdline, "mem=");
	sprintf(cmdline + strlen(cmdline), " mem=%ldM", ddr_size>>20);
	setenv("bootargs", cmdline);
#ifdef CONFIG_OBM_PARAM_ADDR
	struct OBM2OSL *params = 0;
	int keypress = 0;

	strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
	params = (struct OBM2OSL *)(uintptr_t)(*(u32 *)CONFIG_OBM_PARAM_ADDR);
	if (params && params->signature == OBM2OSL_IDENTIFIER)
		keypress = params->booting_mode;

	switch (keypress) {
	case PRODUCT_UART_MODE:
		remove_cmdline_param(cmdline, "androidboot.console=");
		remove_cmdline_param(cmdline, "androidboot.bsp=");
		remove_cmdline_param(cmdline, "console=");
		remove_cmdline_param(cmdline, "earlyprintk=");
		sprintf(cmdline + strlen(cmdline), " androidboot.bsp=2");
		break;

	case PRODUCT_USB_MODE:
		remove_cmdline_param(cmdline, "androidboot.console=");
		remove_cmdline_param(cmdline, "androidboot.bsp=");
		sprintf(cmdline + strlen(cmdline),
			" androidboot.console=ttyS1 androidboot.bsp=1");
		break;

	default:
		break;
	}
	setenv("bootargs", cmdline);
#endif
	/* set cpu, gc3d, gc2d  max frequency */
	set_limit_max_frequency(SKU_CPU_MAX_PREFER, CPU_MAX_FREQ);
	set_limit_max_frequency(SKU_GC3D_MAX_PREFER, GC3D_MAX_FREQ);
	set_limit_max_frequency(SKU_GC2D_MAX_PREFER, GC2D_MAX_FREQ);
#ifdef CONFIG_REVISION_TAG
	struct eeprom_data eeprom_data;
	eeprom_data.index = 2;
	eeprom_data.i2c_num = 4;
	char *ep;
	u8 ddr_speed_info[8] = {0};
	if (eeprom_get_ddr_speed(&eeprom_data, ddr_speed_info))
		ddr_speed = 0;
	else
		ddr_speed = simple_strtoul((const char *)ddr_speed_info, &ep, 10);
	if (0 == ddr_speed)
		printf("Ddr speed info is invalid\n");
	set_limit_max_frequency(SKU_DDR_MAX_PREFER, ddr_speed);
	if (*board_sn != 0) {
		strncpy(cmdline, getenv("bootargs"), COMMAND_LINE_SIZE);
		remove_cmdline_param(cmdline, "androidboot.serialno=");
		sprintf(cmdline + strlen(cmdline), " androidboot.serialno=%s", board_sn);
		setenv("bootargs", cmdline);

		/* this sn is also used as fastboot serial no. */
		setenv("fb_serial", (char *)board_sn);
	}
#endif

#ifdef CONFIG_CMD_FASTBOOT
	setenv("fbenv", "mmc0");
#endif

	if (ddr_size <= LOW_RAM_DDR_SIZE)
		sprintf(cmdline + strlen(cmdline), " androidboot.low_ram=true");

#ifndef CONFIG_GLB_SECURE_EN
	show_dro();
#endif
	/*
	 * bus 0 is used by pmic, set here for debug with
	 * "i2c probe", this should be the called just before exit,
	 * in case the default bus number is changed
	 */
	i2c_set_bus_num(0);

#if defined(CONFIG_PXA1928_DFC)
	pxa1928_fc_init(ddr_type);

	run_command("setcpurate 624", 0);
	run_command("setddrrate hwdfc 312", 0);
	run_command("setaxirate 156", 0);
#endif
	*(u32 *)(CONFIG_CORE_BUSY_ADDR) = 0x0;

	free(cmdline);
	return 0;
}

#ifdef CONFIG_MV_SDHCI
 /* eMMC: BUCK2/VCC_IO_NAND(1.8v)->eMMC(vmmcq), LDO4/V3P3_NAND(2.8v)->eMMC(vmmc) (default on)
 * SD: LDO6/VCC_IO_MMC1(3.3v)->SD(vmmcq), LDO10/V_MMC_CARD(3.3v)->SD(vmmc) (default off)
 */
int board_mmc_init(bd_t *bd)
{
	ulong mmc_base_address[CONFIG_SYS_MMC_NUM] = CONFIG_SYS_MMC_BASE;
	u8 i;
	u32 val;

	for (i = 0; i < CONFIG_SYS_MMC_NUM; i++) {
		if (mv_sdh_init(mmc_base_address[i], 104000000, 0,
				SDHCI_QUIRK_32BIT_DMA_ADDR))
			return 1;
		/*
		 * use default hardware clock gating
		 * by default, SD_FIFO_PARM = 0x70005
		 */
		if (i == 0) {
			/*
			 * emmc need to tune RX/TX under HS50
			 * RX need to set proper delays cycles.
			 * TX can work just invert the internal clock (TX_CFG_REG[30])
			 * but also set the delay cycles here for safety.
			 */
			writel(TX_MUX_DLL | TX_HOLD_DELAY0(0x16A),
						mmc_base_address[i] + TX_CFG_REG);
			writel(SDCLK_DELAY(0xA0) | SDCLK_SEL1(0x1),
						mmc_base_address[i] + RX_CFG_REG);
		} else {
			/*
			 * sd card can work under HS50 by default.
			 * but also invert TX internal clock (TX_CFG_REG[30]) here for safety.
			 */
			val = readl(mmc_base_address[i] + TX_CFG_REG);
			val |= TX_INT_CLK_INV;
			writel(val, mmc_base_address[i] + TX_CFG_REG);
		}
	}

	p_recovery_reg_funcs = &pxa1928_recovery_reg_funcs;
	return 0;
}
#endif

void reset_cpu(ulong ignored)
{
#ifdef CONFIG_POWER_88PM860
	printf("pxa1928 board rebooting...\n");
	pmic_reset_bd();
#endif
}

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
	u32 val;

	val = smp_hw_cpuid();
	if (val >= 0 && val <= 3)
		printf("Boot Core: Core %d\n", val + 1);
	else
		printf("Boot Core is unknown.");

	val = smp_config();
	printf("Available Cores: %s %s %s %s\n"
			, (val & 0x1) ? ("Core 1") : ("")
			, (val & 0x2) ? ("Core 2") : ("")
			, (val & 0x4) ? ("Core 3") : ("")
			, (val & 0x8) ? ("Core 4") : ("")
	);

	return 0;
}
#endif
#ifdef CONFIG_POWER_88PM860
void board_pmic_power_fixup(struct pmic *p_power)
{
	u32 val;
	unsigned int mask, dvc_ctrl;

	/* enable buck1 dual phase mode */
	pmic_reg_read(p_power, 0x8e, &val);
	val |= (1 << 2);
	pmic_reg_write(p_power, 0x8e, val);

	/* set buck1 all the DVC register 16 levels all at 1.2v
	 * dvc_ctrl is the value of the two dvc control bits
	 */
	for (dvc_ctrl = 0; dvc_ctrl < DVC_CTRl_LVL; dvc_ctrl++) {
		pmic_reg_read(p_power, DVC_CONTROL_REG, &val);
		mask = (DVC_SET_ADDR1 | DVC_SET_ADDR2);
		val &= ~mask;
		val |= dvc_ctrl & mask;
		pmic_reg_write(p_power, DVC_CONTROL_REG, val);

		val = 0x30;
		pmic_reg_write(p_power, 0x3c, val);
		pmic_reg_write(p_power, 0x3d, val);
		pmic_reg_write(p_power, 0x3e, val);
		pmic_reg_write(p_power, 0x3f, val);

		pmic_reg_write(p_power, 0x4b, val);
		pmic_reg_write(p_power, 0x4c, val);
		pmic_reg_write(p_power, 0x4d, val);
		pmic_reg_write(p_power, 0x4e, val);

		/* set buck3 all the DVC register at 1.2v */
		val = 0x30;
		pmic_reg_write(p_power, 0x41, val);
		pmic_reg_write(p_power, 0x42, val);
		pmic_reg_write(p_power, 0x43, val);
		pmic_reg_write(p_power, 0x44, val);

		/* set buck5 all the DVC register at 3.3v for WIB_SYS */
		val = 0x72;
		pmic_reg_write(p_power, 0x46, val);
		pmic_reg_write(p_power, 0x47, val);
		pmic_reg_write(p_power, 0x48, val);
		pmic_reg_write(p_power, 0x49, val);
	}
	/* set ldo12 at 2.8v for lcd */
	marvell88pm_set_ldo_vol(p_power, 12, 2800000);
	/* set ldo5 at 2.8v for emmc */
	marvell88pm_set_ldo_vol(p_power, 5, 2800000);

	/* set ldo17 at 2.8v */
	marvell88pm_set_ldo_vol(p_power, 17, 2800000);
}
#endif

__weak int power_init_common(void) {return -1; }
__weak int pmic_init(unsigned char bus) {return -1; }
int power_init_board(void)
{
	/* init PMIC  */
	if (pmic_init(PMIC_I2C_BUS))
		return -1;
	if (power_init_common()) {
		printf("%s: init pmic fails.\n", __func__);
		return -1;
	}

	return 0;
}

#ifdef CONFIG_CMD_NET
int board_eth_init(bd_t *bis)
{
	int res = -1;

#if defined(CONFIG_CI_UDC)
	if (usb_eth_initialize(bis) >= 0)
		res = 0;
#endif
	return res;
}
#endif
