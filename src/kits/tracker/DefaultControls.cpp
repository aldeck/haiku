/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */


#include "DefaultControls.h"

#include <Application.h>
#include <Catalog.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Locale.h>
#include <MenuItem.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include "Attributes.h"
#include "Commands.h"
#include "IconMenuItem.h"
#include "FavoritesMenu.h"
#include "FSUtils.h"
#include "MimeTypes.h"
#include "PoseView.h"
#include "PoseViewController.h"
#include "TemplatesMenu.h"
#include "Tracker.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "Menus"


const float kMaxMenuWidth = 150; // OpenWithMenu


//	#pragma mark - DefaultFileContextMenu


DefaultFileContextMenu::DefaultFileContextMenu(PoseViewController* controller)
	:
	BPopUpMenu("FileContext", false, false)
{
	SetFont(be_plain_font);	// TODO: that's legacy code, is it still usefull on Haiku?

	AddItem(new BMenuItem(B_TRANSLATE("Open"),
		new BMessage(kOpenSelection), 'O'));
	AddItem(new BMenuItem(B_TRANSLATE("Get info"), new BMessage(kGetInfo),
		'I'));
	AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
		new BMessage(kEditItem), 'E'));

	Model* targetModel = controller->PoseView()->TargetModel();

	if (!targetModel->IsTrash()
		&& !targetModel->IsInTrash()
		&& !targetModel->IsPrintersDir()) {

		AddItem(new BMenuItem(B_TRANSLATE("Duplicate"),
			new BMessage(kDuplicateSelection), 'D'));
	}

	if (!targetModel->IsTrash()
		&& !targetModel->IsInTrash()) {

		AddItem(new BMenuItem(TrackerSettings().DontMoveFilesToTrash()
			? B_TRANSLATE("Delete")	: B_TRANSLATE("Move to Trash"),
			new BMessage(kMoveToTrash), 'T'));

		// add separator for copy to/move to items (navigation items)
		AddSeparatorItem();
	} else {
		AddItem(new BMenuItem(B_TRANSLATE("Delete"),
			new BMessage(kDelete), 0));
		AddItem(new BMenuItem(B_TRANSLATE("Restore"),
			new BMessage(kRestoreFromTrash), 0));
	}

