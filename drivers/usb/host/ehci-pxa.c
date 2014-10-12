/*
 * (C) Copyright 2012
 * eInfochips Ltd. <www.einfochips.com>
 * Written-by: Ajay Bhargav <ajay.bhargav@einfochips.com>
 *
 * This driver is based on Kirkwood echi driver
 * (C) Copyright 2009
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Prafulla Wadaskar <prafulla@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <usb.h>
#include "ehci.h"

#define PTS_ENABLE	2
#define PTS(x)		(((x) & 0x3) << 30)
#define PFSC		(1 << 24)

/*
 * EHCI host controller init
 */
int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	mrvl_usb_phy_init();

	*hccr = (struct ehci_hccr *)(PXA1928_USB_REG_BASE + 0x100);
	*hcor = (struct ehci_hcor *)((uint32_t) *hccr
			+ HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));

	printf("pxa-ehci: init hccr %x and hcor %x hc_length %d\n",
		(uint32_t)*hccr, (uint32_t)*hcor,
		(uint32_t)HC_LENGTH(ehci_readl(&(*hccr)->cr_capbase)));

	/* select ULPI phy */
	writel(PTS(PTS_ENABLE) | PFSC, &(*hcor)->or_portsc[0]);

	printf("pxa-ehci: init portsc %x = %x\n",
		(uint32_t)&(*hcor)->or_portsc[0], (uint32_t)(*hcor)->or_portsc[0]);

	return 0;
}

/*
 * EHCI host controller stop
 */
int ehci_hcd_stop(int index)
{
	return 0;
}
