/*
 * Copyright 2004-2008, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "TouchpadInputDevice.h"
#include "kb_mouse_driver.h"
#include "kb_mouse_settings.h"
#include "touchpad_settings.h"

#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>
#include <View.h>

#include <errno.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//#define TRACE_TOUCHPAD_DEVICE
#ifdef TRACE_TOUCHPAD_DEVICE
#	define LOG(text...) debug_printf(text)
#	define LOG_ERR(text...) LOG(text)
#else
#	define LOG(text...) do {} while (0)
#	define LOG_ERR(text...) debug_printf(text)
#endif

#define CALLED() LOG("%s\n", __PRETTY_FUNCTION__)


const static uint32 kMouseThreadPriority = B_FIRST_REAL_TIME_PRIORITY + 4;

const static char* kTouchpadDevicesDirectoryPS2 = "/dev/input/touchpad/ps2";


class TouchpadDevice {
	public:
		TouchpadDevice(TouchpadInputDevice& target, const char* path);
		~TouchpadDevice();

		status_t Start();
		void Stop();

		status_t UpdateSettings();

		const char* Path() const { return fPath.String(); }
		input_device_ref* DeviceRef() { return &fDeviceRef; }

		bool IsTouchpad() { return fIsTouchpad; }
		status_t ReadTouchpadSettingsMsg(BMessage* msg);
		status_t UpdateTouchpadSettings();

	private:
		void _Run();
		static status_t _ThreadFunction(void *arg);

		BMessage* _BuildMouseMessage(uint32 what, uint64 when, uint32 buttons,
					int32 deltaX, int32 deltaY) const;
		void _ComputeAcceleration(const mouse_movement& movements,
					int32& deltaX, int32& deltaY) const;
		uint32 _RemapButtons(uint32 buttons) const;

		char* _BuildShortName() const;

		status_t _GetSettingsPath(BPath &path);

	private:
		TouchpadInputDevice&	fTarget;
		BString					fPath;
		int						fDevice;

		input_device_ref		fDeviceRef;
		mouse_settings			fSettings;
		bool					fDeviceRemapsButtons;

		thread_id				fThread;
		volatile bool			fActive;

		bool					fIsTouchpad;
		touchpad_settings		fTouchpadSettings;
};


extern "C" BInputServerDevice *
instantiate_input_device()
{
	return new (std::nothrow) TouchpadInputDevice();
}


//	#pragma mark -


TouchpadDevice::TouchpadDevice(TouchpadInputDevice& target,
		const char* driverPath)
	:
	fTarget(target),
	fDevice(-1),
	fDeviceRemapsButtons(false),
	fThread(-1),
	fActive(false),
	fIsTouchpad(false)
{
	fPath = driverPath;

	fDeviceRef.name = _BuildShortName();
	fDeviceRef.type = B_POINTING_DEVICE;
	fDeviceRef.cookie = this;

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	fSettings.map.button[0] = B_PRIMARY_MOUSE_BUTTON;
	fSettings.map.button[1] = B_SECONDARY_MOUSE_BUTTON;
	fSettings.map.button[2] = B_TERTIARY_MOUSE_BUTTON;
#endif

};


TouchpadDevice::~TouchpadDevice()
{
	if (fActive)
		Stop();

	free(fDeviceRef.name);
}


status_t
TouchpadDevice::_GetSettingsPath(BPath &path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;
	return path.Append(TOUCHPAD_SETTINGS_FILE);
}


status_t
TouchpadDevice::Start()
{
	fDevice = open(fPath.String(), O_RDWR);
	if (fDevice < 0)
		return errno;

	// touchpad settings
	if (ioctl(fDevice, MS_IS_TOUCHPAD, NULL) == B_OK) {
		LOG("is touchpad %s\n", fPath.String());
		fIsTouchpad = true;
	}

	fTouchpadSettings = kDefaultTouchpadSettings;

	BPath path;
	status_t status = _GetSettingsPath(path);
	BFile settingsFile(path.Path(), B_READ_ONLY);
	if (status == B_OK && settingsFile.InitCheck() == B_OK) {
		if (settingsFile.Read(&fTouchpadSettings, sizeof(touchpad_settings))
				!= sizeof(touchpad_settings)) {
			LOG("failed to load settings\n");
		}
	}

	UpdateTouchpadSettings();
	UpdateSettings();

	char threadName[B_OS_NAME_LENGTH];
	snprintf(threadName, B_OS_NAME_LENGTH, "%s watcher", fDeviceRef.name);

	fThread = spawn_thread(_ThreadFunction, threadName,
		kMouseThreadPriority, (void*)this);

	if (fThread < B_OK)
		status = fThread;
	else {
		fActive = true;
		status = resume_thread(fThread);
	}

	if (status < B_OK) {
		LOG_ERR("%s: can't spawn/resume watching thread: %s\n",
			fDeviceRef.name, strerror(status));
		close(fDevice);
		return status;
	}

	return B_OK;
}


void
TouchpadDevice::Stop()
{
	fActive = false;
		// this will stop the thread as soon as it reads the next packet

	close(fDevice);
	fDevice = -1;

	if (fThread >= B_OK) {
		// unblock the thread, which might wait on a semaphore.
		suspend_thread(fThread);
		resume_thread(fThread);

		status_t dummy;
		wait_for_thread(fThread, &dummy);
	}
}


status_t
TouchpadDevice::UpdateSettings()
{
	// TODO: This is duplicated in MouseInputDevice.cpp -> Refactor

	CALLED();

	// retrieve current values

	if (get_mouse_map(&fSettings.map) != B_OK)
		LOG_ERR("error when get_mouse_map\n");
	else
		fDeviceRemapsButtons = ioctl(fDevice, MS_SET_MAP, &fSettings.map) == B_OK;

	if (get_click_speed(&fSettings.click_speed) != B_OK)
		LOG_ERR("error when get_click_speed\n");
	else
		ioctl(fDevice, MS_SET_CLICKSPEED, &fSettings.click_speed);

	if (get_mouse_speed(&fSettings.accel.speed) != B_OK)
		LOG_ERR("error when get_mouse_speed\n");
	else {
		if (get_mouse_acceleration(&fSettings.accel.accel_factor) != B_OK)
			LOG_ERR("error when get_mouse_acceleration\n");
		else {
			mouse_accel accel;
			ioctl(fDevice, MS_GET_ACCEL, &accel);
			accel.speed = fSettings.accel.speed;
			accel.accel_factor = fSettings.accel.accel_factor;
			ioctl(fDevice, MS_SET_ACCEL, &fSettings.accel);
		}
	}

	if (get_mouse_type(&fSettings.type) != B_OK)
		LOG_ERR("error when get_mouse_type\n");
	else
		ioctl(fDevice, MS_SET_TYPE, &fSettings.type);

	return B_OK;
}


status_t
TouchpadDevice::ReadTouchpadSettingsMsg(BMessage* msg)
{
	msg->FindBool("scroll_twofinger",
		&(fTouchpadSettings.scroll_twofinger));
	msg->FindBool("scroll_multifinger",
		&(fTouchpadSettings.scroll_multifinger));
	msg->FindFloat("scroll_rightrange",
		&(fTouchpadSettings.scroll_rightrange));
	msg->FindFloat("scroll_bottomrange",
		&(fTouchpadSettings.scroll_bottomrange));
	msg->FindInt16("scroll_xstepsize",
		(int16*)&(fTouchpadSettings.scroll_xstepsize));
	msg->FindInt16("scroll_ystepsize",
		(int16*)&(fTouchpadSettings.scroll_ystepsize));
	msg->FindInt8("scroll_acceleration",
		(int8*)&(fTouchpadSettings.scroll_acceleration));
	msg->FindInt8("tapgesture_sensibility",
		(int8*)&(fTouchpadSettings.tapgesture_sensibility));

	return B_OK;
}


status_t
TouchpadDevice::UpdateTouchpadSettings()
{
	if (fIsTouchpad) {
		ioctl(fDevice, MS_SET_TOUCHPAD_SETTINGS, &fTouchpadSettings);
		return B_OK;
	}
	return B_ERROR;
}


void
TouchpadDevice::_Run()
{
	// TODO: Exact duplicate of MouseDevice::_Run() -> Refactor
	uint32 lastButtons = 0;

	while (fActive) {
		mouse_movement movements;
		memset(&movements, 0, sizeof(movements));

		if (ioctl(fDevice, MS_READ, &movements) != B_OK) {
			if (fActive) {
				fThread = -1;
				fTarget._RemoveDevice(fPath.String());
			} else {
				// In case active is already false, another thread
				// waits for this thread to quit, and may already hold
				// locks that _RemoveDevice() wants to acquire. In another
				// words, the device is already being removed, so we simply
				// quit here.
			}
			// TOAST!
			return;
		}

		uint32 buttons = lastButtons ^ movements.buttons;

		uint32 remappedButtons = _RemapButtons(movements.buttons);
		int32 deltaX, deltaY;
		_ComputeAcceleration(movements, deltaX, deltaY);

	  /*LOG("%s: buttons: 0x%lx, x: %ld, y: %ld, clicks:%ld, wheel_x:%ld, wheel_y:%ld\n",
			device->device_ref.name, movements.buttons, movements.xdelta, movements.ydelta,
			movements.clicks, movements.wheel_xdelta, movements.wheel_ydelta);
		LOG("%s: x: %ld, y: %ld\n", device->device_ref.name, deltaX, deltaY);*/

		// Send single messages for each event

		if (buttons != 0) {
			bool pressedButton = (buttons & movements.buttons) > 0;
			BMessage* message = _BuildMouseMessage(
				pressedButton ? B_MOUSE_DOWN : B_MOUSE_UP,
				movements.timestamp, remappedButtons, deltaX, deltaY);
			if (message != NULL) {
				if (pressedButton) {
					message->AddInt32("clicks", movements.clicks);
					LOG("B_MOUSE_DOWN\n");
				} else
					LOG("B_MOUSE_UP\n");

				fTarget.EnqueueMessage(message);
				lastButtons = movements.buttons;
			}
		}

		if (movements.xdelta != 0 || movements.ydelta != 0) {
			BMessage* message = _BuildMouseMessage(B_MOUSE_MOVED,
				movements.timestamp, remappedButtons, deltaX, deltaY);
			if (message != NULL)
				fTarget.EnqueueMessage(message);
		}

		if ((movements.wheel_ydelta != 0) || (movements.wheel_xdelta != 0)) {
			BMessage* message = new BMessage(B_MOUSE_WHEEL_CHANGED);
			if (message == NULL)
				continue;

			if (message->AddInt64("when", movements.timestamp) == B_OK
				&& message->AddFloat("be:wheel_delta_x", movements.wheel_xdelta) == B_OK
				&& message->AddFloat("be:wheel_delta_y", movements.wheel_ydelta) == B_OK)
				fTarget.EnqueueMessage(message);
			else
				delete message;
		}
	}
}