#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	AddSeparatorItem();
	BMenuItem *cutItem, *copyItem;
	AddItem(cutItem = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	AddItem(copyItem = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
#endif

	AddSeparatorItem();
	AddItem(new BMenuItem(B_TRANSLATE("Identify"),
		new BMessage(kIdentifyEntry)));
	BMenu* addOnMenuItem = new BMenu(B_TRANSLATE("Add-ons"));
	addOnMenuItem->SetFont(be_plain_font);
	AddItem(addOnMenuItem);

	// set targets as needed
	SetTargetForItems(controller->PoseView());
#ifdef CUT_COPY_PASTE_IN_CONTEXT_MENU
	cutItem->SetTarget(controller->PoseView()->Window());
	copyItem->SetTarget(controller->PoseView()->Window());
#endif
}


//	#pragma mark - DefaultFileMenu


DefaultFileMenu::DefaultFileMenu(PoseViewController* controller)
	:
	BMenu(B_TRANSLATE("File")),
	fController(controller)
{
	TargetModelChanged();
	SelectionChanged();
}


void
DefaultFileMenu::AttachedToWindow()
{
	// This method is called just before showing the menu
	Model* model = fController->PoseView()->TargetModel();
	if (!model->IsTrash()
		&& !model->IsInTrash()
		&& !model->IsPrintersDir()) {

		fController->ReparentSharedMenus(this);
			// Steal the shared submenus

		// TODO: review the context menu case
	}

	// Set the "Identify" item label
	BMenuItem* item = FindItem(kIdentifyEntry);
	if (item != NULL) {
		if (modifiers() & B_SHIFT_KEY)
			item->SetLabel(B_TRANSLATE("Force identify"));
		else
			item->SetLabel(B_TRANSLATE("Identify"));
	}

	// Set the "Create Link" item label
	item = FindItem(kCreateLink);
	if (item == NULL)
		item = FindItem(kCreateRelativeLink);

	if (item != NULL) {
		if (modifiers() & B_SHIFT_KEY)
			item->SetLabel(B_TRANSLATE("Create relative link"));
		else
			item->SetLabel(B_TRANSLATE("Create link"));
	}

	BMenu::AttachedToWindow();
}


void
DefaultFileMenu::TargetModelChanged()
{
	Model* model = fController->PoseView()->TargetModel();

	// this ensures proper locking before emptying the menu
	if (!IsHidden())
		Hide();

	fController->ReparentSharedMenus(NULL);
		// detach shared menus, so they are not deleted with RemoveItems below

	RemoveItems(0, CountItems(), true);

	if (model->IsRoot()) {
		AddItem(new BMenuItem(B_TRANSLATE("Find"B_UTF8_ELLIPSIS),
			new BMessage(kFindButton), 'F'));
		AddSeparatorItem();

		AddItem(new BMenuItem(B_TRANSLATE("Open"),
			new BMessage(kOpenSelection), 'O'));
		AddItem(new BMenuItem(B_TRANSLATE("Get info"),
			new BMessage(kGetInfo), 'I'));
		AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
			new BMessage(kEditItem), 'E'));

		BMenuItem* item = new BMenuItem(B_TRANSLATE("Unmount"),
			new BMessage(kUnmountVolume), 'U');
		item->SetEnabled(false);
		AddItem(item);

		AddItem(new BMenuItem(B_TRANSLATE("Mount settings" B_UTF8_ELLIPSIS),
			new BMessage(kRunAutomounterSettings)));

		AddSeparatorItem();
		AddItem(new BMenu(B_TRANSLATE("Add-ons")));
		SetTargetForItems(fController->PoseView());
	} else {

		if (!fController->PoseView()->IsFilePanel()) {
			AddItem(new BMenuItem(B_TRANSLATE("Find" B_UTF8_ELLIPSIS),
				new BMessage(kFindButton), 'F'));
		}

		if (!model->IsQuery()
			&& !model->IsTrash()
			&& !model->IsPrintersDir()) {

			if (!fController->PoseView()->IsFilePanel()) {
				TemplatesMenu* templateMenu = new TemplatesMenu(fController->PoseView(),
					B_TRANSLATE("New"));
				AddItem(templateMenu);
				templateMenu->SetTargetForItems(fController->PoseView());
			} else {
				AddItem(new BMenuItem(B_TRANSLATE("New folder"),
					new BMessage(kNewFolder), 'N'));
			}
		}
		AddSeparatorItem();

		AddItem(new BMenuItem(B_TRANSLATE("Open"),
			new BMessage(kOpenSelection), 'O'));
		AddItem(new BMenuItem(B_TRANSLATE("Get info"),
			new BMessage(kGetInfo), 'I'));
		AddItem(new BMenuItem(B_TRANSLATE("Edit name"),
			new BMessage(kEditItem), 'E'));

		if (model->IsTrash() || model->IsInTrash()) {
			AddItem(new BMenuItem(B_TRANSLATE("Restore"),
				new BMessage(kRestoreFromTrash)));
			if (model->IsTrash()) {
				// add as first item in menu
				AddItem(new BMenuItem(B_TRANSLATE("Empty Trash"),
					new BMessage(kEmptyTrash)), 0);
				AddItem(new BSeparatorItem(), 1);
			}
		} else if (model->IsPrintersDir()) {
			AddItem(new BMenuItem(B_TRANSLATE("Add printer"B_UTF8_ELLIPSIS),
				new BMessage(kAddPrinter), 'N'), 0);
			AddItem(new BSeparatorItem(), 1);
			AddItem(new BMenuItem(B_TRANSLATE("Make active printer"),
				new BMessage(kMakeActivePrinter)));
		} else {
			AddItem(new BMenuItem(B_TRANSLATE("Duplicate"),
				new BMessage(kDuplicateSelection), 'D'));

			AddItem(new BMenuItem(TrackerSettings().DontMoveFilesToTrash()
				? B_TRANSLATE("Delete")	: B_TRANSLATE("Move to Trash"),
				new BMessage(kMoveToTrash), 'T'));

			AddSeparatorItem();

			// The "Move To", "Copy To", "Create Link" menus are inserted
			// at this place, have a look at:
			// BContainerWindow::SetupMoveCopyMenus()
		}

		BMenuItem *cutItem = NULL, *copyItem = NULL, *pasteItem = NULL;
		if (!model->IsPrintersDir()) {
			AddSeparatorItem();

			AddItem(cutItem = new BMenuItem(B_TRANSLATE("Cut"),
				new BMessage(B_CUT), 'X'));
			AddItem(copyItem = new BMenuItem(B_TRANSLATE("Copy"),
				new BMessage(B_COPY), 'C'));
			AddItem(pasteItem = new BMenuItem(B_TRANSLATE("Paste"),
				new BMessage(B_PASTE), 'V'));

			AddSeparatorItem();

			AddItem(new BMenuItem(B_TRANSLATE("Identify"),
				new BMessage(kIdentifyEntry)));
			BMenu* addOnMenuItem = new BMenu(B_TRANSLATE("Add-ons"));
			addOnMenuItem->SetFont(be_plain_font);
			AddItem(addOnMenuItem);
		}

		SetTargetForItems(fController->PoseView());
		if (cutItem)
			cutItem->SetTarget(fController->PoseView()->Window());
		if (copyItem)
			copyItem->SetTarget(fController->PoseView()->Window());
		if (pasteItem)
			pasteItem->SetTarget(fController->PoseView()->Window());
	}
}


