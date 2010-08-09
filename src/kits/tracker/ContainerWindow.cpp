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

#include <string.h>
#include <stdlib.h>
#include <image.h>

#include <Alert.h>
#include <Application.h>
#include <AppFileInfo.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <InterfaceDefs.h>
#include <LayoutItem.h>
#include <Locale.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <Roster.h>

#include <fs_attr.h>

#include <memory>

#include "Attributes.h"
#include "AttributeStream.h"
#include "AutoLock.h"
#include "BackgroundImage.h"
#include "Commands.h"
#include "ContainerWindow.h"
#include "CountView.h"
#include "DeskWindow.h"
#include "FavoritesMenu.h"
#include "FindPanel.h"
#include "FSClipboard.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"
#include "IconMenuItem.h"
#include "OpenWithWindow.h"
#include "MimeTypes.h"
#include "Model.h"
#include "MountMenu.h"
#include "Navigator.h"
#include "NavMenu.h"
#include "PoseView.h"
#include "PoseViewController.h"
#include "QueryContainerWindow.h"
#include "SelectionWindow.h"
#include "TitleView.h"
#include "Tracker.h"
#include "TrackerSettings.h"
#include "Thread.h"
#include "TemplatesMenu.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "libtracker"

const uint32 kRedo = 'REDO';
	// this is the same as B_REDO in Dano/Zeta/Haiku


#ifdef _IMPEXP_BE
_IMPEXP_BE
#endif
void do_minimize_team(BRect zoomRect, team_id team, bool zoom);

// Amount you have to move the mouse before a drag starts
const float kDragSlop = 3.0f;

namespace BPrivate {

class DraggableContainerIcon : public BView {
	public:
		DraggableContainerIcon(BRect rect, const char *name, uint32 resizeMask);

		virtual void AttachedToWindow();
		virtual void MouseDown(BPoint where);
		virtual void MouseUp(BPoint where);
		virtual void MouseMoved(BPoint point, uint32 /*transit*/, const BMessage *message);
		virtual void FrameMoved(BPoint newLocation);
		virtual void Draw(BRect updateRect);

	private:
		uint32	fDragButton;
		BPoint	fClickPoint;
		bool	fDragStarted;
};

}	// namespace BPrivate

struct AddOneAddonParams {
	BObjectList<BMenuItem> *primaryList;
	BObjectList<BMenuItem> *secondaryList;
};

struct StaggerOneParams {
	bool rectFromParent;
};

const int32 kContainerWidthMinLimit = 120;
const int32 kContainerWindowHeightLimit = 85;

const int32 kWindowStaggerBy = 17;

BRect BContainerWindow::sNewWindRect(85, 50, 415, 280);


namespace BPrivate {

filter_result
ActivateWindowFilter(BMessage *, BHandler **target, BMessageFilter *)
{
	BView *view = dynamic_cast<BView*>(*target);

	// activate the window if no PoseView or DraggableContainerIcon had been pressed
	// (those will activate the window themselves, if necessary)
	if (view
		&& !dynamic_cast<BPoseView*>(view)
		&& !dynamic_cast<DraggableContainerIcon*>(view)
		&& view->Window())
		view->Window()->Activate(true);

	return B_DISPATCH_MESSAGE;
}


static void
StripShortcut(const Model *model, char *result, uint32 &shortcut)
{
	strcpy(result, model->Name());

	// check if there is a shortcut
	uint32 length = strlen(result);
	shortcut = '\0';
	if (result[length - 2] == '-') {
		shortcut = result[length - 1];
		result[length - 2] = '\0';
	}
}


static const Model *
MatchOne(const Model *model, void *castToName)
{
	char buffer[B_FILE_NAME_LENGTH];
	uint32 dummy;
	StripShortcut(model, buffer, dummy);

	if (strcmp(buffer, (const char *)castToName) == 0) {
		// found match, bail out
		return model;
	}

	return 0;
}


int
CompareLabels(const BMenuItem *item1, const BMenuItem *item2)
{
	return strcasecmp(item1->Label(), item2->Label());
}

}	// namespace BPrivate


static bool
AddOneAddon(const Model *model, const char *name, uint32 shortcut, bool primary, void *context)
{
	AddOneAddonParams *params = (AddOneAddonParams *)context;

	BMessage *message = new BMessage(kLoadAddOn);
	message->AddRef("refs", model->EntryRef());

	ModelMenuItem *item = new ModelMenuItem(model, name, message,
		(char)shortcut, B_OPTION_KEY);

	if (primary)
		params->primaryList->AddItem(item);
	else
		params->secondaryList->AddItem(item);

	return false;
}


static int32
AddOnThread(BMessage *refsMessage, entry_ref addonRef, entry_ref dirRef)
{
	std::auto_ptr<BMessage> refsMessagePtr(refsMessage);

	BEntry entry(&addonRef);
	BPath path;
	status_t result = entry.InitCheck();
	if (result == B_OK)
		result = entry.GetPath(&path);

	if (result == B_OK) {
		image_id addonImage = load_add_on(path.Path());
		if (addonImage >= 0) {
			void (*processRefs)(entry_ref, BMessage *, void *);
			result = get_image_symbol(addonImage, "process_refs", 2, (void **)&processRefs);

#ifndef __INTEL__
			if (result < 0) {
				PRINT(("trying old legacy ppc signature\n"));
				// try old-style addon signature
				result = get_image_symbol(addonImage,
					"process_refs__F9entry_refP8BMessagePv", 2, (void **)&processRefs);
			}
#endif

			if (result >= 0) {

				// call add-on code
				(*processRefs)(dirRef, refsMessagePtr.get(), 0);

				unload_add_on(addonImage);
				return B_OK;
			} else
				PRINT(("couldn't find process_refs\n"));

			unload_add_on(addonImage);
		} else
			result = addonImage;
	}

	BString buffer(B_TRANSLATE("Error %error loading Add-On %name."));
	buffer.ReplaceFirst("%error", strerror(result));
	buffer.ReplaceFirst("%name", addonRef.name);

	BAlert* alert = new BAlert("", buffer.String(),	B_TRANSLATE("Cancel"), 0, 0,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	alert->Go();

	return result;
}


static bool
NodeHasSavedState(const BNode *node)
{
	attr_info info;
	return node->GetAttrInfo(kAttrWindowFrame, &info) == B_OK;
}


static bool
OffsetFrameOne(const char *DEBUG_ONLY(name), uint32, off_t, void *castToRect,
	void *castToParams)
{
	ASSERT(strcmp(name, kAttrWindowFrame) == 0);
	StaggerOneParams *params = (StaggerOneParams *)castToParams;

	if (!params->rectFromParent)
		return false;

	if (!castToRect)
		return false;

	((BRect *)castToRect)->OffsetBy(kWindowStaggerBy, kWindowStaggerBy);
	return true;
}


static void
AddMimeTypeString(BObjectList<BString> &list, Model *model)
{
	BString *mimeType = new BString(model->MimeType());
	if (!mimeType->Length() || !mimeType->ICompare(B_FILE_MIMETYPE)) {
		// if model is of unknown type, try mimeseting it first
		model->Mimeset(true);
		mimeType->SetTo(model->MimeType());
	}

	if (mimeType->Length()) {
		// only add the type if it's not already there
		for (int32 i = list.CountItems(); i-- > 0;) {
			BString *string = list.ItemAt(i);
			if (string != NULL && !string->ICompare(*mimeType)) {
				delete mimeType;
				return;
			}
		}
		list.AddItem(mimeType);
	}
}


//	#pragma mark -


DraggableContainerIcon::DraggableContainerIcon(BRect rect, const char *name,
	uint32 resizeMask)
	: BView(rect, name, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS),
	fDragButton(0),
	fDragStarted(false)
{
}


void
DraggableContainerIcon::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
	FrameMoved(BPoint(0, 0));
		// this decides whether to hide the icon or not
}


void
DraggableContainerIcon::MouseDown(BPoint point)
{
	// we only like container windows
	BContainerWindow *window = dynamic_cast<BContainerWindow *>(Window());
	if (window == NULL)
		return;

	// we don't like the Trash icon (because it cannot be moved)
	if (window->PoseView()->TargetModel()->IsTrash() || window->PoseView()->TargetModel()->IsPrintersDir())
		return;

	uint32 buttons;
	window->CurrentMessage()->FindInt32("buttons", (int32 *)&buttons);

	if (IconCache::sIconCache->IconHitTest(point, window->TargetModel(),
			kNormalIcon, B_MINI_ICON)) {
		// The click hit the icon, initiate a drag
		fDragButton = buttons & (B_PRIMARY_MOUSE_BUTTON | B_SECONDARY_MOUSE_BUTTON);
		fDragStarted = false;
		fClickPoint = point;
	} else
		fDragButton = 0;

	if (!fDragButton)
		Window()->Activate(true);
}


void
DraggableContainerIcon::MouseUp(BPoint /*point*/)
{
	if (!fDragStarted)
		Window()->Activate(true);

	fDragButton = 0;
	fDragStarted = false;
}


void
DraggableContainerIcon::MouseMoved(BPoint point, uint32 /*transit*/,
	const BMessage */*message*/)
{
	if (fDragButton == 0 || fDragStarted
		|| (abs((int32)(point.x - fClickPoint.x)) <= kDragSlop
			&& abs((int32)(point.y - fClickPoint.y)) <= kDragSlop))
		return;

	BContainerWindow *window = static_cast<BContainerWindow *>(Window());
		// we can only get here in a BContainerWindow
	Model *model = window->TargetModel();

	// Find the required height
	BFont font;
	GetFont(&font);

	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float height = fontHeight.ascent + fontHeight.descent + fontHeight.leading + 2
		+ Bounds().Height() + 8;

	BRect rect(0, 0, max_c(Bounds().Width(), font.StringWidth(model->Name()) + 4), height);
	BBitmap *dragBitmap = new BBitmap(rect, B_RGBA32, true);

	dragBitmap->Lock();
	BView *view = new BView(dragBitmap->Bounds(), "", B_FOLLOW_NONE, 0);
	dragBitmap->AddChild(view);
	view->SetOrigin(0, 0);
	BRect clipRect(view->Bounds());
	BRegion newClip;
	newClip.Set(clipRect);
	view->ConstrainClippingRegion(&newClip);

	// Transparent draw magic
	view->SetHighColor(0, 0, 0, 0);
	view->FillRect(view->Bounds());
	view->SetDrawingMode(B_OP_ALPHA);
	view->SetHighColor(0, 0, 0, 128);	// set the level of transparency by  value
	view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);

	// Draw the icon
	float hIconOffset = (rect.Width() - Bounds().Width()) / 2;
	IconCache::sIconCache->Draw(model, view, BPoint(hIconOffset, 0),
		kNormalIcon, B_MINI_ICON, true);

	// See if we need to truncate the string
	BString nameString = model->Name();
	if (view->StringWidth(model->Name()) > rect.Width())
		view->TruncateString(&nameString, B_TRUNCATE_END, rect.Width() - 5);

	// Draw the label
	float leftText = (view->StringWidth(nameString.String()) - Bounds().Width()) / 2;
	view->MovePenTo(BPoint(hIconOffset - leftText + 2, Bounds().Height() + (fontHeight.ascent + 2)));
	view->DrawString(nameString.String());

	view->Sync();
	dragBitmap->Unlock();

	BMessage message(B_SIMPLE_DATA);
	message.AddRef("refs", model->EntryRef());
	message.AddPoint("click_pt", fClickPoint);

	BPoint tmpLoc;
	uint32 button;
	GetMouse(&tmpLoc, &button);
	if (button)
		message.AddInt32("buttons", (int32)button);

	if (button & B_PRIMARY_MOUSE_BUTTON) {
		// add an action specifier to the message, so that it is not copied
		message.AddInt32("be:actions",
			(modifiers() & B_OPTION_KEY) != 0 ? B_COPY_TARGET : B_MOVE_TARGET);
	}

	fDragStarted = true;
	fDragButton = 0;

	DragMessage(&message, dragBitmap, B_OP_ALPHA,
		BPoint(fClickPoint.x + hIconOffset, fClickPoint.y), this);
}


void
DraggableContainerIcon::FrameMoved(BPoint /*newLocation*/)
{
	BMenuBar* bar = dynamic_cast<BMenuBar *>(Parent());
	if (bar == NULL)
		return;

	// TODO: ugly hack following:
	// This is a trick to get the actual width of all menu items
	// (BMenuBar may not have set the item coordinates yet...)
	float width, height;
	uint32 resizingMode = bar->ResizingMode();
	bar->SetResizingMode(B_FOLLOW_NONE);
	bar->GetPreferredSize(&width, &height);
	bar->SetResizingMode(resizingMode);

/*
	BMenuItem* item = bar->ItemAt(bar->CountItems() - 1);
	if (item == NULL)
		return;
*/
	// BeOS shifts the coordinates for hidden views, so we cannot
	// use them to decide if we should be visible or not...

	float gap = bar->Frame().Width() - 2 - width; //item->Frame().right;

	if (gap <= Bounds().Width() && !IsHidden())
		Hide();
	else if (gap > Bounds().Width() && IsHidden())
		Show();
}


