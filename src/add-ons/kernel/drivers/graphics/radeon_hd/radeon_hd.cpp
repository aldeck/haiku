/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Alexander von Gluck, kallisti5@unixzen.com
 */


#include "radeon_hd.h"

#include "AreaKeeper.h"
#include "driver.h"
#include "utility.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <boot_item.h>
#include <driver_settings.h>
#include <util/kernel_cpp.h>
#include <vm/vm.h>


#define TRACE_DEVICE
#ifdef TRACE_DEVICE
#	define TRACE(x...) dprintf("radeon_hd: " x)
#else
#	define TRACE(x) ;
#endif


//	#pragma mark -


#define RHD_FB_BAR   0
#define RHD_MMIO_BAR 2


status_t
radeon_hd_init(radeon_info &info)
{
	TRACE("card(%ld): %s: called\n", info.id, __func__);

	// memory mapped I/O
	AreaKeeper sharedCreator;
	info.shared_area = sharedCreator.Create("radeon hd shared info",
		(void **)&info.shared_info, B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(sizeof(radeon_shared_info)), B_FULL_LOCK, 0);
	if (info.shared_area < B_OK) {
		return info.shared_area;
	}

	memset((void *)info.shared_info, 0, sizeof(radeon_shared_info));

	// R6xx_R7xx_3D.pdf, 5.3.3.1 SET_CONFIG_REG
	AreaKeeper mmioMapper;
	info.registers_area = mmioMapper.Map("radeon hd mmio",
		(void *)info.pci->u.h0.base_registers[RHD_MMIO_BAR],
		info.pci->u.h0.base_register_sizes[RHD_MMIO_BAR],
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&info.registers);
	if (mmioMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": card (%ld): could not map memory I/O!\n",
			info.id);
		return info.registers_area;
	}

	AreaKeeper frambufferMapper;
	info.framebuffer_area = frambufferMapper.Map("radeon hd framebuffer",
		(void *)info.pci->u.h0.base_registers[RHD_FB_BAR],
		info.pci->u.h0.base_register_sizes[RHD_FB_BAR],
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA,
		(void **)&info.shared_info->graphics_memory);
	if (frambufferMapper.InitCheck() < B_OK) {
		dprintf(DEVICE_NAME ": card(%ld): could not map framebuffer!\n",
			info.id);
		return info.framebuffer_area;
	}

	// Turn on write combining for the area
	vm_set_area_memory_type(info.framebuffer_area,
		info.pci->u.h0.base_registers[RHD_FB_BAR], B_MTR_WC);

	sharedCreator.Detach();
	mmioMapper.Detach();
	frambufferMapper.Detach();

	info.shared_info->registers_area = info.registers_area;
	info.shared_info->frame_buffer_offset = 0;
	info.shared_info->physical_graphics_memory
		= info.pci->u.h0.base_registers[RHD_FB_BAR];

	// Pull active monitor VESA EDID from boot loader
	edid1_info* edidInfo = (edid1_info*)get_boot_item(EDID_BOOT_INFO,
		NULL);
	if (edidInfo != NULL) {
		TRACE("card(%ld): %s found BIOS EDID information.\n", info.id,
			__func__);
		info.shared_info->has_edid = true;
		memcpy(&info.shared_info->edid_info, edidInfo, sizeof(edid1_info));
	} else {
		TRACE("card(%ld): %s didn't find BIOS EDID modes.\n", info.id,
			__func__);
		info.shared_info->has_edid = false;
	}

	// Populate graphics_memory/aperture_size with KB
	if (info.shared_info->device_chipset >= RADEON_R800) {
		// R800+ has memory stored in MB
		info.shared_info->graphics_memory_size
			= read32(info.registers + R6XX_CONFIG_MEMSIZE) << 10;
		info.shared_info->graphics_aperture_size
			= read32(info.registers + R6XX_CONFIG_APER_SIZE) << 10;
	} else {
		// R600-R700 has memory stored in bytes
		info.shared_info->graphics_memory_size
			= read32(info.registers + R6XX_CONFIG_MEMSIZE) >> 10;
		info.shared_info->graphics_aperture_size
			= read32(info.registers + R6XX_CONFIG_APER_SIZE) >> 10;
	}

	TRACE("card(%ld): found %ld MB memory on card.\n", info.id,
		info.shared_info->graphics_memory_size >> 10);
	TRACE("card(%ld): found %ld MB aperture on card.\n", info.id,
		info.shared_info->graphics_aperture_size >> 10);

	// if there are more then 512MB memory on the card, only map
	// the aperture size to prevent overflow of gart
	if (info.shared_info->graphics_memory_size > 524288)
		info.shared_info->graphics_memory_size
			= info.shared_info->graphics_aperture_size;

	TRACE("card(%ld): %s completed successfully!\n", info.id, __func__);
	return B_OK;
}


void
radeon_hd_uninit(radeon_info &info)
{
	TRACE("card(%ld): %s called\n", info.id, __func__);

	delete_area(info.shared_area);
	delete_area(info.registers_area);
	delete_area(info.framebuffer_area);
}

