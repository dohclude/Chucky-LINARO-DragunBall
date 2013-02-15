/*
 * Copyright 2004 ATI Technologies Inc., Markham, Ontario
 * Copyright 2007-8 Advanced Micro Devices, Inc.
 * Copyright 2008 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 *          Alex Deucher
 */
#include "drmP.h"
#include "radeon_drm.h"
#include "radeon.h"
#include "atom.h"

#ifdef CONFIG_PPC_PMAC
/* not sure which of these are needed */
#include <asm/machdep.h>
#include <asm/pmac_feature.h>
#include <asm/prom.h>
#include <asm/pci-bridge.h>
#endif /* CONFIG_PPC_PMAC */

/* from radeon_encoder.c */
extern uint32_t
radeon_get_encoder_enum(struct drm_device *dev, uint32_t supported_device,
			uint8_t dac);
extern void radeon_link_encoder_connector(struct drm_device *dev);

/* from radeon_connector.c */
extern void
radeon_add_legacy_connector(struct drm_device *dev,
			    uint32_t connector_id,
			    uint32_t supported_device,
			    int connector_type,
			    struct radeon_i2c_bus_rec *i2c_bus,
			    uint16_t connector_object_id,
			    struct radeon_hpd *hpd);

/* from radeon_legacy_encoder.c */
extern void
radeon_add_legacy_encoder(struct drm_device *dev, uint32_t encoder_enum,
			  uint32_t supported_device);

/* old legacy ATI BIOS routines */

/* COMBIOS table offsets */
enum radeon_combios_table_offset {
	/* absolute offset tables */
	COMBIOS_ASIC_INIT_1_TABLE,
	COMBIOS_BIOS_SUPPORT_TABLE,
	COMBIOS_DAC_PROGRAMMING_TABLE,
	COMBIOS_MAX_COLOR_DEPTH_TABLE,
	COMBIOS_CRTC_INFO_TABLE,
	COMBIOS_PLL_INFO_TABLE,
	COMBIOS_TV_INFO_TABLE,
	COMBIOS_DFP_INFO_TABLE,
	COMBIOS_HW_CONFIG_INFO_TABLE,
	COMBIOS_MULTIMEDIA_INFO_TABLE,
	COMBIOS_TV_STD_PATCH_TABLE,
	COMBIOS_LCD_INFO_TABLE,
	COMBIOS_MOBILE_INFO_TABLE,
	COMBIOS_PLL_INIT_TABLE,
	COMBIOS_MEM_CONFIG_TABLE,
	COMBIOS_SAVE_MASK_TABLE,
	COMBIOS_HARDCODED_EDID_TABLE,
	COMBIOS_ASIC_INIT_2_TABLE,
	COMBIOS_CONNECTOR_INFO_TABLE,
	COMBIOS_DYN_CLK_1_TABLE,
	COMBIOS_RESERVED_MEM_TABLE,
	COMBIOS_EXT_TMDS_INFO_TABLE,
	COMBIOS_MEM_CLK_INFO_TABLE,
	COMBIOS_EXT_DAC_INFO_TABLE,
	COMBIOS_MISC_INFO_TABLE,
	COMBIOS_CRT_INFO_TABLE,
	COMBIOS_INTEGRATED_SYSTEM_INFO_TABLE,
	COMBIOS_COMPONENT_VIDEO_INFO_TABLE,
	COMBIOS_FAN_SPEED_INFO_TABLE,
	COMBIOS_OVERDRIVE_INFO_TABLE,
	COMBIOS_OEM_INFO_TABLE,
	COMBIOS_DYN_CLK_2_TABLE,
	COMBIOS_POWER_CONNECTOR_INFO_TABLE,
	COMBIOS_I2C_INFO_TABLE,
	/* relative offset tables */
	COMBIOS_ASIC_INIT_3_TABLE,	/* offset from misc info */
	COMBIOS_ASIC_INIT_4_TABLE,	/* offset from misc info */
	COMBIOS_DETECTED_MEM_TABLE,	/* offset from misc info */
	COMBIOS_ASIC_INIT_5_TABLE,	/* offset from misc info */
	COMBIOS_RAM_RESET_TABLE,	/* offset from mem config */
	COMBIOS_POWERPLAY_INFO_TABLE,	/* offset from mobile info */
	COMBIOS_GPIO_INFO_TABLE,	/* offset from mobile info */
	COMBIOS_LCD_DDC_INFO_TABLE,	/* offset from mobile info */
	COMBIOS_TMDS_POWER_TABLE,	/* offset from mobile info */
	COMBIOS_TMDS_POWER_ON_TABLE,	/* offset from tmds power */
	COMBIOS_TMDS_POWER_OFF_TABLE,	/* offset from tmds power */
};

enum radeon_combios_ddc {
	DDC_NONE_DETECTED,
	DDC_MONID,
	DDC_DVI,
	DDC_VGA,
	DDC_CRT2,
	DDC_LCD,
	DDC_GPIO,
};