status_t
TouchpadDevice::_ThreadFunction(void* arg)
{
	TouchpadDevice* device = (TouchpadDevice*)arg;
	device->_Run();
	return B_OK;
}


BMessage*
TouchpadDevice::_BuildMouseMessage(uint32 what, uint64 when, uint32 buttons,
	int32 deltaX, int32 deltaY) const
{
	// TODO: Exact duplicate of MouseDevice::_BuildMouseMessage() -> Refactor

	BMessage* message = new BMessage(what);
	if (message == NULL)
		return NULL;

	if (message->AddInt64("when", when) < B_OK
		|| message->AddInt32("buttons", buttons) < B_OK
		|| message->AddInt32("x", deltaX) < B_OK
		|| message->AddInt32("y", deltaY) < B_OK) {
		delete message;
		return NULL;
	}

	return message;
}


void
TouchpadDevice::_ComputeAcceleration(const mouse_movement& movements,
	int32& deltaX, int32& deltaY) const
{
	// TODO: Exact duplicate of MouseDevice::_ComputeAcceleration() -> Refactor

	// basic mouse speed
	deltaX = movements.xdelta * fSettings.accel.speed >> 16;
	deltaY = movements.ydelta * fSettings.accel.speed >> 16;

	// acceleration
	double acceleration = 1;
	if (fSettings.accel.accel_factor) {
		acceleration = 1 + sqrt(deltaX * deltaX + deltaY * deltaY)
			* fSettings.accel.accel_factor / 524288.0;
	}

	// make sure that we move at least one pixel (if there was a movement)
	if (deltaX > 0)
		deltaX = (int32)floor(deltaX * acceleration);
	else
		deltaX = (int32)ceil(deltaX * acceleration);

	if (deltaY > 0)
		deltaY = (int32)floor(deltaY * acceleration);
	else
		deltaY = (int32)ceil(deltaY * acceleration);
}


