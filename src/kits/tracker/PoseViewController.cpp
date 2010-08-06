/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */


#include "PoseViewController.h"

#include <Catalog.h>
#include <GridLayout.h>
#include <GroupLayout.h>
#include <LayoutItem.h>
#include <Locale.h>
#include <MenuBar.h>
#include <ScrollBar.h>

#include "Attributes.h"
#include "Commands.h"
#include "CountView.h"
#include "IconMenuItem.h"
#include "Navigator.h"
#include "PoseView.h"
#include "DesktopPoseView.h"
#include "PublicCommands.h"
#include "TitleView.h"
#include "TemplatesMenu.h"
#include "TrackerSettings.h"

#include <stdio.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "libtracker"


PoseViewController::PoseViewController()
	:
	fMenuBar(NULL),
	fAttrMenu(NULL),
	fWindowMenu(NULL),
	fFileMenu(NULL),
	fNavigator(NULL),
	fTitleView(NULL),
	fPoseView(NULL),
	fCountView(NULL),
	fVerticalScrollBar(NULL),
	fHorizontalScrollBar(NULL)
{
}


PoseViewController::~PoseViewController()	
{	
	// TODO
	if (fTitleView && !fTitleView->Window())
		delete fTitleView;
}


void
PoseViewController::SetPoseView(BPoseView* poseView)
{
	if (fPoseView != NULL)
		printf("Error! PoseViewController::SetPoseView poseview "
			"already set!\n");
	
	if (fPoseView == NULL) {
		fPoseView = poseView;
		fPoseView->SetController(this);
	}
}


// TODO: remove this, containerwindows will just create what they need.
void
PoseViewController::CreateControls(Model *model)
{
	if (fPoseView != NULL) {
		fMenuBar = new BMenuBar("MenuBar"); // BTrackerMenu
		fNavigator = new BNavigator(model);
		fTitleView = new BTitleView(fPoseView);
		fCountView = new BCountView(fPoseView);
		fHorizontalScrollBar =
			new BHScrollBar("HScrollBar", fPoseView, fTitleView);
		fVerticalScrollBar =
			new BScrollBar("VScrollBar", fPoseView, 0, 100, B_VERTICAL);
	} else {
		printf("Error! PoseViewController::CreateControls you must set a "
			"poseview before calling this method!");
	}
}


void
PoseViewController::SetControlVisible(BView* control, bool visible)
{
	if (control != NULL) {
		BLayout* layout = NULL;
		
		if (control->Parent() != NULL) {
			// control is on a view	
			layout = control->Parent()->GetLayout();
		} else if (control->Window() != NULL) {
			// control is on a window
			layout = control->Window()->GetLayout();
		}
		
		if (layout != NULL) {
			int32 index = layout->IndexOfView(control);
			layout->ItemAt(index)->SetVisible(visible);
		}
	}
}


// poseview events


void
PoseViewController::ItemCountChanged(uint32 itemCount)
{
	if (fCountView != NULL){
		fCountView->SetCount(itemCount);
	}
}


void
PoseViewController::SlowOperationStarted()
{
	if (fCountView != NULL){
		fCountView->ShowBarberPole();
	}
}


void
PoseViewController::SlowOperationEnded()
{
	if (fCountView != NULL){
		fCountView->HideBarberPole();
	}
}


// old tracker code imported review usefulness
void
PoseViewController::SetScrollBarsEnabled(bool enabled)
{	
	if (enabled) {
		printf("enabled\n");
		/*if (fHorizontalScrollBar)
			fHorizontalScrollBar->SetTarget((BView*)NULL);
		if (fVerticalScrollBar)
			fVerticalScrollBar->SetTarget((BView*)NULL);*/
	} else {
		printf("disabled\n");
		/*if (fHorizontalScrollBar)
			fHorizontalScrollBar->SetTarget(fPoseView);
		if (fVerticalScrollBar)
			fVerticalScrollBar->SetTarget(fPoseView);*/
	}
}


