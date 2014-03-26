/*
 * (C) Copyright 2013
 * Marvell Semiconductors Ltd. <www.marvell.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>

.ltorg

.globl lowlevel_init
        .type   lowlevel_init, %function
lowlevel_init:
#ifdef CONFIG_SMP
	mrs     x0, mpidr_el1
	tst     x0, #15
	b.eq    master_cpu

slave_cpu:
	b	retn

master_cpu:
	/* Init the mailbox to zero */
	ldr     x1, =SECONDARY_CPU_MAILBOX
	str	xzr, [x1]

retn:
#endif
	ret

.ltorg

.globl loop_delay
	.type loop_delay, %function
loop_delay:
	subs	x0, x0, #1
	bhi	loop_delay
	ret

.ltorg

.globl save_boot_params
save_boot_params:
#ifdef CONFIG_OBM_PARAM_ADDR
	ldr     x0,  =CONFIG_OBM_PARAM_ADDR
	str	w11,	[x0]
#endif
	ret