void
DraggableContainerIcon::Draw(BRect updateRect)
{
	BContainerWindow *window = dynamic_cast<BContainerWindow *>(Window());
	if (window == NULL)
		return;

	if (be_control_look != NULL) {
		BRect rect(Bounds());
		rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);
		be_control_look->DrawMenuBarBackground(this, rect, updateRect, base,
			0, 0);
	}

	// Draw the icon, straddling the border
#ifdef __HAIKU__
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
#else
	SetDrawingMode(B_OP_OVER);
#endif
	float iconOffset = (Bounds().Width() - B_MINI_ICON) / 2;
	IconCache::sIconCache->Draw(window->TargetModel(), this,
		BPoint(iconOffset, iconOffset), kNormalIcon, B_MINI_ICON, true);
}


//	#pragma mark -


BContainerWindow::BContainerWindow(Model* model, 
	LockingList<BWindow> *list,
	uint32 containerWindowFlags,
	window_look look, window_feel feel, uint32 flags, uint32 workspace)
	:
	BWindow(InitialWindowRect(feel), "TrackerWindow", look, feel,
		flags | B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS, workspace),
	fCreationModel(model),
	fFileContextMenu(NULL),
	fWindowContextMenu(NULL),
	fDropContextMenu(NULL),
	fVolumeContextMenu(NULL),
	fDragContextMenu(NULL),
	fMoveToItem(NULL),
	fCopyToItem(NULL),	
	fCreateLinkItem(NULL),
	fOpenWithItem(NULL),
	fNavigationItem(NULL),
	fController(NULL),
	fPoseView(NULL),
	fWindowList(list),
	fSelectionWindow(NULL),
	fTaskLoop(NULL),
	fContainerWindowFlags(containerWindowFlags),
	fBackgroundImage(NULL),
	fSavedZoomRect(0, 0, -1, -1),
	fContextMenu(NULL),
	fDragMessage(NULL),
	fCachedTypesList(NULL),
	fStateNeedsSaving(false),
	fSaveStateIsEnabled(true),
	fIsWatchingPath(false)
{
	InitIconPreloader();

	if (list) {
		ASSERT(list->IsLocked());
		list->AddItem(this);
	}

	AddCommonFilter(new BMessageFilter(B_MOUSE_DOWN, ActivateWindowFilter));

	Run();

	// Watch out for settings changes:
	if (TTracker *app = dynamic_cast<TTracker*>(be_app)) {
		app->Lock();
		app->StartWatching(this, kWindowsShowFullPathChanged);
		app->StartWatching(this, kSingleWindowBrowseChanged);
		app->StartWatching(this, kShowNavigatorChanged);
		app->StartWatching(this, kDontMoveFilesToTrashChanged);
		app->Unlock();
	}
}


BContainerWindow::~BContainerWindow()
{
	ASSERT(IsLocked());

	// stop the watchers
	if (TTracker *app = dynamic_cast<TTracker*>(be_app)) {
		app->Lock();
		app->StopWatching(this, kWindowsShowFullPathChanged);
		app->StopWatching(this, kSingleWindowBrowseChanged);
		app->StopWatching(this, kShowNavigatorChanged);
		app->StopWatching(this, kDontMoveFilesToTrashChanged);
		app->Unlock();
	}

	delete fTaskLoop;
	delete fBackgroundImage;
	delete fDragMessage;
	delete fCachedTypesList;

	if (fSelectionWindow && fSelectionWindow->Lock())
		fSelectionWindow->Quit();
		
	delete fController;
}


BRect
BContainerWindow::InitialWindowRect(window_feel feel)
{
	if (feel != kPrivateDesktopWindowFeel)
		return sNewWindRect;

	// do not offset desktop window
	BRect result = sNewWindRect;
	result.OffsetTo(0, 0);
	return result;
}


void
BContainerWindow::Minimize(bool minimize)
{
	if (minimize && (modifiers() & B_OPTION_KEY) != 0)
		do_minimize_team(BRect(0, 0, 0, 0), be_app->Team(), true);
	else
		_inherited::Minimize(minimize);
}


bool
BContainerWindow::QuitRequested()
{
	// this is a response to the DeskBar sending us a B_QUIT, when it really
	// means to say close all your windows. It might be better to have it
	// send a kCloseAllWindows message and have windowless apps stay running,
	// which is what we will do for the Tracker
	if (CurrentMessage()
		&& (CurrentMessage()->FindInt32("modifiers") & B_CONTROL_KEY))
		be_app->PostMessage(kCloseAllWindows);

	Hide();
		// this will close the window instantly, even if
		// the file system is very busy right now
	return true;
}


void
BContainerWindow::Quit()
{
	// get rid of context menus
	if (fNavigationItem) {
		BMenu *menu = fNavigationItem->Menu();
		if (menu)
			menu->RemoveItem(fNavigationItem);
		delete fNavigationItem;
		fNavigationItem = NULL;
	}

	if (fOpenWithItem && !fOpenWithItem->Menu()) {
		delete fOpenWithItem;
		fOpenWithItem = NULL;
	}

	if (fMoveToItem && !fMoveToItem->Menu()) {
		delete fMoveToItem;
		fMoveToItem = NULL;
	}

	if (fCopyToItem && !fCopyToItem->Menu()) {
		delete fCopyToItem;
		fCopyToItem = NULL;
	}

	if (fCreateLinkItem && !fCreateLinkItem->Menu()) {
		delete fCreateLinkItem;
		fCreateLinkItem = NULL;
	}

	if (Controller()->AttributeMenu() && !Controller()->AttributeMenu()->Supermenu()) {
		// TODO delete Controller()->AttributeMenu();
		//Controller()->AttributeMenu() = NULL;
	}

	delete fFileContextMenu;
	fFileContextMenu = NULL;
	delete fWindowContextMenu;
	fWindowContextMenu = NULL;
	delete fDropContextMenu;
	fDropContextMenu = NULL;
	delete fVolumeContextMenu;
	fVolumeContextMenu = NULL;
	delete fDragContextMenu;
	fDragContextMenu = NULL;

	int32 windowCount = 0;

	// This is a deadlock code sequence - need to change this
	// to acquire the window list while this container window is unlocked
	if (fWindowList) {
		AutoLock<LockingList<BWindow> > lock(fWindowList);
		if (lock.IsLocked()) {
			fWindowList->RemoveItem(this);
			windowCount = fWindowList->CountItems();
		}
	}

	if (StateNeedsSaving())
		SaveState();

	if (fWindowList && windowCount == 0)
		be_app->PostMessage(B_QUIT_REQUESTED);

	_inherited::Quit();
}


void
BContainerWindow::AddContextMenus()
{
	// create context sensitive menus
	fFileContextMenu = new BPopUpMenu("FileContext", false, false);
	fFileContextMenu->SetFont(be_plain_font);
	AddFileContextMenus(fFileContextMenu);

	fVolumeContextMenu = new BPopUpMenu("VolumeContext", false, false);
	fVolumeContextMenu->SetFont(be_plain_font);
	AddVolumeContextMenus(fVolumeContextMenu);

	fWindowContextMenu = new BPopUpMenu("WindowContext", false, false);
	fWindowContextMenu->SetFont(be_plain_font);
	AddWindowContextMenus(fWindowContextMenu);

	fDropContextMenu = new BPopUpMenu("DropContext", false, false);
	fDropContextMenu->SetFont(be_plain_font);
	AddDropContextMenus(fDropContextMenu);

	fDragContextMenu = new BSlowContextMenu("DragContext");
		// will get added and built dynamically in ShowContextMenu

	fTrashContextMenu = new BPopUpMenu("TrashContext", false, false);
	fTrashContextMenu->SetFont(be_plain_font);
	AddTrashContextMenus(fTrashContextMenu);
}


void
BContainerWindow::RepopulateMenus()
{
	// Avoid these menus to be destroyed:
	if (fMoveToItem && fMoveToItem->Menu())
		fMoveToItem->Menu()->RemoveItem(fMoveToItem);

	if (fCopyToItem && fCopyToItem->Menu())
		fCopyToItem->Menu()->RemoveItem(fCopyToItem);

	if (fCreateLinkItem && fCreateLinkItem->Menu())
		fCreateLinkItem->Menu()->RemoveItem(fCreateLinkItem);

	if (fOpenWithItem && fOpenWithItem->Menu()) {
		fOpenWithItem->Menu()->RemoveItem(fOpenWithItem);
		delete fOpenWithItem;
		fOpenWithItem = NULL;
	}

	if (fNavigationItem) {
		BMenu *menu = fNavigationItem->Menu();
		if (menu) {
			menu->RemoveItem(fNavigationItem);
			BMenuItem *item = menu->RemoveItem((int32)0);
			ASSERT(item != fNavigationItem);
			delete item;
		}
	}

	delete fFileContextMenu;
	fFileContextMenu = new BPopUpMenu("FileContext", false, false);
	fFileContextMenu->SetFont(be_plain_font);
	AddFileContextMenus(fFileContextMenu);

	delete fWindowContextMenu;
	fWindowContextMenu = new BPopUpMenu("WindowContext", false, false);
	fWindowContextMenu->SetFont(be_plain_font);
	AddWindowContextMenus(fWindowContextMenu);

	// TODO: disabled until menu system rework
	/*Controller()->MenuBar()->RemoveItem(Controller()->FileMenu());
	delete Controller()->FileMenu();
	///Controller()->FileMenu() = new BMenu(B_TRANSLATE("File"));
	Controller()->AddFileMenu(Controller()->FileMenu());
	Controller()->MenuBar()->AddItem(Controller()->FileMenu());

	Controller()->MenuBar()->RemoveItem(Controller()->WindowMenu());
	delete Controller()->WindowMenu();
	//Controller()->WindowMenu() = new BMenu(B_TRANSLATE("Window"));
	Controller()->MenuBar()->AddItem(Controller()->WindowMenu());
	Controller()->AddWindowMenu(Controller()->WindowMenu());

	// just create the attribute, decide to add it later
	Controller()->MenuBar()->RemoveItem(Controller()->AttributeMenu());
	delete Controller()->AttributeMenu();
	//Controller()->AttributeMenu() = new BMenu(B_TRANSLATE("Attributes"));
	Controller()->NewAttributeMenu(Controller()->AttributeMenu());
	if (PoseView()->ViewMode() == kListMode)
		Controller()->ShowAttributeMenu();*/

	int32 selectCount = PoseView()->SelectionList()->CountItems();

	SetupOpenWithMenu(Controller()->FileMenu());
	SetupMoveCopyMenus(selectCount
			? PoseView()->SelectionList()->FirstItem()->TargetModel()->EntryRef() : NULL,
		Controller()->FileMenu());
}


void
BContainerWindow::_Init(const BMessage *message)
{
	printf("(%p) BBrowserWindow::_Init \n", this);
	
	AutoLock<BWindow> lock(this);
	if (!lock)
		return;

	// create controls
	fPoseView = new BPoseView(fCreationModel, kListMode);
	fController = new PoseViewController();
	
	Controller()->SetPoseView(fPoseView);
	Controller()->CreateControls(fCreationModel);
	
	// layout controls
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(Controller()->MenuBar())
		.Add(Controller()->Navigator())
		.Add(Controller()->TitleView())
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(Controller()->PoseView())
			.Add(Controller()->VerticalScrollBar())
		)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.Add(Controller()->CountView())
			.Add(Controller()->HorizontalScrollBar(), 3.0f)
			.SetInsets(0, 0, B_V_SCROLL_BAR_WIDTH, 0)
				// avoid the window's resize handle
		)
	);
	
	// deal with new unconfigured folders
	if (NeedsDefaultStateSetup())
		SetUpDefaultState();
		
	fMoveToItem = new BMenuItem(new BNavMenu(B_TRANSLATE("Move to"),
		kMoveSelectionTo, this));
	fCopyToItem = new BMenuItem(new BNavMenu(B_TRANSLATE("Copy to"),
		kCopySelectionTo, this));
	fCreateLinkItem = new BMenuItem(new BNavMenu(B_TRANSLATE("Create link"),
		kCreateLink, this), new BMessage(kCreateLink));

	if (message)
		RestoreState(*message);
	else
		RestoreState();		

	Controller()->AddMenus();
	AddContextMenus();
	AddCommonShortcuts();
	
	CheckScreenIntersect();
		// check window frame TODO: should be done after/in restorewindowstate
	
	Controller()->TitleView()->Reset();
		// TODO find a more elegant way for the titleview to get updates
	
	if (PoseView()->ViewMode() == kListMode) {
		Controller()->ShowAttributeMenu();
		Controller()->MarkAttributeMenu();
	}
	
	TrackerSettings settings;	
	if (settings.SingleWindowBrowse()
		&& fCreationModel->IsDirectory()
		&& !fPoseView->IsFilePanel()) {
			
		Controller()->SetControlVisible(Controller()->Navigator(), settings.ShowNavigator());
	}
	
	Controller()->TitleView()->Reset();
		// TODO look for a more robust way for the titleview to get updates

	if (fBackgroundImage && PoseView()->ViewMode() != kListMode)
		fBackgroundImage->Show(PoseView(), current_workspace());

	// add folder icon to menu bar
	// TODO
	/*if (!TargetModel()->IsRoot() && !TargetModel()->IsTrash()) {
		float iconSize = 16;//Controller()->MenuBar()->Bounds().Height() - 2;
		//if (iconSize < 16)
		//	iconSize = 16;
		float iconPosY = 1 + (Controller()->MenuBar()->Bounds().Height() - 2 - iconSize) / 2;
		BView *icon = new DraggableContainerIcon(BRect(Bounds().Width() - 4 - iconSize + 1,
				iconPosY, Bounds().Width() - 4, iconPosY + iconSize - 1),
			"ThisContainer", B_FOLLOW_RIGHT);
		Controller()->MenuBar()->AddChild(icon);
	}*/		


	/* TODO 
	Controller()->SetScrollBarsEnabled(false);	
	BPoint origin;
	if (ViewMode() == kListMode)
		origin = fViewState->ListOrigin();
	else
		origin = fViewState->IconOrigin();

	ScrollTo(origin);*/
	Controller()->UpdateScrollRange();
	/*Controller()->SetScrollBarsTo(origin);
	Controller()->SetScrollBarsEnabled(true);*/

	Show();

	// done showing, turn the B_NO_WORKSPACE_ACTIVATION flag off;
	// it was on to prevent workspace jerking during boot
	SetFlags(Flags() & ~B_NO_WORKSPACE_ACTIVATION);
}