void
PoseViewController::SetScrollBarsTo(const BPoint& point)
{
	if (fHorizontalScrollBar && fVerticalScrollBar) {
		printf("cas 1\n");
		fHorizontalScrollBar->SetValue(point.x);
		fVerticalScrollBar->SetValue(point.y);
	} else {
		// TODO: I don't know what this was supposed to work around
		// (ie why it wasn't calling ScrollTo(point) simply). Although
		// it cannot have been tested, since it was broken before, I am
		// still leaving this, since I know there can be a subtle change in
		// behaviour (BView<->BScrollBar feedback effects) when scrolling
		// both directions at once versus separately.
		printf("cas 2\n");
		debugger("PoseViewController::SetScrollBarsTo CAS 2");
		BPoint origin = PoseView()->LeftTop();
		PoseView()->ScrollTo(BPoint(origin.x, point.y));
		PoseView()->ScrollTo(point);
	}
}


void
PoseViewController::UpdateScrollRange()
{
	// TODO: some calls to UpdateScrollRange don't do the right thing because
	// Extent doesn't return the right value (too early in PoseView lifetime??)
	//
	// This happened most with file panels, when opening a parent - added
	// an extra call to UpdateScrollRange in SelectChildInParent to work
	// around this

	AutoLock<BWindow> lock(fPoseView->Window());
	if (!lock)
		return;

	BRect bounds(fPoseView->Bounds());
	BPoint origin(fPoseView->LeftTop());
	BRect extent(fPoseView->Extent());
	
	lock.Unlock();

	BPoint minVal(std::min(extent.left, origin.x),
		std::min(extent.top, origin.y));

	BPoint maxVal((extent.right - bounds.right) + origin.x,
		(extent.bottom - bounds.bottom) + origin.y);

	maxVal.x = std::max(maxVal.x, origin.x);
	maxVal.y = std::max(maxVal.y, origin.y);

	if (fHorizontalScrollBar) {
		float scrollMin;
		float scrollMax;
		fHorizontalScrollBar->GetRange(&scrollMin, &scrollMax);
		if (minVal.x != scrollMin || maxVal.x != scrollMax) {
			fHorizontalScrollBar->SetRange(minVal.x, maxVal.x);
			fHorizontalScrollBar->SetSteps(kSmallStep, bounds.Width());
		}
	}

	if (fVerticalScrollBar) {
		float scrollMin;
		float scrollMax;
		fVerticalScrollBar->GetRange(&scrollMin, &scrollMax);

		if (minVal.y != scrollMin || maxVal.y != scrollMax) {
			fVerticalScrollBar->SetRange(minVal.y, maxVal.y);
			fVerticalScrollBar->SetSteps(kSmallStep, bounds.Height());
		}
	}

	// set proportions for bars
	BRect totalExtent(extent | bounds);
	
	if (fHorizontalScrollBar && totalExtent.Width() != 0.0) {
		float proportion = bounds.Width() / totalExtent.Width();
		if (fHorizontalScrollBar->Proportion() != proportion)
			fHorizontalScrollBar->SetProportion(proportion);
	}

	if (fVerticalScrollBar && totalExtent.Height() != 0.0) {
		float proportion = bounds.Height() / totalExtent.Height();
		if (fVerticalScrollBar->Proportion() != proportion)
			fVerticalScrollBar->SetProportion(proportion);
	}
	
	// TODO: autohiding scrollbars doesn't play well with the window"s
	//	 sizelimits
	/*if (fHorizontalScrollBar) {
		SetControlVisible(fHorizontalScrollBar,
			fHorizontalScrollBar->Proportion() < 1.0);
	}
	
	if (fVerticalScrollBar) {
		SetControlVisible(fVerticalScrollBar,
			fVerticalScrollBar->Proportion() < 1.0);
	}*/
}


void
PoseViewController::AddMenus()
{
	fFileMenu = new BMenu(B_TRANSLATE("File"));
	AddFileMenu(fFileMenu);
	fMenuBar->AddItem(fFileMenu);
	fWindowMenu = new BMenu(B_TRANSLATE("Window"));
	fMenuBar->AddItem(fWindowMenu);
	AddWindowMenu(fWindowMenu);
	// just create the attribute, decide to add it later
	fAttrMenu = new BMenu(B_TRANSLATE("Attributes"));
	NewAttributeMenu(fAttrMenu);
}