enum radeon_combios_connector {
	CONNECTOR_NONE_LEGACY,
	CONNECTOR_PROPRIETARY_LEGACY,
	CONNECTOR_CRT_LEGACY,
	CONNECTOR_DVI_I_LEGACY,
	CONNECTOR_DVI_D_LEGACY,
	CONNECTOR_CTV_LEGACY,
	CONNECTOR_STV_LEGACY,
	CONNECTOR_UNSUPPORTED_LEGACY
};

const int legacy_connector_convert[] = {
	DRM_MODE_CONNECTOR_Unknown,
	DRM_MODE_CONNECTOR_DVID,
	DRM_MODE_CONNECTOR_VGA,
	DRM_MODE_CONNECTOR_DVII,
	DRM_MODE_CONNECTOR_DVID,
	DRM_MODE_CONNECTOR_Composite,
	DRM_MODE_CONNECTOR_SVIDEO,
	DRM_MODE_CONNECTOR_Unknown,
};

static uint16_t combios_get_table_offset(struct drm_device *dev,
					 enum radeon_combios_table_offset table)
{
	struct radeon_device *rdev = dev->dev_private;
	int rev;
	uint16_t offset = 0, check_offset;

	if (!rdev->bios)
		return 0;

	switch (table) {
		/* absolute offset tables */
	case COMBIOS_ASIC_INIT_1_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0xc);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_BIOS_SUPPORT_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x14);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_DAC_PROGRAMMING_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x2a);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MAX_COLOR_DEPTH_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x2c);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_CRTC_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x2e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_PLL_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x30);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_TV_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x32);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_DFP_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x34);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_HW_CONFIG_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x36);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MULTIMEDIA_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x38);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_TV_STD_PATCH_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x3e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_LCD_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x40);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MOBILE_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x42);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_PLL_INIT_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x46);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MEM_CONFIG_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x48);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_SAVE_MASK_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x4a);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_HARDCODED_EDID_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x4c);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_ASIC_INIT_2_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x4e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_CONNECTOR_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x50);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_DYN_CLK_1_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x52);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_RESERVED_MEM_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x54);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_EXT_TMDS_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x58);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MEM_CLK_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x5a);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_EXT_DAC_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x5c);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_MISC_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x5e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_CRT_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x60);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_INTEGRATED_SYSTEM_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x62);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_COMPONENT_VIDEO_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x64);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_FAN_SPEED_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x66);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_OVERDRIVE_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x68);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_OEM_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x6a);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_DYN_CLK_2_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x6c);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_POWER_CONNECTOR_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x6e);
		if (check_offset)
			offset = check_offset;
		break;
	case COMBIOS_I2C_INFO_TABLE:
		check_offset = RBIOS16(rdev->bios_header_start + 0x70);
		if (check_offset)
			offset = check_offset;
		break;
		/* relative offset tables */
	case COMBIOS_ASIC_INIT_3_TABLE:	/* offset from misc info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MISC_INFO_TABLE);
		if (check_offset) {
			rev = RBIOS8(check_offset);
			if (rev > 0) {
				check_offset = RBIOS16(check_offset + 0x3);
				if (check_offset)
					offset = check_offset;
			}
		}
		break;
	case COMBIOS_ASIC_INIT_4_TABLE:	/* offset from misc info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MISC_INFO_TABLE);
		if (check_offset) {
			rev = RBIOS8(check_offset);
			if (rev > 0) {
				check_offset = RBIOS16(check_offset + 0x5);
				if (check_offset)
					offset = check_offset;
			}
		}
		break;
	case COMBIOS_DETECTED_MEM_TABLE:	/* offset from misc info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MISC_INFO_TABLE);
		if (check_offset) {
			rev = RBIOS8(check_offset);
			if (rev > 0) {
				check_offset = RBIOS16(check_offset + 0x7);
				if (check_offset)
					offset = check_offset;
			}
		}
		break;
	case COMBIOS_ASIC_INIT_5_TABLE:	/* offset from misc info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MISC_INFO_TABLE);
		if (check_offset) {
			rev = RBIOS8(check_offset);
			if (rev == 2) {
				check_offset = RBIOS16(check_offset + 0x9);
				if (check_offset)
					offset = check_offset;
			}
		}
		break;
	case COMBIOS_RAM_RESET_TABLE:	/* offset from mem config */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MEM_CONFIG_TABLE);
		if (check_offset) {
			while (RBIOS8(check_offset++));
			check_offset += 2;
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_POWERPLAY_INFO_TABLE:	/* offset from mobile info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MOBILE_INFO_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x11);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_GPIO_INFO_TABLE:	/* offset from mobile info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MOBILE_INFO_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x13);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_LCD_DDC_INFO_TABLE:	/* offset from mobile info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MOBILE_INFO_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x15);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_TMDS_POWER_TABLE:	/* offset from mobile info */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_MOBILE_INFO_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x17);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_TMDS_POWER_ON_TABLE:	/* offset from tmds power */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_TMDS_POWER_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x2);
			if (check_offset)
				offset = check_offset;
		}
		break;
	case COMBIOS_TMDS_POWER_OFF_TABLE:	/* offset from tmds power */
		check_offset =
		    combios_get_table_offset(dev, COMBIOS_TMDS_POWER_TABLE);
		if (check_offset) {
			check_offset = RBIOS16(check_offset + 0x4);
			if (check_offset)
				offset = check_offset;
		}
		break;
	default:
		break;
	}

	return offset;

}

