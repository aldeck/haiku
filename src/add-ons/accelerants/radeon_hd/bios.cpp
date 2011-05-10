/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include <Debug.h>

#include "bios.h"

#include "accelerant.h"
#include "accelerant_protos.h"


#undef TRACE

#define TRACE_ATOM
#ifdef TRACE_ATOM
#   define TRACE(x...) _sPrintf("radeon_hd: " x)
#else
#   define TRACE(x...) ;
#endif


status_t
AtomParser(void *parameterSpace, uint8_t index, void *handle, void *biosBase)
{
	DEVICE_DATA deviceData;

	deviceData.pParameterSpace = (UINT32*)parameterSpace;
	deviceData.CAIL = handle;
	deviceData.pBIOS_Image = (UINT8*)biosBase;
	deviceData.format = TABLE_FORMAT_BIOS;

	switch (ParseTable(&deviceData, index)) {
		case CD_SUCCESS:
			TRACE("%s: CD_SUCCESS : success\n", __func__);
			return B_OK;
			break;
		case CD_CALL_TABLE:
			TRACE("%s: CD_CALL_TABLE : success\n", __func__);
			return B_OK;
			break;
		case CD_COMPLETED:
			TRACE("%s: CD_COMPLETED : success\n", __func__);
			return B_OK;
			break;
		default:
			TRACE("%s: UNKNOWN ERROR\n", __func__);
	}
	return B_ERROR;
}


/*	Begin AtomBIOS OS callbacks
	These functions are used by AtomBios to access
	functions and data provided by the accelerant
*/
extern "C" {


VOID*
CailAllocateMemory(VOID *CAIL, UINT16 size)
{
	TRACE("AtomBios callback %s, size = %d\n", __func__, size);
	return malloc(size);
}


VOID
CailReleaseMemory(VOID *CAIL, VOID *addr)
{
	TRACE("AtomBios callback %s\n", __func__);
	free(addr);
}


VOID
CailDelayMicroSeconds(VOID *CAIL, UINT32 delay)
{
	usleep(delay);
}


UINT32
CailReadATIRegister(VOID* CAIL, UINT32 idx)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx << 2);
	return read32(idx << 2);
}


VOID
CailWriteATIRegister(VOID *CAIL, UINT32 idx, UINT32 data)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx << 2);

	// TODO : save MMIO via atomSaveRegisters in CailWriteATIRegister
	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterMMIO, idx << 2);
	write32(idx << 2, data);
}


VOID
CailReadPCIConfigData(VOID *CAIL, VOID* ret, UINT32 idx, UINT16 size)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx);
	// TODO : CailReadPCIConfigData

	// pci_device_cfg_read(RHDPTRI((atomBiosHandlePtr)CAIL)->PciInfo,
	//	ret, idx << 2 , size >> 3, NULL);
}


VOID
CailWritePCIConfigData(VOID *CAIL, VOID *src, UINT32 idx, UINT16 size)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx);
	// TODO : CailWritePCIConfigData

	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterPCICFG, idx << 2);
	// pci_device_cfg_write(RHDPTRI((atomBiosHandlePtr)CAIL)->PciInfo,
	//	src, idx << 2, size >> 3, NULL);
}


ULONG
CailReadPLL(VOID *CAIL, ULONG Address)
{
	TRACE("AtomBios callback %s, addr (0x%X)\n", __func__, Address);
	// TODO : Assumed screen index 0
	return ReadPLL(0, Address);
}


VOID
CailWritePLL(VOID *CAIL, ULONG Address, ULONG Data)
{
	TRACE("AtomBios callback %s, addr (0x%X)\n", __func__, Address);

	// TODO : save PLL registers
	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterPLL, Address);
	// TODO : Assumed screen index 0
	WritePLL(0, Address, Data);
}


ULONG
CailReadMC(VOID *CAIL, ULONG Address)
{
	TRACE("AtomBios callback %s, addr (0x%X)\n", __func__, Address);
	// TODO : CailReadMC

	ULONG ret = 0;

	// ret = RHDReadMC(((atomBiosHandlePtr)CAIL), Address | MC_IND_ALL);
	return ret;
}


VOID
CailWriteMC(VOID *CAIL, ULONG Address, ULONG data)
{
	TRACE("AtomBios callback %s, addr (0x%X)\n", __func__, Address);
	// TODO : CailWriteMC

	// atomSaveRegisters((atomBiosHandlePtr)CAIL, atomRegisterMC, Address);
	// RHDWriteMC(((atomBiosHandlePtr)CAIL),
	//	Address | MC_IND_ALL | MC_IND_WR_EN, data);
}


UINT32
CailReadFBData(VOID* CAIL, UINT32 idx)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx);
	// TODO : CailReadFBData

	UINT32 ret = 0;
	/*
	if (((atomBiosHandlePtr)CAIL)->fbBase) {
		CARD8 *FBBase = (CARD8*)
			RHDPTRI((atomBiosHandlePtr)CAIL)->FbBase;
		ret = *((CARD32*)(FBBase + (((atomBiosHandlePtr)CAIL)->fbBase)
			+ idx));
		TRACE(("%s(%x) = %x\n", __func__, idx, ret));
	} else if (((atomBiosHandlePtr)CAIL)->scratchBase) {
		ret = *(CARD32*)((CARD8*)(((atomBiosHandlePtr)CAIL)->scratchBase)
			+ idx);
		TRACE(("%s(%x) = %x\n", __func__, idx, ret));
	} else {
		TRACE(("%s: no fbbase set\n", __func__));
		return 0;
	}
	*/
	return ret;
}


VOID
CailWriteFBData(VOID *CAIL, UINT32 idx, UINT32 data)
{
	TRACE("AtomBios callback %s, idx (0x%X)\n", __func__, idx);
	// TODO : CailReadFBData

	/*
	if (((atomBiosHandlePtr)CAIL)->fbBase) {
		CARD8 *FBBase = (CARD8*)
			RHDPTRI((atomBiosHandlePtr)CAIL)->FbBase;
		*((CARD32*)(FBBase + (((atomBiosHandlePtr)CAIL)->fbBase) + idx)) = data;
	} else if (((atomBiosHandlePtr)CAIL)->scratchBase) {
		*(CARD32*)((CARD8*)(((atomBiosHandlePtr)CAIL)->scratchBase) + idx) = data;
	} else
		TRACE(("%s: no fbbase set\n", __func__));
	*/
}


} // end extern "C"