void
PoseViewController::AddFileMenu(BMenu *menu)
{
	if (!fPoseView->IsFilePanel()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS),
			new BMessage(kFindButton), 'F'));
	}

	if (!fPoseView->TargetModel()->IsQuery()
		&& !fPoseView->TargetModel()->IsTrash()
		&& !fPoseView->TargetModel()->IsPrintersDir()) {

		if (!fPoseView->IsFilePanel()) {
			TemplatesMenu* templateMenu = new TemplatesMenu(fPoseView,
				B_TRANSLATE("New"));
			menu->AddItem(templateMenu);
			templateMenu->SetTargetForItems(fPoseView);
		} else {
			menu->AddItem(new BMenuItem(B_TRANSLATE("New folder"),
				new BMessage(kNewFolder), 'N'));
		}
	}
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Get info"),
		new BMessage(kGetInfo), 'I'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));

	if (fPoseView->TargetModel()->IsTrash()
		|| fPoseView->TargetModel()->IsInTrash()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Restore"),
			new BMessage(kRestoreFromTrash)));
		if (fPoseView->TargetModel()->IsTrash()) {
			// add as first item in menu
			menu->AddItem(new BMenuItem(B_TRANSLATE("Empty Trash"),
				new BMessage(kEmptyTrash)), 0);
			menu->AddItem(new BSeparatorItem(), 1);
		}
	} else if (fPoseView->TargetModel()->IsPrintersDir()) {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Add printer"B_UTF8_ELLIPSIS),
			new BMessage(kAddPrinter), 'N'), 0);
		menu->AddItem(new BSeparatorItem(), 1);
		menu->AddItem(new BMenuItem(B_TRANSLATE("Make active printer"),
			new BMessage(kMakeActivePrinter)));
	} else {
		menu->AddItem(new BMenuItem(B_TRANSLATE("Duplicate"),
			new BMessage(kDuplicateSelection), 'D'));

		menu->AddItem(new BMenuItem(TrackerSettings().DontMoveFilesToTrash()
			? B_TRANSLATE("Delete")	: B_TRANSLATE("Move to Trash"),
			new BMessage(kMoveToTrash), 'T'));

		menu->AddSeparatorItem();

		// The "Move To", "Copy To", "Create Link" menus are inserted
		// at this place, have a look at:
		// BContainerWindow::SetupMoveCopyMenus()
	}

	BMenuItem *cutItem = NULL, *copyItem = NULL, *pasteItem = NULL;
	if (!fPoseView->TargetModel()->IsPrintersDir()) {
		menu->AddSeparatorItem();

		menu->AddItem(cutItem = new BMenuItem(B_TRANSLATE("Cut"),
			new BMessage(B_CUT), 'X'));
		menu->AddItem(copyItem = new BMenuItem(B_TRANSLATE("Copy"),
			new BMessage(B_COPY), 'C'));
		menu->AddItem(pasteItem = new BMenuItem(B_TRANSLATE("Paste"),
			new BMessage(B_PASTE), 'V'));

		menu->AddSeparatorItem();

		menu->AddItem(new BMenuItem(B_TRANSLATE("Identify"),
			new BMessage(kIdentifyEntry)));
		BMenu* addOnMenuItem = new BMenu(B_TRANSLATE("Add-ons"));
		addOnMenuItem->SetFont(be_plain_font);
		menu->AddItem(addOnMenuItem);
	}

	menu->SetTargetForItems(PoseView());
	if (cutItem)
		cutItem->SetTarget(PoseView()->ContainerWindow());
	if (copyItem)
		copyItem->SetTarget(PoseView()->ContainerWindow());
	if (pasteItem)
		pasteItem->SetTarget(PoseView()->ContainerWindow());
}


