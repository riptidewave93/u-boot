/*
 * (C) Copyright 2018 Chris Blake <chrisrblake93 at gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without
 * any warranty of any kind, whether express or implied.
 */

#include <common.h>
#include <watchdog.h>
#include <asm/arch/timer.h>
#include <asm/io.h>

#define WDT_CTRL_RESTART	(0x1 << 0)
#define WDT_CTRL_KEY		(0x0a57 << 1)
#define WDT_TIMEOUT_MASK	(0xf)

struct sunxi_wdt_reg {
	u32 wdt_ctrl;
	u32 wdt_cfg;
	u32 wdt_mode;
	u32 wdt_timeout_shift;
	u32 wdt_reset_mask;
	u32 wdt_reset_val;
};

static const struct sunxi_wdt_reg sun4i_dog_regs = {
	.wdt_ctrl = 0x00,
	.wdt_cfg = 0x04,
	.wdt_mode = 0x04,
	.wdt_timeout_shift = 3,
	.wdt_reset_mask = 0x02,
	.wdt_reset_val = 0x02,
};

static const struct sunxi_wdt_reg sun6i_dog_regs = {
	.wdt_ctrl = 0x10,
	.wdt_cfg = 0x14,
	.wdt_mode = 0x18,
	.wdt_timeout_shift = 4,
	.wdt_reset_mask = 0x03,
	.wdt_reset_val = 0x01,
};

static const int wdt_timeout_map[] = {
	[1] = 0x1,  /* 1s  */
	[2] = 0x2,  /* 2s  */
	[3] = 0x3,  /* 3s  */
	[4] = 0x4,  /* 4s  */
	[5] = 0x5,  /* 5s  */
	[6] = 0x6,  /* 6s  */
	[8] = 0x7,  /* 8s  */
	[10] = 0x8, /* 10s */
	[12] = 0x9, /* 12s */
	[14] = 0xA, /* 14s */
	[16] = 0xB, /* 16s */
};

#if defined(CONFIG_SUNXI_GEN_SUN6I)
static const struct sunxi_wdt_reg *regs = &sun6i_dog_regs;
#else
static const struct sunxi_wdt_reg *regs = &sun4i_dog_regs;
#endif

static void *wdt_base = &((struct sunxi_timer_reg *)SUNXI_TIMER_BASE)->wdog;

void hw_watchdog_reset(void)
{
	/* reload the watchdog */
	writel(WDT_CTRL_KEY | WDT_CTRL_RESTART, wdt_base + regs->wdt_ctrl);
}

void hw_watchdog_disable(void)
{
	/* Reset WDT Config */
	writel(0, wdt_base + regs->wdt_mode);
}

void hw_watchdog_init(void)
{
	const u32 timeout = CONFIG_SUNXI_WDT_TIMEOUT;
	u32 reg;

	reg = readl(wdt_base + regs->wdt_mode);
	reg &= ~(WDT_TIMEOUT_MASK << regs->wdt_timeout_shift);
	reg |= wdt_timeout_map[timeout] << regs->wdt_timeout_shift;
	writel(reg, wdt_base + regs->wdt_mode);

	hw_watchdog_reset();

	/* Set system reset function */
	reg = readl(wdt_base + regs->wdt_cfg);
	reg &= ~(regs->wdt_reset_mask);
	reg |= regs->wdt_reset_val;
	writel(reg, wdt_base + regs->wdt_cfg);

	/* Enable watchdog */
	reg = readl(wdt_base + regs->wdt_mode);
	reg |= WDT_MODE_EN;
	writel(reg, wdt_base + regs->wdt_mode);
 }
