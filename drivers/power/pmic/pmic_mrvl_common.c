/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Xiang Wang <wangx@marvell.com>
 *  Yi Zhang <yizhang@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/marvell88pm_pmic.h>

__weak void pmic_base_init(struct pmic *p_base) {}
__weak void pmic_power_init(struct pmic *p_power) {}
__weak void pmic_gpadc_init(struct pmic *p_gpadc) {}

__weak void board_pmic_base_fixup(struct pmic *p_base) {}
__weak void board_pmic_power_fixup(struct pmic *p_power) {}
__weak void board_pmic_gpadc_fixup(struct pmic *p_gpadc) {}

int power_init_common(void)
{
	struct pmic *p_base, *p_power, *p_gpadc;

	/*------------base page setting-----------*/
	p_base = pmic_get(MARVELL_PMIC_BASE);
	if (!p_base)
		return -1;
	if (pmic_probe(p_base))
		return -1;
	pmic_base_init(p_base);
	board_pmic_base_fixup(p_base);

	/*------------gpadc page setting -----------*/
	p_gpadc = pmic_get(MARVELL_PMIC_GPADC);
	if (!p_gpadc)
		return -1;
	if (pmic_probe(p_gpadc))
		return -1;
	pmic_gpadc_init(p_gpadc);
	board_pmic_gpadc_fixup(p_gpadc);

	/*------------power page setting -----------*/
	p_power = pmic_get(MARVELL_PMIC_POWER);
	if (!p_power)
		return -1;
	if (pmic_probe(p_power))
		return -1;
	pmic_power_init(p_power);
	board_pmic_power_fixup(p_power);

	printf("PMIC init done!\n");
	return 0;
}

void pmic_reset_bd(void)
{
	u32 data;
	struct pmic *p_base;

	p_base = pmic_get(MARVELL_PMIC_BASE);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return;
	}
	/* 1.Enable FAULT_WU and FAULT_WU_EN */
	pmic_reg_read(p_base, 0xe7, &data);
	data |= ((1 << 3) | (1 << 2));
	pmic_reg_write(p_base, 0xe7, data);
	/* 2.Issue SW power down */
	data = 0x20;
	pmic_reg_write(p_base, 0x0d, data);
	/* Rebooting... */
}

/* get_powerup_down_log(void) function do following things:
 * save power up/down log in DDR for debug,
 * print them out
 */
void get_powerup_down_log(void)
{
	u32 val, powerup_l, powerD_l1, powerD_l2, *address, bit;
	address = (unsigned int *)(EMMD_INDICATOR + EMMD_PMIC_OFFSET);
	struct pmic *p_base;
	char *powerup_name[7] = {
		"ONKEY_WAKEUP    ",
		"CHG_WAKEUP      ",
		"EXTON_WAKEUP    ",
		"RSVD            ",
		"RTC_ALARM_WAKEUP",
		"FAULT_WAKEUP    ",
		"BAT_WAKEUP      "
	};
	char *powerD1_name[8] = {
		"OVER_TEMP ",
		"UV_VSYS1  ",
		"SW_PDOWN  ",
		"FL_ALARM  ",
		"WD        ",
		"LONG_ONKEY",
		"OV_VSYS   ",
		"RTC_RESET "
	};
	char *powerD2_name[5] = {
		"HYB_DONE   ",
		"UV_VSYS2   ",
		"HW_RESET   ",
		"PGOOD_PDOWN",
		"LONKEY_RTC "
	};


	p_base = pmic_get(MARVELL_PMIC_BASE);
	if (!p_base || pmic_probe(p_base)) {
		printf("access pmic failed...\n");
		return;
	}

	/*
	 * dump power up/down log
	 * save the power down log registers in  pmic rtc expire registers
	 */
	pmic_reg_read(p_base, POWER_UP_LOG, &powerup_l);
	pmic_reg_read(p_base, POWER_DOWN_LOG1, &powerD_l1);
	pmic_reg_read(p_base, POWER_DOWN_LOG2, &powerD_l2);

	printf("PowerUP log register:0x%x\n", powerup_l);
	printf(" ------------------------------------\n");
	printf("|     name(power up)      |  status  |\n");
	printf("|-------------------------|----------|\n");
	for (bit = 0; bit < 7; bit++)
		printf("|    %s     |    %x     |\n", powerup_name[bit], (powerup_l >> bit) & 1);
	printf(" ------------------------------------\n");

	printf("PowerDown log register1:0x%x\n", powerD_l1);
	printf(" -------------------------------\n");
	printf("|  name(power down1) |  status  |\n");
	printf("|--------------------|----------|\n");
	for (bit = 0; bit < 8; bit++)
		printf("|    %s      |    %x     |\n", powerD1_name[bit], (powerD_l1 >> bit) & 1);
	printf(" -------------------------------\n");

	printf("PowerDown log register2:0x%x\n", powerD_l2);
	printf(" -------------------------------\n");
	printf("|  name(power down2) |  status  |\n");
	printf("|--------------------|----------|\n");
	for (bit = 0; bit < 5; bit++)
		printf("|    %s     |    %x     |\n", powerD2_name[bit], (powerD_l2 >> bit) & 1);
	printf(" -------------------------------\n");

	/* write power up/down log to DDR*/
	val = powerup_l | (powerD_l1 << 8) | (powerD_l2 << 16);
	*address = val;
}

enum sys_boot_up_reason get_boot_up_reason(u32 *emmd_pmic_addr)
{
	u32 pmic_log = *emmd_pmic_addr;
	/* [power down log2: 0xe6][power down log1: 0xe5][power up log: 0x10] */

	if (!(pmic_log & 0xffff00))
		return SYS_BR_REBOOT;

	/* get power up log */
	pmic_log &= 0xff;

	switch (pmic_log) {
	case 0x1:  return SYS_BR_ONKEY;
	case 0x2:  return SYS_BR_CHARGE;
	case 0x10: return SYS_BR_RTC_ALARM;
	case 0x20:  return SYS_BR_FAULT_WAKEUP;
	case 0x40:  return SYS_BR_BAT_WAKEUP;
	default: return SYS_BR_POWER_OFF;
	}
}
