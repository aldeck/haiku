/*
 * Copyright (c) 2009 Clemens Zeidler
 * Copyright (c) 2003-2007 Nate Lawson
 * Copyright (c) 2000 Michael Smith
 * Copyright (c) 2000 BSDi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include "acpi_embedded_controller.h"

#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <condition_variable.h>
#include <Errors.h>
#include <KernelExport.h>
#include <drivers/PCI.h>

#include "SmallResourceData.h"


#define ACPI_EC_DRIVER_NAME "drivers/power/acpi_embedded_controller/driver_v1"

#define ACPI_EC_DEVICE_NAME "drivers/power/acpi_embedded_controller/device_v1"

/* Base Namespace devices are published to */
#define ACPI_EC_BASENAME "power/embedded_controller/%d"

// name of pnp generator of path ids
#define ACPI_EC_PATHID_GENERATOR "embedded_controller/path_id"


uint8
bus_space_read_1(int address)
{
	return gPCIManager->read_io_8(address);
}


void
bus_space_write_1(int address, uint8 value)
{
	gPCIManager->write_io_8(address, value);
}


status_t
acpi_GetInteger(acpi_device_module_info* acpi, acpi_device& acpiCookie,
	const char* path, int* number)
{
	acpi_data buf;
	acpi_object_type object;
	buf.pointer = &object;
	buf.length = sizeof(acpi_object_type);

	// Assume that what we've been pointed at is an Integer object, or
	// a method that will return an Integer.
	status_t status = acpi->evaluate_method(acpiCookie, path, NULL, &buf);
	if (status == B_OK) {
		if (object.object_type == ACPI_TYPE_INTEGER)
			*number = object.data.integer;
		else
			status = B_BAD_VALUE;
	}
	return status;
}


acpi_handle
acpi_GetReference(acpi_module_info* acpi, acpi_handle scope,
	acpi_object_type* obj)
{
	if (obj == NULL)
		return NULL;

	switch (obj->object_type) {
		case ACPI_TYPE_LOCAL_REFERENCE:
		case ACPI_TYPE_ANY:
			return obj->data.reference.handle;

		case ACPI_TYPE_STRING:
		{
			// The String object usually contains a fully-qualified path, so
			// scope can be NULL.
			// TODO: This may not always be the case.
			acpi_handle handle;
			if (acpi->get_handle(scope, obj->data.string.string, &handle)
					== B_OK)
				return handle;
		}
	}

	return NULL;
}


status_t
acpi_PkgInt(acpi_object_type* res, int idx, int* dst)
{
	acpi_object_type* obj = &res->data.package.objects[idx];
	if (obj == NULL || obj->object_type != ACPI_TYPE_INTEGER)
		return B_BAD_VALUE;
	*dst = obj->data.integer;

	return B_OK;
}


status_t
acpi_PkgInt32(acpi_object_type* res, int idx, uint32* dst)
{
	int tmp;

	status_t status = acpi_PkgInt(res, idx, &tmp);
	if (status == B_OK)
		*dst = (uint32) tmp;

	return status;
}


// #pragma mark -


static status_t
embedded_controller_open(void* initCookie, const char* path, int flags,
	void** cookie)
{
	acpi_ec_cookie* device = (acpi_ec_cookie*) initCookie;
	*cookie = device;

	return B_OK;
}


static status_t
embedded_controller_close(void* cookie)
{
	return B_OK;
}


static status_t
embedded_controller_read(void* _cookie, off_t position, void* buffer,
	size_t* numBytes)
{
	return B_IO_ERROR;
}


static status_t
embedded_controller_write(void* cookie, off_t position, const void* buffer,
	size_t* numBytes)
{
	return B_IO_ERROR;
}


status_t
embedded_controller_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	return B_ERROR;
}


static status_t
embedded_controller_free(void* cookie)
{
	return B_OK;
}


//	#pragma mark - driver module API


int32
acpi_get_type(device_node* dev)
{
	const char *bus;
	if (gDeviceManager->get_attr_string(dev, B_DEVICE_BUS, &bus, false))
		return -1;

	if (strcmp(bus, "acpi"))
		return -1;

	uint32 deviceType;
	if (gDeviceManager->get_attr_uint32(dev, ACPI_DEVICE_TYPE_ITEM,
			&deviceType, false) != B_OK)
		return -1;

	return deviceType;
}