void
DefaultFileMenu::SelectionChanged()
{
	PoseList* selection = fController->PoseView()->SelectionList();
	Model* model = fController->PoseView()->TargetModel();

	if (model->IsRoot()) {
		BVolume boot;
		BVolumeRoster().GetBootVolume(&boot);

		bool ejectableVolumeSelected = false;
		int32 count = selection->CountItems();
		for (int32 index = 0; index < count; index++) {
			Model *model = selection->ItemAt(index)->TargetModel();
			if (model->IsVolume()) {
				BVolume volume;
				volume.SetTo(model->NodeRef()->device);
				if (volume != boot) {
					ejectableVolumeSelected = true;
					break;
				}
			}
		}

		BMenuItem* item = FindItem(kUnmountVolume);
		if (item)
			item->SetEnabled(ejectableVolumeSelected);

	} else if (model->IsPrintersDir()) {

		BMenuItem* item = FindItem(kMakeActivePrinter);
		if (item)
			item->SetEnabled(selection->CountItems() == 1);
	}
}


//	#pragma mark - DefaultMoveMenu


DefaultMoveMenu::DefaultMoveMenu(const char* itemName, uint32 messageWhat,
	PoseViewController* controller)
	:
	BNavMenu(itemName, messageWhat, controller->PoseView()->ContainerWindow()),
	fController(controller),
	fMessageWhat(messageWhat)
{
	TargetModelChanged();
	SelectionChanged();
}


void
DefaultMoveMenu::TargetModelChanged()
{
}


void
DefaultMoveMenu::SelectionChanged()
{
	const entry_ref* firstRef
		= fController->PoseView()->SelectionList()->CountItems() > 0
		? fController->PoseView()->SelectionList()->FirstItem()->TargetModel()->EntryRef()
		: NULL;

	_Populate(firstRef, false);
}


