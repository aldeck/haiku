//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		DisplayDriver.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Gabe Yoder <gyoder@stny.rr.com>
//	Description:	Mostly abstract class which handles all graphics output
//					for the server
//  
//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

#include "CursorData.h"
#include "ServerCursor.h"

#include "DisplayDriver.h"

/*!
	\brief Sets up internal variables needed by all DisplayDriver subclasses
*/
DisplayDriver::DisplayDriver()
	: fDPMSState(B_DPMS_ON),
	  fDPMSCaps(B_DPMS_ON)
{
	fDisplayMode.virtual_width = 640;
	fDisplayMode.virtual_height = 480;
	fDisplayMode.space = B_RGBA32;
}


/*!
	\brief Does nothing
*/
DisplayDriver::~DisplayDriver()
{
}

/*!
	\brief Initializes the driver object.
	\return true if successful, false if not
	
	Initializes the CursorHandler with the default system cursor.

	Derived versions should set up the driver for display,
	including the initial clearing of the screen. If things
	do not go as they should, false should be returned.
*/
bool
DisplayDriver::Initialize()
{
	SetCursor(new ServerCursor(default_cursor_data));
	return true;
}

/*!
	\brief Shuts down the driver's video subsystem
	
	Any work done by Initialize() should be undone here. Note that Shutdown() is
	called even if Initialize() was unsuccessful.
*/
void
DisplayDriver::Shutdown()
{
}

// Protected Internal Functions
/*
	\brief Sets the screen mode to specified resolution and color depth.
	\param mode Data structure as defined in Screen.h
	
	Subclasses must include calls to _SetDepth, _SetHeight, _SetWidth, and _SetMode
	to update the state variables kept internally by the DisplayDriver class.
*/
void
DisplayDriver::SetMode(const display_mode &mode)
{
	fDisplayMode = mode;
}

// GetMode
void
DisplayDriver::GetMode(display_mode *mode)
{
	if (!mode)
		return;
	
	Lock();
	*mode = fDisplayMode;
	Unlock();
}

/*!
	\brief Sets the driver's Display Power Management System state
	\param state The state which the driver should enter
	\return B_OK if successful, B_ERROR for failure
	
	This function will fail if the driver's rendering context does not support a 
	particular DPMS state. Use DPMSCapabilities to find out the supported states.
	The default implementation supports only B_DPMS_ON.
*/
status_t
DisplayDriver::SetDPMSMode(const uint32 &state)
{
	if (state != B_DPMS_ON)
		return B_ERROR;

	fDPMSState = state;

	return B_OK;
}

/*!
	\brief Returns the driver's current DPMS state
	\return The driver's current DPMS state
*/
uint32
DisplayDriver::DPMSMode() const
{
	return fDPMSState;
}

/*!
	\brief Returns the driver's DPMS capabilities
	\return The driver's DPMS capabilities
	
	The capabilities are the modes supported by the driver. The default implementation 
	allows only B_DPMS_ON. Other possible states are B_DPMS_STANDBY, SUSPEND, and OFF.
*/
uint32
DisplayDriver::DPMSCapabilities() const
{
	return fDPMSCaps;
}

