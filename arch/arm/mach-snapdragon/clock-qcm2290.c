// SPDX-License-Identifier: BSD-3-Clause
/*
 * Clock drivers for Qualcomm QCM2290
 *
 * (C) Copyright 2023 Bhupesh Sharma <bhupesh.sharma@linaro.org>
 *
 * Based on Kernel driver, simplified
 */

#include <common.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/delay.h>

#include <clk/qcom.h>
#include <dt-bindings/clock/qcom,gcc-qcm2290.h>

ulong msm_set_rate(struct clk *clk, ulong rate)
{
	return 0;
}

int msm_enable(struct clk *clk)
{
	return 0;
}

static const struct qcom_reset_map qcm2290_gcc_resets[] = {
	[GCC_CAMSS_OPE_BCR] = { 0x55000 },
	[GCC_CAMSS_TFE_BCR] = { 0x52000 },
	[GCC_CAMSS_TOP_BCR] = { 0x58000 },
	[GCC_GPU_BCR] = { 0x36000 },
	[GCC_MMSS_BCR] = { 0x17000 },
	[GCC_PDM_BCR] = { 0x20000 },
	[GCC_QUPV3_WRAPPER_0_BCR] = { 0x1f000 },
	[GCC_QUSB2PHY_PRIM_BCR] = { 0x1c000 },
	[GCC_SDCC1_BCR] = { 0x38000 },
	[GCC_SDCC2_BCR] = { 0x1e000 },
	[GCC_USB30_PRIM_BCR] = { 0x1a000 },
	[GCC_USB3_PHY_PRIM_SP0_BCR] = { 0x1b000 },
	[GCC_USB3PHY_PHY_PRIM_SP0_BCR] = { 0x1b008 },
	[GCC_USB_PHY_CFG_AHB2PHY_BCR] = { 0x1d000 },
	[GCC_VCODEC0_BCR] = { 0x58094 },
	[GCC_VENUS_BCR] = { 0x58078 },
	[GCC_VIDEO_INTERFACE_BCR] = { 0x6e000 },
};

static const struct qcom_cc_data qcm2290_gcc_data = {
	.resets = qcm2290_gcc_resets,
	.num_resets = ARRAY_SIZE(qcm2290_gcc_resets),
};

static const struct udevice_id gcc_qcm2290_of_match[] = {
	{
		.compatible = "qcom,gcc-qcm2290",
		.data = (ulong)&qcm2290_gcc_data
	},
	{ }
};

U_BOOT_DRIVER(gcc_qcm2290) ={
	.name		= "gcc_qcm2290",
	.id		= UCLASS_NOP,
	.of_match	= gcc_qcm2290_of_match,
	.bind		= qcom_cc_bind,
	.flags		= DM_FLAG_PRE_RELOC,
};
