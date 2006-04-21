/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */

/*!
	Handler for allocating and freeing area memory for BBitmaps 
	on the server side. Utilizes the BGET pool allocator.

	Whenever a ServerBitmap associated with a client-side BBitmap needs to be 
	created or destroyed, the BitmapManager needs to handle it. It takes care of 
	all memory management related to them.
*/


#include "BitmapManager.h"

#include "ClientMemoryAllocator.h"
#include "HWInterface.h"
#include "ServerBitmap.h"
#include "ServerProtocol.h"
#include "ServerTokenSpace.h"

#include <video_overlay.h>

#include <Autolock.h>
#include <Bitmap.h>

#include <new>
#include <stdio.h>
#include <string.h>

using std::nothrow;


//! The bitmap allocator for the server. Memory is allocated/freed by the AppServer class
BitmapManager *gBitmapManager = NULL;

//! Number of bytes to allocate to each area used for bitmap storage
#define BITMAP_AREA_SIZE	B_PAGE_SIZE * 16


//! Sets up stuff to be ready to allocate space for bitmaps
BitmapManager::BitmapManager()
	:
	fBitmapList(1024),
	fLock("BitmapManager Lock")
{
}


//! Deallocates everything associated with the manager
BitmapManager::~BitmapManager()
{
	int32 count = fBitmapList.CountItems();
	for (int32 i = 0; i < count; i++) {
		if (ServerBitmap* bitmap = (ServerBitmap*)fBitmapList.ItemAt(i)) {
			if (bitmap->AllocationCookie() != NULL)
				debugger("We're not supposed to keep our cookies...");

			delete bitmap;
		}
	}
}


/*!
	\brief Allocates a new ServerBitmap.
	\param bounds Size of the bitmap
	\param space Color space of the bitmap
	\param flags Bitmap flags as defined in Bitmap.h
	\param bytesPerRow Number of bytes per row.
	\param screen Screen id of the screen associated with it. Unused.
	\return A new ServerBitmap or NULL if unable to allocate one.
*/
ServerBitmap*
BitmapManager::CreateBitmap(ClientMemoryAllocator* allocator, HWInterface& hwInterface,
	BRect bounds, color_space space, int32 flags, int32 bytesPerRow, screen_id screen,
	int8* _allocationType)
{
	BAutolock locker(fLock);

	if (!locker.IsLocked())
		return NULL;

// TODO: create an overlay bitmap if graphics card supports it
	if (flags & B_BITMAP_WILL_OVERLAY) {
		if (!hwInterface.WriteLock()
			|| !hwInterface.CheckOverlayRestrictions(bounds.IntegerWidth() + 1,
					bounds.IntegerHeight() + 1, space)) {
			hwInterface.WriteUnlock();
			return NULL;
		}

		// We now hold the HWInterface write lock!
		// Keeping the interface locked makes sure the overlay is still
		// available when we allocate the buffer
	}

	ServerBitmap* bitmap = new(nothrow) ServerBitmap(bounds, space, flags, bytesPerRow);
	if (bitmap == NULL) {
		if (flags & B_BITMAP_WILL_OVERLAY)
			hwInterface.WriteUnlock();

		return NULL;
	}

	void* cookie = NULL;
	uint8* buffer = NULL;

	if (flags & B_BITMAP_WILL_OVERLAY) {
		OverlayCookie* overlayCookie = new (std::nothrow) OverlayCookie(hwInterface);
		const overlay_buffer* overlayBuffer = NULL;

		if (overlayCookie != NULL) {
			overlayBuffer = hwInterface.AllocateOverlayBuffer(bitmap->Width(),
				bitmap->Height(), space);
		}

		hwInterface.WriteUnlock();

		if (overlayBuffer != NULL) {
			overlayCookie->SetOverlayBuffer(overlayBuffer);
			bitmap->fAllocationCookie = overlayCookie;
			bitmap->fBytesPerRow = overlayBuffer->bytes_per_row;

			buffer = (uint8*)overlayBuffer->buffer;
			if (_allocationType)
				*_allocationType = kFramebuffer;
		} else
			delete overlayCookie;
	} else if (allocator != NULL) {
		bool newArea;
		cookie = allocator->Allocate(bitmap->BitsLength(), (void**)&buffer, newArea);
		if (cookie != NULL) {
			bitmap->fAllocator = allocator;
			bitmap->fAllocationCookie = cookie;

			if (_allocationType)
				*_allocationType = newArea ? kNewAllocatorArea : kAllocator;
		}
	} else {
		buffer = (uint8*)malloc(bitmap->BitsLength());
		if (buffer != NULL) {
			bitmap->fAllocator = NULL;
			bitmap->fAllocationCookie = NULL;

			if (_allocationType)
				*_allocationType = kHeap;
		}
	}

	if (buffer && fBitmapList.AddItem(bitmap)) {
		bitmap->fBuffer = buffer;
		bitmap->fToken = gTokenSpace.NewToken(kBitmapToken, bitmap);
		bitmap->fInitialized = true;

		if (flags & B_BITMAP_CLEAR_TO_WHITE) {
			if (space == B_CMAP8)
				// "255" is the "transparent magic" index for B_CMAP8 bitmaps
				memset(bitmap->Bits(), 65, bitmap->BitsLength());
			else
				// should work for most colorspaces
				memset(bitmap->Bits(), 255, bitmap->BitsLength());
		}
	} else {
		// Allocation failed for buffer or bitmap list
		delete bitmap;
		bitmap = NULL;
	}

	return bitmap;
}


/*!
	\brief Deletes a ServerBitmap.
	\param bitmap The bitmap to delete
*/
void
BitmapManager::DeleteBitmap(ServerBitmap* bitmap)
{
	if (bitmap == NULL || !bitmap->_Release()) {
		// there are other references to this bitmap, we don't have to delete it yet
		return;
	}

	BAutolock locker(fLock);
	if (!locker.IsLocked())
		return;

	if (fBitmapList.RemoveItem(bitmap))
		delete bitmap;
}