uint32
TouchpadDevice::_RemapButtons(uint32 buttons) const
{
	// TODO: Exact duplicate of MouseDevice::_RemapButtons() -> Refactor

	if (fDeviceRemapsButtons)
		return buttons;

	uint32 newButtons = 0;
	for (int32 i = 0; buttons; i++) {
		if (buttons & 0x1) {
#if defined(HAIKU_TARGET_PLATFORM_HAIKU) || defined(HAIKU_TARGET_PLATFORM_DANO)
			newButtons |= fSettings.map.button[i];
#else
			if (i == 0)
				newButtons |= fSettings.map.left;
			if (i == 1)
				newButtons |= fSettings.map.right;
			if (i == 2)
				newButtons |= fSettings.map.middle;
#endif
		}
		buttons >>= 1;
	}

	return newButtons;
}


char *
TouchpadDevice::_BuildShortName() const
{
	BString string(fPath);
	BString name;

	int32 slash = string.FindLast("/");
	string.CopyInto(name, slash + 1, string.Length() - slash);
	//int32 index = atoi(name.String()) + 1;

	BString final = "Touchpad ";
	final += name;

	LOG("NAME %s, %s\n", final.String(), fPath.String());

	return strdup(final.String());
}


//	#pragma mark -


TouchpadInputDevice::TouchpadInputDevice()
{
	CALLED();

	StartMonitoringDevice(kTouchpadDevicesDirectoryPS2);
	_RecursiveScan(kTouchpadDevicesDirectoryPS2);
}


