/*
 * Copyright 2007, Hugo Santos, hugosantos@gmail.com. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Copyright 2004, Marcus Overhagen. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include "device.h"

#include <stdio.h>

#include <KernelExport.h>
#include <image.h>

#include <compat/machine/resource.h>
#include <compat/dev/mii/mii.h>
#include <compat/sys/bus.h>

#include <compat/dev/mii/miivar.h>


spinlock __haiku_intr_spinlock;

struct net_stack_module_info *gStack;
pci_module_info *gPci;

static struct list sRootDevices;
static int sNextUnit;


//	#pragma mark - private functions


static device_t
init_device(device_t device, driver_t *driver)
{
	list_init_etc(&device->children, offsetof(struct device, link));
	device->unit = sNextUnit++;

	if (driver != NULL && device_set_driver(device, driver) < 0)
		return NULL;

	return device;
}


static device_t
new_device(driver_t *driver)
{
	device_t dev = malloc(sizeof(struct device));
	if (dev == NULL)
		return NULL;

	memset(dev, 0, sizeof(struct device));

	if (init_device(dev, driver) == NULL) {
		free(dev);
		return NULL;
	}

	return dev;
}


static image_id
find_own_image()
{
	int32 cookie = 0;
	image_info info;
	while (get_next_image_info(B_SYSTEM_TEAM, &cookie, &info) == B_OK) {
		if (((uint32)info.text <= (uint32)find_own_image
			&& (uint32)info.text + (uint32)info.text_size
					> (uint32)find_own_image)) {
			// found our own image
			return info.id;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


static device_method_signature_t
resolve_method(driver_t *driver, const char *name)
{
	device_method_signature_t method = NULL;
	int i;

	for (i = 0; method == NULL && driver->methods[i].name != NULL; i++) {
		if (strcmp(driver->methods[i].name, name) == 0)
			method = driver->methods[i].method;
	}

	return method;
}


//	#pragma mark - Device


void
driver_printf(const char *format, ...)
{
	va_list vl;
	va_start(vl, format);
	driver_vprintf(format, vl);
	va_end(vl);
}


static void
driver_vprintf_etc(const char *extra, const char *format, va_list vl)
{
	char buf[256];
	vsnprintf(buf, sizeof(buf), format, vl);

	if (extra)
		dprintf("[%s] (%s) %s", gDriverName, extra, buf);
	else
		dprintf("[%s] %s", gDriverName, buf);
}


void
driver_vprintf(const char *format, va_list vl)
{
	driver_vprintf_etc(NULL, format, vl);
}


int
device_printf(device_t dev, const char *format, ...)
{
	va_list vl;

	va_start(vl, format);
	driver_vprintf_etc(dev->device_name, format, vl);
	va_end(vl);
	return 0;
}


void
device_set_desc(device_t dev, const char *desc)
{
	dev->description = desc;
}


void
device_set_desc_copy(device_t dev, const char *desc)
{
	dev->description = strdup(desc);
	dev->flags |= DEVICE_DESC_ALLOCED;
}


const char *
device_get_desc(device_t dev)
{
	return dev->description;
}


device_t
device_get_parent(device_t dev)
{
	return dev->parent;
}


void
device_set_ivars(device_t dev, void *ivars)
{
	dev->ivars = ivars;
}


void *
device_get_ivars(device_t dev)
{
	return dev->ivars;
}


const char *
device_get_name(device_t dev)
{
	if (dev == NULL)
		return NULL;

	return dev->device_name;
}


int
device_get_unit(device_t dev)
{
	return dev->unit;
}


const char *
device_get_nameunit(device_t dev)
{
	return dev->nameunit;
}


void *
device_get_softc(device_t dev)
{
	return dev->softc;
}


int
device_set_driver(device_t dev, driver_t *driver)
{
	device_method_signature_t method = NULL;
	int i;

	dev->softc = malloc(driver->softc_size);
	if (dev->softc == NULL)
		return -1;

	memset(dev->softc, 0, driver->softc_size);
	dev->driver = driver;

	for (i = 0; method == NULL && driver->methods[i].name != NULL; i++) {
		device_method_t *mth = &driver->methods[i];

		if (strcmp(mth->name, "device_probe") == 0)
			dev->methods.probe = (void *)mth->method;
		else if (strcmp(mth->name, "device_attach") == 0)
			dev->methods.attach = (void *)mth->method;
		else if (strcmp(mth->name, "device_detach") == 0)
			dev->methods.detach = (void *)mth->method;
		else if (strcmp(mth->name, "device_suspend") == 0)
			dev->methods.suspend = (void *)mth->method;
		else if (strcmp(mth->name, "device_resume") == 0)
			dev->methods.resume = (void *)mth->method;
		else if (strcmp(mth->name, "device_shutdown") == 0)
			dev->methods.shutdown = (void *)mth->method;
		else if (strcmp(mth->name, "miibus_readreg") == 0)
			dev->methods.miibus_readreg = (void *)mth->method;
		else if (strcmp(mth->name, "miibus_writereg") == 0)
			dev->methods.miibus_writereg = (void *)mth->method;
		else if (strcmp(mth->name, "miibus_statchg") == 0)
			dev->methods.miibus_statchg = (void *)mth->method;
		else if (!strcmp(mth->name, "miibus_linkchg"))
			dev->methods.miibus_linkchg = (void *)mth->method;
		else if (!strcmp(mth->name, "miibus_mediainit"))
			dev->methods.miibus_mediainit = (void *)mth->method;
	}

	return 0;
}


int
device_is_alive(device_t device)
{
	return (device->flags & DEVICE_ATTACHED) != 0;
}


device_t
device_add_child(device_t parent, const char *name, int unit)
{
	device_t child = NULL;

	if (name != NULL) {
		if (strcmp(name, "miibus") == 0)
			child = new_device(&miibus_driver);
		else {
			// find matching driver structure
			driver_t **driver;
			char symbol[128];

			snprintf(symbol, sizeof(symbol), "__fbsd_%s%s", name,
				parent->driver->name);
			if (get_image_symbol(find_own_image(), symbol, B_SYMBOL_TYPE_DATA,
					(void **)&driver) == B_OK)
				child = new_device(*driver);
		}
	} else
		child = new_device(NULL);

	if (child == NULL)
		return NULL;

	if (name != NULL)
		strlcpy(child->device_name, name, sizeof(child->device_name));

	child->parent = parent;

	if (parent != NULL) {
		list_add_item(&parent->children, child);
		child->root = parent->root;
	} else {
		if (sRootDevices.link.next == NULL)
			list_init_etc(&sRootDevices, offsetof(struct device, link));
		list_add_item(&sRootDevices, child);
	}

	return child;
}


/*!	Delete the child and all of its children. Detach as necessary.
*/
int
device_delete_child(device_t parent, device_t child)
{
	if (child == NULL)
		return 0;

	if (parent != NULL)
		list_remove_item(&parent->children, child);

	parent = child;

	while ((child = list_remove_head_item(&parent->children)) != NULL) {
		device_delete_child(NULL, child);
	}

	if ((atomic_and(&parent->flags, ~DEVICE_ATTACHED) & DEVICE_ATTACHED) != 0
		&& parent->methods.detach != NULL) {
		int status = parent->methods.detach(parent);
		if (status != 0)
			return status;
	}

	if (parent->flags & DEVICE_DESC_ALLOCED)
		free((char *)parent->description);

	free(parent->softc);
	free(parent);
	return 0;
}