void
DefaultMoveMenu::_Populate(const entry_ref *ref, bool addLocalOnly)
{
	if (ref == NULL) {
		SetEnabled(false);
		return;
	}

	BEntry entry;
	if (entry.SetTo(ref) != B_OK)
		return;

	// TODO check if the entry has changed since the last _Populate?

	BHandler* target = fController->PoseView()->ContainerWindow();

	BVolume volume;
	BVolumeRoster volumeRoster;
	BDirectory directory;
	BPath path;
	Model model;
	dev_t device = ref->device;

	int32 volumeCount = 0;

	// disable while populating
	SetEnabled(false);

	RemoveItems(0, CountItems(), true);

	// count persistent writable volumes
	volumeRoster.Rewind();
	while (volumeRoster.GetNextVolume(&volume) == B_OK)
		if (!volume.IsReadOnly() && volume.IsPersistent())
			volumeCount++;

	// add the current folder
	if (entry.SetTo(ref) == B_OK
		&& entry.GetParent(&entry) == B_OK
		&& model.SetTo(&entry) == B_OK) {
		BNavMenu* menu = new BNavMenu(B_TRANSLATE("Current folder"), fMessageWhat,
			target);

		menu->SetNavDir(model.EntryRef());
		menu->SetShowParent(true);

		BMenuItem *item = new SpecialModelMenuItem(&model,menu);
		item->SetMessage(new BMessage((uint32)fMessageWhat));

		AddItem(item);
	}

	// add the recent folder menu
	// the "Tracker" settings directory is only used to get its icon
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("Tracker");
		if (entry.SetTo(path.Path()) == B_OK
			&& model.SetTo(&entry) == B_OK) {
			BMenu* menu = new RecentsMenu(B_TRANSLATE("Recent folders"),
				kRecentFolders, fMessageWhat, target);

			BMenuItem *item = new SpecialModelMenuItem(&model,menu);
			item->SetMessage(new BMessage(fMessageWhat));

			AddItem(item);
		}
	}

	// add Desktop
	FSGetBootDeskDir(&directory);
	if (directory.InitCheck() == B_OK
		&& directory.GetEntry(&entry) == B_OK
		&& model.SetTo(&entry) == B_OK)
		AddNavDir(&model, fMessageWhat, target, true);
			// ask NavMenu to populate submenu for us

	// add the home dir
	if (find_directory(B_USER_DIRECTORY, &path) == B_OK
		&& entry.SetTo(path.Path()) == B_OK
		&& model.SetTo(&entry) == B_OK)
		AddNavDir(&model, fMessageWhat, target, true);

	AddSeparatorItem();

	// either add all mounted volumes (for copy), or all the top-level
	// directories from the same device (for move)
	// ToDo: can be changed if cross-device moves are implemented

	if (addLocalOnly || volumeCount < 2) {
		// add volume this item lives on
		if (volume.SetTo(device) == B_OK
			&& volume.GetRootDirectory(&directory) == B_OK
			&& directory.GetEntry(&entry) == B_OK
			&& model.SetTo(&entry) == B_OK) {
			AddNavDir(&model, fMessageWhat, target, false);
				// do not have submenu populated

			SetNavDir(model.EntryRef());
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
				AddNavDir(&model, fMessageWhat, target, true);
					// ask NavMenu to populate submenu for us
		}
	}

	SetEnabled(true);
}


//	#pragma mark - DefaultWindowMenu


