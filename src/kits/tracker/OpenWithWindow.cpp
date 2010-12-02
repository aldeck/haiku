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

#include "AutoLock.h"
#include "Commands.h"
#include "CountView.h"
#include "IconMenuItem.h"
#include "OpenWithWindow.h"
#include "PoseViewController.h"
#include "MimeTypes.h"
#include "StopWatch.h"
#include "Tracker.h"

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <Volume.h>
#include <VolumeRoster.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


const char *kDefaultOpenWithTemplate = "OpenWithSettings";

// ToDo:
// filter out trash
// allow column configuring
// make SaveState/RestoreState save the current window setting for
// other windows

const int32 kDocumentKnobWidth = 16;
const int32 kOpenAndMakeDefault = 'OpDf';
const rgb_color kOpenWithDefaultColor = { 0xFF, 0xFF, 0xCC, 255};


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "libtracker"

OpenWithContainerWindow::OpenWithContainerWindow(BMessage *entriesToOpen,
		LockingList<BWindow> *windowList, window_look look, window_feel feel,
		uint32 flags, uint32 workspace)
	:
	BContainerWindow(NULL, windowList, 0, look, feel, flags, workspace),
	fEntriesToOpen(entriesToOpen)
{	
}


OpenWithContainerWindow::~OpenWithContainerWindow()
{
	delete fEntriesToOpen;
}


OpenWithPoseView *
OpenWithContainerWindow::PoseView() const
{
	ASSERT(dynamic_cast<OpenWithPoseView *>(fPoseView));
	return static_cast<OpenWithPoseView *>(fPoseView);
}


const BMessage *
OpenWithContainerWindow::EntryList() const
{
	return fEntriesToOpen;
}


void
OpenWithContainerWindow::OpenWithSelection()
{
	int32 count = PoseView()->SelectionList()->CountItems();
	ASSERT(count == 1);
	if (!count)
		return;

	PoseView()->OpenSelection(PoseView()->SelectionList()->FirstItem(), 0);
}


static const entry_ref *
AddOneUniqueDocumentType(const entry_ref *ref, void *castToList)
{
	BObjectList<BString> *list = (BObjectList<BString> *)castToList;

	BEntry entry(ref, true);
		// traverse symlinks

	// get this documents type
	char type[B_MIME_TYPE_LENGTH];
	BFile file(&entry, O_RDONLY);
	if (file.InitCheck() != B_OK)
		return 0;

	BNodeInfo info(&file);
	if (info.GetType(type) != B_OK)
		return 0;

	if (list->EachElement(OpenWithUtils::FindOne, &type))
		// type already in list, bail
		return 0;

	// add type to list
	list->AddItem(new BString(type));
	return 0;
}


static const BString *
SetDefaultAppForOneType(const BString *element, void *castToEntryRef)
{
	const entry_ref *appRef = (const entry_ref *)castToEntryRef;

	// set entry as default handler for one mime string
	BMimeType mime(element->String());
	if (!mime.IsInstalled())
		return 0;

	// first set it's app signature as the preferred type
	BFile appFile(appRef, O_RDONLY);
	if (appFile.InitCheck() != B_OK)
		return 0;

	char appSignature[B_MIME_TYPE_LENGTH];
	if (GetAppSignatureFromAttr(&appFile, appSignature) != B_OK)
		return 0;

	if (mime.SetPreferredApp(appSignature) != B_OK)
		return 0;

	// set the app hint on the metamime for this signature
	mime.SetTo(appSignature);
#if xDEBUG
	status_t result =
#endif
	mime.SetAppHint(appRef);

#if xDEBUG
	BEntry debugEntry(appRef);
	BPath debugPath;
	debugEntry.GetPath(&debugPath);

	PRINT(("setting %s, sig %s as default app for %s, result %s\n",
		debugPath.Path(), appSignature, element->String(), strerror(result)));
#endif

	return 0;
}