int
device_is_attached(device_t device)
{
	return (device->flags & DEVICE_ATTACHED) != 0;
}


int
device_attach(device_t device)
{
	int result;

	if (device->driver == NULL)
		return B_ERROR;

	result = device->methods.attach(device);
	if (result == 0)
		atomic_or(&device->flags, DEVICE_ATTACHED);

	return result;
}


int
bus_generic_attach(device_t dev)
{
	device_t child = NULL;

	while ((child = list_get_next_item(&dev->children, child)) != NULL) {
		if (child->driver == NULL) {
			driver_t *driver = __haiku_select_miibus_driver(child);
			if (driver == NULL) {
				struct mii_attach_args *ma = device_get_ivars(child);

				device_printf(dev, "No PHY module found (%x/%x)!\n",
					MII_OUI(ma->mii_id1, ma->mii_id2), MII_MODEL(ma->mii_id2));
			} else
				device_set_driver(child, driver);
		} else if (child->driver != &miibus_driver)
			child->methods.probe(child);

		if (child->driver != NULL) {
			int result = device_attach(child);
			if (result != 0)
				return result;
		}
	}

	return 0;
}


//	#pragma mark - Misc, Malloc


device_t
find_root_device(int unit)
{
	device_t device = NULL;

	while ((device = list_get_next_item(&sRootDevices, device)) != NULL) {
		if (device->unit <= unit)
			return device;
	}

	return NULL;
}