void
PoseViewController::AddWindowMenu(BMenu *menu)
{
	BMenuItem *item;

	BMenu* iconSizeMenu = new BMenu(B_TRANSLATE("Icon view"));

	BMessage* message = new BMessage(kIconMode);
	message->AddInt32("size", 32);
	item = new BMenuItem(B_TRANSLATE("32 x 32"), message);
	item->SetTarget(fPoseView);
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 40);
	item = new BMenuItem(B_TRANSLATE("40 x 40"), message);
	item->SetTarget(fPoseView);
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 48);
	item = new BMenuItem(B_TRANSLATE("48 x 48"), message);
	item->SetTarget(fPoseView);
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 64);
	item = new BMenuItem(B_TRANSLATE("64 x 64"), message);
	item->SetTarget(fPoseView);
	iconSizeMenu->AddItem(item);

	iconSizeMenu->AddSeparatorItem();

	message = new BMessage(kIconMode);
	message->AddInt32("scale", 0);
	item = new BMenuItem(B_TRANSLATE("Decrease size"), message, '-');
	item->SetTarget(fPoseView);
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("scale", 1);
	item = new BMenuItem(B_TRANSLATE("Increase size"), message, '+');
	item->SetTarget(fPoseView);
	iconSizeMenu->AddItem(item);

	// A sub menu where the super item can be invoked.
	menu->AddItem(iconSizeMenu);
	iconSizeMenu->Superitem()->SetShortcut('1', B_COMMAND_KEY);
	iconSizeMenu->Superitem()->SetMessage(new BMessage(kIconMode));
	iconSizeMenu->Superitem()->SetTarget(fPoseView);

	item = new BMenuItem(B_TRANSLATE("Mini icon view"),
		new BMessage(kMiniIconMode), '2');
	item->SetTarget(fPoseView);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("List view"),
		new BMessage(kListMode), '3');
	item->SetTarget(fPoseView);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Resize to fit"),
		new BMessage(kResizeToFit), 'Y');
	item->SetTarget(PoseView()->ContainerWindow());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Clean up"), new BMessage(kCleanup), 'K');
	item->SetTarget(fPoseView);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select"B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY);
	item->SetTarget(fPoseView);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select all"),	new BMessage(B_SELECT_ALL),
		'A');
	item->SetTarget(fPoseView);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Invert selection"),
		new BMessage(kInvertSelection),	'S');
	item->SetTarget(fPoseView);
	menu->AddItem(item);

	if (!fPoseView->TargetModel()->IsTrash()) {
		item = new BMenuItem(B_TRANSLATE("Open parent"),
			new BMessage(kOpenParentDir), B_UP_ARROW);
		item->SetTarget(fPoseView);
		menu->AddItem(item);
	}

	item = new BMenuItem(B_TRANSLATE("Close"), new BMessage(B_QUIT_REQUESTED),
		'W');
	item->SetTarget(PoseView()->ContainerWindow());
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Close all in workspace"),
		new BMessage(kCloseAllInWorkspace), 'Q');
	item->SetTarget(be_app);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Preferences" B_UTF8_ELLIPSIS),
		new BMessage(kShowSettingsWindow));
	item->SetTarget(be_app);
	menu->AddItem(item);
}


//


BMenuItem *
PoseViewController::NewAttributeMenuItem(const char *label, const char *name,
	int32 type, float width, int32 align, bool editable, bool statField)
{
	return NewAttributeMenuItem(label, name, type, NULL, width, align,
		editable, statField);
}


BMenuItem *
PoseViewController::NewAttributeMenuItem(const char *label, const char *name,
	int32 type, const char* displayAs, float width, int32 align,
	bool editable, bool statField)
{
	BMessage *message = new BMessage(kAttributeItem);
	message->AddString("attr_name", name);
	message->AddInt32("attr_type", type);
	message->AddInt32("attr_hash", (int32)AttrHashString(name, (uint32)type));
	message->AddFloat("attr_width", width);
	message->AddInt32("attr_align", align);
	if (displayAs != NULL)
		message->AddString("attr_display_as", displayAs);
	message->AddBool("attr_editable", editable);
	message->AddBool("attr_statfield", statField);

	BMenuItem *menuItem = new BMenuItem(label, message);
	menuItem->SetTarget(fPoseView);

	return menuItem;
}


void
PoseViewController::NewAttributeMenu(BMenu *menu)
{
	ASSERT(fPoseView);

	BMenuItem *item;
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Copy layout"),
		new BMessage(kCopyAttributes)));
	item->SetTarget(fPoseView);
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Paste layout"),
		new BMessage(kPasteAttributes)));
	item->SetTarget(fPoseView);
	menu->AddSeparatorItem();

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Name"),
		kAttrStatName, B_STRING_TYPE, 145, B_ALIGN_LEFT, true, true));

	menu->AddItem(NewAttributeMenuItem (B_TRANSLATE("Size"),
		kAttrStatSize, B_OFF_T_TYPE, 80, B_ALIGN_RIGHT, false, true));

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Modified"),
		kAttrStatModified, B_TIME_TYPE, 150, B_ALIGN_LEFT, false, true));

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Created"),
		kAttrStatCreated, B_TIME_TYPE, 150, B_ALIGN_LEFT, false, true));

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Kind"),
		kAttrMIMEType, B_MIME_STRING_TYPE, 145, B_ALIGN_LEFT, false, false));

	if (fPoseView->TargetModel()->IsTrash()
		|| fPoseView->TargetModel()->IsInTrash()) {
		menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Original name"),
			kAttrOriginalPath, B_STRING_TYPE, 225, B_ALIGN_LEFT, false, false));
	} else {
		menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Location"), kAttrPath,
			B_STRING_TYPE, 225, B_ALIGN_LEFT, false, false));
	}