bool radeon_combios_check_hardcoded_edid(struct radeon_device *rdev)
{
	int edid_info, size;
	struct edid *edid;
	unsigned char *raw;
	edid_info = combios_get_table_offset(rdev->ddev, COMBIOS_HARDCODED_EDID_TABLE);
	if (!edid_info)
		return false;

	raw = rdev->bios + edid_info;
	size = EDID_LENGTH * (raw[0x7e] + 1);
	edid = kmalloc(size, GFP_KERNEL);
	if (edid == NULL)
		return false;

	memcpy((unsigned char *)edid, raw, size);

	if (!drm_edid_is_valid(edid)) {
		kfree(edid);
		return false;
	}

	rdev->mode_info.bios_hardcoded_edid = edid;
	rdev->mode_info.bios_hardcoded_edid_size = size;
	return true;
}

/* this is used for atom LCDs as well */
struct edid *
radeon_bios_get_hardcoded_edid(struct radeon_device *rdev)
{
	struct edid *edid;

	if (rdev->mode_info.bios_hardcoded_edid) {
		edid = kmalloc(rdev->mode_info.bios_hardcoded_edid_size, GFP_KERNEL);
		if (edid) {
			memcpy((unsigned char *)edid,
			       (unsigned char *)rdev->mode_info.bios_hardcoded_edid,
			       rdev->mode_info.bios_hardcoded_edid_size);
			return edid;
		}
	}
	return NULL;
}

