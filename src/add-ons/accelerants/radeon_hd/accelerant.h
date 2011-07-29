/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef RADEON_HD_ACCELERANT_H
#define RADEON_HD_ACCELERANT_H


#include "mode.h"
#include "radeon_hd.h"
#include "pll.h"
#include "dac.h"
#include "tmds.h"
#include "lvds.h"


#include <edid.h>


#define MAX_DISPLAY 2
	// Maximum displays (more then two requires AtomBIOS)


struct accelerant_info {
	vuint8			*regs;
	area_id			regs_area;

	radeon_shared_info *shared_info;
	area_id			shared_info_area;

	display_mode	*mode_list;		// cloned list of standard display modes
	area_id			mode_list_area;

	edid1_info		edid_info;
	bool			has_edid;

	int				device;
	bool			is_clone;

	// LVDS panel mode passed from the bios/startup.
	display_mode	lvds_panel_mode;
};


struct register_info {
	uint16	vgaControl;
	uint16	grphEnable;
	uint16	grphUpdate;
	uint16	grphControl;
	uint16	grphSwapControl;
	uint16	grphPrimarySurfaceAddr;
	uint16	grphPrimarySurfaceAddrHigh;
	uint16	grphSecondarySurfaceAddr;
	uint16	grphSecondarySurfaceAddrHigh;
	uint16	grphPitch;
	uint16	grphSurfaceOffsetX;
	uint16	grphSurfaceOffsetY;
	uint16	grphXStart;
	uint16	grphYStart;
	uint16	grphXEnd;
	uint16	grphYEnd;
	uint16	crtControl;
	uint16	crtCountControl;
	uint16	crtInterlace;
	uint16	crtHPolarity;
	uint16	crtVPolarity;
	uint16	crtHSync;
	uint16	crtVSync;
	uint16	crtHBlank;
	uint16	crtVBlank;
	uint16	crtHTotal;
	uint16	crtVTotal;
	uint16	modeDesktopHeight;
	uint16	modeDataFormat;
	uint16	modeCenter;
	uint16	viewportStart;
	uint16	viewportSize;
	uint16	sclUpdate;
	uint16	sclEnable;
	uint16	sclTapControl;
};


typedef struct {
	bool			active;
	uint32			connection_type;
	uint8			connection_id;
	register_info	*regs;
	bool			found_ranges;
	uint32			vfreq_max;
	uint32			vfreq_min;
	uint32			hfreq_max;
	uint32			hfreq_min;
} display_info;


// display_info connection_type
#define CONNECTION_DAC			0x0001
#define CONNECTION_TMDS			0x0002
#define CONNECTION_LVDS			0x0004

// register MMIO modes
#define OUT 0x1	// direct MMIO calls
#define CRT 0x2	// crt controler calls
#define VGA 0x3 // vga calls
#define PLL 0x4 // PLL calls
#define MC	0x5 // Memory Controler calls


extern accelerant_info *gInfo;
extern display_info *gDisplay[MAX_DISPLAY];


// register access

inline uint32
_read32(uint32 offset)
{
	return *(volatile uint32 *)(gInfo->regs + offset);
}


inline void
_write32(uint32 offset, uint32 value)
{
	*(volatile uint32 *)(gInfo->regs + offset) = value;
}


inline uint32
_read32PLL(uint16 offset)
{
	_write32(CLOCK_CNTL_INDEX, offset & PLL_ADDR);
	return _read32(CLOCK_CNTL_DATA);
}


inline void
_write32PLL(uint16 offset, uint32 data)
{
	_write32(CLOCK_CNTL_INDEX, (offset & PLL_ADDR) | PLL_WR_EN);
	_write32(CLOCK_CNTL_DATA, data);
}


inline uint32
Read32(uint32 subsystem, uint32 offset)
{
	switch (subsystem) {
		default:
		case OUT:
		case VGA:
		case MC:
			return _read32(offset);
		case CRT:
			return _read32(offset);
		case PLL:
			return _read32(offset);
			//return _read32PLL(offset);
	};
}


inline void
Write32(uint32 subsystem, uint32 offset, uint32 value)
{
	switch (subsystem) {
		default:
		case OUT:
		case VGA:
		case MC:
			_write32(offset, value);
			return;
		case CRT:
			_write32(offset, value);
			return;
		case PLL:
			_write32(offset, value);
			//_write32PLL(offset, value);
			return;
	};
}


inline void
Write32Mask(uint32 subsystem, uint32 offset, uint32 value, uint32 mask)
{
	uint32 temp;
	switch (subsystem) {
		default:
		case OUT:
		case VGA:
		case MC:
			temp = _read32(offset);
			break;
		case CRT:
			temp = _read32(offset);
			break;
		case PLL:
			temp = _read32(offset);
			//temp = _read32PLL(offset);
			break;
	};

	// only effect mask
	temp &= ~mask;
	temp |= value & mask;

	switch (subsystem) {
		default:
		case OUT:
		case VGA:
		case MC:
			_write32(offset, temp);
			return;
		case CRT:
			_write32(offset, temp);
			return;
		case PLL:
			_write32(offset, temp);
			//_write32PLL(offset, temp);
			return;
	};
}


#endif	/* RADEON_HD_ACCELERANT_H */