DefaultWindowMenu::DefaultWindowMenu(PoseViewController* controller)
	:
	BMenu(B_TRANSLATE("Window"))
{
	BMenuItem *item;

	BPoseView* poseView = controller->PoseView();

	BMenu* iconSizeMenu = new BMenu(B_TRANSLATE("Icon view"));

	BMessage* message = new BMessage(kIconMode);
	message->AddInt32("size", 32);
	item = new BMenuItem(B_TRANSLATE("32 x 32"), message);
	item->SetTarget(poseView);
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 40);
	item = new BMenuItem(B_TRANSLATE("40 x 40"), message);
	item->SetTarget(poseView);
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 48);
	item = new BMenuItem(B_TRANSLATE("48 x 48"), message);
	item->SetTarget(poseView);
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("size", 64);
	item = new BMenuItem(B_TRANSLATE("64 x 64"), message);
	item->SetTarget(poseView);
	iconSizeMenu->AddItem(item);

	iconSizeMenu->AddSeparatorItem();

	message = new BMessage(kIconMode);
	message->AddInt32("scale", 0);
	item = new BMenuItem(B_TRANSLATE("Decrease size"), message, '-');
	item->SetTarget(poseView);
	iconSizeMenu->AddItem(item);

	message = new BMessage(kIconMode);
	message->AddInt32("scale", 1);
	item = new BMenuItem(B_TRANSLATE("Increase size"), message, '+');
	item->SetTarget(poseView);
	iconSizeMenu->AddItem(item);

	// A sub menu where the super item can be invoked.
	AddItem(iconSizeMenu);
	iconSizeMenu->Superitem()->SetShortcut('1', B_COMMAND_KEY);
	iconSizeMenu->Superitem()->SetMessage(new BMessage(kIconMode));
	iconSizeMenu->Superitem()->SetTarget(controller->PoseView());

	item = new BMenuItem(B_TRANSLATE("Mini icon view"),
		new BMessage(kMiniIconMode), '2');
	item->SetTarget(poseView);
	AddItem(item);

	item = new BMenuItem(B_TRANSLATE("List view"),
		new BMessage(kListMode), '3');
	item->SetTarget(poseView);
	AddItem(item);

	AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Resize to fit"),
		new BMessage(kResizeToFit), 'Y');
	item->SetTarget(poseView->ContainerWindow());
	AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Clean up"), new BMessage(kCleanup), 'K');
	item->SetTarget(poseView);
	AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select"B_UTF8_ELLIPSIS),
		new BMessage(kShowSelectionWindow), 'A', B_SHIFT_KEY);
	item->SetTarget(poseView);
	AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Select all"),	new BMessage(B_SELECT_ALL),
		'A');
	item->SetTarget(poseView);
	AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Invert selection"),
		new BMessage(kInvertSelection),	'S');
	item->SetTarget(poseView);
	AddItem(item);

	if (!poseView->TargetModel()->IsTrash()) {
		item = new BMenuItem(B_TRANSLATE("Open parent"),
			new BMessage(kOpenParentDir), B_UP_ARROW);
		item->SetTarget(poseView);
		AddItem(item);
	}

	item = new BMenuItem(B_TRANSLATE("Close"), new BMessage(B_QUIT_REQUESTED),
		'W');
	item->SetTarget(poseView->Window());
	AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Close all in workspace"),
		new BMessage(kCloseAllInWorkspace), 'Q');
	item->SetTarget(be_app);
	AddItem(item);

	AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Preferences" B_UTF8_ELLIPSIS),
		new BMessage(kShowSettingsWindow));
	item->SetTarget(be_app);
	AddItem(item);
}


//	#pragma mark - DefaultAttributeMenu


DefaultAttributeMenu::DefaultAttributeMenu(PoseViewController* controller)
	:
	BPopUpMenu(B_TRANSLATE("Attributes"), false, false),
	fController(controller)
{
	BPoseView* poseView = controller->PoseView();

	BMenuItem *item;
	AddItem(item = new BMenuItem(B_TRANSLATE("Copy layout"),
		new BMessage(kCopyAttributes)));
	item->SetTarget(poseView);
	AddItem(item = new BMenuItem(B_TRANSLATE("Paste layout"),
		new BMessage(kPasteAttributes)));
	item->SetTarget(poseView);
	AddSeparatorItem();

	AddItem(_NewItem(B_TRANSLATE("Name"),
		kAttrStatName, B_STRING_TYPE, 145, B_ALIGN_LEFT, true, true));

	if (gLocalizedNamePreferred) {
		AddItem(_NewItem(B_TRANSLATE("Real name"),
			kAttrRealName, B_STRING_TYPE, 145, B_ALIGN_LEFT, true, true));
	}

	AddItem(_NewItem (B_TRANSLATE("Size"), kAttrStatSize, B_OFF_T_TYPE,
		80, B_ALIGN_RIGHT, false, true));

	AddItem(_NewItem(B_TRANSLATE("Modified"),
		kAttrStatModified, B_TIME_TYPE, 150, B_ALIGN_LEFT, false, true));

	AddItem(_NewItem(B_TRANSLATE("Created"),
		kAttrStatCreated, B_TIME_TYPE, 150, B_ALIGN_LEFT, false, true));

	AddItem(_NewItem(B_TRANSLATE("Kind"),
		kAttrMIMEType, B_MIME_STRING_TYPE, 145, B_ALIGN_LEFT, false, false));

	if (poseView->TargetModel()->IsTrash()
		|| poseView->TargetModel()->IsInTrash()) {
		AddItem(_NewItem(B_TRANSLATE("Original name"),
			kAttrOriginalPath, B_STRING_TYPE, 225, B_ALIGN_LEFT, false, false));
	} else {
		AddItem(_NewItem(B_TRANSLATE("Location"), kAttrPath,
			B_STRING_TYPE, 225, B_ALIGN_LEFT, false, false));
	}

#ifdef OWNER_GROUP_ATTRIBUTES
	AddItem(_NewItem(B_TRANSLATE("Owner"), kAttrStatOwner,
		B_STRING_TYPE, 60, B_ALIGN_LEFT, false, true));

	AddItem(_NewItem(B_TRANSLATE("Group"), kAttrStatGroup,
		B_STRING_TYPE, 60, B_ALIGN_LEFT, false, true));
#endif

	AddItem(_NewItem(B_TRANSLATE("Permissions"),
		kAttrStatMode, B_STRING_TYPE, 80, B_ALIGN_LEFT, false, true));
}


