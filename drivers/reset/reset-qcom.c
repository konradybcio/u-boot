// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 Sartura Ltd.
 * Copyright (c) 2022 Linaro Ltd.
 *
 * Author: Robert Marko <robert.marko@sartura.hr>
 *         Sumit Garg <sumit.garg@linaro.org>
 *
 * Based on Linux driver
 */

#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <reset-uclass.h>
#include <linux/bitops.h>
#include <malloc.h>

#include <clk/qcom.h>

#ifdef CONFIG_TARGET_QRB4210RB2
#include <dt-bindings/clock/qcom,gcc-sm6115.h>
static const struct qcom_reset_map gcc_qcom_resets[] = {
	[GCC_QUSB2PHY_PRIM_BCR] = { 0x1c000 },
	[GCC_QUSB2PHY_SEC_BCR] = { 0x1c004 },
	[GCC_SDCC1_BCR] = { 0x38000 },
	[GCC_SDCC2_BCR] = { 0x1e000 },
	[GCC_UFS_PHY_BCR] = { 0x45000 },
	[GCC_USB30_PRIM_BCR] = { 0x1a000 },
	[GCC_USB_PHY_CFG_AHB2PHY_BCR] = { 0x1d000 },
	[GCC_USB3PHY_PHY_PRIM_SP0_BCR] = { 0x1b008 },
	[GCC_USB3_PHY_PRIM_SP0_BCR] = { 0x1b000 },
	[GCC_VCODEC0_BCR] = { 0x58094 },
	[GCC_VENUS_BCR] = { 0x58078 },
	[GCC_VIDEO_INTERFACE_BCR] = { 0x6e000 },
};
#endif

static int qcom_reset_assert(struct reset_ctl *rst)
{
	struct udevice *cdev = (struct udevice *)dev_get_driver_data(rst->dev);
	struct qcom_cc_priv *priv = dev_get_priv(cdev);
	const struct qcom_reset_map *map;
	u32 value;

	map = &priv->data->resets[rst->id];

	value = readl(priv->base + map->reg);
	value |= BIT(map->bit);
	writel(value, priv->base + map->reg);

	return 0;
}

static int qcom_reset_deassert(struct reset_ctl *rst)
{
	struct udevice *cdev = (struct udevice *)dev_get_driver_data(rst->dev);
	struct qcom_cc_priv *priv = dev_get_priv(cdev);
	const struct qcom_reset_map *map;
	u32 value;

	map = &priv->data->resets[rst->id];

	value = readl(priv->base + map->reg);
	value &= ~BIT(map->bit);
	writel(value, priv->base + map->reg);

	return 0;
}

static const struct reset_ops qcom_reset_ops = {
	.rst_assert = qcom_reset_assert,
	.rst_deassert = qcom_reset_deassert,
};

U_BOOT_DRIVER(qcom_reset) = {
	.name = "qcom_reset",
	.id = UCLASS_RESET,
	.ops = &qcom_reset_ops,
};