void
BContainerWindow::RestoreState()
{
	UpdateTitle();

	WindowStateNodeOpener opener(this, false);
	RestoreWindowState(opener.StreamNode());
	fPoseView->Init(opener.StreamNode());

	RestoreStateCommon();
}


void
BContainerWindow::RestoreState(const BMessage &message)
{
	UpdateTitle();

	RestoreWindowState(message);
	fPoseView->Init(message);

	RestoreStateCommon();
}


void
BContainerWindow::RestoreStateCommon()
{
	if (BootedInSafeMode())
		// don't pick up backgrounds in safe mode
		return;

	WindowStateNodeOpener opener(this, false);

	bool isDesktop = dynamic_cast<BDeskWindow *>(this);
	if (!TargetModel()->IsRoot() && opener.Node())
		// don't pick up background image for root disks
		// to do this, would have to have a unique attribute for the
		// disks window that doesn't collide with the desktop
		// for R4 this was not done to make things simpler
		// the default image will still work though
		fBackgroundImage = BackgroundImage::GetBackgroundImage(
			opener.Node(), isDesktop);
			// look for background image info in the window's node

	BNode defaultingNode;
	if (!fBackgroundImage && !isDesktop
		&& DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode))
		// look for background image info in the source for defaults
		fBackgroundImage = BackgroundImage::GetBackgroundImage(&defaultingNode, isDesktop);
}


void
BContainerWindow::UpdateTitle()
{
	// set title to full path, if necessary
	if (TrackerSettings().ShowFullPathInTitleBar()) {
		// use the Entry's full path
		BPath path;
		TargetModel()->GetPath(&path);
		SetTitle(path.Path());
	} else
		// use the default look
		SetTitle(TargetModel()->Name());

	// TODO: strange shouldn't be done here, more like in a ModelChanged hook
	if (Controller()->Navigator())
		Controller()->Navigator()->UpdateLocation(PoseView()->TargetModel(), kActionUpdatePath);
}


void
BContainerWindow::UpdateBackgroundImage()
{
	if (BootedInSafeMode())
		return;

	bool isDesktop = dynamic_cast<BDeskWindow *>(this) != NULL;
	WindowStateNodeOpener opener(this, false);

	if (!TargetModel()->IsRoot() && opener.Node())
		fBackgroundImage = BackgroundImage::Refresh(fBackgroundImage,
			opener.Node(), isDesktop, PoseView());

		// look for background image info in the window's node
	BNode defaultingNode;
	if (!fBackgroundImage && !isDesktop
		&& DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode))
		// look for background image info in the source for defaults
		fBackgroundImage = BackgroundImage::Refresh(fBackgroundImage,
			&defaultingNode, isDesktop, PoseView());
}


void
BContainerWindow::FrameResized(float, float)
{
	if (PoseView() && dynamic_cast<BDeskWindow *>(this) == NULL) {
		BRect extent = PoseView()->Extent();
		float offsetX = extent.left - PoseView()->Bounds().left;
		float offsetY = extent.top - PoseView()->Bounds().top;

		// scroll when the size augmented, there is a negative offset
		// and we have resized over the bottom right corner of the extent
		BPoint scroll(B_ORIGIN);
		if (offsetX < 0 && PoseView()->Bounds().right > extent.right
			&& Bounds().Width() > fPreviousBounds.Width())
			scroll.x
				= max_c(fPreviousBounds.Width() - Bounds().Width(), offsetX);

		if (offsetY < 0 && PoseView()->Bounds().bottom > extent.bottom
			&& Bounds().Height() > fPreviousBounds.Height())
			scroll.y
				= max_c(fPreviousBounds.Height() - Bounds().Height(), offsetY);

		if (scroll != B_ORIGIN)
			PoseView()->ScrollBy(scroll.x, scroll.y);

		Controller()->UpdateScrollRange();
		PoseView()->ResetPosePlacementHint();
	}

	fPreviousBounds = Bounds();
	fStateNeedsSaving = true;
}


void
BContainerWindow::FrameMoved(BPoint)
{
	fStateNeedsSaving = true;
}


void
BContainerWindow::WorkspacesChanged(uint32, uint32)
{
	fStateNeedsSaving = true;
}


void
BContainerWindow::ViewModeChanged(uint32 oldMode, uint32 newMode)
{
	BView *view = FindView("MenuBar");
	if (view != NULL) {
		// make sure the draggable icon hides if it doesn't have space left anymore
		view = view->FindView("ThisContainer");
		if (view != NULL)
			view->FrameMoved(view->Frame().LeftTop());
	}

	if (!fBackgroundImage)
		return;

	if (newMode == kListMode)
		fBackgroundImage->Remove();
	else if (oldMode == kListMode)
		fBackgroundImage->Show(PoseView(), current_workspace());
}


void
BContainerWindow::CheckScreenIntersect()
{
	BScreen screen(this);
	BRect screenFrame(screen.Frame());
	BRect frame(Frame());

	if (sNewWindRect.bottom > screenFrame.bottom)
		sNewWindRect.OffsetTo(85, 50);

	if (sNewWindRect.right > screenFrame.right)
		sNewWindRect.OffsetTo(85, 50);

	if (!frame.Intersects(screenFrame))
		MoveTo(sNewWindRect.LeftTop());
}


void
BContainerWindow::SaveState(bool hide)
{
	if (SaveStateIsEnabled()) {
		WindowStateNodeOpener opener(this, true);
		if (opener.StreamNode())
			SaveWindowState(opener.StreamNode());
		if (hide)
			Hide();
		if (opener.StreamNode())
			fPoseView->SaveState(opener.StreamNode());

		fStateNeedsSaving = false;
	}
}


void
BContainerWindow::SaveState(BMessage &message) const
{
	if (SaveStateIsEnabled()) {
		SaveWindowState(message);
		fPoseView->SaveState(message);
	}
}


bool
BContainerWindow::StateNeedsSaving() const
{
	return fStateNeedsSaving || PoseView()->StateNeedsSaving();
}


status_t
BContainerWindow::GetLayoutState(BNode *node, BMessage *message)
{
	// ToDo:
	// get rid of this, use AttrStream instead
	status_t result = node->InitCheck();
	if (result != B_OK)
		return result;

	node->RewindAttrs();
	char attrName[256];
	while (node->GetNextAttrName(attrName) == B_OK) {
		attr_info info;
		node->GetAttrInfo(attrName, &info);

		// filter out attributes that are not related to window position
		// and column resizing
		// more can be added as needed
		if (strcmp(attrName, kAttrWindowFrame) != 0
			&& strcmp(attrName, kAttrColumns) != 0
			&& strcmp(attrName, kAttrViewState) != 0
			&& strcmp(attrName, kAttrColumnsForeign) != 0
			&& strcmp(attrName, kAttrViewStateForeign) != 0)
			continue;

		char *buffer = new char[info.size];
		if (node->ReadAttr(attrName, info.type, 0, buffer, (size_t)info.size) == info.size)
			message->AddData(attrName, info.type, buffer, (ssize_t)info.size);
		delete [] buffer;
	}
	return B_OK;
}


status_t
BContainerWindow::SetLayoutState(BNode *node, const BMessage *message)
{
	status_t result = node->InitCheck();
	if (result != B_OK)
		return result;

	for (int32 globalIndex = 0; ;) {
#if B_BEOS_VERSION_DANO
 		const char *name;
#else
		char *name;
#endif
		type_code type;
		int32 count;
		status_t result = message->GetInfo(B_ANY_TYPE, globalIndex, &name,
			&type, &count);
		if (result != B_OK)
			break;

		for (int32 index = 0; index < count; index++) {
			const void *buffer;
			int32 size;
			result = message->FindData(name, type, index, &buffer, &size);
			if (result != B_OK) {
				PRINT(("error reading %s \n", name));
				return result;
			}

			if (node->WriteAttr(name, type, 0, buffer, (size_t)size) != size) {
				PRINT(("error writing %s \n", name));
				return result;
			}
			globalIndex++;
		}
	}
	return B_OK;
}


Model *
BContainerWindow::TargetModel() const
{
	return fPoseView->TargetModel();
}


void
BContainerWindow::SelectionChanged()
{
}


void
BContainerWindow::Zoom(BPoint, float, float)
{
	BRect oldZoomRect(fSavedZoomRect);
	fSavedZoomRect = Frame();
	ResizeToFit();

	if (fSavedZoomRect == Frame())
		if (oldZoomRect.IsValid())
			ResizeTo(oldZoomRect.Width(), oldZoomRect.Height());
}


void
BContainerWindow::ResizeToFit()
{
	BScreen screen(this);
	BRect screenFrame(screen.Frame());

	screenFrame.InsetBy(5, 5);
	screenFrame.top += 15;		// keeps title bar of window visible

	BRect frame(Frame());

	float widthDiff = frame.Width() - PoseView()->Frame().Width();
	float heightDiff = frame.Height() - PoseView()->Frame().Height();

	// move frame left top on screen
	BPoint leftTop(frame.LeftTop());
	leftTop.ConstrainTo(screenFrame);
	frame.OffsetTo(leftTop);

	// resize to extent size
	BRect extent(PoseView()->Extent());
	frame.right = frame.left + extent.Width() + widthDiff;
	frame.bottom = frame.top + extent.Height() + heightDiff;

	// make sure entire window fits on screen
	frame = frame & screenFrame;

	ResizeTo(frame.Width(), frame.Height());
	MoveTo(frame.LeftTop());
	Controller()->SetScrollBarsEnabled(false);

	// scroll if there is an offset
	PoseView()->ScrollBy(
		extent.left - PoseView()->Bounds().left,
		extent.top - PoseView()->Bounds().top);

	Controller()->UpdateScrollRange();
	Controller()->SetScrollBarsEnabled(true);
}