static float
embedded_controller_support(device_node* dev)
{
	TRACE("embedded_controller_support()\n");

	// Check that this is a device
	if (acpi_get_type(dev) != ACPI_TYPE_DEVICE)
		return 0.0;

	const char* name;
	if (gDeviceManager->get_attr_string(dev, ACPI_DEVICE_HID_ITEM, &name, false)
			!= B_OK)
		return 0.0;

	// Test all known IDs

	static const char* kEmbeddedControllerIDs[] = { "PNP0C09" };

	for (size_t i = 0; i < sizeof(kEmbeddedControllerIDs)
			/ sizeof(kEmbeddedControllerIDs[0]); i++) {
		if (!strcmp(name, kEmbeddedControllerIDs[i])) {
			TRACE("supported device found %s\n", name);
			return 0.6;
		}
	}

	return 0.0;
}


static status_t
embedded_controller_register_device(device_node* node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ string: "ACPI embedded controller" }},
		{ NULL }
	};

	return gDeviceManager->register_node(node, ACPI_EC_DRIVER_NAME, attrs,
		NULL, NULL);
}


static status_t
embedded_controller_init_driver(device_node* dev, void** _driverCookie)
{
	TRACE("init driver\n");

	acpi_ec_cookie* sc;
	sc = (acpi_ec_cookie*)malloc(sizeof(acpi_ec_cookie));
	if (sc == NULL)
		return B_NO_MEMORY;

	memset(sc, 0, sizeof(acpi_ec_cookie));

	*_driverCookie = sc;
	sc->ec_dev = dev;

	sc->ec_condition_var.Init(NULL, "ec condition variable");
	mutex_init(&sc->ec_lock, "ec lock");
	device_node* parent = gDeviceManager->get_parent_node(dev);
	gDeviceManager->get_driver(parent, (driver_module_info**)&sc->ec_acpi,
		(void**)&sc->ec_handle);
	gDeviceManager->put_node(parent);

	SmallResourceData resourceData(sc->ec_acpi, sc->ec_handle, "_CRS");
	if (resourceData.InitCheck() != B_OK) {
		TRACE("failed to read _CRS resource\n")	;
		return B_ERROR;
	}
	io_port portData;

	if (get_module(B_ACPI_MODULE_NAME, (module_info**)&sc->ec_acpi_module)
			!= B_OK)
		return B_ERROR;

	acpi_data buf;
	buf.pointer = NULL;
	buf.length = ACPI_ALLOCATE_BUFFER;

	// Read the unit ID to check for duplicate attach and the
	// global lock value to see if we should acquire it when
	// accessing the EC.
	status_t status = acpi_GetInteger(sc->ec_acpi, sc->ec_handle, "_UID",
		&sc->ec_uid);
	if (status != B_OK)
		sc->ec_uid = 0;
	status = acpi_GetInteger(sc->ec_acpi, sc->ec_handle, "_GLK", &sc->ec_glk);
	if (status != B_OK)
		sc->ec_glk = 0;

	// Evaluate the _GPE method to find the GPE bit used by the EC to
	// signal status (SCI).  If it's a package, it contains a reference
	// and GPE bit, similar to _PRW.
	status = sc->ec_acpi->evaluate_method(sc->ec_handle, "_GPE", NULL, &buf);
	if (status != B_OK) {
		TRACE("can't evaluate _GPE\n");
		goto error;
	}

	acpi_object_type* obj;
	obj = (acpi_object_type*)buf.pointer;
	if (obj == NULL)
		goto error;

	switch (obj->object_type) {
		case ACPI_TYPE_INTEGER:
			sc->ec_gpehandle = NULL;
			sc->ec_gpebit = obj->data.integer;
			break;
		case ACPI_TYPE_PACKAGE:
			if (!ACPI_PKG_VALID(obj, 2))
				goto error;
			sc->ec_gpehandle = acpi_GetReference(sc->ec_acpi_module, NULL,
				&obj->data.package.objects[0]);
			if (sc->ec_gpehandle == NULL
				|| acpi_PkgInt32(obj, 1, (uint32*)&sc->ec_gpebit) != B_OK)
				goto error;
			break;
		default:
			TRACE("_GPE has invalid type %i\n", int(obj->object_type));
			goto error;
	}

	sc->ec_suspending = FALSE;

	// Attach bus resources for data and command/status ports.
	if (resourceData.ReadIOPort(&portData) != B_OK)
		goto error;

	sc->ec_data_pci_address = portData.minimumBase;

	if (resourceData.ReadIOPort(&portData) != B_OK)
		goto error;

	sc->ec_csr_pci_address = portData.minimumBase;

	// Install a handler for this EC's GPE bit.  We want edge-triggered
	// behavior.
	TRACE("attaching GPE handler\n");
	status = sc->ec_acpi_module->install_gpe_handler(sc->ec_gpehandle,
		sc->ec_gpebit, ACPI_GPE_EDGE_TRIGGERED, &EcGpeHandler, sc);
	if (status != B_OK) {
		TRACE("can't install ec GPE handler\n");
		goto error;
	}

	// Install address space handler
	TRACE("attaching address space handler\n");
	status = sc->ec_acpi->install_address_space_handler(sc->ec_handle,
		ACPI_ADR_SPACE_EC, &EcSpaceHandler, &EcSpaceSetup, sc);
	if (status != B_OK) {
		TRACE("can't install address space handler\n");
		goto error;
	}

	// Enable runtime GPEs for the handler.
	status = sc->ec_acpi_module->set_gpe_type(sc->ec_gpehandle, sc->ec_gpebit,
		ACPI_GPE_TYPE_RUNTIME);
	if (status != B_OK) {
		TRACE("AcpiSetGpeType failed.\n");
		goto error;
	}
	status = sc->ec_acpi_module->enable_gpe(sc->ec_gpehandle, sc->ec_gpebit,
		ACPI_NOT_ISR);
	if (status != B_OK) {
		TRACE("AcpiEnableGpe failed.\n");
		goto error;
	}

	return 0;

error:
	free(buf.pointer);

	sc->ec_acpi_module->remove_gpe_handler(sc->ec_gpehandle, sc->ec_gpebit,
		&EcGpeHandler);
	sc->ec_acpi->remove_address_space_handler(sc->ec_handle, ACPI_ADR_SPACE_EC,
		EcSpaceHandler);

	return ENXIO;
}