void
OpenWithContainerWindow::MakeDefaultAndOpen()
{
	int32 count = PoseView()->SelectionList()->CountItems();
	ASSERT(count == 1);
	if (!count)
		return;

	BPose *selectedAppPose = PoseView()->SelectionList()->FirstItem();
	ASSERT(selectedAppPose);
	if (!selectedAppPose)
		return;

	// collect all the types of all the opened documents into a list
	BObjectList<BString> openedFileTypes(10, true);
	EachEntryRef(EntryList(), AddOneUniqueDocumentType, &openedFileTypes, 100);

	// set the default application to be the selected pose for all the
	// mime types in the list
	openedFileTypes.EachElement(SetDefaultAppForOneType,
		(void *)selectedAppPose->TargetModel()->EntryRef());

	// done setting the default application, now launch the app with the
	// documents
	OpenWithSelection();
}


void
OpenWithContainerWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kDefaultButton:
			OpenWithSelection();
			PostMessage(B_QUIT_REQUESTED);
			return;

		case kOpenAndMakeDefault:
			MakeDefaultAndOpen();
			PostMessage(B_QUIT_REQUESTED);
			return;

		case kCancelButton:
			PostMessage(B_QUIT_REQUESTED);
			return;

		case B_OBSERVER_NOTICE_CHANGE:
			return;
	}
	_inherited::MessageReceived(message);
}


filter_result
OpenWithContainerWindow::KeyDownFilter(BMessage *message, BHandler **,
	BMessageFilter *filter)
{
	uchar key;
	if (message->FindInt8("byte", (int8 *)&key) != B_OK)
		return B_DISPATCH_MESSAGE;

	int32 modifier=0;
	message->FindInt32("modifiers", &modifier);
	if (!modifier && key == B_ESCAPE) {
		filter->Looper()->PostMessage(kCancelButton);
		return B_SKIP_MESSAGE;
	}

	return B_DISPATCH_MESSAGE;
}


void
OpenWithContainerWindow::ShowContextMenu(BPoint, const entry_ref *, BView *)
{
}


void
OpenWithContainerWindow::AddShortcuts()
{
	// add get info here
}


void
OpenWithContainerWindow::NewAttributeMenu(BMenu *menu)
{
	// TODO _inherited::NewAttributeMenu(menu);
	BMessage *message = new BMessage(kAttributeItem);
	message->AddString("attr_name", kAttrOpenWithRelation);
	message->AddInt32("attr_type", B_STRING_TYPE);
	message->AddInt32("attr_hash", (int32)AttrHashString(kAttrOpenWithRelation, B_STRING_TYPE));
	message->AddFloat("attr_width", 180);
	message->AddInt32("attr_align", B_ALIGN_LEFT);
	message->AddBool("attr_editable", false);
	message->AddBool("attr_statfield", false);
	BMenuItem *item = new BMenuItem(B_TRANSLATE("Relation"), message);
	menu->AddItem(item);
	message = new BMessage(kAttributeItem);
	message->AddString("attr_name", kAttrAppVersion);
	message->AddInt32("attr_type", B_STRING_TYPE);
	message->AddInt32("attr_hash", (int32)AttrHashString(kAttrAppVersion, B_STRING_TYPE));
	message->AddFloat("attr_width", 70);
	message->AddInt32("attr_align", B_ALIGN_LEFT);
	message->AddBool("attr_editable", false);
	message->AddBool("attr_statfield", false);
	item = new BMenuItem(B_TRANSLATE("Version"), message);
	menu->AddItem(item);
}


void
OpenWithContainerWindow::SaveState(bool)
{
	BNode defaultingNode;
	if (DefaultStateSourceNode(kDefaultOpenWithTemplate, &defaultingNode,
			true, false)) {
		AttributeStreamFileNode streamNodeDestination(&defaultingNode);
		SaveWindowState(&streamNodeDestination);
		fPoseView->SaveState(&streamNodeDestination);
	}
}