static struct radeon_i2c_bus_rec combios_setup_i2c_bus(struct radeon_device *rdev,
						       enum radeon_combios_ddc ddc,
						       u32 clk_mask,
						       u32 data_mask)
{
	struct radeon_i2c_bus_rec i2c;
	int ddc_line = 0;

	/* ddc id            = mask reg
	 * DDC_NONE_DETECTED = none
	 * DDC_DVI           = RADEON_GPIO_DVI_DDC
	 * DDC_VGA           = RADEON_GPIO_VGA_DDC
	 * DDC_LCD           = RADEON_GPIOPAD_MASK
	 * DDC_GPIO          = RADEON_MDGPIO_MASK
	 * r1xx
	 * DDC_MONID         = RADEON_GPIO_MONID
	 * DDC_CRT2          = RADEON_GPIO_CRT2_DDC
	 * r200
	 * DDC_MONID         = RADEON_GPIO_MONID
	 * DDC_CRT2          = RADEON_GPIO_DVI_DDC
	 * r300/r350
	 * DDC_MONID         = RADEON_GPIO_DVI_DDC
	 * DDC_CRT2          = RADEON_GPIO_DVI_DDC
	 * rv2xx/rv3xx
	 * DDC_MONID         = RADEON_GPIO_MONID
	 * DDC_CRT2          = RADEON_GPIO_MONID
	 * rs3xx/rs4xx
	 * DDC_MONID         = RADEON_GPIOPAD_MASK
	 * DDC_CRT2          = RADEON_GPIO_MONID
	 */
	switch (ddc) {
	case DDC_NONE_DETECTED:
	default:
		ddc_line = 0;
		break;
	case DDC_DVI:
		ddc_line = RADEON_GPIO_DVI_DDC;
		break;
	case DDC_VGA:
		ddc_line = RADEON_GPIO_VGA_DDC;
		break;
	case DDC_LCD:
		ddc_line = RADEON_GPIOPAD_MASK;
		break;
	case DDC_GPIO:
		ddc_line = RADEON_MDGPIO_MASK;
		break;
	case DDC_MONID:
		if (rdev->family == CHIP_RS300 ||
		    rdev->family == CHIP_RS400 ||
		    rdev->family == CHIP_RS480)
			ddc_line = RADEON_GPIOPAD_MASK;
		else if (rdev->family == CHIP_R300 ||
			 rdev->family == CHIP_R350) {
			ddc_line = RADEON_GPIO_DVI_DDC;
			ddc = DDC_DVI;
		} else
			ddc_line = RADEON_GPIO_MONID;
		break;
	case DDC_CRT2:
		if (rdev->family == CHIP_R200 ||
		    rdev->family == CHIP_R300 ||
		    rdev->family == CHIP_R350) {
			ddc_line = RADEON_GPIO_DVI_DDC;
			ddc = DDC_DVI;
		} else if (rdev->family == CHIP_RS300 ||
			   rdev->family == CHIP_RS400 ||
			   rdev->family == CHIP_RS480)
			ddc_line = RADEON_GPIO_MONID;
		else if (rdev->family >= CHIP_RV350) {
			ddc_line = RADEON_GPIO_MONID;
			ddc = DDC_MONID;
		} else
			ddc_line = RADEON_GPIO_CRT2_DDC;
		break;
	}

	if (ddc_line == RADEON_GPIOPAD_MASK) {
		i2c.mask_clk_reg = RADEON_GPIOPAD_MASK;
		i2c.mask_data_reg = RADEON_GPIOPAD_MASK;
		i2c.a_clk_reg = RADEON_GPIOPAD_A;
		i2c.a_data_reg = RADEON_GPIOPAD_A;
		i2c.en_clk_reg = RADEON_GPIOPAD_EN;
		i2c.en_data_reg = RADEON_GPIOPAD_EN;
		i2c.y_clk_reg = RADEON_GPIOPAD_Y;
		i2c.y_data_reg = RADEON_GPIOPAD_Y;
	} else if (ddc_line == RADEON_MDGPIO_MASK) {
		i2c.mask_clk_reg = RADEON_MDGPIO_MASK;
		i2c.mask_data_reg = RADEON_MDGPIO_MASK;
		i2c.a_clk_reg = RADEON_MDGPIO_A;
		i2c.a_data_reg = RADEON_MDGPIO_A;
		i2c.en_clk_reg = RADEON_MDGPIO_EN;
		i2c.en_data_reg = RADEON_MDGPIO_EN;
		i2c.y_clk_reg = RADEON_MDGPIO_Y;
		i2c.y_data_reg = RADEON_MDGPIO_Y;
	} else {
		i2c.mask_clk_reg = ddc_line;
		i2c.mask_data_reg = ddc_line;
		i2c.a_clk_reg = ddc_line;
		i2c.a_data_reg = ddc_line;
		i2c.en_clk_reg = ddc_line;
		i2c.en_data_reg = ddc_line;
		i2c.y_clk_reg = ddc_line;
		i2c.y_data_reg = ddc_line;
	}

	if (clk_mask && data_mask) {
		/* system specific masks */
		i2c.mask_clk_mask = clk_mask;
		i2c.mask_data_mask = data_mask;
		i2c.a_clk_mask = clk_mask;
		i2c.a_data_mask = data_mask;
		i2c.en_clk_mask = clk_mask;
		i2c.en_data_mask = data_mask;
		i2c.y_clk_mask = clk_mask;
		i2c.y_data_mask = data_mask;
	} else if ((ddc_line == RADEON_GPIOPAD_MASK) ||
		   (ddc_line == RADEON_MDGPIO_MASK)) {
		/* default gpiopad masks */
		i2c.mask_clk_mask = (0x20 << 8);
		i2c.mask_data_mask = 0x80;
		i2c.a_clk_mask = (0x20 << 8);
		i2c.a_data_mask = 0x80;
		i2c.en_clk_mask = (0x20 << 8);
		i2c.en_data_mask = 0x80;
		i2c.y_clk_mask = (0x20 << 8);
		i2c.y_data_mask = 0x80;
	} else {
		/* default masks for ddc pads */
		i2c.mask_clk_mask = RADEON_GPIO_MASK_1;
		i2c.mask_data_mask = RADEON_GPIO_MASK_0;
		i2c.a_clk_mask = RADEON_GPIO_A_1;
		i2c.a_data_mask = RADEON_GPIO_A_0;
		i2c.en_clk_mask = RADEON_GPIO_EN_1;
		i2c.en_data_mask = RADEON_GPIO_EN_0;
		i2c.y_clk_mask = RADEON_GPIO_Y_1;
		i2c.y_data_mask = RADEON_GPIO_Y_0;
	}

	switch (rdev->family) {
	case CHIP_R100:
	case CHIP_RV100:
	case CHIP_RS100:
	case CHIP_RV200:
	case CHIP_RS200:
	case CHIP_RS300:
		switch (ddc_line) {
		case RADEON_GPIO_DVI_DDC:
			i2c.hw_capable = true;
			break;
		default:
			i2c.hw_capable = false;
			break;
		}
		break;
	case CHIP_R200:
		switch (ddc_line) {
		case RADEON_GPIO_DVI_DDC:
		case RADEON_GPIO_MONID:
			i2c.hw_capable = true;
			break;
		default:
			i2c.hw_capable = false;
			break;
		}
		break;
	case CHIP_RV250:
	case CHIP_RV280:
		switch (ddc_line) {
		case RADEON_GPIO_VGA_DDC:
		case RADEON_GPIO_DVI_DDC:
		case RADEON_GPIO_CRT2_DDC:
			i2c.hw_capable = true;
			break;
		default:
			i2c.hw_capable = false;
			break;
		}
		break;
	case CHIP_R300:
	case CHIP_R350:
		switch (ddc_line) {
		case RADEON_GPIO_VGA_DDC:
		case RADEON_GPIO_DVI_DDC:
			i2c.hw_capable = true;
			break;
		default:
			i2c.hw_capable = false;
			break;
		}
		break;
	case CHIP_RV350:
	case CHIP_RV380:
	case CHIP_RS400:
	case CHIP_RS480:
		switch (ddc_line) {
		case RADEON_GPIO_VGA_DDC:
		case RADEON_GPIO_DVI_DDC:
			i2c.hw_capable = true;
			break;
		case RADEON_GPIO_MONID:
			/* hw i2c on RADEON_GPIO_MONID doesn't seem to work
			 * reliably on some pre-r4xx hardware; not sure why.
			 */
			i2c.hw_capable = false;
			break;
		default:
			i2c.hw_capable = false;
			break;
		}
		break;
	default:
		i2c.hw_capable = false;
		break;
	}
	i2c.mm_i2c = false;

	i2c.i2c_id = ddc;
	i2c.hpd = RADEON_HPD_NONE;

	if (ddc_line)
		i2c.valid = true;
	else
		i2c.valid = false;

	return i2c;
}

