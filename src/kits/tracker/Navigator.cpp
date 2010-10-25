/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#include "Bitmaps.h"
#include "Commands.h"
#include "ContainerWindow.h"
#include "FSUtils.h"
#include "IconButton.h"
#include "Model.h"
#include "Navigator.h"
#include "Tracker.h"

#include <ControlLook.h>
#include <GroupLayoutBuilder.h>
#include <Region.h>
#include <Window.h>
#include <Picture.h>
#include <TextControl.h>

namespace BPrivate {

static const int32 kMaxHistory = 32;

}


BNavigator::BNavigator(const Model *model)
	:	
	BView("Navigator", B_WILL_DRAW | B_DRAW_ON_CHILDREN),
	fBackButton(NULL),
	fForwardButton(NULL),
	fUpButton(NULL),
	fBackHistory(8, true),
	fForwHistory(8, true)
{
	// Get initial path
	model->GetPath(&fPath);
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// Back, Forward & Up
	fBackButton = new IconButton("Back", 0, 32, NULL,
		new BMessage(kNavigatorCommandBackward), this);
	fBackButton->SetIcon(201);
	fBackButton->TrimIcon();

	fForwardButton = new IconButton("Forward", 0, 32, NULL,
		new BMessage(kNavigatorCommandForward), this);
	fForwardButton->SetIcon(202);
	fForwardButton->TrimIcon();

	fUpButton = new IconButton("Up", 0, 32, NULL,
		new BMessage(kNavigatorCommandUp), this);
	fUpButton->SetIcon(203);
	fUpButton->TrimIcon();

	fLocation = new BTextControl("Location", "", "",
		new BMessage(kNavigatorCommandLocation));
	fLocation->SetDivider(50.0);

	// layout
	const float kInsetSpacing = 2;
	
	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_HORIZONTAL, 0.0f)
		.Add(fBackButton)
		.Add(fForwardButton)
		.Add(fUpButton)
		.Add(fLocation)
		.SetInsets(kInsetSpacing, kInsetSpacing, kInsetSpacing,	kInsetSpacing)
	);
}

BNavigator::~BNavigator()
{
}

void 
BNavigator::AttachedToWindow()
{	
	// Inital setup of widget states
	UpdateLocation(0, kActionSet);

	fLocation->SetTarget(this);
}

void 
BNavigator::Draw(BRect updateRect)
{
	BRegion clipper(updateRect);
	clipper.Exclude(fLocation->Frame());
	ConstrainClippingRegion(&clipper);
		// workaround, since we use B_DRAW_ON_CHILDREN to draw the gradient
		// also on the background of the buttons we need to avoid the fLocation
		// textfield as it doesn't blank some parts of itself properly.
		// TODO: either find a way to avoid using draw on children and achieve
		// the same effect and also look if we could do something in BTextField

	BRect bounds(Bounds());			

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	SetHighColor(tint_color(base, B_DARKEN_2_TINT));
	StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
	bounds.bottom--;

	be_control_look->DrawButtonBackground(this, bounds, bounds, base, 0,
		BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER);
}

void 
BNavigator::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kNavigatorCommandBackward:
			GoBackward((modifiers() & B_OPTION_KEY) == B_OPTION_KEY);
			break;

		case kNavigatorCommandForward:
			GoForward((modifiers() & B_OPTION_KEY) == B_OPTION_KEY);
			break;

		case kNavigatorCommandUp:
			GoUp((modifiers() & B_OPTION_KEY) == B_OPTION_KEY);
			break;

		case kNavigatorCommandLocation:
			GoTo();
			break;
				
		default:
			{
				// Catch any dropped refs and try 
				// to switch to this new directory
				entry_ref ref;
				if (message->FindRef("refs", &ref) == B_OK) {
					BMessage message(kSwitchDirectory);
					BEntry entry(&ref, true);
					if (!entry.IsDirectory()) {
						entry.GetRef(&ref);
						BPath path(&ref);
							path.GetParent(&path);
							get_ref_for_path(path.Path(), &ref);
					}
					message.AddRef("refs", &ref);
					message.AddInt32("action", kActionSet);
					Window()->PostMessage(&message);
				}
			}
	}
}

void 
BNavigator::GoBackward(bool option)
{
	int32 itemCount = fBackHistory.CountItems();
	if (itemCount >= 2 && fBackHistory.ItemAt(itemCount - 2)) {
		BEntry entry;
		if (entry.SetTo(fBackHistory.ItemAt(itemCount - 2)->Path()) == B_OK)
			SendNavigationMessage(kActionBackward, &entry, option);
	}
}