#ifdef OWNER_GROUP_ATTRIBUTES
	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Owner"), kAttrStatOwner,
		B_STRING_TYPE, 60, B_ALIGN_LEFT, false, true));

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Group"), kAttrStatGroup,
		B_STRING_TYPE, 60, B_ALIGN_LEFT, false, true));
#endif

	menu->AddItem(NewAttributeMenuItem(B_TRANSLATE("Permissions"),
		kAttrStatMode, B_STRING_TYPE, 80, B_ALIGN_LEFT, false, true));
}


void
PoseViewController::ShowAttributeMenu()
{
	ASSERT(fAttrMenu);
	fMenuBar->AddItem(fAttrMenu);
}


void
PoseViewController::HideAttributeMenu()
{
	ASSERT(fAttrMenu);
	fMenuBar->RemoveItem(fAttrMenu);
}


void
PoseViewController::MarkAttributeMenu()
{
	MarkAttributeMenu(fAttrMenu);
}


void
PoseViewController::MarkAttributeMenu(BMenu *menu)
{
	if (!menu)
		return;

	int32 count = menu->CountItems();
	for (int32 index = 0; index < count; index++) {
		BMenuItem *item = menu->ItemAt(index);
		int32 attrHash;
		if (item->Message()) {
			if (item->Message()->FindInt32("attr_hash", &attrHash) == B_OK)
				item->SetMarked(fPoseView->ColumnFor((uint32)attrHash) != 0);
			else
				item->SetMarked(false);
		}

		BMenu *submenu = item->Submenu();
		if (submenu) {
			int32 count2 = submenu->CountItems();
			for (int32 subindex = 0; subindex < count2; subindex++) {
				item = submenu->ItemAt(subindex);
				if (item->Message()) {
					if (item->Message()->FindInt32("attr_hash", &attrHash)
						== B_OK) {
						item->SetMarked(fPoseView->ColumnFor((uint32)attrHash)
							!= 0);
					} else
						item->SetMarked(false);
				}
			}
		}
	}
}


void
PoseViewController::AddMimeTypesToMenu()
{
	AddMimeTypesToMenu(fAttrMenu);
}


/*!	Adds a menu for a specific MIME type if it doesn't exist already.
	Returns the menu, if it existed or not.
*/
BMenu*
PoseViewController::AddMimeMenu(const BMimeType& mimeType, bool isSuperType,
	BMenu* menu, int32 start)
{
	if (!mimeType.IsValid())
		return NULL;

	// Check if we already have an entry for this MIME type in the menu.
	for (int32 i = start; BMenuItem* item = menu->ItemAt(i); i++) {
		BMessage* message = item->Message();
		if (message == NULL)
			continue;

		const char* type;
		if (message->FindString("mimetype", &type) == B_OK
			&& !strcmp(mimeType.Type(), type)) {
			return item->Submenu();
		}
	}

	BMessage attrInfo;
	char description[B_MIME_TYPE_LENGTH];
	const char* label = mimeType.Type();

	if (!mimeType.IsInstalled())
		return NULL;

	// only add things to menu which have "user-visible" data
	if (mimeType.GetAttrInfo(&attrInfo) != B_OK)
		return NULL;

	if (mimeType.GetShortDescription(description) == B_OK && description[0])
		label = description;

	// go through each field in meta mime and add it to a menu
	BMenu* mimeMenu = NULL;
	if (isSuperType) {
		// If it is a supertype, we create the menu anyway as it may have
		// submenus later on.
		mimeMenu = new BMenu(label);
		BFont font;
		menu->GetFont(&font);
		mimeMenu->SetFont(&font);
	}

	int32 index = -1;
	const char* publicName;
	while (attrInfo.FindString("attr:public_name", ++index, &publicName)
			== B_OK) {
		if (!attrInfo.FindBool("attr:viewable", index)) {
			// don't add if attribute not viewable
			continue;
		}

		int32 type;
		int32 align;
		int32 width;
		bool editable;
		const char* attrName;
		if (attrInfo.FindString("attr:name", index, &attrName) != B_OK
			|| attrInfo.FindInt32("attr:type", index, &type) != B_OK
			|| attrInfo.FindBool("attr:editable", index, &editable) != B_OK
			|| attrInfo.FindInt32("attr:width", index, &width) != B_OK
			|| attrInfo.FindInt32("attr:alignment", index, &align) != B_OK)
			continue;

		BString displayAs;
		attrInfo.FindString("attr:display_as", index, &displayAs);

		if (mimeMenu == NULL) {
			// do a lazy allocation of the menu
			mimeMenu = new BMenu(label);
			BFont font;
			menu->GetFont(&font);
			mimeMenu->SetFont(&font);
		}
		mimeMenu->AddItem(NewAttributeMenuItem(publicName, attrName, type,
			displayAs.String(), width, align, editable, false));
	}

	if (mimeMenu == NULL)
		return NULL;

	BMessage* message = new BMessage(kMIMETypeItem);
	message->AddString("mimetype", mimeType.Type());
	menu->AddItem(new IconMenuItem(mimeMenu, message, mimeType.Type(),
		B_MINI_ICON));

	return mimeMenu;
}