void radeon_combios_i2c_init(struct radeon_device *rdev)
{
	struct drm_device *dev = rdev->ddev;
	struct radeon_i2c_bus_rec i2c;

	/* actual hw pads
	 * r1xx/rs2xx/rs3xx
	 * 0x60, 0x64, 0x68, 0x6c, gpiopads, mm
	 * r200
	 * 0x60, 0x64, 0x68, mm
	 * r300/r350
	 * 0x60, 0x64, mm
	 * rv2xx/rv3xx/rs4xx
	 * 0x60, 0x64, 0x68, gpiopads, mm
	 */

	/* 0x60 */
	i2c = combios_setup_i2c_bus(rdev, DDC_DVI, 0, 0);
	rdev->i2c_bus[0] = radeon_i2c_create(dev, &i2c, "DVI_DDC");
	/* 0x64 */
	i2c = combios_setup_i2c_bus(rdev, DDC_VGA, 0, 0);
	rdev->i2c_bus[1] = radeon_i2c_create(dev, &i2c, "VGA_DDC");

	/* mm i2c */
	i2c.valid = true;
	i2c.hw_capable = true;
	i2c.mm_i2c = true;
	i2c.i2c_id = 0xa0;
	rdev->i2c_bus[2] = radeon_i2c_create(dev, &i2c, "MM_I2C");

	if (rdev->family == CHIP_R300 ||
	    rdev->family == CHIP_R350) {
		/* only 2 sw i2c pads */
	} else if (rdev->family == CHIP_RS300 ||
		   rdev->family == CHIP_RS400 ||
		   rdev->family == CHIP_RS480) {
		u16 offset;
		u8 id, blocks, clk, data;
		int i;

		/* 0x68 */
		i2c = combios_setup_i2c_bus(rdev, DDC_CRT2, 0, 0);
		rdev->i2c_bus[3] = radeon_i2c_create(dev, &i2c, "MONID");

		offset = combios_get_table_offset(dev, COMBIOS_I2C_INFO_TABLE);
		if (offset) {
			blocks = RBIOS8(offset + 2);
			for (i = 0; i < blocks; i++) {
				id = RBIOS8(offset + 3 + (i * 5) + 0);
				if (id == 136) {
					clk = RBIOS8(offset + 3 + (i * 5) + 3);
					data = RBIOS8(offset + 3 + (i * 5) + 4);
					/* gpiopad */
					i2c = combios_setup_i2c_bus(rdev, DDC_MONID,
								    (1 << clk), (1 << data));
					rdev->i2c_bus[4] = radeon_i2c_create(dev, &i2c, "GPIOPAD_MASK");
					break;
				}
			}
		}
	} else if ((rdev->family == CHIP_R200) ||
		   (rdev->family >= CHIP_R300)) {
		/* 0x68 */
		i2c = combios_setup_i2c_bus(rdev, DDC_MONID, 0, 0);
		rdev->i2c_bus[3] = radeon_i2c_create(dev, &i2c, "MONID");
	} else {
		/* 0x68 */
		i2c = combios_setup_i2c_bus(rdev, DDC_MONID, 0, 0);
		rdev->i2c_bus[3] = radeon_i2c_create(dev, &i2c, "MONID");
		/* 0x6c */
		i2c = combios_setup_i2c_bus(rdev, DDC_CRT2, 0, 0);
		rdev->i2c_bus[4] = radeon_i2c_create(dev, &i2c, "CRT2_DDC");
	}
}