void
DefaultAttributeMenu::MimeTypesChanged()
{
	BPoseView* poseView = fController->PoseView();

	// Remove old mime type menus
	int32 start = CountItems();
	while (start > 0 && ItemAt(start - 1)->Submenu() != NULL) {
		delete RemoveItem(start - 1);
		start--;
	}

	// Add a separator item if there is none yet
	if (start > 0
		&& dynamic_cast<BSeparatorItem *>(ItemAt(start - 1)) == NULL)
		AddSeparatorItem();

	// Add MIME type in case we're a default query type window
	BPath path;
	if (poseView->TargetModel() != NULL) {
		poseView->TargetModel()->GetPath(&path);
		if (path.InitCheck() == B_OK
			&& strstr(path.Path(), "/" kQueryTemplates "/") != NULL) {
			// demangle MIME type name
			BString name(poseView->TargetModel()->Name());
			name.ReplaceFirst('_', '/');

			poseView->AddMimeType(name.String());		// TODO: review
		}
	}

	// Add MIME type menus

	int32 typeCount = poseView->CountMimeTypes();

	for (int32 index = 0; index < typeCount; index++) {
		BMimeType mimeType(poseView->MimeTypeAt(index));
		if (mimeType.InitCheck() == B_OK) {
			BMimeType superType;
			mimeType.GetSupertype(&superType);
			if (superType.InitCheck() == B_OK) {
				BMenu* superMenu = _AddMimeMenu(superType, true, this, start);
				if (superMenu != NULL) {
					// We have a supertype menu.
					_AddMimeMenu(mimeType, false, superMenu, 0);
				}
			}
		}
	}

	// remove empty super menus, promote sub-types if needed

	for (int32 index = 0; index < typeCount; index++) {
		BMimeType mimeType(poseView->MimeTypeAt(index));
		BMimeType superType;
		mimeType.GetSupertype(&superType);

		BMenu* superMenu = _AddMimeMenu(superType, true, this, start);
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
					AddItem(item);
				}
			}

			RemoveItem(superMenu->Superitem());
			delete superMenu->Superitem();
		}
	}

	// remove separator if it's the only item in menu
	BMenuItem *item = ItemAt(CountItems() - 1);
	if (dynamic_cast<BSeparatorItem *>(item) != NULL) {
		RemoveItem(item);
		delete item;
	}

	_MarkItems();
}


void
DefaultAttributeMenu::ColumnsChanged()
{
	_MarkItems();
}


void
DefaultAttributeMenu::_MarkItems()
{
	int32 count = CountItems();
	for (int32 index = 0; index < count; index++) {
		BMenuItem *item = ItemAt(index);
		int32 attrHash;
		if (item->Message()) {
			if (item->Message()->FindInt32("attr_hash", &attrHash) == B_OK)
				item->SetMarked(fController->PoseView()->ColumnFor((uint32)attrHash) != 0);
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
						item->SetMarked(fController->PoseView()->ColumnFor((uint32)attrHash)
							!= 0);
					} else
						item->SetMarked(false);
				}
			}
		}
	}
}