void
PoseViewController::AddMimeTypesToMenu(BMenu *menu)
{
	if (!menu)
		return;

	// Remove old mime type menus
	int32 start = menu->CountItems();
	while (start > 0 && menu->ItemAt(start - 1)->Submenu() != NULL) {
		delete menu->RemoveItem(start - 1);
		start--;
	}

	// Add a separator item if there is none yet
	if (start > 0
		&& dynamic_cast<BSeparatorItem *>(menu->ItemAt(start - 1)) == NULL)
		menu->AddSeparatorItem();

	// Add MIME type in case we're a default query type window
	BPath path;
	if (fPoseView->TargetModel() != NULL) {
		fPoseView->TargetModel()->GetPath(&path);
		if (strstr(path.Path(), "/" kQueryTemplates "/") != NULL) {
			// demangle MIME type name
			BString name(fPoseView->TargetModel()->Name());
			name.ReplaceFirst('_', '/');

			fPoseView->AddMimeType(name.String());
		}
	}

	// Add MIME type menus

	int32 typeCount = fPoseView->CountMimeTypes();

	for (int32 index = 0; index < typeCount; index++) {
		BMimeType mimeType(fPoseView->MimeTypeAt(index));
		if (mimeType.InitCheck() == B_OK) {
			BMimeType superType;
			mimeType.GetSupertype(&superType);
			if (superType.InitCheck() == B_OK) {
				BMenu* superMenu = AddMimeMenu(superType, true, menu, start);
				if (superMenu != NULL) {
					// We have a supertype menu.
					AddMimeMenu(mimeType, false, superMenu, 0);
				}
			}
		}
	}

	// remove empty super menus, promote sub-types if needed

	for (int32 index = 0; index < typeCount; index++) {
		BMimeType mimeType(fPoseView->MimeTypeAt(index));
		BMimeType superType;
		mimeType.GetSupertype(&superType);

		BMenu* superMenu = AddMimeMenu(superType, true, menu, start);
		if (superMenu == NULL)
			continue;

		int32 itemsFound = 0;
		int32 menusFound = 0;
		for (int32 i = 0; BMenuItem* item = superMenu->ItemAt(i); i++) {
			if (item->Submenu() != NULL)
				menusFound++;
			else
				itemsFound++;
		}

		if (itemsFound == 0) {
			if (menusFound != 0) {
				// promote types to the top level
				while (BMenuItem* item = superMenu->RemoveItem((int32)0)) {
					menu->AddItem(item);
				}
			}

			menu->RemoveItem(superMenu->Superitem());
			delete superMenu->Superitem();
		}
	}

	// remove separator if it's the only item in menu
	BMenuItem *item = menu->ItemAt(menu->CountItems() - 1);
	if (dynamic_cast<BSeparatorItem *>(item) != NULL) {
		menu->RemoveItem(item);
		delete item;
	}

	MarkAttributeMenu(menu);
}


//	#pragma mark -


BHScrollBar::BHScrollBar(const char *name, BView *target,
	BTitleView *titleView)
	:
	BScrollBar(name, target, 0, 100, B_HORIZONTAL),
	fTitleView(titleView)
{
}


void
BHScrollBar::ValueChanged(float value)
{
	if (fTitleView) {
		BPoint origin = fTitleView->LeftTop();
		fTitleView->ScrollTo(BPoint(value, origin.y));
	}

	_inherited::ValueChanged(value);
}