void
BContainerWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_CUT:
		case B_COPY:
		case B_PASTE:
		case kCutMoreSelectionToClipboard:
		case kCopyMoreSelectionToClipboard:
		case kPasteLinksFromClipboard:
		{
			BView *view = CurrentFocus();
			if (view->LockLooper()) {
				view->MessageReceived(message);
				view->UnlockLooper();
			}
			break;
		}

		case kNewFolder:
			PostMessage(message, PoseView());
			break;

		case kRestoreState:
			if (message->HasMessage("state")) {
				BMessage state;
				message->FindMessage("state", &state);
				_Init(&state);
			} else
				_Init();
			break;

		case kResizeToFit:
			ResizeToFit();
			break;

		case kLoadAddOn:
			LoadAddOn(message);
			break;

		case kCopySelectionTo:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			BRoster().AddToRecentFolders(&ref);

			Model model(&ref);
			if (model.InitCheck() != B_OK)
				break;

			if (*model.NodeRef() == *TargetModel()->NodeRef())
				PoseView()->DuplicateSelection();
			else
				PoseView()->MoveSelectionInto(&model, this, true);

			break;
		}
		case kMoveSelectionTo:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			BRoster().AddToRecentFolders(&ref);

			Model model(&ref);
			if (model.InitCheck() != B_OK)
				break;

			PoseView()->MoveSelectionInto(&model, this, false, true);
			break;
		}

		case kCreateLink:
		case kCreateRelativeLink:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BRoster().AddToRecentFolders(&ref);

				Model model(&ref);
				if (model.InitCheck() != B_OK)
					break;

				PoseView()->MoveSelectionInto(&model, this, false, false,
					message->what == kCreateLink, message->what == kCreateRelativeLink);
			} else {
				// no destination specified, create link in same dir as item
				if (!TargetModel()->IsQuery())
					PoseView()->MoveSelectionInto(TargetModel(), this, false, false,
						message->what == kCreateLink, message->what == kCreateRelativeLink);
			}
			break;
		}

		case kShowSelectionWindow:
			ShowSelectionWindow();
			break;

		case kSelectMatchingEntries:
			PoseView()->SelectMatchingEntries(message);
			break;

		case kFindButton:
			(new FindWindow())->Show();
			break;

		case kQuitTracker:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case kRestoreBackgroundImage:
			UpdateBackgroundImage();
			break;

		case kSwitchDirectory:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BEntry entry;
				if (entry.SetTo(&ref) == B_OK) {
					if (StateNeedsSaving())
						SaveState(false);

					bool wasInTrash = PoseView()->TargetModel()->IsTrash()
						|| PoseView()->TargetModel()->IsInTrash();
					bool isRoot = PoseView()->TargetModel()->IsRoot();

					// Switch dir and apply new state
					WindowStateNodeOpener opener(this, false);
					opener.SetTo(&entry, false);

					// Update PoseView
					PoseView()->SwitchDir(&ref, opener.StreamNode());

					if (wasInTrash ^ (PoseView()->TargetModel()->IsTrash()
							|| PoseView()->TargetModel()->IsInTrash())
						|| isRoot != PoseView()->TargetModel()->IsRoot())
						RepopulateMenus();

					// Update Navigation bar
					if (Controller()->Navigator() != NULL) {
						int32 action = kActionSet;
						if (message->FindInt32("action", &action) != B_OK)
							// Design problem? Why does FindInt32 touch
							// 'action' at all if he can't find it??
							action = kActionSet;

						Controller()->Navigator()->UpdateLocation(PoseView()->TargetModel(), action);
					}

					TrackerSettings settings;
					if (settings.ShowNavigator() || settings.ShowFullPathInTitleBar())
						SetPathWatchingEnabled(true);

					// Update draggable folder icon
					BView *view = FindView("MenuBar");
					if (view != NULL) {
						view = view->FindView("ThisContainer");
						if (view != NULL) {
							IconCache::sIconCache->IconChanged(TargetModel());
							view->Invalidate();
						}
					}

					// Update window title
					UpdateTitle();
				}
			}
			break;
		}

		case B_REFS_RECEIVED:
			if (Dragging()) {
				//
				//	ref in this message is the target,
				//	the end point of the drag
				//
				entry_ref ref;
				if (message->FindRef("refs", &ref) == B_OK) {
					//printf("BContainerWindow::MessageReceived - refs received\n");
					fWaitingForRefs = false;
					BEntry entry(&ref, true);
					//
					//	don't copy to printers dir
					if (!FSIsPrintersDir(&entry)) {
						if (entry.InitCheck() == B_OK && entry.IsDirectory()) {
							//
							//	build of list of entry_refs from the list
							//	in the drag message
							entry_ref dragref;
							int32 index = 0;
							BObjectList<entry_ref> *list = new BObjectList<entry_ref>(0, true);
							while (fDragMessage->FindRef("refs", index++, &dragref) == B_OK)
								list->AddItem(new entry_ref(dragref));
							//
							//	compare the target and one of the drag items' parent
							//
							uint32 moveMode = kMoveSelectionTo;
							Model targetModel(&entry, true, false);
							if (!CheckDevicesEqual(list->ItemAt(0), &targetModel))
								moveMode = kCopySelectionTo;
							//	copy drag contents to target ref in message
							FSMoveToFolder(list, new BEntry(entry),
									moveMode);
							targetModel.CloseNode();
						} else {
							// 	current message sent to apps is only B_REFS_RECEIVED
							fDragMessage->what = B_REFS_RECEIVED;
							FSLaunchItem(&ref, (const BMessage *)fDragMessage, true, true);
						}
					}
				}
				DragStop();
			}
			break;

		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 observerWhat;
			if (message->FindInt32("be:observe_change_what", &observerWhat) == B_OK) {
				TrackerSettings settings;
				switch (observerWhat) {
					case kWindowsShowFullPathChanged:
						UpdateTitle();
						if (!IsPathWatchingEnabled() && settings.ShowFullPathInTitleBar())
							SetPathWatchingEnabled(true);
						if (IsPathWatchingEnabled() && !(settings.ShowNavigator() || settings.ShowFullPathInTitleBar()))
							SetPathWatchingEnabled(false);
						break;

					case kSingleWindowBrowseChanged:
						if (settings.SingleWindowBrowse()
							&& !Controller()->Navigator()
							&& TargetModel()->IsDirectory()
							&& !PoseView()->IsFilePanel()
							&& !PoseView()->IsDesktopWindow()) {

							Controller()->SetControlVisible(Controller()->Navigator(), settings.ShowNavigator());
							SetPathWatchingEnabled(settings.ShowNavigator() || settings.ShowFullPathInTitleBar());
							
						}
						SetSingleWindowBrowseShortcuts(settings.SingleWindowBrowse());
						break;

					case kShowNavigatorChanged:
						printf("kShowNavigatorChanged");
						Controller()->SetControlVisible(Controller()->Navigator(),
							settings.ShowNavigator());
						if (!IsPathWatchingEnabled() && settings.ShowNavigator())
							SetPathWatchingEnabled(true);
						if (IsPathWatchingEnabled() && !(settings.ShowNavigator() || settings.ShowFullPathInTitleBar()))
							SetPathWatchingEnabled(false);
						break;

					case kDontMoveFilesToTrashChanged:
						{
							bool dontMoveToTrash = settings.DontMoveFilesToTrash();

							BMenuItem *item = fFileContextMenu->FindItem(kMoveToTrash);
							if (item) {
								item->SetLabel(dontMoveToTrash
									? B_TRANSLATE("Delete")
									: B_TRANSLATE("Move to Trash"));
							}
							// Deskbar doesn't have a menu bar, so check if there is Controller()->MenuBar()
							if (Controller()->MenuBar() && Controller()->FileMenu()) {
								item = Controller()->FileMenu()->FindItem(kMoveToTrash);
								if (item) {
									item->SetLabel(dontMoveToTrash
										? B_TRANSLATE("Delete")
										: B_TRANSLATE("Move to Trash"));
								}
							}
							UpdateIfNeeded();
						}
						break;

					default:
						_inherited::MessageReceived(message);
				}
			}
			break;
		}

		case B_NODE_MONITOR:
			UpdateTitle();
			break;

		case B_UNDO:
			FSUndo();
			break;

		//case B_REDO:	/* only defined in Dano/Zeta/OpenBeOS */
		case kRedo:
			FSRedo();
			break;

		default:
			_inherited::MessageReceived(message);
	}
}


void
BContainerWindow::SetCutItem(BMenu *menu)
{
	BMenuItem *item;
	if ((item = menu->FindItem(B_CUT)) == NULL
		&& (item = menu->FindItem(kCutMoreSelectionToClipboard)) == NULL)
		return;

	item->SetEnabled(PoseView()->SelectionList()->CountItems() > 0
		|| PoseView() != CurrentFocus());

	if (modifiers() & B_SHIFT_KEY) {
		item->SetLabel(B_TRANSLATE("Cut more"));
		item->SetShortcut('X', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCutMoreSelectionToClipboard));
	} else {
		item->SetLabel(B_TRANSLATE("Cut"));
		item->SetShortcut('X', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_CUT));
	}
}


void
BContainerWindow::SetCopyItem(BMenu *menu)
{
	BMenuItem *item;
	if ((item = menu->FindItem(B_COPY)) == NULL
		&& (item = menu->FindItem(kCopyMoreSelectionToClipboard)) == NULL)
		return;

	item->SetEnabled(PoseView()->SelectionList()->CountItems() > 0
		|| PoseView() != CurrentFocus());

	if (modifiers() & B_SHIFT_KEY) {
		item->SetLabel(B_TRANSLATE("Copy more"));
		item->SetShortcut('C', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCopyMoreSelectionToClipboard));
	} else {
		item->SetLabel(B_TRANSLATE("Copy"));
		item->SetShortcut('C', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_COPY));
	}
}


void
BContainerWindow::SetPasteItem(BMenu *menu)
{
	BMenuItem *item;
	if ((item = menu->FindItem(B_PASTE)) == NULL
		&& (item = menu->FindItem(kPasteLinksFromClipboard)) == NULL)
		return;

	item->SetEnabled(FSClipboardHasRefs() || PoseView() != CurrentFocus());

	if (modifiers() & B_SHIFT_KEY) {
		item->SetLabel(B_TRANSLATE("Paste links"));
		item->SetShortcut('V', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kPasteLinksFromClipboard));
	} else {
		item->SetLabel(B_TRANSLATE("Paste"));
		item->SetShortcut('V', B_COMMAND_KEY);
		item->SetMessage(new BMessage(B_PASTE));
	}
}


void
BContainerWindow::SetCleanUpItem(BMenu *menu)
{
	BMenuItem *item;
	if ((item = menu->FindItem(kCleanup)) == NULL
		&& (item = menu->FindItem(kCleanupAll)) == NULL)
		return;

	item->SetEnabled(PoseView()->CountItems() > 0
		&& (PoseView()->ViewMode() != kListMode));

	if (modifiers() & B_SHIFT_KEY) {
		item->SetLabel(B_TRANSLATE("Clean up all"));
		item->SetShortcut('K', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetMessage(new BMessage(kCleanupAll));
	} else {
		item->SetLabel(B_TRANSLATE("Clean up"));
		item->SetShortcut('K', B_COMMAND_KEY);
		item->SetMessage(new BMessage(kCleanup));
	}
}


void
BContainerWindow::SetCloseItem(BMenu *menu)
{
	BMenuItem *item;
	if ((item = menu->FindItem(B_QUIT_REQUESTED)) == NULL
		&& (item = menu->FindItem(kCloseAllWindows)) == NULL)
		return;

	if (modifiers() & B_SHIFT_KEY) {
		item->SetLabel(B_TRANSLATE("Close all"));
		item->SetShortcut('W', B_COMMAND_KEY | B_SHIFT_KEY);
		item->SetTarget(be_app);
		item->SetMessage(new BMessage(kCloseAllWindows));
	} else {
		item->SetLabel(B_TRANSLATE("Close"));
		item->SetShortcut('W', B_COMMAND_KEY);
		item->SetTarget(this);
		item->SetMessage(new BMessage(B_QUIT_REQUESTED));
	}
}


bool
BContainerWindow::IsShowing(const node_ref *node) const
{
	return PoseView()->Represents(node);
}


bool
BContainerWindow::IsShowing(const entry_ref *entry) const
{
	return PoseView()->Represents(entry);
}


void
BContainerWindow::AddCommonShortcuts()
{
	AddShortcut('Z', B_COMMAND_KEY, new BMessage(B_UNDO), this);
 	AddShortcut('Z', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kRedo), this);
	
	AddShortcut('T', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kDelete), PoseView());
	AddShortcut('K', B_COMMAND_KEY | B_SHIFT_KEY, new BMessage(kCleanupAll),
		PoseView());
	AddShortcut('Q', B_COMMAND_KEY | B_OPTION_KEY | B_SHIFT_KEY | B_CONTROL_KEY,
		new BMessage(kQuitTracker));

	AddShortcut(B_DOWN_ARROW, B_COMMAND_KEY, new BMessage(kOpenSelection),
		PoseView());
		
	TrackerSettings settings;
	SetSingleWindowBrowseShortcuts(settings.SingleWindowBrowse());

#if DEBUG
	// add some debugging shortcuts
	AddShortcut('D', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage('dbug'), PoseView());
	AddShortcut('C', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage('dpcc'), PoseView());
	AddShortcut('F', B_COMMAND_KEY | B_CONTROL_KEY, new BMessage('dpfl'), PoseView());
	AddShortcut('F', B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY,
		new BMessage('dpfL'), PoseView());
#endif	
}


void
BContainerWindow::MenusBeginning()
{
	if (!Controller()->MenuBar())
		return;

	if (CurrentMessage() && CurrentMessage()->what == B_MOUSE_DOWN)
		// don't commit active pose if only a keyboard shortcut is
		// invoked - this would prevent Cut/Copy/Paste from working
		fPoseView->CommitActivePose();

	// File menu
	int32 selectCount = PoseView()->SelectionList()->CountItems();

	SetupOpenWithMenu(Controller()->FileMenu());
	SetupMoveCopyMenus(selectCount
		? PoseView()->SelectionList()->FirstItem()->TargetModel()->EntryRef() : NULL, Controller()->FileMenu());

	UpdateMenu(Controller()->MenuBar(), kMenuBarContext);

	fController->AddMimeTypesToMenu(Controller()->AttributeMenu());

	if (PoseView()->TargetModel()->IsPrintersDir()) {
		EnableNamedMenuItem(Controller()->FileMenu(), B_TRANSLATE("Make active printer"),
			selectCount == 1);
	}
}