void
OpenWithContainerWindow::SaveState(BMessage &message) const
{
	_inherited::SaveState(message);
}


void
OpenWithContainerWindow::_Init(const BMessage *message)
{
	printf("(%p) OpenWithContainerWindow::_Init \n", this);

	AutoLock<BWindow> lock(this);
	if (!lock)
		return;

	// create controls
	fPoseView = new OpenWithPoseView();
	fPoseView->SetFlags(fPoseView->Flags() | B_NAVIGABLE);
	fPoseView->SetPoseEditing(false);

	fController = new PoseViewController();
	
	Controller()->SetPoseView(fPoseView);
	Controller()->CreateControls(fCreationModel);

	fLaunchButton = new BButton("ok", B_TRANSLATE("Open"),
		new BMessage(kDefaultButton));

	fLaunchAndMakeDefaultButton = new BButton("make default",
		B_TRANSLATE("Open and make preferred"),
		new BMessage(kOpenAndMakeDefault));
	fLaunchAndMakeDefaultButton->SetEnabled(false);

	BButton *cancelButton = new BButton("cancel", B_TRANSLATE("Cancel"),
		new BMessage(kCancelButton));	
	fLaunchButton->MakeDefault(true);

	
	// layout controls
	const float spacing = be_control_look->DefaultItemSpacing();
	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_VERTICAL)
			.AddGroup(B_VERTICAL, 0.0f)
				.Add(Controller()->TitleView())
				.AddGroup(B_HORIZONTAL, 0.0f)
					.Add(Controller()->PoseView())
					.Add(Controller()->VerticalScrollBar())
				.End()
				.AddGroup(B_HORIZONTAL, 0.0f)
					.Add(Controller()->CountView())
					.Add(Controller()->HorizontalScrollBar(), 3.0f)
					.SetInsets(0, 0, B_V_SCROLL_BAR_WIDTH, 0)
						// make room for the window's resize handle
				.End()
			.End()
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(cancelButton)				
			.Add(fLaunchAndMakeDefaultButton)
			.Add(fLaunchButton)
			.SetInsets(0, 0, 16, 0)	// avoid the window's resize handle
		.End()
		.SetInsets(spacing, spacing, spacing, spacing);
	
	// deal with new unconfigured folders
	if (NeedsDefaultStateSetup())
		SetUpDefaultState();

	if (message)
		RestoreState(*message);
	else
		RestoreState();
		
	//Controller()->CreateMenus();
	//AddContextMenus();
	//AddCommonShortcuts();
	
	CheckScreenIntersect();
		// check window frame TODO: should be done in restorestate	
	
	Controller()->TitleView()->Reset();
		// TODO check for a more robust way for the titleview to get updates
	
	// set the window title
	if (CountRefs(fEntriesToOpen) == 1) {
		// if opening just one file, use it in the title
		entry_ref ref;
		fEntriesToOpen->FindRef("refs", &ref);
		BString buffer(B_TRANSLATE("Open %name with:"));
		buffer.ReplaceFirst("%name", ref.name);
		SetTitle(buffer.String());
	} else
		// use generic title
		SetTitle(B_TRANSLATE("Open selection with:"));
	
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN, &OpenWithContainerWindow::KeyDownFilter));

	Show();
}


void
OpenWithContainerWindow::RestoreState()
{
	BNode defaultingNode;
	if (DefaultStateSourceNode(kDefaultOpenWithTemplate, &defaultingNode, false)) {
		printf("OpenWithContainerWindow::RestoreState stream\n");
		AttributeStreamFileNode streamNodeSource(&defaultingNode);
		RestoreWindowState(&streamNodeSource);
		fPoseView->Init(&streamNodeSource);
	} else {
		printf("OpenWithContainerWindow::RestoreState none\n");
		RestoreWindowState(NULL);
		fPoseView->Init(NULL);
	}
}