static void
embedded_controller_uninit_driver(void* driverCookie)
{
	acpi_ec_cookie* sc = (struct acpi_ec_cookie*)driverCookie;
	mutex_destroy(&sc->ec_lock);
	free(sc);
	put_module(B_ACPI_MODULE_NAME);
}


static status_t
embedded_controller_register_child_devices(void* _cookie)
{
	device_node* node = ((acpi_ec_cookie*)_cookie)->ec_dev;

	int pathID = gDeviceManager->create_id(ACPI_EC_PATHID_GENERATOR);
	if (pathID < 0) {
		TRACE("register_child_device couldn't create a path_id\n");
		return B_ERROR;
	}

	char name[128];
	snprintf(name, sizeof(name), ACPI_EC_BASENAME, pathID);

	return gDeviceManager->publish_device(node, name, ACPI_EC_DEVICE_NAME);
}


static status_t
embedded_controller_init_device(void* driverCookie, void** cookie)
{
	return B_ERROR;
}


static void
embedded_controller_uninit_device(void* _cookie)
{
	acpi_ec_cookie* device = (acpi_ec_cookie*)_cookie;
	free(device);
}


driver_module_info embedded_controller_driver_module = {
	{
		ACPI_EC_DRIVER_NAME,
		0,
		NULL
	},

	embedded_controller_support,
	embedded_controller_register_device,
	embedded_controller_init_driver,
	embedded_controller_uninit_driver,
	embedded_controller_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info embedded_controller_device_module = {
	{
		ACPI_EC_DEVICE_NAME,
		0,
		NULL
	},

	embedded_controller_init_device,
	embedded_controller_uninit_device,
	NULL,

	embedded_controller_open,
	embedded_controller_close,
	embedded_controller_free,
	embedded_controller_read,
	embedded_controller_write,
	NULL,
	embedded_controller_control,

	NULL,
	NULL
};


// #pragma mark -


static void
EcGpeQueryHandler(void* context)
{
	struct acpi_ec_cookie* sc = (struct acpi_ec_cookie*)context;

	ASSERT(context != NULL);//, ("EcGpeQueryHandler called with NULL"));

	// Serialize user access with EcSpaceHandler().
	status_t status = EcLock(sc);
	if (status != B_OK) {
		TRACE("GpeQuery lock error.\n");
		return;
	}

	// Send a query command to the EC to find out which _Qxx call it
	// wants to make.  This command clears the SCI bit and also the
	// interrupt source since we are edge-triggered.  To prevent the GPE
	// that may arise from running the query from causing another query
	// to be queued, we clear the pending flag only after running it.
	status = EcCommand(sc, EC_COMMAND_QUERY);
	sc->ec_sci_pending = FALSE;
	if (status != B_OK) {
		EcUnlock(sc);
		TRACE("GPE query failed.\n");
		return;
	}
	uint8 data = EC_GET_DATA(sc);

	// We have to unlock before running the _Qxx method below since that
	// method may attempt to read/write from EC address space, causing
	// recursive acquisition of the lock.
	EcUnlock(sc);

	// Ignore the value for "no outstanding event". (13.3.5)
	TRACE("query ok,%s running _Q%02X\n", data ? "" : " not", data);
	if (data == 0)
		return;

	// Evaluate _Qxx to respond to the controller.
	char qxx[5];
	snprintf(qxx, sizeof(qxx), "_Q%02X", data);
	AcpiUtStrupr(qxx);
	status = sc->ec_acpi->evaluate_method(sc->ec_handle, qxx, NULL, NULL);
	if (status != B_OK) {
		TRACE("evaluation of query method %s failed\n", qxx);
	}
}


/*!	The GPE handler is called when IBE/OBF or SCI events occur.  We are
	called from an unknown lock context.
*/
static uint32
EcGpeHandler(void* context)
{
	struct acpi_ec_cookie* sc = (acpi_ec_cookie*)context;

	ASSERT(context != NULL);//, ("EcGpeHandler called with NULL"));
	TRACE("gpe handler start\n");

	// Notify EcWaitEvent() that the status register is now fresh.  If we
	// didn't do this, it wouldn't be possible to distinguish an old IBE
	// from a new one, for example when doing a write transaction (writing
	// address and then data values.)
	atomic_add(&sc->ec_gencount, 1);
	sc->ec_condition_var.NotifyAll();

	// If the EC_SCI bit of the status register is set, queue a query handler.
	// It will run the query and _Qxx method later, under the lock.
	EC_STATUS EcStatus = EC_GET_CSR(sc);
	if ((EcStatus & EC_EVENT_SCI) && !sc->ec_sci_pending) {
		TRACE("gpe queueing query handler\n");
		ACPI_STATUS status = AcpiOsExecute(OSL_GPE_HANDLER, EcGpeQueryHandler,
			context);
		if (status == AE_OK)
			sc->ec_sci_pending = TRUE;
		else
			dprintf("EcGpeHandler: queuing GPE query handler failed\n");
	}
	return B_INVOKE_SCHEDULER;
}


static acpi_status
EcSpaceSetup(acpi_handle region, uint32 function, void* context,
	void** regionContext)
{
	// If deactivating a region, always set the output to NULL.  Otherwise,
	// just pass the context through.
	if (function == ACPI_REGION_DEACTIVATE)
		*regionContext = NULL;
	else
		*regionContext = context;

	return AE_OK;
}


static acpi_status
EcSpaceHandler(uint32 function, acpi_physical_address address, uint32 width,
	int* value, void* context, void* regionContext)
{
	TRACE("enter EcSpaceHandler\n");
	struct acpi_ec_cookie* sc = (struct acpi_ec_cookie*)context;
	uint8 ecData;

	if (width % 8 != 0 || value == NULL || context == NULL)
		return AE_BAD_PARAMETER;
	if (address + (width / 8) - 1 > 0xFF)
		return AE_BAD_ADDRESS;

	if (function == ACPI_READ)
		*value = 0;
	uint8 ecAddr = address;
	acpi_status status = AE_ERROR;

	/*
	 * If booting, check if we need to run the query handler.  If so, we
	 * we call it directly here since our thread taskq is not active yet.
	 */
	/*
	if (cold || rebooting || sc->ec_suspending) {
		if ((EC_GET_CSR(sc) & EC_EVENT_SCI)) {
			//CTR0(KTR_ACPI, "ec running gpe handler directly");
			EcGpeQueryHandler(sc);
		}
	} */

	// Serialize with EcGpeQueryHandler() at transaction granularity.
	status = EcLock(sc);
	if (status != B_OK)
		return AE_NOT_ACQUIRED;

	// Perform the transaction(s), based on width.
	for (uint32 i = 0; i < width; i += 8, ecAddr++) {
		switch (function) {
			case ACPI_READ:
				status = EcRead(sc, ecAddr, &ecData);
				if (status == AE_OK)
					*value |= ((int) ecData) << i;
				break;
			case ACPI_WRITE:
				ecData = (uint8)((*value) >> i);
				status = EcWrite(sc, ecAddr, &ecData);
				break;
			default:
				TRACE("invalid EcSpaceHandler function\n");
				status = AE_BAD_PARAMETER;
				break;
		}
		if (status != AE_OK)
			break;
	}

	EcUnlock(sc);
	return status;
}


static acpi_status
EcCheckStatus(struct acpi_ec_cookie* sc, const char* msg, EC_EVENT event)
{
	acpi_status status = AE_NO_HARDWARE_RESPONSE;
	EC_STATUS ec_status = EC_GET_CSR(sc);

	if (sc->ec_burstactive && !(ec_status & EC_FLAG_BURST_MODE)) {
		TRACE("burst disabled in waitevent (%s)\n", msg);
		sc->ec_burstactive = false;
	}
	if (EVENT_READY(event, ec_status)) {
		TRACE("%s wait ready, status %#x\n", msg, ec_status);
		status = AE_OK;
	}
	return status;
}


static acpi_status
EcWaitEvent(struct acpi_ec_cookie* sc, EC_EVENT event, int32 generationCount)
{
	acpi_status status = AE_NO_HARDWARE_RESPONSE;
	int32 count, i;

	// int need_poll = cold || rebooting || ec_polled_mode || sc->ec_suspending;
	int needPoll = ec_polled_mode || sc->ec_suspending || gKernelStartup;

	// The main CPU should be much faster than the EC.  So the status should
	// be "not ready" when we start waiting.  But if the main CPU is really
	// slow, it's possible we see the current "ready" response.  Since that
	// can't be distinguished from the previous response in polled mode,
	// this is a potential issue.  We really should have interrupts enabled
	// during boot so there is no ambiguity in polled mode.
	//
	// If this occurs, we add an additional delay before actually entering
	// the status checking loop, hopefully to allow the EC to go to work
	// and produce a non-stale status.
	if (needPoll) {
		static int once;

		if (EcCheckStatus(sc, "pre-check", event) == B_OK) {
			if (!once) {
				TRACE("warning: EC done before starting event wait\n");
				once = 1;
			}
			spin(10);
		}
	}

	// Wait for event by polling or GPE (interrupt).
	if (needPoll) {
		count = (ec_timeout * 1000) / EC_POLL_DELAY;
		if (count == 0)
			count = 1;

		for (i = 0; i < count; i++) {
			status = EcCheckStatus(sc, "poll", event);
			if (status == AE_OK)
				break;
			spin(EC_POLL_DELAY);
		}
	} else {
		bigtime_t sleepInterval = system_time() + ec_timeout * 1000;

		// Wait for the GPE to signal the status changed, checking the
		// status register each time we get one.  It's possible to get a
		// GPE for an event we're not interested in here (i.e., SCI for
		// EC query).
		status_t waitStatus = B_NO_ERROR;
		while (waitStatus != B_TIMED_OUT) {
			if (generationCount != sc->ec_gencount) {
				// Record new generation count.  It's possible the GPE was
				// just to notify us that a query is needed and we need to
				// wait for a second GPE to signal the completion of the
				// event we are actually waiting for.
				generationCount = sc->ec_gencount;
				status = EcCheckStatus(sc, "sleep", event);
				if (status == AE_OK)
					break;
			}
			waitStatus = sc->ec_condition_var.Wait(B_ABSOLUTE_TIMEOUT,
				sleepInterval);
		}

		// We finished waiting for the GPE and it never arrived.  Try to
		// read the register once and trust whatever value we got.  This is
		// the best we can do at this point.  Then, force polled mode on
		// since this system doesn't appear to generate GPEs.
		if (status != AE_OK) {
			status = EcCheckStatus(sc, "sleep_end", event);
			TRACE("wait timed out (%sresponse), forcing polled mode\n",
				status == AE_OK ? "" : "no ");
			ec_polled_mode = TRUE;
		}
	}
	if (status != AE_OK)
		TRACE("error: ec wait timed out\n");

	return status;
}


static acpi_status
EcCommand(struct acpi_ec_cookie* sc, EC_COMMAND cmd)
{
	// Don't use burst mode if user disabled it.
	if (!ec_burst_mode && cmd == EC_COMMAND_BURST_ENABLE)
		return AE_ERROR;

	// Decide what to wait for based on command type.
	EC_EVENT event;
	switch (cmd) {
		case EC_COMMAND_READ:
		case EC_COMMAND_WRITE:
		case EC_COMMAND_BURST_DISABLE:
			event = EC_EVENT_INPUT_BUFFER_EMPTY;
			break;
		case EC_COMMAND_QUERY:
		case EC_COMMAND_BURST_ENABLE:
			event = EC_EVENT_OUTPUT_BUFFER_FULL;
			break;
		default:
			TRACE("EcCommand: invalid command %#x\n", cmd);
			return AE_BAD_PARAMETER;
	}

	// Run the command and wait for the chosen event.
	TRACE("running command %#x\n", cmd);
	u_int gen_count = sc->ec_gencount;
	EC_SET_CSR(sc, cmd);
	acpi_status status = EcWaitEvent(sc, event, gen_count);
	if (status == AE_OK) {
		// If we succeeded, burst flag should now be present.
		if (cmd == EC_COMMAND_BURST_ENABLE) {
			EC_STATUS ec_status = EC_GET_CSR(sc);
			if ((ec_status & EC_FLAG_BURST_MODE) == 0)
				status = AE_ERROR;
		}
	} else
		TRACE("EcCommand: no response to %#x\n", cmd);

	return status;
}


static acpi_status
EcRead(struct acpi_ec_cookie* sc, uint8 address, uint8* readData)
{
	TRACE("read from %#x\n", address);

	// If we can't start burst mode, continue anyway.
	acpi_status status = EcCommand(sc, EC_COMMAND_BURST_ENABLE);
	if (status == AE_OK) {
		uint8 data = EC_GET_DATA(sc);
		if (data == EC_BURST_ACK) {
			TRACE("burst enabled\n");
			sc->ec_burstactive = TRUE;
		}
	}

	status = EcCommand(sc, EC_COMMAND_READ);
	if (status != AE_OK)
		return status;

	u_int generationCount = sc->ec_gencount;

	EC_SET_DATA(sc, address);
	status = EcWaitEvent(sc, EC_EVENT_OUTPUT_BUFFER_FULL, generationCount);
	if (status != AE_OK) {
		TRACE("EcRead: failed waiting to get data\n");
		return status;
	}
	*readData = EC_GET_DATA(sc);

	if (sc->ec_burstactive) {
		sc->ec_burstactive = FALSE;
		status = EcCommand(sc, EC_COMMAND_BURST_DISABLE);
		if (status != AE_OK)
			return status;
		TRACE("disabled burst ok\n");
	}

	return AE_OK;
}


static acpi_status
EcWrite(struct acpi_ec_cookie* sc, uint8 address, uint8* writeData)
{
	/* If we can't start burst mode, continue anyway. */
	acpi_status status = EcCommand(sc, EC_COMMAND_BURST_ENABLE);
	if (status == AE_OK) {
		uint8 data = EC_GET_DATA(sc);
		if (data == EC_BURST_ACK) {
			TRACE("burst enabled\n");
			sc->ec_burstactive = TRUE;
		}
	}

	status = EcCommand(sc, EC_COMMAND_WRITE);
	if (status != AE_OK)
		return status;

	u_int generationCount = sc->ec_gencount;
	EC_SET_DATA(sc, address);
	status = EcWaitEvent(sc, EC_EVENT_INPUT_BUFFER_EMPTY, generationCount);
	if (status != AE_OK) {
		TRACE("EcRead: failed waiting for sent address\n");
		return status;
	}

	generationCount = sc->ec_gencount;
	EC_SET_DATA(sc, *writeData);
	status = EcWaitEvent(sc, EC_EVENT_INPUT_BUFFER_EMPTY, generationCount);
	if (status != AE_OK) {
		TRACE("EcWrite: failed waiting for sent data\n");
		return status;
	}

	if (sc->ec_burstactive) {
		sc->ec_burstactive = FALSE;
		status = EcCommand(sc, EC_COMMAND_BURST_DISABLE);
		if (status != AE_OK)
			return (status);
		TRACE("disabled burst ok\n");
	}

	return AE_OK;
}