void
BContainerWindow::MenusEnded()
{
	// when we're done we want to clear nav menus for next time
	DeleteSubmenu(fNavigationItem);
	DeleteSubmenu(fMoveToItem);
	DeleteSubmenu(fCopyToItem);
	DeleteSubmenu(fCreateLinkItem);
	DeleteSubmenu(fOpenWithItem);
}


void
BContainerWindow::SetupNavigationMenu(const entry_ref *ref, BMenu *parent)
{
	// start by removing nav item (and separator) from old menu
	if (fNavigationItem) {
		BMenu *menu = fNavigationItem->Menu();
		if (menu) {
			menu->RemoveItem(fNavigationItem);
			BMenuItem *item = menu->RemoveItem((int32)0);
			ASSERT(item != fNavigationItem);
			delete item;
		}
	}

	// if we weren't passed a ref then we're navigating this window
	if (!ref)
		ref = TargetModel()->EntryRef();

	BEntry entry;
	if (entry.SetTo(ref) != B_OK)
		return;

	// only navigate directories and queries (check for symlink here)
	Model model(&entry);
	entry_ref resolvedRef;

	if (model.InitCheck() != B_OK
		|| (!model.IsContainer() && !model.IsSymLink()))
		return;

	if (model.IsSymLink()) {
		if (entry.SetTo(model.EntryRef(), true) != B_OK)
			return;

		Model resolvedModel(&entry);
		if (resolvedModel.InitCheck() != B_OK || !resolvedModel.IsContainer())
			return;

		entry.GetRef(&resolvedRef);
		ref = &resolvedRef;
	}

	if (!fNavigationItem) {
		fNavigationItem = new ModelMenuItem(&model,
			new BNavMenu(model.Name(), B_REFS_RECEIVED, be_app, this));
	}

	// setup a navigation menu item which will dynamically load items
	// as menu items are traversed
	BNavMenu *navMenu = dynamic_cast<BNavMenu *>(fNavigationItem->Submenu());
	navMenu->SetNavDir(ref);
	fNavigationItem->SetLabel(model.Name());
	fNavigationItem->SetEntry(&entry);

	parent->AddItem(fNavigationItem, 0);
	parent->AddItem(new BSeparatorItem(), 1);

	BMessage *message = new BMessage(B_REFS_RECEIVED);
	message->AddRef("refs", ref);
	fNavigationItem->SetMessage(message);
	fNavigationItem->SetTarget(be_app);

	if (!Dragging())
		parent->SetTrackingHook(NULL, NULL);
}


void
BContainerWindow::SetUpEditQueryItem(BMenu *menu)
{
	ASSERT(menu);
	// File menu
	int32 selectCount = PoseView()->SelectionList()->CountItems();

	// add Edit query if appropriate
	bool queryInSelection = false;
	if (selectCount && selectCount < 100) {
		// only do this for a limited number of selected poses

		// if any queries selected, add an edit query menu item
		for (int32 index = 0; index < selectCount; index++) {
			BPose *pose = PoseView()->SelectionList()->ItemAt(index);
			Model model(pose->TargetModel()->EntryRef(), true);
			if (model.InitCheck() != B_OK)
				continue;

			if (model.IsQuery() || model.IsQueryTemplate()) {
				queryInSelection = true;
				break;
			}
		}
	}

	bool poseViewIsQuery = TargetModel()->IsQuery();
		// if the view is a query pose view, add edit query menu item

	BMenuItem* item = menu->FindItem(kEditQuery);
	if (!poseViewIsQuery && !queryInSelection && item)
		item->Menu()->RemoveItem(item);

	else if ((poseViewIsQuery || queryInSelection) && !item) {

		// add edit query item after Open
		item = menu->FindItem(kOpenSelection);
		if (item) {
			int32 itemIndex = item->Menu()->IndexOf(item);
			BMenuItem* query = new BMenuItem(B_TRANSLATE("Edit query"),
				new BMessage(kEditQuery), 'G');
			item->Menu()->AddItem(query, itemIndex + 1);
			query->SetTarget(PoseView());
		}
	}
}


void
BContainerWindow::SetupOpenWithMenu(BMenu *parent)
{
	// start by removing nav item (and separator) from old menu
	if (fOpenWithItem) {
		BMenu *menu = fOpenWithItem->Menu();
		if (menu)
			menu->RemoveItem(fOpenWithItem);

		delete fOpenWithItem;
		fOpenWithItem = 0;
	}

	if (PoseView()->SelectionList()->CountItems() == 0)
		// no selection, nothing to open
		return;

	if (TargetModel()->IsRoot())
		// don't add ourselves if we are root
		return;

	// ToDo:
	// check if only item in selection list is the root
	// and do not add if true

	// add after "Open"
	BMenuItem *item = parent->FindItem(kOpenSelection);

	int32 count = PoseView()->SelectionList()->CountItems();
	if (!count)
		return;

	// build a list of all refs to open
	BMessage message(B_REFS_RECEIVED);
	for (int32 index = 0; index < count; index++) {
		BPose *pose = PoseView()->SelectionList()->ItemAt(index);
		message.AddRef("refs", pose->TargetModel()->EntryRef());
	}

	// add Tracker token so that refs received recipients can script us
	message.AddMessenger("TrackerViewToken", BMessenger(PoseView()));

	int32 index = item->Menu()->IndexOf(item);
	fOpenWithItem = new BMenuItem(
		new OpenWithMenu(B_TRANSLATE("Open with" B_UTF8_ELLIPSIS),
			&message, this, be_app), new BMessage(kOpenSelectionWith));
	fOpenWithItem->SetTarget(PoseView());
	fOpenWithItem->SetShortcut('O', B_COMMAND_KEY | B_CONTROL_KEY);

	item->Menu()->AddItem(fOpenWithItem, index + 1);
}


void
BContainerWindow::PopulateMoveCopyNavMenu(BNavMenu *navMenu, uint32 what,
	const entry_ref *ref, bool addLocalOnly)
{
	BVolume volume;
	BVolumeRoster volumeRoster;
	BDirectory directory;
	BEntry entry;
	BPath path;
	Model model;
	dev_t device = ref->device;

	int32 volumeCount = 0;

	navMenu->RemoveItems(0, navMenu->CountItems(), true);

	// count persistent writable volumes
	volumeRoster.Rewind();
	while (volumeRoster.GetNextVolume(&volume) == B_OK)
		if (!volume.IsReadOnly() && volume.IsPersistent())
			volumeCount++;

	// add the current folder
	if (entry.SetTo(ref) == B_OK
		&& entry.GetParent(&entry) == B_OK
		&& model.SetTo(&entry) == B_OK) {
		BNavMenu* menu = new BNavMenu(B_TRANSLATE("Current folder"), what,
			this);
		menu->SetNavDir(model.EntryRef());
		menu->SetShowParent(true);

		BMenuItem *item = new SpecialModelMenuItem(&model,menu);
		item->SetMessage(new BMessage((uint32)what));

		navMenu->AddItem(item);
	}

	// add the recent folder menu
	// the "Tracker" settings directory is only used to get its icon
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("Tracker");
		if (entry.SetTo(path.Path()) == B_OK
			&& model.SetTo(&entry) == B_OK) {
			BMenu* menu = new RecentsMenu(B_TRANSLATE("Recent folders"),
				kRecentFolders, what, this);

			BMenuItem *item = new SpecialModelMenuItem(&model,menu);
			item->SetMessage(new BMessage((uint32)what));

			navMenu->AddItem(item);
		}
	}

	// add Desktop
	FSGetBootDeskDir(&directory);
	if (directory.InitCheck() == B_OK
		&& directory.GetEntry(&entry) == B_OK
		&& model.SetTo(&entry) == B_OK)
		navMenu->AddNavDir(&model, what, this, true);
			// ask NavMenu to populate submenu for us

	// add the home dir
	if (find_directory(B_USER_DIRECTORY, &path) == B_OK
		&& entry.SetTo(path.Path()) == B_OK
		&& model.SetTo(&entry) == B_OK)
		navMenu->AddNavDir(&model, what, this, true);

	navMenu->AddSeparatorItem();

	// either add all mounted volumes (for copy), or all the top-level
	// directories from the same device (for move)
	// ToDo: can be changed if cross-device moves are implemented

	if (addLocalOnly || volumeCount < 2) {
		// add volume this item lives on
		if (volume.SetTo(device) == B_OK
			&& volume.GetRootDirectory(&directory) == B_OK
			&& directory.GetEntry(&entry) == B_OK
			&& model.SetTo(&entry) == B_OK) {
			navMenu->AddNavDir(&model, what, this, false);
				// do not have submenu populated

			navMenu->SetNavDir(model.EntryRef());
		}
	} else {
		// add all persistent writable volumes
		volumeRoster.Rewind();
		while (volumeRoster.GetNextVolume(&volume) == B_OK) {
			if (volume.IsReadOnly() || !volume.IsPersistent())
				continue;

			// add root dir
			if (volume.GetRootDirectory(&directory) == B_OK
				&& directory.GetEntry(&entry) == B_OK
				&& model.SetTo(&entry) == B_OK)
				navMenu->AddNavDir(&model, what, this, true);
					// ask NavMenu to populate submenu for us
		}
	}
}


void
BContainerWindow::SetupMoveCopyMenus(const entry_ref *item_ref, BMenu *parent)
{
	if (PoseView()->TargetModel()->IsTrash()
		|| PoseView()->TargetModel()->IsInTrash()
		|| PoseView()->TargetModel()->IsPrintersDir()
		|| !fMoveToItem || !fCopyToItem || !fCreateLinkItem)
		return;

	// Grab the modifiers state since we use it twice
	uint32 modifierKeys = modifiers();

	// re-parent items to this menu since they're shared
 	int32 index;
 	BMenuItem *trash = parent->FindItem(kMoveToTrash);
 	if (trash)
 		index = parent->IndexOf(trash) + 2;
 	else
		index = 0;

	if (fMoveToItem->Menu() != parent) {
		if (fMoveToItem->Menu())
			fMoveToItem->Menu()->RemoveItem(fMoveToItem);

		parent->AddItem(fMoveToItem, index++);
	}

	if (fCopyToItem->Menu() != parent) {
		if (fCopyToItem->Menu())
			fCopyToItem->Menu()->RemoveItem(fCopyToItem);

		parent->AddItem(fCopyToItem, index++);
	}

	if (fCreateLinkItem->Menu() != parent) {
		if (fCreateLinkItem->Menu())
			fCreateLinkItem->Menu()->RemoveItem(fCreateLinkItem);

		parent->AddItem(fCreateLinkItem, index);
	}

	// Set the "Create Link" item label here so it
	// appears correctly when menus are disabled, too.
	if (modifierKeys & B_SHIFT_KEY)
		fCreateLinkItem->SetLabel(B_TRANSLATE("Create relative link"));
	else
		fCreateLinkItem->SetLabel(B_TRANSLATE("Create link"));

	// only enable once the menus are built
	fMoveToItem->SetEnabled(false);
	fCopyToItem->SetEnabled(false);
	fCreateLinkItem->SetEnabled(false);

	// get ref for item which is selected
	BEntry entry;
	if (entry.SetTo(item_ref) != B_OK)
		return;

	Model tempModel(&entry);
	if (tempModel.InitCheck() != B_OK)
		return;

	if (tempModel.IsRoot() || tempModel.IsVolume())
		return;

	// configure "Move to" menu item
	PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu *>(fMoveToItem->Submenu()),
		kMoveSelectionTo, item_ref, true);

	// configure "Copy to" menu item
	// add all mounted volumes (except the one this item lives on)
	PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu *>(fCopyToItem->Submenu()),
		kCopySelectionTo, item_ref, false);

	// Set "Create Link" menu item message and
	// add all mounted volumes (except the one this item lives on)
	if (modifierKeys & B_SHIFT_KEY) {
		fCreateLinkItem->SetMessage(new BMessage(kCreateRelativeLink));
		PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu *>(fCreateLinkItem->Submenu()),
			kCreateRelativeLink, item_ref, false);
	} else {
		fCreateLinkItem->SetMessage(new BMessage(kCreateLink));
		PopulateMoveCopyNavMenu(dynamic_cast<BNavMenu *>(fCreateLinkItem->Submenu()),
		kCreateLink, item_ref, false);
	}

	fMoveToItem->SetEnabled(true);
	fCopyToItem->SetEnabled(true);
	fCreateLinkItem->SetEnabled(true);

	// Set the "Identify" item label
	BMenuItem *identifyItem = parent->FindItem(kIdentifyEntry);
	if (identifyItem != NULL) {
		if (modifierKeys & B_SHIFT_KEY)
			identifyItem->SetLabel(B_TRANSLATE("Force identify"));
		else
			identifyItem->SetLabel(B_TRANSLATE("Identify"));
	}
}