void
OpenWithContainerWindow::RestoreState(const BMessage &message)
{
	printf("OpenWithContainerWindow::RestoreState msg\n");
	_inherited::RestoreState(message);
}


void
OpenWithContainerWindow::RestoreWindowState(AttributeStreamNode *node)
{
	if (!node)
		return;

	const char *rectAttributeName = kAttrWindowFrame;
	BRect frame(Frame());
	if (node->Read(rectAttributeName, 0, B_RECT_TYPE, sizeof(BRect), &frame)
		== sizeof(BRect)) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}
}


void
OpenWithContainerWindow::RestoreWindowState(const BMessage &message)
{
	_inherited::RestoreWindowState(message);
}


bool
OpenWithContainerWindow::NeedsDefaultStateSetup()
{
	return true;
}


void
OpenWithContainerWindow::SetUpDefaultState()
{
}


bool
OpenWithContainerWindow::IsShowing(const node_ref *) const
{
	return false;
}


bool
OpenWithContainerWindow::IsShowing(const entry_ref *) const
{
	return false;
}


void
OpenWithContainerWindow::SetCanSetAppAsDefault(bool on)
{
	fLaunchAndMakeDefaultButton->SetEnabled(on);
}


void
OpenWithContainerWindow::SetCanOpen(bool on)
{
	fLaunchButton->SetEnabled(on);
}


//	#pragma mark -


OpenWithPoseView::OpenWithPoseView()
	: 
	BPoseView(NULL, kListMode),
	fHaveCommonPreferredApp(false),
	fIterator(NULL)
{
	printf("(%p) OpenWithPoseView::OpenWithPoseView\n", this);
	fSavePoseLocations = false;
	fMultipleSelection = false;
	fDragEnabled = false;
}


OpenWithContainerWindow *
OpenWithPoseView::ContainerWindow() const
{
	ASSERT(dynamic_cast<OpenWithContainerWindow *>(Window()));
	return static_cast<OpenWithContainerWindow *>(Window());
}


void
OpenWithPoseView::AttachedToWindow()
{
	_inherited::AttachedToWindow();
	SetViewColor(kOpenWithDefaultColor);
	SetLowColor(kOpenWithDefaultColor);
}


bool
OpenWithPoseView::CanHandleDragSelection(const Model *, const BMessage *, bool)
{
	return false;
}


EntryListBase *
OpenWithPoseView::InitDirentIterator(const entry_ref *)
{
	OpenWithContainerWindow *window = ContainerWindow();

	const BMessage *entryList = window->EntryList();

	fIterator = new SearchForSignatureEntryList(true);

	// push all the supporting apps from all the entries into the
	// search for signature iterator
	EachEntryRef(entryList, OpenWithUtils::AddOneRefSignatures, fIterator,
		100);

	// push superhandlers
	OpenWithUtils::AddSupportingAppForTypeToQuery(fIterator,
		B_FILE_MIMETYPE);
	fHaveCommonPreferredApp = fIterator->GetPreferredApp(&fPreferredRef);

	if (fIterator->Rewind() != B_OK) {
		delete fIterator;
		fIterator = NULL;
		Controller()->SlowOperationEnded();
		return NULL;
	}
	return fIterator;
}