BMenuItem *
DefaultAttributeMenu::_NewItem(const char *label, const char *name,
	int32 type, float width, int32 align, bool editable, bool statField)
{
	return _NewItem(label, name, type, NULL, width, align,
		editable, statField);
}


BMenuItem *
DefaultAttributeMenu::_NewItem(const char *label, const char *name,
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
	menuItem->SetTarget(fController->PoseView());

	return menuItem;
}


/*!	Adds a menu for a specific MIME type if it doesn't exist already.
	Returns the menu, if it existed or not.
*/
BMenu*
DefaultAttributeMenu::_AddMimeMenu(const BMimeType& mimeType, bool isSuperType,
	BMenu* menu, int32 start)
{
	AutoLock<BLooper> _(menu->Looper());
	
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
		mimeMenu->AddItem(_NewItem(publicName, attrName, type,
			displayAs.String(), width, align, editable, false));
	}

	if (mimeMenu == NULL)
		return NULL;

	BMessage* message = new BMessage(kMIMETypeItem);
	message->AddString("mimetype", mimeType.Type());
	AddItem(new IconMenuItem(mimeMenu, message, mimeType.Type(),
		B_MINI_ICON));

	return mimeMenu;
}


//	#pragma mark - OpenWithMenu


OpenWithMenu::OpenWithMenu(const char *label, PoseViewController* controller)
	: 
	BSlowMenu(label),
	fIterator(NULL),
	fSupportingAppList(NULL),
	fController(controller)
{
	InitIconPreloader();

	SetFont(be_plain_font);

	// too long to have triggers
	SetTriggersEnabled(false);
}

namespace BPrivate {

int
SortByRelationAndName(const RelationCachingModelProxy *model1,
	const RelationCachingModelProxy *model2, void *castToMenu)
{
	OpenWithMenu *menu = (OpenWithMenu *)castToMenu;

	// find out the relations of app models to the opened entries
	int32 relation1 = model1->Relation(menu->fIterator, &menu->fEntriesToOpen);
	int32 relation2 = model2->Relation(menu->fIterator, &menu->fEntriesToOpen);

	if (relation1 < relation2) {
		// relation with the lowest number goes first
		return 1;
	} else if (relation1 > relation2)
		return -1;

	// if relations match, sort by app name
	return strcmp(model1->fModel->Name(), model2->fModel->Name());
}

} // namespace BPrivate


void
OpenWithMenu::AttachedToWindow()
{
	// Emtpy the menu
	RemoveItems(0, CountItems(), true);

	int32 count = fController->PoseView()->SelectionList()->CountItems();

	if (count == 0 || fController->PoseView()->TargetModel()->IsRoot())
		return;

	// old TODO:
	// check if only item in selection list is the root
	// and do not add if true

	// build a list of all refs to open	
	BMessage message(B_REFS_RECEIVED);
	message.AddMessenger("TrackerViewToken", BMessenger(fController->PoseView()));
	for (int32 index = 0; index < count; index++) {
		BPose *pose = fController->PoseView()->SelectionList()->ItemAt(index);
		message.AddRef("refs", pose->TargetModel()->EntryRef());
	}
	fEntriesToOpen = message;
	fMenuBuilt = false;

	BSlowMenu::AttachedToWindow();
}


void
OpenWithMenu::TargetModelChanged()
{
}


void
OpenWithMenu::SelectionChanged()
{
	// TODO asynchronous rebuild
		
	if (Superitem() != NULL) {
		int32 count = fController->PoseView()->SelectionList()->CountItems();
		Superitem()->SetEnabled(count > 0);
	}
}