uint32
BContainerWindow::ShowDropContextMenu(BPoint loc)
{
	BPoint global(loc);

	PoseView()->ConvertToScreen(&global);
	PoseView()->CommitActivePose();

	// Change the "Create Link" item - allow user to
	// create relative links with the Shift key down.
	BMenuItem *item = fDropContextMenu->FindItem(kCreateLink);
	if (item == NULL)
		item = fDropContextMenu->FindItem(kCreateRelativeLink);
	if (item && (modifiers() & B_SHIFT_KEY)) {
		item->SetLabel(B_TRANSLATE("Create relative link here"));
		item->SetMessage(new BMessage(kCreateRelativeLink));
	} else if (item) {
		item->SetLabel(B_TRANSLATE("Create link here"));
		item->SetMessage(new BMessage(kCreateLink));
	}

	global += BPoint(2, 2);
	item = fDropContextMenu->Go(global, true, true);
	if (item)
		return item->Command();

	return 0;
}


void
BContainerWindow::ShowContextMenu(BPoint loc, const entry_ref *ref, BView *)
{
	ASSERT(IsLocked());
	BPoint global(loc);
	PoseView()->ConvertToScreen(&global);
	global += BPoint(2, 2);
	PoseView()->CommitActivePose();

	if (ref) {
		// clicked on a pose, show file or volume context menu
		Model model(ref);
		
		if (model.IsTrash()) {

			if (fTrashContextMenu->Window() || Dragging())
				return;
			
			DeleteSubmenu(fNavigationItem);
	
			// selected item was trash, show the trash context menu instead

			EnableNamedMenuItem(fTrashContextMenu, kEmptyTrash,
				static_cast<TTracker *>(be_app)->TrashFull());
	
			SetupNavigationMenu(ref, fTrashContextMenu);
			fTrashContextMenu->Go(global, true, true, true);
		} else {
		
			bool showAsVolume = false;
			bool filePanel = PoseView()->IsFilePanel();
		
			if (Dragging()) {
				fContextMenu = NULL;
		
				BEntry entry;
				model.GetEntry(&entry);
		
				// only show for directories (directory, volume, root)
				//
				// don't show a popup for the trash or printers
				// trash is handled in DeskWindow
				//
				// since this menu is opened asynchronously
				// we need to make sure we don't open it more
				// than once, the IsShowing flag is set in
				// SlowContextPopup::AttachedToWindow and
				// reset in DetachedFromWindow
				// see the notes in SlowContextPopup::AttachedToWindow
		
				if (!FSIsPrintersDir(&entry) && !fDragContextMenu->IsShowing()) {
					// printf("ShowContextMenu - target is %s %i\n", ref->name, IsShowing(ref));
					fDragContextMenu->ClearMenu();
		
					// in case the ref is a symlink, resolve it
					// only pop open for directories
					BEntry resolvedEntry(ref, true);
					if (!resolvedEntry.IsDirectory())
						return;
		
					entry_ref resolvedRef;
					resolvedEntry.GetRef(&resolvedRef);
		
					// use the resolved ref for the menu
					fDragContextMenu->SetNavDir(&resolvedRef);
					fDragContextMenu->SetTypesList(fCachedTypesList);
					fDragContextMenu->SetTarget(BMessenger(this));
					BPoseView *poseView = PoseView();
					if (poseView) {
						BMessenger target(poseView);
						fDragContextMenu->InitTrackingHook(
							&BPoseView::MenuTrackingHook, &target, fDragMessage);
					}
		
					// this is now asynchronous so that we don't
					// deadlock in Window::Quit,
					fDragContextMenu->Go(global, true, false, true);
				}
				return;
			} else if (TargetModel()->IsRoot() || model.IsVolume()) {
				fContextMenu = fVolumeContextMenu;
				showAsVolume = true;
			} else
				fContextMenu = fFileContextMenu;
		
			// clean up items from last context menu
		
			if (fContextMenu) {
				if (fContextMenu->Window())
					return;
				else
					MenusEnded();
		
				if (model.InitCheck() == B_OK) { // ??? Do I need this ???
					if (showAsVolume) {
						// non-volume enable/disable copy, move, identify
						EnableNamedMenuItem(fContextMenu, kDuplicateSelection, false);
						EnableNamedMenuItem(fContextMenu, kMoveToTrash, false);
						EnableNamedMenuItem(fContextMenu, kIdentifyEntry, false);
		
						// volume model, enable/disable the Unmount item
						bool ejectableVolumeSelected = false;
		
						BVolume boot;
						BVolumeRoster().GetBootVolume(&boot);
						BVolume volume;
						volume.SetTo(model.NodeRef()->device);
						if (volume != boot)
							ejectableVolumeSelected = true;
		
						EnableNamedMenuItem(fContextMenu,
							B_TRANSLATE("Unmount"),	ejectableVolumeSelected);
					}
				}
		
				SetupNavigationMenu(ref, fContextMenu);
				if (!showAsVolume && !filePanel) {
					SetupMoveCopyMenus(ref, fContextMenu);
					SetupOpenWithMenu(fContextMenu);
				}
		
				UpdateMenu(fContextMenu, kPosePopUpContext);
		
				fContextMenu->Go(global, true, true, true);
			}
		}
	} else if (fWindowContextMenu) {
		if (fWindowContextMenu->Window())
			return;

		MenusEnded();

		// clicked on a window, show window context menu

		SetupNavigationMenu(ref, fWindowContextMenu);
		UpdateMenu(fWindowContextMenu, kWindowPopUpContext);

		fWindowContextMenu->Go(global, true, true, true);
	}
	fContextMenu = NULL;
}


void
BContainerWindow::AddFileContextMenus(BMenu *menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"), new BMessage(kGetInfo),
		'I'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));

	if (!PoseView()->TargetModel()->IsTrash() && !PoseView()->TargetModel()->IsInTrash() && !PoseView()->TargetModel()->IsPrintersDir()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Duplicate"),
			new BMessage(kDuplicateSelection), 'D'));
	}

	if (!PoseView()->TargetModel()->IsTrash() && !PoseView()->TargetModel()->IsInTrash()) {
		menu->AddItem(new BMenuItem(TrackerSettings().DontMoveFilesToTrash()
			? B_TRANSLATE("Delete")	: B_TRANSLATE("Move to Trash"),
			new BMessage(kMoveToTrash), 'T'));

		// add separator for copy to/move to items (navigation items)
		menu->AddSeparatorItem();
	} else {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Delete"),
			new BMessage(kDelete), 0));
		menu->AddItem(new BMenuItem(B_TRANSLATE("Restore"),
			new BMessage(kRestoreFromTrash), 0));
	}

#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	menu->AddSeparatorItem();
	BMenuItem *cutItem, *copyItem;
	menu->AddItem(cutItem = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	menu->AddItem(copyItem = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
#endif

	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Identify"),
		new BMessage(kIdentifyEntry)));
	BMenu* addOnMenuItem = new BMenu(B_TRANSLATE("Add-ons"));
	addOnMenuItem->SetFont(be_plain_font);
	menu->AddItem(addOnMenuItem);

	// set targets as needed
	menu->SetTargetForItems(PoseView());
#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	cutItem->SetTarget(this);
	copyItem->SetTarget(this);
#endif
}


void
BContainerWindow::AddVolumeContextMenus(BMenu *menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));

	menu->AddSeparatorItem();
	menu->AddItem(new MountMenu(B_TRANSLATE("Mount")));

	BMenuItem *item = new BMenuItem(B_TRANSLATE("Unmount"),
		new BMessage(kUnmountVolume), 'U');
	item->SetEnabled(false);
	menu->AddItem(item);

	menu->AddSeparatorItem();
	menu->AddItem(new BMenu(B_TRANSLATE("Add-ons")));

	menu->SetTargetForItems(PoseView());
}


void
BContainerWindow::AddWindowContextMenus(BMenu *menu)
{
	// create context sensitive menu for empty area of window
	// since we check view mode before display, this should be a radio
	// mode menu

	bool needSeparator = true;
	if (PoseView()->TargetModel()->IsTrash()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Empty Trash"),
			new BMessage(kEmptyTrash)));
	} else if (PoseView()->TargetModel()->IsPrintersDir()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Add printer"B_UTF8_ELLIPSIS),
			new BMessage(kAddPrinter), 'N'));
	} else if (PoseView()->TargetModel()->IsInTrash())
		needSeparator = false;
	else {
		TemplatesMenu* templateMenu = new TemplatesMenu(PoseView(),
			B_TRANSLATE("New"));
		menu->AddItem(templateMenu);
		templateMenu->SetTargetForItems(PoseView());
		templateMenu->SetFont(be_plain_font);
	}

	if (needSeparator)
		menu->AddSeparatorItem();

#if 0
	BMenuItem *pasteItem = new BMenuItem("Paste", new BMessage(B_PASTE), 'V');
	menu->AddItem(pasteItem);
	menu->AddSeparatorItem();
#endif
	menu->AddItem(new BMenuItem(B_TRANSLATE("Clean up"),
		new BMessage(kCleanup), 'K'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select"B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(B_SELECT_ALL), 'A'));
	if (!PoseView()->TargetModel()->IsTrash()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Open parent"),
			new BMessage(kOpenParentDir), B_UP_ARROW));
	}

	menu->AddSeparatorItem();
	BMenu* addOnMenuItem = new BMenu(B_TRANSLATE("Add-ons"));
	addOnMenuItem->SetFont(be_plain_font);
	menu->AddItem(addOnMenuItem);

#if DEBUG
	menu->AddSeparatorItem();
	BMenuItem *testing = new BMenuItem("Test icon cache", new BMessage(kTestIconCache));
	menu->AddItem(testing);
#endif

	// target items as needed
	menu->SetTargetForItems(PoseView());
#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	pasteItem->SetTarget(this);
#endif
}


void
BContainerWindow::AddDropContextMenus(BMenu *menu)
{
	menu->AddItem(new BMenuItem(B_TRANSLATE("Create link here"),
		new BMessage(kCreateLink)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Move here"),
		new BMessage(kMoveSelectionTo)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Copy here"),
		new BMessage(kCopySelectionTo)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Cancel"),
		new BMessage(kCancelButton)));
}


