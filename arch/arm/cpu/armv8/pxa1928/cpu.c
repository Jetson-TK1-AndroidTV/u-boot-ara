/*
 * (C) Copyright 2011
 * Marvell Semiconductor <www.marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/pxa1928.h>

#define UARTCLK14745KHZ	(APBC_APBCLK | APBC_FNCLK | APBC_FNCLKSEL(1))

/* Get SoC Access to Generic Timer */
int timer_init(void)
{
	u32 tmp;

	/* Enable WDTR2*/
	tmp  = readl(PXA1928_MPMU_BASE + MPMU_CPRR);
	tmp = tmp | MPMU_APRR_WDTR;
	writel(tmp, PXA1928_MPMU_BASE + MPMU_CPRR);

	/* Initialize Counter to zero */
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	writel(0x0, PXA1928_TMR2_BASE + GEN_TMR_LD1);

	/* Program Generic Timer Clk Frequency */
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	tmp = readl(PXA1928_TMR2_BASE + GEN_TMR_CFG);
	tmp |= (3 << 4); /* 3.25MHz/32KHz Counter auto switch enabled */
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	writel(tmp, PXA1928_TMR2_BASE + GEN_TMR_CFG);

	/* Start the Generic Timer Counter */
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	tmp = readl(PXA1928_TMR2_BASE + GEN_TMR_CFG);
	tmp |= 0x3;
	writel(0xbaba, PXA1928_TMR2_BASE + TMR_WFAR);
	writel(0xeb10, PXA1928_TMR2_BASE + TMR_WSAR);
	writel(tmp, PXA1928_TMR2_BASE + GEN_TMR_CFG);

	return 0;
}

int arch_cpu_init(void)
{
	__attribute__((unused))	struct pxa1928apmu_registers *apmu =
		(struct pxa1928apmu_registers *)PXA1928_APMU_BASE;
	struct pxa1928mpmu_registers *mpmu =
		(struct pxa1928mpmu_registers *)PXA1928_MPMU_BASE;
	struct pxa1928apbc_registers *apbc =
		(struct pxa1928apbc_registers *)PXA1928_APBC_BASE;
	u32 val;

	/* Turn on APB, PLL1, PLL2 clock */
	writel(0x3FFFF, &apmu->gbl_clkctrl);

	/* Turn on APB2 clock, select APB2 clock 26MHz */
	writel(0x12, &apmu->apb2_clkctrl);

	/* Turn on MPMU register clock */
	writel(APBC_APBCLK, &apbc->mpmu);

	/*
	 * FIXME: This is a secure register so system may hang in global secure
	 * mode. This register should control timer2/timer3 clock while
	 * apbc->timers control timer1 clock.
	 */
	/* Turn on MPMU1 Timer register clock */
	writel(0, &apbc->mpmu1);

	/* Turn on clock gating (PMUM_CGR_PJ) */
	/*writel(0xFFFFFFFF, &mpmu->acgr);*/
	val = readl(&mpmu->cgr_pj);
	val |= (0x1<<19) | (0x1<<17) | (0x7<<13) | (0x1<<9) | (0x1F<<1);
	writel(val, &mpmu->cgr_pj);

	/* Turn on AIB clock */
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->aib);

	/* Turn on uart1 clock */
	writel(UARTCLK14745KHZ, &apbc->uart1);

	/* Turn on uart3 clock */
	writel(UARTCLK14745KHZ, &apbc->uart3);

	/* Turn on GPIO clock */
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->gpio);

#ifdef CONFIG_I2C_MV
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi1);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi2);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi3);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi4);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi5);
	writel(APBC_APBCLK | APBC_FNCLK, &apbc->twsi6);
#endif

#ifdef CONFIG_MV_SDHCI
	/* Enable SD1 clock */
	writel(APMU_PERIPH_CLK_EN | APMU_AXI_CLK_EN | APMU_PERIPH_RESET |
		APMU_AXI_RESET | (6 << 10), &apmu->sd1); /* PLL1(624Mhz)/6 */

	/* Enable SD3 clock */
	writel(APMU_PERIPH_CLK_EN | APMU_AXI_CLK_EN | APMU_PERIPH_RESET |
		APMU_AXI_RESET, &apmu->sd3);
#endif

#ifdef CONFIG_USB_GADGET_MV
	/* Enable usb clock */
	writel(APMU_AXI_CLK_EN | APMU_AXI_RESET , &apmu->usb);
#endif
	return 0;
}

#ifdef CONFIG_I2C_MV
void i2c_clk_enable(void)
{
}
#endif

#if defined(CONFIG_DISPLAY_CPUINFO)
int print_cpuinfo(void)
{
	printf("SoC:   PXA1928 (CA53 Core)\n");

	return 0;
}
#endif

u32 smp_hw_cpuid(void)
{
	return 0;
}

u32 smp_config(void)
{
	return 1;
}