TouchpadInputDevice::~TouchpadInputDevice()
{
	CALLED();
	StopMonitoringDevice(kTouchpadDevicesDirectoryPS2);
	fDevices.MakeEmpty();
}


status_t
TouchpadInputDevice::InitCheck()
{
	CALLED();
	return BInputServerDevice::InitCheck();
}


status_t
TouchpadInputDevice::Start(const char *name, void *cookie)
{
	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);
	TouchpadDevice* device = (TouchpadDevice*)cookie;

	return device->Start();
}


status_t
TouchpadInputDevice::Stop(const char *name, void *cookie)
{
	LOG("%s(%s)\n", __PRETTY_FUNCTION__, name);
	TouchpadDevice* device = (TouchpadDevice*)cookie;

	device->Stop();
	return B_OK;
}


status_t
TouchpadInputDevice::Control(const char* name, void* cookie,
	uint32 command, BMessage* message)
{
	LOG("%s(%s, code: %lu)\n", __PRETTY_FUNCTION__, name, command);
	TouchpadDevice* device = (TouchpadDevice*)cookie;

	if (command == B_NODE_MONITOR)
		return _HandleMonitor(message);

	if (command == MS_SET_TOUCHPAD_SETTINGS) {
		device->ReadTouchpadSettingsMsg(message);
		return device->UpdateTouchpadSettings();
	}

	if (command >= B_MOUSE_TYPE_CHANGED
		&& command <= B_MOUSE_ACCELERATION_CHANGED)
		return device->UpdateSettings();

	return B_BAD_VALUE;
}


status_t
TouchpadInputDevice::_HandleMonitor(BMessage* message)
{
	CALLED();

	const char* path;
	int32 opcode;
	if (message->FindInt32("opcode", &opcode) != B_OK
		|| (opcode != B_ENTRY_CREATED && opcode != B_ENTRY_REMOVED)
		|| message->FindString("path", &path) != B_OK)
		return B_BAD_VALUE;

	if (opcode == B_ENTRY_CREATED)
		return _AddDevice(path);

#if 0
	return _RemoveDevice(path);
#else
	// Don't handle B_ENTRY_REMOVED, let the control thread take care of it.
	return B_OK;
#endif
}


void
TouchpadInputDevice::_RecursiveScan(const char* directory)
{
	LOG("TouchpadInputDevice::_RecursiveScan(%s)\n", directory);

	BEntry entry;
	BDirectory dir(directory);
	while (dir.GetNextEntry(&entry) == B_OK) {
		BPath path;
		entry.GetPath(&path);

		if (entry.IsDirectory())
			_RecursiveScan(path.Path());
		else
			_AddDevice(path.Path());
	}
}


TouchpadDevice*
TouchpadInputDevice::_FindDevice(const char *path)
{
	CALLED();

	for (int32 i = fDevices.CountItems() - 1; i >= 0; i--) {
		TouchpadDevice* device = fDevices.ItemAt(i);
		if (strcmp(device->Path(), path) == 0)
			return device;
	}

	return NULL;
}


status_t
TouchpadInputDevice::_AddDevice(const char *path)
{
	CALLED();

	TouchpadDevice* device = new (std::nothrow) TouchpadDevice(*this, path);
	if (!device) {
		LOG("No memory\n");
		return B_NO_MEMORY;
	}

	if (!fDevices.AddItem(device)) {
		delete device;
		return B_NO_MEMORY;
	}

	LOG_ERR("TouchpadInputDevice::_AddDevice(%s)\n", path);

	input_device_ref* devices[2];
	devices[0] = device->DeviceRef();
	devices[1] = NULL;

	return RegisterDevices(devices);
}


status_t
TouchpadInputDevice::_RemoveDevice(const char *path)
{
	CALLED();

	TouchpadDevice* device = _FindDevice(path);
	if (device == NULL)
		return B_ENTRY_NOT_FOUND;

	LOG_ERR("TouchpadInputDevice::_RemoveDevice(%s)\n", path);

	input_device_ref* devices[2];
	devices[0] = device->DeviceRef();
	devices[1] = NULL;

	UnregisterDevices(devices);

	fDevices.RemoveItem(device);

	return B_OK;
}