void
BContainerWindow::AddTrashContextMenus(BMenu *menu)
{
	// setup special trash context menu
	menu->AddItem(new BMenuItem(B_TRANSLATE("Empty Trash"),
		new BMessage(kEmptyTrash)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->SetTargetForItems(PoseView());
}


void
BContainerWindow::EachAddon(bool (*eachAddon)(const Model *, const char *,
	uint32 shortcut, bool primary, void *context), void *passThru)
{
	BObjectList<Model> uniqueList(10, true);
	BPath path;
	bool bail = false;
	if (find_directory(B_BEOS_ADDONS_DIRECTORY, &path) == B_OK)
		bail = EachAddon(path, eachAddon, &uniqueList, passThru);

	if (!bail && find_directory(B_USER_ADDONS_DIRECTORY, &path) == B_OK)
		bail = EachAddon(path, eachAddon, &uniqueList, passThru);

	if (!bail && find_directory(B_COMMON_ADDONS_DIRECTORY, &path) == B_OK)
		EachAddon(path, eachAddon, &uniqueList, passThru);
}


bool
BContainerWindow::EachAddon(BPath &path, bool (*eachAddon)(const Model *,
	const char *, uint32 shortcut, bool primary, void *),
	BObjectList<Model> *uniqueList, void *params)
{
	path.Append("Tracker");

	BDirectory dir;
	BEntry entry;

	if (dir.SetTo(path.Path()) != B_OK)
		return false;

	// build a list of the MIME types of the selected items

	BObjectList<BString> mimeTypes(10, true);

	int32 count = PoseView()->SelectionList()->CountItems();
	if (!count) {
		// just add the type of the current directory
		AddMimeTypeString(mimeTypes, TargetModel());
	} else {
		for (int32 index = 0; index < count; index++) {
			BPose *pose = PoseView()->SelectionList()->ItemAt(index);
			AddMimeTypeString(mimeTypes, pose->TargetModel());
			// If it's a symlink, resolves it and add the Target's MimeType
			if (pose->TargetModel()->IsSymLink()) {
				Model* resolved = new Model(
					pose->TargetModel()->EntryRef(), true, true);
				if (resolved->InitCheck() == B_OK) {
					AddMimeTypeString(mimeTypes, resolved);
				}
				delete resolved;
			}
		}
	}

	dir.Rewind();
	while (dir.GetNextEntry(&entry) == B_OK) {
		Model *model = new Model(&entry);

		if (model->InitCheck() == B_OK && model->IsSymLink()) {
			// resolve symlinks
			Model* resolved = new Model(model->EntryRef(), true, true);
			if (resolved->InitCheck() == B_OK)
				model->SetLinkTo(resolved);
			else
				delete resolved;
		}
		if (model->InitCheck() != B_OK || !model->ResolveIfLink()->IsExecutable()) {
			delete model;
			continue;
		}

		// check if it supports at least one of the selected entries

		bool primary = false;

		if (mimeTypes.CountItems()) {
			BFile file(&entry, B_READ_ONLY);
			if (file.InitCheck() == B_OK) {
				BAppFileInfo info(&file);
				if (info.InitCheck() == B_OK) {
					bool secondary = true;

					// does this add-on has types set at all?
					BMessage message;
					if (info.GetSupportedTypes(&message) == B_OK) {
						type_code type;
						int32 count;
						if (message.GetInfo("types", &type, &count) == B_OK)
							secondary = false;
					}

					// check all supported types if it has some set
					if (!secondary) {
						for (int32 i = mimeTypes.CountItems(); !primary && i-- > 0;) {
							BString *type = mimeTypes.ItemAt(i);
							if (info.IsSupportedType(type->String())) {
								BMimeType mimeType(type->String());
								if (info.Supports(&mimeType))
									primary = true;
								else
									secondary = true;
							}
						}
					}

					if (!secondary && !primary) {
						delete model;
						continue;
					}
				}
			}
		}

		char name[B_FILE_NAME_LENGTH];
		uint32 key;
		StripShortcut(model, name, key);

		// do a uniqueness check
		if (uniqueList->EachElement(MatchOne, name)) {
			// found one already in the list
			delete model;
			continue;
		}
		uniqueList->AddItem(model);

		if ((eachAddon)(model, name, key, primary, params))
			return true;
	}
	return false;
}


void
BContainerWindow::BuildAddOnMenu(BMenu *menu)
{
	BMenuItem* item = menu->FindItem(B_TRANSLATE("Add-ons"));
	if (menu->IndexOf(item) == 0) {
		// the folder of the context menu seems to be named "Add-Ons"
		// so we just take the last menu item, which is correct if not
		// build with debug option
		item = menu->ItemAt(menu->CountItems() - 1);
	}
	if (item == NULL)
		return;

	menu = item->Submenu();
	if (!menu)
		return;

	menu->SetFont(be_plain_font);

	// found the addons menu, empty it first
	for (;;) {
		item = menu->RemoveItem(0L);
		if (!item)
			break;
		delete item;
	}

	BObjectList<BMenuItem> primaryList;
	BObjectList<BMenuItem> secondaryList;

	AddOneAddonParams params;
	params.primaryList = &primaryList;
	params.secondaryList = &secondaryList;

	EachAddon(AddOneAddon, &params);

	primaryList.SortItems(CompareLabels);
	secondaryList.SortItems(CompareLabels);

	int32 count = primaryList.CountItems();
	for (int32 index = 0; index < count; index++)
		menu->AddItem(primaryList.ItemAt(index));

	if (count != 0)
		menu->AddSeparatorItem();

	count = secondaryList.CountItems();
	for (int32 index = 0; index < count; index++)
		menu->AddItem(secondaryList.ItemAt(index));

	menu->SetTargetForItems(this);
}


void
BContainerWindow::UpdateMenu(BMenu *menu, UpdateMenuContext context)
{
	const int32 selectCount = PoseView()->SelectionList()->CountItems();
	const int32 count = PoseView()->CountItems();

	if (context == kMenuBarContext) {
		EnableNamedMenuItem(menu, kOpenSelection, selectCount > 0);
		EnableNamedMenuItem(menu, kGetInfo, selectCount > 0);
		EnableNamedMenuItem(menu, kIdentifyEntry, selectCount > 0);
		EnableNamedMenuItem(menu, kMoveToTrash, selectCount > 0);
		EnableNamedMenuItem(menu, kRestoreFromTrash, selectCount > 0);
		EnableNamedMenuItem(menu, kDelete, selectCount > 0);
		EnableNamedMenuItem(menu, kDuplicateSelection, selectCount > 0);
	}

	if (context == kMenuBarContext || context == kPosePopUpContext) {
		SetUpEditQueryItem(menu);
		EnableNamedMenuItem(menu, kEditItem, selectCount == 1
			&& (context == kPosePopUpContext || !PoseView()->ActivePose()));
		SetCutItem(menu);
		SetCopyItem(menu);
		SetPasteItem(menu);
	}

	if (context == kMenuBarContext || context == kWindowPopUpContext) {
		BMenu* sizeMenu = NULL;
		if (BMenuItem* item = menu->FindItem(kIconMode)) {
			sizeMenu = item->Submenu();
		}

		uint32 viewMode = PoseView()->ViewMode();
		if (sizeMenu) {
			if (viewMode == kIconMode) {
				int32 iconSize = (int32)PoseView()->IconSizeInt();
				for (int32 i = 0; BMenuItem* item = sizeMenu->ItemAt(i); i++) {
					BMessage* message = item->Message();
					if (!message) {
						item->SetMarked(false);
						continue;
					}
					int32 size;
					if (message->FindInt32("size", &size) < B_OK)
						size = -1;
					item->SetMarked(iconSize == size);
				}
			} else {
				for (int32 i = 0; BMenuItem* item = sizeMenu->ItemAt(i); i++)
					item->SetMarked(false);
			}
		}
		MarkNamedMenuItem(menu, kIconMode, viewMode == kIconMode);
		MarkNamedMenuItem(menu, kListMode, viewMode == kListMode);
		MarkNamedMenuItem(menu, kMiniIconMode, viewMode == kMiniIconMode);

		SetCloseItem(menu);
		SetCleanUpItem(menu);
		SetPasteItem(menu);

		EnableNamedMenuItem(menu, kOpenParentDir, !TargetModel()->IsRoot());
		EnableNamedMenuItem(menu, kEmptyTrash, count > 0);
		EnableNamedMenuItem(menu, B_SELECT_ALL, count > 0);

		BMenuItem* item = menu->FindItem(B_TRANSLATE("New"));
		if (item) {
			TemplatesMenu *templateMenu = dynamic_cast<TemplatesMenu *>(
				item->Submenu());
			if (templateMenu)
				templateMenu->UpdateMenuState();
		}
	}

	BuildAddOnMenu(menu);
}


void
BContainerWindow::LoadAddOn(BMessage *message)
{
	UpdateIfNeeded();

	entry_ref addonRef;
	status_t result = message->FindRef("refs", &addonRef);
	if (result != B_OK) {
		BString buffer(B_TRANSLATE("Error %error loading add-On %name."));
		buffer.ReplaceFirst("%error", strerror(result));
		buffer.ReplaceFirst("%name", addonRef.name);

		BAlert* alert = new BAlert("", buffer.String(),	B_TRANSLATE("Cancel"),
			0, 0, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->Go();
		return;
	}

	// add selected refs to message
	BMessage *refs = new BMessage(B_REFS_RECEIVED);

	BObjectList<BPose> *list = PoseView()->SelectionList();

	int32 index = 0;
	BPose *pose;
	while ((pose = list->ItemAt(index++)) != NULL)
		refs->AddRef("refs", pose->TargetModel()->EntryRef());

	refs->AddMessenger("TrackerViewToken", BMessenger(PoseView()));

	LaunchInNewThread("Add-on", B_NORMAL_PRIORITY, &AddOnThread, refs, addonRef,
		*TargetModel()->EntryRef());
}


BHandler *
BContainerWindow::ResolveSpecifier(BMessage *message, int32 index,
	BMessage *specifier, int32 form, const char	*property)
{
	if (strcmp(property, "Poses") == 0) {
//		PRINT(("BContainerWindow::ResolveSpecifier %s\n", property));
		message->PopSpecifier();
		return PoseView();
	}

	return _inherited::ResolveSpecifier(message, index, specifier,
		form, property);
}


PiggybackTaskLoop *
BContainerWindow::DelayedTaskLoop()
{
	if (!fTaskLoop)
		fTaskLoop = new PiggybackTaskLoop;

	return fTaskLoop;
}


bool
BContainerWindow::NeedsDefaultStateSetup()
{
	if (!TargetModel())
		return false;

	if (TargetModel()->IsRoot())
		// don't try to set up anything if we are root
		return false;

	WindowStateNodeOpener opener(this, false);
	if (!opener.StreamNode())
		// can't read state, give up
		return false;

	return !NodeHasSavedState(opener.Node());
}


bool
BContainerWindow::DefaultStateSourceNode(const char *name, BNode *result,
	bool createNew, bool createFolder)
{
//	PRINT(("looking for default state in tracker settings dir\n"));
	BPath settingsPath;
	if (FSFindTrackerSettingsDir(&settingsPath) != B_OK)
		return false;

	BDirectory dir(settingsPath.Path());

	BPath path(settingsPath);
	path.Append(name);
	if (!BEntry(path.Path()).Exists()) {
		if (!createNew)
			return false;

		BPath tmpPath(settingsPath);
		for (;;) {
			// deal with several levels of folders
			const char *nextSlash = strchr(name, '/');
			if (!nextSlash)
				break;

			BString tmp;
			tmp.SetTo(name, nextSlash - name);
			tmpPath.Append(tmp.String());

			mkdir(tmpPath.Path(), 0777);

			name = nextSlash + 1;
			if (!name[0]) {
				// can't deal with a slash at end
				return false;
			}
		}

		if (createFolder) {
			if (mkdir(path.Path(), 0777) < 0)
				return false;
		} else {
			BFile file;
			if (dir.CreateFile(name, &file) != B_OK)
				return false;
		}
	}

// 	PRINT(("using default state from %s\n", path.Path()));
	result->SetTo(path.Path());
	return result->InitCheck() == B_OK;
}


void
BContainerWindow::SetUpDefaultState()
{
	BNode defaultingNode;
		// this is where we'll ulitimately get the state from
	bool gotDefaultingNode = 0;
	bool shouldStagger = false;

	ASSERT(TargetModel());

	PRINT(("folder %s does not have any saved state\n", TargetModel()->Name()));

	WindowStateNodeOpener opener(this, true);
		// this is our destination node, whatever it is for this window
	if (!opener.StreamNode())
		return;

	if (!TargetModel()->IsRoot()) {
		BDirectory desktop;
		FSGetDeskDir(&desktop);

		// try copying state from our parent directory, unless it is the desktop folder
		BEntry entry(TargetModel()->EntryRef());
		BDirectory parent;
		if (entry.GetParent(&parent) == B_OK && parent != desktop) {
			PRINT(("looking at parent for state\n"));
			if (NodeHasSavedState(&parent)) {
				PRINT(("got state from parent\n"));
				defaultingNode = parent;
				gotDefaultingNode = true;
				// when getting state from parent, stagger the window
				shouldStagger = true;
			}
		}
	}

	if (!gotDefaultingNode
		// parent didn't have any state, use the template directory from
		// tracker settings folder for what our state should be
		// For simplicity we are not picking up the most recent
		// changes that didn't get committed if home is still open in
		// a window, that's probably not a problem; would be OK if state got committed
		// after every change
		&& !DefaultStateSourceNode(kDefaultFolderTemplate, &defaultingNode, true))
		return;

	// copy over the attributes

	// set up a filter of the attributes we want copied
	const char *allowAttrs[] = {
		kAttrWindowFrame,
		kAttrWindowWorkspace,
		kAttrViewState,
		kAttrViewStateForeign,
		kAttrColumns,
		kAttrColumnsForeign,
		0
	};

	// copy over attributes that apply; transform them properly, stripping
	// parts that do not apply, adding a window stagger, etc.

	StaggerOneParams params;
	params.rectFromParent = shouldStagger;
	SelectiveAttributeTransformer frameOffsetter(kAttrWindowFrame, OffsetFrameOne, &params);
	SelectiveAttributeTransformer scrollOriginCleaner(kAttrViewState,
		ClearViewOriginOne, &params);

	// do it
	AttributeStreamMemoryNode memoryNode;
	NamesToAcceptAttrFilter filter(allowAttrs);
	AttributeStreamFileNode fileNode(&defaultingNode);

	*opener.StreamNode() << scrollOriginCleaner << frameOffsetter
		<< memoryNode << filter << fileNode;
}


void
BContainerWindow::RestoreWindowState(AttributeStreamNode *node)
{
	if (!node || dynamic_cast<BDeskWindow *>(this))
		// don't restore any window state if we are a desktop window
		return;

	const char *rectAttributeName;
	const char *workspaceAttributeName;
	if (TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}

	BRect frame(Frame());
	if (node->Read(rectAttributeName, 0, B_RECT_TYPE, sizeof(BRect), &frame) == sizeof(BRect)) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	} else
		sNewWindRect.OffsetBy(kWindowStaggerBy, kWindowStaggerBy);

	fPreviousBounds = Bounds();

	uint32 workspace;
	if ((fContainerWindowFlags & kRestoreWorkspace)
		&& node->Read(workspaceAttributeName, 0, B_INT32_TYPE, sizeof(uint32), &workspace) == sizeof(uint32))
		SetWorkspaces(workspace);

	if (fContainerWindowFlags & kIsHidden)
		Minimize(true);

#ifdef __HAIKU__
	// restore window decor settings
	int32 size = node->Contains(kAttrWindowDecor, B_RAW_TYPE);
	if (size > 0) {
		char buffer[size];
		if ((fContainerWindowFlags & kRestoreDecor)
			&& node->Read(kAttrWindowDecor, 0, B_RAW_TYPE, size, buffer) == size) {
			BMessage decorSettings;
			if (decorSettings.Unflatten(buffer) == B_OK)
				SetDecoratorSettings(decorSettings);
		}
	}
#endif // __HAIKU__
}


void
BContainerWindow::RestoreWindowState(const BMessage &message)
{
	if (dynamic_cast<BDeskWindow *>(this))
		// don't restore any window state if we are a desktop window
		return;

	const char *rectAttributeName;
	const char *workspaceAttributeName;
	if (TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}

	BRect frame(Frame());
	if (message.FindRect(rectAttributeName, &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	} else
		sNewWindRect.OffsetBy(kWindowStaggerBy, kWindowStaggerBy);

	uint32 workspace;
	if ((fContainerWindowFlags & kRestoreWorkspace)
		&& message.FindInt32(workspaceAttributeName, (int32 *)&workspace) == B_OK)
		SetWorkspaces(workspace);

	if (fContainerWindowFlags & kIsHidden)
		Minimize(true);

#ifdef __HAIKU__
	// restore window decor settings
	BMessage decorSettings;
	if ((fContainerWindowFlags & kRestoreDecor)
		&& message.FindMessage(kAttrWindowDecor, &decorSettings) == B_OK) {
		SetDecoratorSettings(decorSettings);
	}
#endif // __HAIKU__
}


void
BContainerWindow::SaveWindowState(AttributeStreamNode *node)
{
	ASSERT(node);
	const char *rectAttributeName;
	const char *workspaceAttributeName;
	if (TargetModel() && TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}

	// node is null if it already got deleted
	BRect frame(Frame());
	node->Write(rectAttributeName, 0, B_RECT_TYPE, sizeof(BRect), &frame);

	uint32 workspaces = Workspaces();
	node->Write(workspaceAttributeName, 0, B_INT32_TYPE, sizeof(uint32),
		&workspaces);

#ifdef __HAIKU__
	BMessage decorSettings;
	if (GetDecoratorSettings(&decorSettings) == B_OK) {
		int32 size = decorSettings.FlattenedSize();
		char buffer[size];
		if (decorSettings.Flatten(buffer, size) == B_OK) {
			node->Write(kAttrWindowDecor, 0, B_RAW_TYPE, size, buffer);
		}
	}
#endif // __HAIKU__
}


void
BContainerWindow::SaveWindowState(BMessage &message) const
{
	const char *rectAttributeName;
	const char *workspaceAttributeName;

	if (TargetModel() && TargetModel()->IsRoot()) {
		rectAttributeName = kAttrDisksFrame;
		workspaceAttributeName = kAttrDisksWorkspace;
	} else {
		rectAttributeName = kAttrWindowFrame;
		workspaceAttributeName = kAttrWindowWorkspace;
	}

	// node is null if it already got deleted
	BRect frame(Frame());
	message.AddRect(rectAttributeName, frame);
	message.AddInt32(workspaceAttributeName, (int32)Workspaces());

#ifdef __HAIKU__
	BMessage decorSettings;
	if (GetDecoratorSettings(&decorSettings) == B_OK) {
		message.AddMessage(kAttrWindowDecor, &decorSettings);
	}
#endif // __HAIKU__
}


status_t
BContainerWindow::DragStart(const BMessage *incoming)
{
	if (!incoming)
		return B_ERROR;

	//	if already dragging, or
	//	if all the refs match
	if (Dragging() && SpringLoadedFolderCompareMessages(incoming, fDragMessage))
		return B_OK;

	//	cache the current drag message
	//	build a list of the mimetypes in the message
	SpringLoadedFolderCacheDragData(incoming, &fDragMessage, &fCachedTypesList);

	fWaitingForRefs = true;

	return B_OK;
}


void
BContainerWindow::DragStop()
{
	delete fDragMessage;
	fDragMessage = NULL;

	delete fCachedTypesList;
	fCachedTypesList = NULL;

	fWaitingForRefs = false;
}


void
BContainerWindow::ShowSelectionWindow()
{
	if (fSelectionWindow == NULL) {
		fSelectionWindow = new SelectionWindow(this);
		fSelectionWindow->Show();
	} else if (fSelectionWindow->Lock()) {
		if (fSelectionWindow->IsHidden()) {
			fSelectionWindow->MoveCloseToMouse();
			fSelectionWindow->Show();
		}
		fSelectionWindow->Unlock();
	}
}


// TODO: shouldn't this be done in BNavigator? or controller maybe?
void
BContainerWindow::SetSingleWindowBrowseShortcuts(bool enabled)
{
	if (PoseView()->IsDesktopWindow())
		return;

	if (enabled) {
		if (!Controller()->Navigator())
			return;

		RemoveShortcut(B_DOWN_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY | B_CONTROL_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY | B_CONTROL_KEY);

		AddShortcut(B_LEFT_ARROW, B_COMMAND_KEY,
			new BMessage(kNavigatorCommandBackward), Controller()->Navigator());
		AddShortcut(B_RIGHT_ARROW, B_COMMAND_KEY,
			new BMessage(kNavigatorCommandForward), Controller()->Navigator());
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY,
			new BMessage(kNavigatorCommandUp), Controller()->Navigator());

		AddShortcut(B_LEFT_ARROW, B_OPTION_KEY | B_COMMAND_KEY,
			new BMessage(kNavigatorCommandBackward), Controller()->Navigator());
		AddShortcut(B_RIGHT_ARROW, B_OPTION_KEY | B_COMMAND_KEY,
			new BMessage(kNavigatorCommandForward), Controller()->Navigator());
		AddShortcut(B_UP_ARROW, B_OPTION_KEY | B_COMMAND_KEY,
			new BMessage(kNavigatorCommandUp), Controller()->Navigator());

	} else {
		RemoveShortcut(B_LEFT_ARROW, B_COMMAND_KEY);
		RemoveShortcut(B_RIGHT_ARROW, B_COMMAND_KEY);
		RemoveShortcut(B_UP_ARROW, B_COMMAND_KEY);
			// This is added again, below, with a new meaning.

		RemoveShortcut(B_LEFT_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
		RemoveShortcut(B_RIGHT_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
		RemoveShortcut(B_UP_ARROW, B_OPTION_KEY | B_COMMAND_KEY);
			// This also changes meaning, added again below.

		AddShortcut(B_DOWN_ARROW, B_OPTION_KEY | B_COMMAND_KEY,
			new BMessage(kOpenSelection), PoseView());
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY,
			new BMessage(kOpenParentDir), PoseView());
			// We change the meaning from kNavigatorCommandUp to kOpenParentDir.
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY,
			new BMessage(kOpenParentDir), PoseView());
		AddShortcut(B_UP_ARROW, B_COMMAND_KEY | B_OPTION_KEY | B_CONTROL_KEY,
			new BMessage(kOpenParentDir), PoseView());
			// the command option results in closing the parent window
	}
}


void
BContainerWindow::SetPathWatchingEnabled(bool enable)
{
	if (IsPathWatchingEnabled()) {
		stop_watching(this);
		fIsWatchingPath = false;
	}

	if (enable) {
		if (TargetModel() != NULL) {
			BEntry entry;

			TargetModel()->GetEntry(&entry);
			status_t err;
			do {
				err = entry.GetParent(&entry);
				if (err != B_OK)
					break;

				char name[B_FILE_NAME_LENGTH];
				entry.GetName(name);
				if (strcmp(name, "/") == 0)
					break;

				node_ref ref;
				entry.GetNodeRef(&ref);
				watch_node(&ref, B_WATCH_NAME, this);
			} while (err == B_OK);

			fIsWatchingPath = err == B_OK;
		} else
			fIsWatchingPath = false;
	}
}


void
BContainerWindow::PulseTaskLoop()
{
	if (fTaskLoop)
		fTaskLoop->PulseMe();
}


//	#pragma mark -


WindowStateNodeOpener::WindowStateNodeOpener(BContainerWindow *window, bool forWriting)
	:	fModelOpener(NULL),
		fNode(NULL),
		fStreamNode(NULL)
{
	if (window->TargetModel() && window->TargetModel()->IsRoot()) {
		BDirectory dir;
		if (FSGetDeskDir(&dir) == B_OK) {
			fNode = new BDirectory(dir);
			fStreamNode = new AttributeStreamFileNode(fNode);
		}
	} else if (window->TargetModel()){
		fModelOpener = new ModelNodeLazyOpener(window->TargetModel(), forWriting, false);
		if (fModelOpener->IsOpen(forWriting))
			fStreamNode = new AttributeStreamFileNode(fModelOpener->TargetModel()->Node());
	}
}

WindowStateNodeOpener::~WindowStateNodeOpener()
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;
}


void
WindowStateNodeOpener::SetTo(const BDirectory *node)
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;

	fModelOpener = NULL;
	fNode = new BDirectory(*node);
	fStreamNode = new AttributeStreamFileNode(fNode);
}