bool radeon_combios_get_clock_info(struct drm_device *dev)
{
	struct radeon_device *rdev = dev->dev_private;
	uint16_t pll_info;
	struct radeon_pll *p1pll = &rdev->clock.p1pll;
	struct radeon_pll *p2pll = &rdev->clock.p2pll;
	struct radeon_pll *spll = &rdev->clock.spll;
	struct radeon_pll *mpll = &rdev->clock.mpll;
	int8_t rev;
	uint16_t sclk, mclk;

	pll_info = combios_get_table_offset(dev, COMBIOS_PLL_INFO_TABLE);
	if (pll_info) {
		rev = RBIOS8(pll_info);

		/* pixel clocks */
		p1pll->reference_freq = RBIOS16(pll_info + 0xe);
		p1pll->reference_div = RBIOS16(pll_info + 0x10);
		p1pll->pll_out_min = RBIOS32(pll_info + 0x12);
		p1pll->pll_out_max = RBIOS32(pll_info + 0x16);
		p1pll->lcd_pll_out_min = p1pll->pll_out_min;
		p1pll->lcd_pll_out_max = p1pll->pll_out_max;

		if (rev > 9) {
			p1pll->pll_in_min = RBIOS32(pll_info + 0x36);
			p1pll->pll_in_max = RBIOS32(pll_info + 0x3a);
		} else {
			p1pll->pll_in_min = 40;
			p1pll->pll_in_max = 500;
		}
		*p2pll = *p1pll;

		/* system clock */
		spll->reference_freq = RBIOS16(pll_info + 0x1a);
		spll->reference_div = RBIOS16(pll_info + 0x1c);
		spll->pll_out_min = RBIOS32(pll_info + 0x1e);
		spll->pll_out_max = RBIOS32(pll_info + 0x22);

		if (rev > 10) {
			spll->pll_in_min = RBIOS32(pll_info + 0x48);
			spll->pll_in_max = RBIOS32(pll_info + 0x4c);
		} else {
			/* ??? */
			spll->pll_in_min = 40;
			spll->pll_in_max = 500;
		}

		/* memory clock */
		mpll->reference_freq = RBIOS16(pll_info + 0x26);
		mpll->reference_div = RBIOS16(pll_info + 0x28);
		mpll->pll_out_min = RBIOS32(pll_info + 0x2a);
		mpll->pll_out_max = RBIOS32(pll_info + 0x2e);

		if (rev > 10) {
			mpll->pll_in_min = RBIOS32(pll_info + 0x5a);
			mpll->pll_in_max = RBIOS32(pll_info + 0x5e);
		} else {
			/* ??? */
			mpll->pll_in_min = 40;
			mpll->pll_in_max = 500;
		}

		/* default sclk/mclk */
		sclk = RBIOS16(pll_info + 0xa);
		mclk = RBIOS16(pll_info + 0x8);
		if (sclk == 0)
			sclk = 200 * 100;
		if (mclk == 0)
			mclk = 200 * 100;

		rdev->clock.default_sclk = sclk;
		rdev->clock.default_mclk = mclk;

		if (RBIOS32(pll_info + 0x16))
			rdev->clock.max_pixel_clock = RBIOS32(pll_info + 0x16);
		else
			rdev->clock.max_pixel_clock = 35000; /* might need something asic specific */

		return true;
	}
	return false;
}

bool radeon_combios_sideport_present(struct radeon_device *rdev)
{
	struct drm_device *dev = rdev->ddev;
	u16 igp_info;

	/* sideport is AMD only */
	if (rdev->family == CHIP_RS400)
		return false;

	igp_info = combios_get_table_offset(dev, COMBIOS_INTEGRATED_SYSTEM_INFO_TABLE);

	if (igp_info) {
		if (RBIOS16(igp_info + 0x4))
			return true;
	}
	return false;
}

static const uint32_t default_primarydac_adj[CHIP_LAST] = {
	0x00000808,		/* r100  */
	0x00000808,		/* rv100 */
	0x00000808,		/* rs100 */
	0x00000808,		/* rv200 */
	0x00000808,		/* rs200 */
	0x00000808,		/* r200  */
	0x00000808,		/* rv250 */
	0x00000000,		/* rs300 */
	0x00000808,		/* rv280 */
	0x00000808,		/* r300  */
	0x00000808,		/* r350  */
	0x00000808,		/* rv350 */
	0x00000808,		/* rv380 */
	0x00000808,		/* r420  */
	0x00000808,		/* r423  */
	0x00000808,		/* rv410 */
	0x00000000,		/* rs400 */
	0x00000000,		/* rs480 */
};

static void radeon_legacy_get_primary_dac_info_from_table(struct radeon_device *rdev,
							  struct radeon_encoder_primary_dac *p_dac)
{
	p_dac->ps2_pdac_adj = default_primarydac_adj[rdev->family];
	return;
}

struct radeon_encoder_primary_dac *radeon_combios_get_primary_dac_info(struct
								       radeon_encoder
								       *encoder)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	uint16_t dac_info;
	uint8_t rev, bg, dac;
	struct radeon_encoder_primary_dac *p_dac = NULL;
	int found = 0;

	p_dac = kzalloc(sizeof(struct radeon_encoder_primary_dac),
			GFP_KERNEL);

	if (!p_dac)
		return NULL;

	/* check CRT table */
	dac_info = combios_get_table_offset(dev, COMBIOS_CRT_INFO_TABLE);
	if (dac_info) {
		rev = RBIOS8(dac_info) & 0x3;
		if (rev < 2) {
			bg = RBIOS8(dac_info + 0x2) & 0xf;
			dac = (RBIOS8(dac_info + 0x2) >> 4) & 0xf;
			p_dac->ps2_pdac_adj = (bg << 8) | (dac);
		} else {
			bg = RBIOS8(dac_info + 0x2) & 0xf;
			dac = RBIOS8(dac_info + 0x3) & 0xf;
			p_dac->ps2_pdac_adj = (bg << 8) | (dac);
		}
		/* if the values are all zeros, use the table */
		if (p_dac->ps2_pdac_adj)
			found = 1;
	}

	if (!found) /* fallback to defaults */
		radeon_legacy_get_primary_dac_info_from_table(rdev, p_dac);

	return p_dac;
}