void
OpenWithPoseView::OpenSelection(BPose *pose, int32 *)
{
	OpenWithContainerWindow *window = ContainerWindow();

	int32 count = fSelectionList->CountItems();
	if (!count)
		return;

	if (!pose)
		pose = fSelectionList->FirstItem();

	ASSERT(pose);

	BEntry entry(pose->TargetModel()->EntryRef());
	if (entry.InitCheck() != B_OK) {
		BString errorString(
			B_TRANSLATE("Could not find application \"%appname\""));
		errorString.ReplaceFirst("%appname", pose->TargetModel()->Name());

		(new BAlert("", errorString.String(), B_TRANSLATE("OK"), 0, 0,
			B_WIDTH_AS_USUAL, B_WARNING_ALERT))->Go();
		return;
	}

	if (OpenWithRelation(pose->TargetModel()) == kNoRelation) {
		if (!fIterator->GenericFilesOnly()) {
			BString warning(B_TRANSLATE(
				"The application \"%appname\" does not support the type of "
				"document you are about to open. Are you sure you want to "
				"proceed? If you know that the application supports the "
				"document type, you should contact the publisher of the "
				"application and ask them to update their application to list "
				"the type of your document as supported."));
			warning.ReplaceFirst("%appname", pose->TargetModel()->Name());

			BAlert* alert = new BAlert("", warning.String(),
				B_TRANSLATE("Cancel"), B_TRANSLATE("Open"),	0, B_WIDTH_AS_USUAL,
				B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			if (alert->Go() == 0)
				return;
		}
		// else - once we have an extensible sniffer, tell users to ask
		// publishers to fix up sniffers
	}

	BMessage message(*window->EntryList());
		// make a clone to send
	message.RemoveName("launchUsingSelector");
		// make sure the old selector is not in the message
	message.AddRef("handler", pose->TargetModel()->EntryRef());
		// add ref of the selected handler

	ASSERT(fSelectionHandler);

	if (fSelectionHandler)
		fSelectionHandler->PostMessage(&message);

	window->PostMessage(B_QUIT_REQUESTED);
}


void
OpenWithPoseView::Pulse()
{
	// disable the Open and make default button if the default
	// app matches the selected app
	//
	// disable the Open button if no apps selected

	OpenWithContainerWindow *window = ContainerWindow();

	if (!fSelectionList->CountItems()) {
		window->SetCanSetAppAsDefault(false);
		window->SetCanOpen(false);
		_inherited::Pulse();
		return;
	}

	// if we selected a non-handling application, don't allow setting
	// it as preferred
	Model *firstSelected = fSelectionList->FirstItem()->TargetModel();
	if (OpenWithRelation(firstSelected) == kNoRelation) {
		window->SetCanSetAppAsDefault(false);
		window->SetCanOpen(true);
		_inherited::Pulse();
		return;
	}

	// make the open button enabled, because we have na app selected
	window->SetCanOpen(true);
	if (!fHaveCommonPreferredApp) {
		window->SetCanSetAppAsDefault(true);
		_inherited::Pulse();
		return;
	}

	ASSERT(fSelectionList->CountItems() == 1);

	// enable the Open and make default if selected application different
	// from preferred app ref
	window->SetCanSetAppAsDefault((*fSelectionList->FirstItem()->
		TargetModel()->EntryRef()) != fPreferredRef);

	_inherited::Pulse();
}


void
OpenWithPoseView::SetUpDefaultColumnsIfNeeded()
{
	// in case there were errors getting some columns
	if (fColumnList->CountItems() != 0)
		return;

	BColumn *nameColumn = new BColumn(B_TRANSLATE("Name"), kColumnStart, 125,
		B_ALIGN_LEFT, kAttrStatName, B_STRING_TYPE, true, true);
	fColumnList->AddItem(nameColumn);
	BColumn *relationColumn = new BColumn(B_TRANSLATE("Relation"), 180, 100,
		B_ALIGN_LEFT, kAttrOpenWithRelation, B_STRING_TYPE, false, false);
	fColumnList->AddItem(relationColumn);
	fColumnList->AddItem(new BColumn(B_TRANSLATE("Location"), 290, 225,
		B_ALIGN_LEFT, kAttrPath, B_STRING_TYPE, true, false));
	fColumnList->AddItem(new BColumn(B_TRANSLATE("Version"), 525, 70,
		B_ALIGN_LEFT, kAttrAppVersion, B_STRING_TYPE, false, false));

	// sort by relation and by name
	SetPrimarySort(relationColumn->AttrHash());
	SetSecondarySort(nameColumn->AttrHash());
}


bool
OpenWithPoseView::AddPosesThreadValid(const entry_ref *) const
{
	return true;
}


void
OpenWithPoseView::CreatePoses(Model **models, PoseInfo *poseInfoArray, int32 count,
	BPose **resultingPoses, bool insertionSort,	int32 *lastPoseIndexPtr,
	BRect *boundsPtr, bool forceDraw)
{
	printf("(%p) OpenWithPoseView::CreatePoses\n", this);
	
	// overridden to try to select the preferred handling app
	_inherited::CreatePoses(models, poseInfoArray, count, resultingPoses, insertionSort,
		lastPoseIndexPtr, boundsPtr, forceDraw);

	if (resultingPoses) {
		for (int32 index = 0; index < count; index++) {
			if (resultingPoses[index] && fHaveCommonPreferredApp
				&& *(models[index]->EntryRef()) == fPreferredRef) {
				// this is our preferred app, select it's pose
				SelectPose(resultingPoses[index], IndexOfPose(resultingPoses[index]));
			}
		}
	}
}


void
OpenWithPoseView::KeyDown(const char *bytes, int32 count)
{
	if (bytes[0] == B_TAB) {
		// just shift the focus, don't tab to the next pose
		BView::KeyDown(bytes, count);
	} else
		_inherited::KeyDown(bytes, count);
}


void
OpenWithPoseView::SaveState(AttributeStreamNode *node)
{
	_inherited::SaveState(node);
}


void
OpenWithPoseView::RestoreState(AttributeStreamNode *node)
{
	_inherited::RestoreState(node);
	fViewState->SetViewMode(kListMode);
}


void
OpenWithPoseView::SaveState(BMessage &message) const
{
	_inherited::SaveState(message);
}


void
OpenWithPoseView::RestoreState(const BMessage &message)
{
	_inherited::RestoreState(message);
	fViewState->SetViewMode(kListMode);
}


void
OpenWithPoseView::SavePoseLocations(BRect *)
{
	// do nothing
}


void
OpenWithPoseView::MoveSelectionToTrash(bool)
{
}


void
OpenWithPoseView::MoveSelectionTo(BPoint, BPoint, BContainerWindow *)
{
}


void
OpenWithPoseView::MoveSelectionInto(Model *, BContainerWindow *, bool, bool)
{
}


bool
OpenWithPoseView::Represents(const node_ref *) const
{
	return false;
}


bool
OpenWithPoseView::Represents(const entry_ref *) const
{
	return false;
}


bool
OpenWithPoseView::HandleMessageDropped(BMessage *DEBUG_ONLY(message))
{
#if DEBUG
	// in debug mode allow tweaking the colors
	const rgb_color *color;
	int32 size;
	// handle roColour-style color drops
	if (message->FindData("RGBColor", 'RGBC', (const void **)&color, &size) == B_OK) {
		SetViewColor(*color);
		SetLowColor(*color);
		Invalidate();
		return true;
	}
#endif
	return false;
}


int32
OpenWithPoseView::OpenWithRelation(const Model *model) const
{
	OpenWithContainerWindow *window = ContainerWindow();

	return SearchForSignatureEntryList::Relation(window->EntryList(),
		model, fHaveCommonPreferredApp ? &fPreferredRef : 0, 0);
}


void
OpenWithPoseView::OpenWithRelationDescription(const Model *model,
	BString *description) const
{
	OpenWithContainerWindow *window = ContainerWindow();

	SearchForSignatureEntryList::RelationDescription(window->EntryList(),
		model, description, fHaveCommonPreferredApp ? &fPreferredRef : 0, 0);
}


bool
OpenWithPoseView::ShouldShowPose(const Model *model, const PoseInfo *poseInfo)
{
	OpenWithContainerWindow *window = ContainerWindow();
	// filter for add_poses
	if (!fIterator->CanOpenWithFilter(model, window->EntryList(),
		fHaveCommonPreferredApp ? &fPreferredRef : 0))
		return false;

	return _inherited::ShouldShowPose(model, poseInfo);
}