bool
OpenWithMenu::StartBuildingItemList()
{
	fIterator = new SearchForSignatureEntryList(false);
	// push all the supporting apps from all the entries into the
	// search for signature iterator
	EachEntryRef(&fEntriesToOpen, OpenWithUtils::AddOneRefSignatures,
		fIterator, 100);
	// add superhandlers
	OpenWithUtils::AddSupportingAppForTypeToQuery(fIterator,
		B_FILE_MIMETYPE);

	fHaveCommonPreferredApp = fIterator->GetPreferredApp(&fPreferredRef);
	status_t error = fIterator->Rewind();
	if (error != B_OK) {
		PRINT(("failed to initialize iterator %s\n", strerror(error)));
		return false;
	}

	fSupportingAppList = new BObjectList<RelationCachingModelProxy>(20, true);

	//queryRetrieval = new BStopWatch("get next entry on BQuery");
	return true;
}


bool
OpenWithMenu::AddNextItem()
{
	BEntry entry;
	if (fIterator->GetNextEntry(&entry) != B_OK)
		return false;

	Model *model = new Model(&entry, true);
	if (model->InitCheck() != B_OK
		|| !fIterator->CanOpenWithFilter(model, &fEntriesToOpen,
				fHaveCommonPreferredApp ? &fPreferredRef : 0)) {
		// only allow executables, filter out multiple copies of the
		// Tracker, filter out version that don't list the correct types,
		// etc.
		delete model;
	} else
		fSupportingAppList->AddItem(new RelationCachingModelProxy(model));

	return true;
}


void
OpenWithMenu::DoneBuildingItemList()
{
	// sort by app name
	fSupportingAppList->SortItems(SortByRelationAndName, this);

	// check if each app is unique
	bool unique = true;
	int32 count = fSupportingAppList->CountItems();
	for (int32 index = 0; index < count - 1; index++) {
		// the list is sorted, just compare two adjacent models
		if (strcmp(fSupportingAppList->ItemAt(index)->fModel->Name(),
			fSupportingAppList->ItemAt(index + 1)->fModel->Name()) == 0) {
			unique = false;
			break;
		}
	}

	// add apps as menu items
	BFont font;
	GetFont(&font);

	int32 lastRelation = -1;
	for (int32 index = 0; index < count ; index++) {
		RelationCachingModelProxy *modelProxy = fSupportingAppList->ItemAt(index);
		Model *model = modelProxy->fModel;
		BMessage *message = new BMessage(fEntriesToOpen);
		message->AddRef("handler", model->EntryRef());
		message->AddData("nodeRefsToClose", B_RAW_TYPE,
			fController->PoseView()->TargetModel()->NodeRef(),
			sizeof(node_ref));

		BString result;
		if (unique) {
			// just use the app name
			result = model->Name();
		} else {
			// get a truncated full path
			BPath path;
			BEntry entry(model->EntryRef());
			if (entry.GetPath(&path) != B_OK) {
				PRINT(("stale entry ref %s\n", model->Name()));
				delete message;
				continue;
			}
			result = path.Path();
			font.TruncateString(&result, B_TRUNCATE_MIDDLE, kMaxMenuWidth);
		}
#if DEBUG
		BString relationDescription;
		fIterator->RelationDescription(&fEntriesToOpen, model, &relationDescription);
		result += " (";
		result += relationDescription;
		result += ")";
#endif

		// divide different relations of opening with a separator
		int32 relation = modelProxy->Relation(fIterator, &fEntriesToOpen);
		if (lastRelation != -1 && relation != lastRelation)
			AddSeparatorItem();
		lastRelation = relation;

		ModelMenuItem *item = new ModelMenuItem(model, result.String(), message);
		AddItem(item);
		// mark item if it represents the preferred app
		if (fHaveCommonPreferredApp && *(model->EntryRef()) == fPreferredRef) {
			//PRINT(("marking item for % as preferred", model->Name()));
			item->SetMarked(true);
		}
	}

	// target the menu
	SetTargetForItems(be_app);

	if (!CountItems()) {
		BMenuItem* item = new BMenuItem(B_TRANSLATE("no supporting apps"), 0);
		item->SetEnabled(false);
		AddItem(item);
	}
}


void
OpenWithMenu::ClearMenuBuildingState()
{
	delete fIterator;
	fIterator = NULL;
	delete fSupportingAppList;
	fSupportingAppList = NULL;
}



