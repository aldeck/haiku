/*****************************************************************************/
// BeUtils.cpp
//
// Version: 1.0.0d1
//
// Several utilities for writing applications for the BeOS. It are small
// very specific functions, but generally useful (could be here because of a
// lack in the APIs, or just sheer lazyness :))
//
// Author
//   Ithamar R. Adema
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <Application.h>
#include <Bitmap.h>
#include <Messenger.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>

#include "BeUtils.h"


// ---------------------------------------------------------------
// TestForAddonExistence
//
// [Method Description]
//
// Parameters:
//
// Returns:
// ---------------------------------------------------------------
status_t TestForAddonExistence(const char* name, directory_which which, const char* section, BPath& outPath)
{
	status_t err = B_OK;
	
	if ((err=find_directory(which, &outPath)) == B_OK &&
		(err=outPath.Append(section)) == B_OK &&
		(err=outPath.Append(name)) == B_OK)
	{
		struct stat buf;
		err = stat(outPath.Path(), &buf);
	}
	
	return err;
}

// Implementation of AutoReply

AutoReply::AutoReply(BMessage* sender, uint32 what)
	: fSender(sender)
	, fReply(what) 
{
}

AutoReply::~AutoReply() {
	fSender->SendReply(&fReply);
	delete fSender;
}

bool MimeTypeForSender(BMessage* sender, BString& mime) {
	BMessenger msgr = sender->ReturnAddress();
	team_id team = msgr.Team();
	app_info info;
	if (be_roster->GetRunningAppInfo(team, &info) == B_OK) {
		mime = info.signature;
		return true;
	}
	return false;
}

bool AddFields(BMessage* to, const BMessage* from) {
	char* name;
	uint32 type;
	int32 count;
	for (int32 i = 0; from->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i ++) {
		// replace existing data
		to->RemoveName(name);
		
		const void* data;
		ssize_t size;
		for (int32 i = 0; i < count; i ++) {
			if (from->FindData(name, type, i, &data, &size) == B_OK) {
				to->AddData(name, type, data, size);
			}
		}
	}
}

BBitmap* LoadBitmap(const char* name, uint32 type_code = B_TRANSLATOR_BITMAP) {
	if (type_code == B_TRANSLATOR_BITMAP) {
		return BTranslationUtils::GetBitmap(type_code, name);
	} else {
		BResources *res = BApplication::AppResources();
		if (res != NULL) {
			BMessage m;
			size_t length;
			const void *bits = res->LoadResource(type_code, name, &length);
			if (bits && m.Unflatten((char*)bits) == B_OK) {
				return (BBitmap*)BBitmap::Instantiate(&m);
			}
		}
		return NULL;
	}
}

BPicture *BitmapToPicture(BView* view, BBitmap *bitmap) {
	if (bitmap) {
		view->BeginPicture(new BPicture());
		view->DrawBitmap(bitmap);
		return view->EndPicture();
	}
	return NULL;
}

BPicture *BitmapToGrayedPicture(BView* view, BBitmap *bitmap) {
	if (bitmap) {
		BRect rect(bitmap->Bounds());
		view->BeginPicture(new BPicture());
		view->DrawBitmap(bitmap);
		view->SetHighColor(255, 255, 255, 128);
		view->SetDrawingMode(B_OP_ALPHA);
		view->FillRect(rect);
		return view->EndPicture();
	}
	return NULL;
}