enum radeon_tv_std
radeon_combios_get_tv_info(struct radeon_device *rdev)
{
	struct drm_device *dev = rdev->ddev;
	uint16_t tv_info;
	enum radeon_tv_std tv_std = TV_STD_NTSC;

	tv_info = combios_get_table_offset(dev, COMBIOS_TV_INFO_TABLE);
	if (tv_info) {
		if (RBIOS8(tv_info + 6) == 'T') {
			switch (RBIOS8(tv_info + 7) & 0xf) {
			case 1:
				tv_std = TV_STD_NTSC;
				DRM_DEBUG_KMS("Default TV standard: NTSC\n");
				break;
			case 2:
				tv_std = TV_STD_PAL;
				DRM_DEBUG_KMS("Default TV standard: PAL\n");
				break;
			case 3:
				tv_std = TV_STD_PAL_M;
				DRM_DEBUG_KMS("Default TV standard: PAL-M\n");
				break;
			case 4:
				tv_std = TV_STD_PAL_60;
				DRM_DEBUG_KMS("Default TV standard: PAL-60\n");
				break;
			case 5:
				tv_std = TV_STD_NTSC_J;
				DRM_DEBUG_KMS("Default TV standard: NTSC-J\n");
				break;
			case 6:
				tv_std = TV_STD_SCART_PAL;
				DRM_DEBUG_KMS("Default TV standard: SCART-PAL\n");
				break;
			default:
				tv_std = TV_STD_NTSC;
				DRM_DEBUG_KMS
				    ("Unknown TV standard; defaulting to NTSC\n");
				break;
			}

			switch ((RBIOS8(tv_info + 9) >> 2) & 0x3) {
			case 0:
				DRM_DEBUG_KMS("29.498928713 MHz TV ref clk\n");
				break;
			case 1:
				DRM_DEBUG_KMS("28.636360000 MHz TV ref clk\n");
				break;
			case 2:
				DRM_DEBUG_KMS("14.318180000 MHz TV ref clk\n");
				break;
			case 3:
				DRM_DEBUG_KMS("27.000000000 MHz TV ref clk\n");
				break;
			default:
				break;
			}
		}
	}
	return tv_std;
}

static const uint32_t default_tvdac_adj[CHIP_LAST] = {
	0x00000000,		/* r100  */
	0x00280000,		/* rv100 */
	0x00000000,		/* rs100 */
	0x00880000,		/* rv200 */
	0x00000000,		/* rs200 */
	0x00000000,		/* r200  */
	0x00770000,		/* rv250 */
	0x00290000,		/* rs300 */
	0x00560000,		/* rv280 */
	0x00780000,		/* r300  */
	0x00770000,		/* r350  */
	0x00780000,		/* rv350 */
	0x00780000,		/* rv380 */
	0x01080000,		/* r420  */
	0x01080000,		/* r423  */
	0x01080000,		/* rv410 */
	0x00780000,		/* rs400 */
	0x00780000,		/* rs480 */
};

static void radeon_legacy_get_tv_dac_info_from_table(struct radeon_device *rdev,
						     struct radeon_encoder_tv_dac *tv_dac)
{
	tv_dac->ps2_tvdac_adj = default_tvdac_adj[rdev->family];
	if ((rdev->flags & RADEON_IS_MOBILITY) && (rdev->family == CHIP_RV250))
		tv_dac->ps2_tvdac_adj = 0x00880000;
	tv_dac->pal_tvdac_adj = tv_dac->ps2_tvdac_adj;
	tv_dac->ntsc_tvdac_adj = tv_dac->ps2_tvdac_adj;
	return;
}

