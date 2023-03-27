// SPDX-License-Identifier: BSD-3-Clause
/*
 * Clock drivers for Qualcomm APQ8016, APQ8096
 *
 * (C) Copyright 2015 Mateusz Kulikowski <mateusz.kulikowski@gmail.com>
 *
 * Based on Little Kernel driver, simplified
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/bitops.h>

#include <clk/qcom.h>

/* CBCR register fields */
#define CBCR_BRANCH_ENABLE_BIT  BIT(0)
#define CBCR_BRANCH_OFF_BIT     BIT(31)

extern ulong msm_set_rate(struct clk *clk, ulong rate);
extern int msm_enable(struct clk *clk);

/* Enable clock controlled by CBC soft macro */
void clk_enable_cbc(phys_addr_t cbcr)
{
	setbits_le32(cbcr, CBCR_BRANCH_ENABLE_BIT);

	while (readl(cbcr) & CBCR_BRANCH_OFF_BIT)
		;
}

void clk_enable_gpll0(phys_addr_t base, const struct pll_vote_clk *gpll0)
{
	if (readl(base + gpll0->status) & gpll0->status_bit)
		return; /* clock already enabled */

	setbits_le32(base + gpll0->ena_vote, gpll0->vote_bit);

	while ((readl(base + gpll0->status) & gpll0->status_bit) == 0)
		;
}

#define BRANCH_ON_VAL (0)
#define BRANCH_NOC_FSM_ON_VAL BIT(29)
#define BRANCH_CHECK_MASK GENMASK(31, 28)

void clk_enable_vote_clk(phys_addr_t base, const struct vote_clk *vclk)
{
	u32 val;

	setbits_le32(base + vclk->ena_vote, vclk->vote_bit);
	do {
		val = readl(base + vclk->cbcr_reg);
		val &= BRANCH_CHECK_MASK;
	} while ((val != BRANCH_ON_VAL) && (val != BRANCH_NOC_FSM_ON_VAL));
}

#define APPS_CMD_RCGR_UPDATE BIT(0)

/* Update clock command via CMD_RCGR */
void clk_bcr_update(phys_addr_t apps_cmd_rcgr)
{
	setbits_le32(apps_cmd_rcgr, APPS_CMD_RCGR_UPDATE);

	/* Wait for frequency to be updated. */
	while (readl(apps_cmd_rcgr) & APPS_CMD_RCGR_UPDATE)
		;
}

#define CFG_MODE_DUAL_EDGE (0x2 << 12) /* Counter mode */

#define CFG_MASK 0x3FFF

#define CFG_DIVIDER_MASK 0x1F

/* root set rate for clocks with half integer and MND divider */
void clk_rcg_set_rate_mnd(phys_addr_t base, const struct bcr_regs *regs,
			  int div, int m, int n, int source)
{
	u32 cfg;
	/* M value for MND divider. */
	u32 m_val = m;
	/* NOT(N-M) value for MND divider. */
	u32 n_val = ~((n) - (m)) * !!(n);
	/* NOT 2D value for MND divider. */
	u32 d_val = ~(n);

	/* Program MND values */
	writel(m_val, base + regs->M);
	writel(n_val, base + regs->N);
	writel(d_val, base + regs->D);

	/* setup src select and divider */
	cfg  = readl(base + regs->cfg_rcgr);
	cfg &= ~CFG_MASK;
	cfg |= source & CFG_CLK_SRC_MASK; /* Select clock source */

	/* Set the divider; HW permits fraction dividers (+0.5), but
	   for simplicity, we will support integers only */
	if (div)
		cfg |= (2 * div - 1) & CFG_DIVIDER_MASK;

	if (n_val)
		cfg |= CFG_MODE_DUAL_EDGE;

	writel(cfg, base + regs->cfg_rcgr); /* Write new clock configuration */

	/* Inform h/w to start using the new config. */
	clk_bcr_update(base + regs->cmd_rcgr);
}

/* root set rate for clocks with half integer and mnd_width=0 */
void clk_rcg_set_rate(phys_addr_t base, const struct bcr_regs *regs, int div,
		      int source)
{
	u32 cfg;

	/* setup src select and divider */
	cfg  = readl(base + regs->cfg_rcgr);
	cfg &= ~CFG_MASK;
	cfg |= source & CFG_CLK_SRC_MASK; /* Select clock source */

	/*
	 * Set the divider; HW permits fraction dividers (+0.5), but
	 * for simplicity, we will support integers only
	 */
	if (div)
		cfg |= (2 * div - 1) & CFG_DIVIDER_MASK;

	writel(cfg, base + regs->cfg_rcgr); /* Write new clock configuration */

	/* Inform h/w to start using the new config. */
	clk_bcr_update(base + regs->cmd_rcgr);
}

static int msm_clk_probe(struct udevice *dev)
{
	struct qcom_cc_data *data = (struct qcom_cc_data *)dev_get_driver_data(dev);
	struct qcom_cc_priv *priv = dev_get_priv(dev);

	priv->base = dev_read_addr(dev);
	if (priv->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	priv->data = data;

	return 0;
}

static ulong msm_clk_set_rate(struct clk *clk, ulong rate)
{
	return msm_set_rate(clk, rate);
}

static int msm_clk_enable(struct clk *clk)
{
	return msm_enable(clk);
}

static struct clk_ops msm_clk_ops = {
	.set_rate = msm_clk_set_rate,
	.enable = msm_clk_enable,
};

U_BOOT_DRIVER(qcom_clk) = {
	.name		= "qcom_clk",
	.id		= UCLASS_CLK,
	.ops		= &msm_clk_ops,
	.priv_auto	= sizeof(struct qcom_cc_priv),
	.probe		= msm_clk_probe,
	.flags		= DM_FLAG_PRE_RELOC,
};

int qcom_cc_bind(struct udevice *parent)
{
	struct qcom_cc_data *data = (struct qcom_cc_data *)dev_get_driver_data(parent);
	struct udevice *clkdev, *rstdev;
	struct driver *drv;
	int ret;

	/* Get a handle to the common clk handler */
	drv = lists_driver_lookup_name("qcom_clk");
	if (!drv)
		return -ENOENT;

	/* Register the clock controller */
	ret = device_bind_with_driver_data(parent, drv, "qcom_clk", (ulong)data,
					   dev_ofnode(parent), &clkdev);
	if (ret)
		return ret;

	/* Bail out early if resets are not specified for this platform */
	if (!data->resets)
		return ret;

	/* Get a handle to the common reset handler */
	drv = lists_driver_lookup_name("qcom_reset");
	if (!drv)
		return -ENOENT;

	/* Register the reset controller */
	ret = device_bind_with_driver_data(parent, drv, "qcom_reset", (ulong)clkdev,
					   dev_ofnode(parent), &rstdev);
	if (ret)
		device_unbind(clkdev);

	return ret;
}