driver_t *
__haiku_probe_miibus(device_t dev, driver_t *drivers[])
{
	driver_t *selected = NULL;
	int i, selectedResult = 0;

	if (drivers == NULL)
		return NULL;

	for (i = 0; drivers[i]; i++) {
		device_probe_t *probe = (device_probe_t *)
			resolve_method(drivers[i], "device_probe");
		if (probe) {
			int result = probe(dev);
			if (result >= 0) {
				if (selected == NULL || result < selectedResult) {
					selected = drivers[i];
					selectedResult = result;
					device_printf(dev, "Found MII: %s\n", selected->name);
				}
			}
		}
	}

	return selected;
}


int
printf(const char *format, ...)
{
	char buf[256];
	va_list vl;
	va_start(vl, format);
	vsnprintf(buf, sizeof(buf), format, vl);
	va_end(vl);
	dprintf(buf);

	return 0;
}


int
ffs(int value)
{
	int i = 1;

	if (value == 0)
		return 0;

	for (; !(value & 1); i++)
		value >>= 1;

	return i;
}


int resource_int_value(const char *name, int unit, const char *resname,
	int *result)
{
	/* no support for hints */
	return -1;
}


void *
_kernel_malloc(size_t size, int flags)
{
	// our kernel malloc() is insufficent, must handle M_WAIT

	void *ptr = malloc(size);
	if (ptr == NULL)
		return ptr;

	if (flags & M_ZERO)
		memset(ptr, 0, size);

	return ptr;
}


void
_kernel_free(void *ptr)
{
	free(ptr);
}


void *
_kernel_contigmalloc(const char *file, int line, size_t size, int flags,
	vm_paddr_t low, vm_paddr_t high, unsigned long alignment,
	unsigned long boundary)
{
	char name[256];
	area_id area;
	void *addr;

	snprintf(name, sizeof(name), "contig:%s:%d", file, line);

	area = create_area(name, &addr, B_ANY_KERNEL_ADDRESS, size,
		B_FULL_LOCK | B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (area < 0)
		return NULL;

	if (flags & M_ZERO)
		memset(addr, 0, size);

	driver_printf("(%s) addr = %p, area = %ld, size = %lu\n",
		name, addr, area, size);

	return addr;
}


void
_kernel_contigfree(void *addr, size_t size)
{
	delete_area(area_for(addr));
}


vm_paddr_t
pmap_kextract(vm_offset_t virtualAddress)
{
	physical_entry entry;
	status_t status = get_memory_map((void *)virtualAddress, 1, &entry, 1);
	if (status < B_OK) {
		panic("fbsd compat: get_memory_map failed for %p, error %08lx\n",
			(void *)virtualAddress, status);
	}

	return (vm_paddr_t)entry.address;
}