struct radeon_encoder_tv_dac *radeon_combios_get_tv_dac_info(struct
							     radeon_encoder
							     *encoder)
{
	struct drm_device *dev = encoder->base.dev;
	struct radeon_device *rdev = dev->dev_private;
	uint16_t dac_info;
	uint8_t rev, bg, dac;
	struct radeon_encoder_tv_dac *tv_dac = NULL;
	int found = 0;

	tv_dac = kzalloc(sizeof(struct radeon_encoder_tv_dac), GFP_KERNEL);
	if (!tv_dac)
		return NULL;

	/* first check TV table */
	dac_info = combios_get_table_offset(dev, COMBIOS_TV_INFO_TABLE);
	if (dac_info) {
		rev = RBIOS8(dac_info + 0x3);
		if (rev > 4) {
			bg = RBIOS8(dac_info + 0xc) & 0xf;
			dac = RBIOS8(dac_info + 0xd) & 0xf;
			tv_dac->ps2_tvdac_adj = (bg << 16) | (dac << 20);

			bg = RBIOS8(dac_info + 0xe) & 0xf;
			dac = RBIOS8(dac_info + 0xf) & 0xf;
			tv_dac->pal_tvdac_adj = (bg << 16) | (dac << 20);

			bg = RBIOS8(dac_info + 0x10) & 0xf;
			dac = RBIOS8(dac_info + 0x11) & 0xf;
			tv_dac->ntsc_tvdac_adj = (bg << 16) | (dac << 20);
			/* if the values are all zeros, use the table */
			if (tv_dac->ps2_tvdac_adj)
				found = 1;
		} else if (rev > 1) {
			bg = RBIOS8(dac_info + 0xc) & 0xf;
			dac = (RBIOS8(dac_info + 0xc) >> 4) & 0xf;
			tv_dac->ps2_tvdac_adj = (bg << 16) | (dac << 20);

			bg = RBIOS8(dac_info + 0xd) & 0xf;
			dac = (RBIOS8(dac_info + 0xd) >> 4) & 0xf;
			tv_dac->pal_tvdac_adj = (bg << 16) | (dac << 20);

			bg = RBIOS8(dac_info + 0xe) & 0xf;
			dac = (RBIOS8(dac_info + 0xe) >> 4) & 0xf;
			tv_dac->ntsc_tvdac_adj = (bg << 16) | (dac << 20);
			/* if the values are all zeros, use the table */
			if (tv_dac->ps2_tvdac_adj)
				found = 1;
		}
		tv_dac->tv_std = radeon_combios_get_tv_info(rdev);
	}
	if (!found) {
		/* then check CRT table */
		dac_info =
		    combios_get_table_offset(dev, COMBIOS_CRT_INFO_TABLE);
		if (dac_info) {
			rev = RBIOS8(dac_info) & 0x3;
			if (rev < 2) {
				bg = RBIOS8(dac_info + 0x3) & 0xf;
				dac = (RBIOS8(dac_info + 0x3) >> 4) & 0xf;
				tv_dac->ps2_tvdac_adj =
				    (bg << 16) | (dac << 20);
				tv_dac->pal_tvdac_adj = tv_dac->ps2_tvdac_adj;
				tv_dac->ntsc_tvdac_adj = tv_dac->ps2_tvdac_adj;
				/* if the values are all zeros, use the table */
				if (tv_dac->ps2_tvdac_adj)
					found = 1;
			} else {
				bg = RBIOS8(dac_info + 0x4) & 0xf;
				dac = RBIOS8(dac_info + 0x5) & 0xf;
				tv_dac->ps2_tvdac_adj =
				    (bg << 16) | (dac << 20);
				tv_dac->pal_tvdac_adj = tv_dac->ps2_tvdac_adj;
				tv_dac->ntsc_tvdac_adj = tv_dac->ps2_tvdac_adj;
				/* if the values are all zeros, use the table */
				if (tv_dac->ps2_tvdac_adj)
					found = 1;
			}
		} else {
			DRM_INFO("No TV DAC info found in BIOS\n");
		}
	}

	if (!found) /* fallback to defaults */
		radeon_legacy_get_tv_dac_info_from_table(rdev, tv_dac);

	return tv_dac;
}

static struct radeon_encoder_lvds *radeon_legacy_get_lvds_info_from_regs(struct
									 radeon_device
									 *rdev)
{
	struct radeon_encoder_lvds *lvds = NULL;
	uint32_t fp_vert_stretch, fp_horz_stretch;
	uint32_t ppll_div_sel, ppll_val;
	uint32_t lvds_ss_gen_cntl = RREG32(RADEON_LVDS_SS_GEN_CNTL);

	lvds = kzalloc(sizeof(struct radeon_encoder_lvds), GFP_KERNEL);

	if (!lvds)
		return NULL;

	fp_vert_stretch = RREG32(RADEON_FP_VERT_STRETCH);
	fp_horz_stretch = RREG32(RADEON_FP_HORZ_STRETCH);

	/* These should be fail-safe defaults, fingers crossed */
	lvds->panel_pwr_delay = 200;
	lvds->panel_vcc_delay = 2000;

	lvds->lvds_gen_cntl = RREG32(RADEON_LVDS_GEN_CNTL);
	lvds->panel_digon_delay = (lvds_ss_gen_cntl >> RADEON_LVDS_PWRSEQ_DELAY1_SHIFT) & 0xf;
	lvds->panel_blon_delay = (lvds_ss_gen_cntl >> RADEON_LVDS_PWRSEQ_DELAY2_SHIFT) & 0xf;

	if (fp_vert_stretch & RADEON_VERT_STRETCH_ENABLE)
		lvds->native_mode.vdisplay =
		    ((fp_vert_stretch & RADEON_VERT_PANEL_SIZE) >>
		     RADEON_VERT_PANEL_SHIFT) + 1;
	else
		lvds->native_mode.vdisplay =
		    (RREG32(RADEON_CRTC_V_TOTAL_DISP) >> 16) + 1;

	if (fp_horz_stretch & RADEON_HORZ_STRETCH_ENABLE)
		lvds->native_mode.hdisplay =
		    (((fp_horz_stretch & RADEON_HORZ_PANEL_SIZE) >>
		      RADEON_HORZ_PANEL_SHIFT) + 1) * 8;
	else
		lvds->native_mode.hdisplay =
		    ((RREG32(RADEON_CRTC_H_TOTAL_DISP) >> 16) + 1) * 8;

	if ((lvds->native_mode.hdisplay < 640) ||
	    (lvds->native_mode.vdisplay < 480)) {
		lvds->native_mode.hdisplay = 640;
		lvds->native_mode.vdisplay = 480;
	}

	ppll_div_sel = RREG8(RADEON_CLOCK_CNTL_INDEX + 1) & 0x3;
	ppll_val = RREG32_PLL(RADEON_PPLL_DIV_0 + ppll_div_sel);
	if ((ppll_val & 0x000707ff) == 0x1bb)
		lvds->use_bios_dividers = false;
	else {
		lvds->panel_ref_divider =
		    RREG32_PLL(RADEON_PPLL_REF_DIV) & 0x3ff;
		lvds->panel_post_divider = (ppll_val
