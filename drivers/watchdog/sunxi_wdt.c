// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Chris Blake <chrisrblake93 at gmail.com>
 */
#include <common.h>
#include <dm.h>
#include <wdt.h>
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

static const struct sunxi_wdt_reg sun4i_wdt_reg = {
	.wdt_ctrl = 0x00,
	.wdt_cfg = 0x04,
	.wdt_mode = 0x04,
	.wdt_timeout_shift = 3,
	.wdt_reset_mask = 0x02,
	.wdt_reset_val = 0x02,
};

static const struct sunxi_wdt_reg sun6i_wdt_reg = {
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

static void *wdt_base = &((struct sunxi_timer_reg *)SUNXI_TIMER_BASE)->wdog;

void hw_watchdog_reset()
{
#if defined(CONFIG_SUNXI_GEN_SUN6I)
	static const struct sunxi_wdt_reg *regs = &sun6i_wdt_reg;
#else
	static const struct sunxi_wdt_reg *regs = &sun4i_wdt_reg;
#endif

	/* reload the watchdog */
	writel(WDT_CTRL_KEY | WDT_CTRL_RESTART, wdt_base + regs->wdt_ctrl);
}

static int sunxi_wdt_start(struct udevice *dev, u32 tout, ulong flags)
{
	struct sunxi_wdt_priv *priv = dev_get_priv(dev);
	struct sunxi_wdt_reg *driver_data = dev_get_driver_data(dev);
	const u32 timeout = CONFIG_WDT_SUNXI_TIMEOUT;
	u32 reg;

	reg = readl(wdt_base + driver_data->wdt_mode);
	reg &= ~(WDT_TIMEOUT_MASK << driver_data->wdt_timeout_shift);
	reg |= wdt_timeout_map[timeout] << driver_data->wdt_timeout_shift;
	writel(reg, wdt_base + driver_data->wdt_mode);

	hw_watchdog_reset();

	/* Set system reset function */
	reg = readl(wdt_base + driver_data->wdt_cfg);
	reg &= ~(driver_data->wdt_reset_mask);
	reg |= driver_data->wdt_reset_val;
	writel(reg, wdt_base + driver_data->wdt_cfg);

	/* Enable watchdog */
	reg = readl(wdt_base + driver_data->wdt_mode);
	reg |= WDT_MODE_EN;
	writel(reg, wdt_base + driver_data->wdt_mode);

	info("Sunxi Watchdog Loaded")

	return 0;
}

static int sunxi_wdt_stop(struct udevice *dev)
{
	struct sunxi_wdt_priv *priv = dev_get_priv(dev);
	struct sunxi_wdt_reg *driver_data = dev_get_driver_data(dev);

	/* Reset WDT Config */
	writel(0, wdt_base + driver_data->wdt_mode);

	return 0;
}

static int sunxi_wdt_reset(struct udevice *dev)
{
	struct sunxi_wdt_priv *priv = dev_get_priv(dev);
	struct sunxi_wdt_reg *driver_data = dev_get_driver_data(dev);

	hw_watchdog_reset();

	return 0;
}

static const struct wdt_ops sunxi_wdt_ops = {
	.start = sunxi_wdt_start,
	.reset = sunxi_wdt_reset,
	.stop = sunxi_wdt_stop,
};

static const struct udevice_id sunxi_wdt_ids[] = {
	{ .compatible = "allwinner,sun4i-a10-wdt", .data = &sun4i_wdt_reg },
	{ .compatible = "allwinner,sun6i-a31-wdt", .data = &sun6i_wdt_reg },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(wdt_sandbox) = {
	.name = "wdt_sunxi",
	.id = UCLASS_WDT,
	.of_match = sunxi_wdt_ids,
	.ops = &sunxi_wdt_ops,
};