void
WindowStateNodeOpener::SetTo(const BEntry *entry, bool forWriting)
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;

	fModelOpener = NULL;
	fNode = new BFile(entry, (uint32)(forWriting ? O_RDWR : O_RDONLY));
	fStreamNode = new AttributeStreamFileNode(fNode);
}


void
WindowStateNodeOpener::SetTo(Model *model, bool forWriting)
{
	delete fModelOpener;
	delete fNode;
	delete fStreamNode;

	fNode = NULL;
	fStreamNode = NULL;
	fModelOpener = new ModelNodeLazyOpener(model, forWriting, false);
	if (fModelOpener->IsOpen(forWriting))
		fStreamNode = new AttributeStreamFileNode(fModelOpener->TargetModel()->Node());
}


AttributeStreamNode *
WindowStateNodeOpener::StreamNode() const
{
	return fStreamNode;
}


BNode *
WindowStateNodeOpener::Node() const
{
	if (!fStreamNode)
		return NULL;

	if (fNode)
		return fNode;

	return fModelOpener->TargetModel()->Node();
}


//	#pragma mark -


BackgroundView::BackgroundView(BRect frame)
	:	BView(frame, "", B_FOLLOW_ALL,
			B_FRAME_EVENTS | B_WILL_DRAW | B_PULSE_NEEDED)
{
}


void
BackgroundView::AttachedToWindow()
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
BackgroundView::FrameResized(float, float)
{
	Invalidate();
}


void
BackgroundView::PoseViewFocused(bool focused)
{
	Invalidate();

	BContainerWindow* window = dynamic_cast<BContainerWindow*>(Window());
	if (!window)
		return;

	/* TODO BScrollBar* hScrollBar = window->PoseView()->HScrollBar();
	if (hScrollBar != NULL)
		hScrollBar->SetBorderHighlighted(focused);

	BScrollBar* vScrollBar = window->PoseView()->VScrollBar();
	if (vScrollBar != NULL)
		vScrollBar->SetBorderHighlighted(focused);

	BCountView* countView = window->PoseView()->CountView();
	if (countView != NULL)
		countView->SetBorderHighlighted(focused);*/
}


void
BackgroundView::WindowActivated(bool)
{
	Invalidate();
}


void
BackgroundView::Draw(BRect updateRect)
{
	BContainerWindow *window = dynamic_cast<BContainerWindow *>(Window());
	if (!window)
		return;

	/* TODO BPoseView* poseView = window->PoseView();
	BRect frame(poseView->Frame());
	frame.InsetBy(-1, -1);
	frame.top -= kTitleViewHeight;
	frame.bottom += B_H_SCROLL_BAR_HEIGHT;
	frame.right += B_V_SCROLL_BAR_WIDTH;

	if (be_control_look != NULL) {
		uint32 flags = 0;
		if (window->IsActive() && window->PoseView()->IsFocus())
			flags |= BControlLook::B_FOCUSED;

		frame.top--;
		frame.InsetBy(-1, -1);
		BRect rect(frame);
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

		BScrollBar* hScrollBar = poseView->HScrollBar();
		BScrollBar* vScrollBar = poseView->VScrollBar();

		BRect verticalScrollBarFrame(0, 0, -1, -1);
		if (vScrollBar)
			verticalScrollBarFrame = vScrollBar->Frame();
		BRect horizontalScrollBarFrame(0, 0, -1, -1);
		if (hScrollBar) {
			horizontalScrollBarFrame = hScrollBar->Frame();
			// CountView extends horizontal scroll bar frame:
			horizontalScrollBarFrame.left = frame.left + 1;
		}

		be_control_look->DrawScrollViewFrame(this, rect, updateRect,
			verticalScrollBarFrame, horizontalScrollBarFrame, base,
			B_FANCY_BORDER, flags);

		return;
	}

	SetHighColor(100, 100, 100);
	StrokeRect(frame);

	// draw the pose view focus
	if (window->IsActive() && window->PoseView()->IsFocus()) {
		frame.InsetBy(-2, -2);
		SetHighColor(keyboard_navigation_color());
		StrokeRect(frame);
	}*/
}


void
BackgroundView::Pulse()
{
	BContainerWindow *window = dynamic_cast<BContainerWindow *>(Window());
	if (window)
		window->PulseTaskLoop();
}

