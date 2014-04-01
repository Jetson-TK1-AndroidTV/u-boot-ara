/*
 *  Copyright (C) 2014 Marvell International Ltd.
 *  Hongyan Song <hysong@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MARVELL88PM_PMIC_H_
#define __MARVELL88PM_PMIC_H_


#define MARVELL88PM_BUCK_VOLT_MASK		0x7f
#define MARVELL88PM_LDO_VOLT_MASK		0xf

#define MARVELL88PM_I2C_ADDR		0x30
#define PMIC_NUM_OF_REGS	0xff

#define MARVELL_PMIC_BASE	"MARVELL_PMIC_BASE"
#define MARVELL_PMIC_POWER	"MARVELL_PMIC_POWER"
#define MARVELL_PMIC_GPADC	"MARVELL_PMIC_GPADC"

#define RTC_USR_DATA6 (0xEF)
#define POWER_UP_LOG  (0x10)
#define CHG_WAKEUP	(1 << 1)

#define POWER_DOWN_LOG1		(0xE5)
#define POWER_DOWN_LOG2		(0xE6)

/* this comes from SoC configuration */
#define EMMD_INDICATOR		0x8140000
#define EMMD_PMIC_OFFSET	0x40

enum sys_boot_up_reason {
	SYS_BR_POWER_OFF,
	SYS_BR_ONKEY,
	SYS_BR_CHARGE,
	SYS_BR_RTC_ALARM,
	SYS_BR_FAULT_WAKEUP,
	SYS_BR_BAT_WAKEUP,
	SYS_BR_REBOOT,
	SYS_BR_MAX,
};

int marvell88pm_set_buck_vol(struct pmic *p, unsigned int buck, unsigned int uV);
int marvell88pm_set_ldo_vol(struct pmic *p, unsigned int ldo, unsigned int uV);
int marvell88pm_get_buck_vol(struct pmic *p, unsigned int buck);
int marvell88pm_reg_update(struct pmic *p, int reg, unsigned int regval);

void pmic_base_init(struct pmic *p_base);
void pmic_power_init(struct pmic *p_power);
void pmic_gpadc_init(struct pmic *p_gpadc);

void board_pmic_base_fixup(struct pmic *p_base);
void board_pmic_power_fixup(struct pmic *p_power);
void board_pmic_gpadc_fixup(struct pmic *p_gpadc);

int power_init_common(void);
void pmic_reset_bd(void);
void get_powerup_down_log(void);
enum sys_boot_up_reason get_boot_up_reason(u32 *emmd_pmic_addr);

#endif /* __MARVELL88PM_PMIC_H_ */