void 
BNavigator::GoForward(bool option)
{
	if (fForwHistory.CountItems() >= 1) {
		BEntry entry;
		if (entry.SetTo(fForwHistory.LastItem()->Path()) == B_OK)
			SendNavigationMessage(kActionForward, &entry, option);
	}
}

void 
BNavigator::GoUp(bool option)
{
	BEntry entry;
	if (entry.SetTo(fPath.Path()) == B_OK) {
		BEntry parentEntry;
		if (entry.GetParent(&parentEntry) == B_OK && !FSIsDeskDir(&parentEntry))
			SendNavigationMessage(kActionUp, &parentEntry, option);
	}
}

void
BNavigator::SendNavigationMessage(NavigationAction action, BEntry *entry, bool option)
{
	entry_ref ref;

	if (entry->GetRef(&ref) == B_OK) {
		BMessage message;
		message.AddRef("refs", &ref);
		message.AddInt32("action", action);
		
		// get the node of this folder for selecting it in the new location
		const node_ref *nodeRef;
		if (Window() && Window()->TargetModel())
			nodeRef = Window()->TargetModel()->NodeRef();
		else
			nodeRef = NULL;
		
		// if the option key was held down, open in new window (send message to be_app)
		// otherwise send message to this window. TTracker (be_app) understands nodeRefToSlection,
		// BContainerWindow doesn't, so we have to select the item manually
		if (option) {
			message.what = B_REFS_RECEIVED;
			if (nodeRef)
				message.AddData("nodeRefToSelect", B_RAW_TYPE, nodeRef, sizeof(node_ref));
			be_app->PostMessage(&message);
		} else {
			message.what = kSwitchDirectory;
			Window()->PostMessage(&message);
			UnlockLooper();
				// This is to prevent a dead-lock situation. SelectChildInParentSoon()
				// eventually locks the TaskLoop::fLock. Later, when StandAloneTaskLoop::Run()
				// runs, it also locks TaskLoop::fLock and subsequently locks this window's looper.
				// Therefore we can't call SelectChildInParentSoon with our Looper locked,
				// because we would get different orders of locking (thus the risk of dead-locking).
				//
				// Todo: Change the locking behaviour of StandAloneTaskLoop::Run() and sub-
				// sequently called functions.
			if (nodeRef)
				dynamic_cast<TTracker *>(be_app)->SelectChildInParentSoon(&ref, nodeRef);
			LockLooper();
		}
	}
}

void 
BNavigator::GoTo()
{
	BString pathname = fLocation->Text();

	if (pathname.Compare("") == 0)
		pathname = "/";

	BEntry entry;
	entry_ref ref;

	if (entry.SetTo(pathname.String()) == B_OK
		&& !FSIsDeskDir(&entry)
		&& entry.GetRef(&ref) == B_OK) {
		BMessage message(kSwitchDirectory);
		message.AddRef("refs", &ref);
		message.AddInt32("action", kActionLocation);
		Window()->PostMessage(&message);		
	} else {
		BPath path;
		
		if (Window()
			&& Window()->TargetModel()) {
			Window()->TargetModel()->GetPath(&path);
			fLocation->SetText(path.Path());
		}
	}
}

void 
BNavigator::UpdateLocation(const Model *newmodel, int32 action)
{
	if (newmodel)
		newmodel->GetPath(&fPath);


	// Modify history according to commands
	switch (action) {
		case kActionBackward:
			fForwHistory.AddItem(fBackHistory.RemoveItemAt(fBackHistory.CountItems()-1));
			break;
		case kActionForward:
			fBackHistory.AddItem(fForwHistory.RemoveItemAt(fForwHistory.CountItems()-1));
			break;
		case kActionUpdatePath:
			break;
		default:
			fForwHistory.MakeEmpty();
			fBackHistory.AddItem(new BPath(fPath));

			for (;fBackHistory.CountItems()>kMaxHistory;)
				fBackHistory.RemoveItem(fBackHistory.FirstItem(), true);
			break;			
	}

	// Enable Up button when there is any parent
	BEntry entry;
	if (entry.SetTo(fPath.Path()) == B_OK) {
		BEntry parentEntry;
		fUpButton->SetEnabled(entry.GetParent(&parentEntry) == B_OK
			&& !FSIsDeskDir(&parentEntry));
	}

	// Enable history buttons if history contains something
	fForwardButton->SetEnabled(fForwHistory.CountItems() > 0);
	fBackButton->SetEnabled(fBackHistory.CountItems() > 1);

	// Avoid loss of selection and cursor position
	if (action != kActionLocation)
		fLocation->SetText(fPath.Path());
}
