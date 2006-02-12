/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypes.h"
#include "FileTypesWindow.h"
#include "MimeTypeListView.h"

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Mime.h>
#include <OutlineListView.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <TextControl.h>

#include <be_apps/Tracker/RecentItems.h>

#include <stdio.h>


const uint32 kMsgTypeSelected = 'typs';
const uint32 kMsgExtensionSelected = 'exts';
const uint32 kMsgAttributeSelected = 'atrs';
const uint32 kMsgPreferredAppChosen = 'papc';


const struct type_map {
	const char*	name;
	type_code	type;
} kTypeMap[] = {
	{"String",			B_STRING_TYPE},
	{"Boolean",			B_BOOL_TYPE},
	{"Integer 8 bit",	B_INT8_TYPE},
	{"Integer 16 bit",	B_INT16_TYPE},
	{"Integer 32 bit",	B_INT32_TYPE},
	{"Integer 64 bit",	B_INT64_TYPE},
	{"Float",			B_FLOAT_TYPE},
	{"Double",			B_DOUBLE_TYPE},
	{"Time",			B_TIME_TYPE},
	{NULL,				0}
};

class IconView : public BControl {
	public:
		IconView(BRect frame, const char* name, BMessage* message);
		virtual ~IconView();

		void SetTo(BMimeType* type);

		virtual void Draw(BRect updateRect);

#if 0
		virtual void MouseDown(BPoint where);
		virtual void MouseMoved(BPoint where, uint32 transit,
			BMessage* dragMessage);
#endif

	private:
		BBitmap*	fIcon;
};

class AttributeListView : public BListView {
	public:
		AttributeListView(BRect frame, const char* name, uint32 resizingMode);
		virtual ~AttributeListView();

		void SetTo(BMimeType* type);

		virtual void Draw(BRect updateRect);

	private:
		void _DeleteItems();
};

class AttributeItem : public BStringItem {
	public:
		AttributeItem(const char* name, const char* publicName, type_code type,
			int32 alignment, int32 width, bool visible, bool editable);
		~AttributeItem();

		virtual void DrawItem(BView* owner, BRect itemRect,
			bool drawEverything = false);

	private:
		BString		fName;
		BString		fPublicName;
		type_code	fType;
		int32		fAlignment;
		int32		fWidth;
		bool		fVisible;
		bool		fEditable;
};


//	#pragma mark -


static int
compare_menu_items(const void* _a, const void* _b)
{
	BMenuItem* a = *(BMenuItem**)_a;
	BMenuItem* b = *(BMenuItem**)_b;

	return strcasecmp(a->Label(), b->Label());
}


static void
name_for_type(BString& string, type_code type)
{
	for (int32 i = 0; kTypeMap[i].name != NULL; i++) {
		if (kTypeMap[i].type == type) {
			string = kTypeMap[i].name;
			return;
		}
	}

	char buffer[32];
	buffer[0] = '\'';
	buffer[1] = 0xff & (type >> 24);
	buffer[2] = 0xff & (type >> 16);
	buffer[3] = 0xff & (type >> 8);
	buffer[4] = 0xff & (type);
	buffer[5] = '\'';
	buffer[6] = 0;
	for (int16 i = 0;i < 4;i++) {
		if (buffer[i] < ' ')
			buffer[i] = '.';
	}

	snprintf(buffer + 6, sizeof(buffer), " (0x%lx)", type);
	string = buffer;
}


//	#pragma mark -


IconView::IconView(BRect frame, const char* name, BMessage* message)
	: BControl(frame, name, NULL, message,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	fIcon(NULL)
{
}


IconView::~IconView()
{
	delete fIcon;
}


void
IconView::SetTo(BMimeType* type)
{
	bool hadIcon = fIcon != NULL;

	if (type != NULL) {
		if (fIcon == NULL)
			fIcon = new BBitmap(BRect(0, 0, B_LARGE_ICON - 1, B_LARGE_ICON - 1), B_CMAP8);

		if (type->GetIcon(fIcon, B_LARGE_ICON) == B_OK) {
			Invalidate();
			return;
		}
	}

	delete fIcon;
	fIcon = NULL;

	if (hadIcon)
		Invalidate();
}


void
IconView::Draw(BRect updateRect)
{
	if (fIcon != NULL) {
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fIcon, BPoint(0, 0));
	} else if (IsEnabled()) {
		BRect rect = Bounds();

		rgb_color light = tint_color(ViewColor(), B_LIGHTEN_MAX_TINT);
		rgb_color shadow = tint_color(ViewColor(), B_DARKEN_3_TINT);

		BeginLineArray(8);

		AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), shadow);
		AddLine(BPoint(rect.left + 1.0f, rect.top),
				BPoint(rect.right, rect.top), shadow);
		AddLine(BPoint(rect.left + 1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), light);
		AddLine(BPoint(rect.right, rect.bottom - 1.0f),
				BPoint(rect.right, rect.top + 1.0f), light);

		rect.InsetBy(1.0, 1.0);

		AddLine(BPoint(rect.left, rect.bottom),
				BPoint(rect.left, rect.top), light);
		AddLine(BPoint(rect.left + 1.0f, rect.top),
				BPoint(rect.right, rect.top), light);
		AddLine(BPoint(rect.left + 1.0f, rect.bottom),
				BPoint(rect.right, rect.bottom), shadow);
		AddLine(BPoint(rect.right, rect.bottom - 1.0f),
				BPoint(rect.right, rect.top + 1.0f), shadow);

		EndLineArray();
	} else {
		SetHighColor(ViewColor());
		FillRect(updateRect);
	}
}


//	#pragma mark -


AttributeItem::AttributeItem(const char* name, const char* publicName,
		type_code type, int32 alignment, int32 width, bool visible,
		bool editable)
	: BStringItem(publicName),
	fName(name),
	fType(type),
	fAlignment(alignment),
	fWidth(width),
	fVisible(visible),
	fEditable(editable)
{
}


AttributeItem::~AttributeItem()
{
}


void
AttributeItem::DrawItem(BView* owner, BRect frame, bool drawEverything)
{
	BStringItem::DrawItem(owner, frame, drawEverything);

	rgb_color highColor = owner->HighColor();
	rgb_color lowColor = owner->LowColor();

	if (IsSelected())
		owner->SetLowColor(tint_color(lowColor, B_DARKEN_2_TINT));

	rgb_color black = {0, 0, 0, 255};

	if (!IsEnabled())
		owner->SetHighColor(tint_color(black, B_LIGHTEN_2_TINT));
	else
		owner->SetHighColor(black);

	owner->MovePenTo(frame.left + frame.Width() / 2.0f + 5.0f, owner->PenLocation().y);

	BString type;
	name_for_type(type, fType);
	owner->DrawString(type.String());

	owner->SetHighColor(tint_color(owner->ViewColor(), B_DARKEN_1_TINT));

	float middle = frame.left + frame.Width() / 2.0f;
	owner->StrokeLine(BPoint(middle, 0.0f), BPoint(middle, frame.bottom));

	owner->SetHighColor(highColor);
	owner->SetLowColor(lowColor);
}


//	#pragma mark -


AttributeListView::AttributeListView(BRect frame, const char* name,
		uint32 resizingMode)
	: BListView(frame, name, B_SINGLE_SELECTION_LIST, resizingMode)
{
}


AttributeListView::~AttributeListView()
{
	_DeleteItems();
}


void
AttributeListView::_DeleteItems()
{
	for (int32 i = CountItems(); i-- > 0;) {
		delete ItemAt(i);
	}
	MakeEmpty();
}


void
AttributeListView::SetTo(BMimeType* type)
{
	_DeleteItems();

	// fill it again
	
	if (type == NULL)
		return;

	BMessage attributes;
	if (type->GetAttrInfo(&attributes) != B_OK)
		return;

	const char* publicName;
	int32 i = 0;
	while (attributes.FindString("attr:public_name", i, &publicName) == B_OK) {
		const char* name;
		if (attributes.FindString("attr:name", i, &name) != B_OK)
			name = "-";

		type_code type;
		if (attributes.FindInt32("attr:type", i, (int32 *)&type) != B_OK)
			type = B_STRING_TYPE;

		bool editable;
		if (attributes.FindBool("attr:editable", i, &editable) != B_OK)
			editable = false;
		bool visible;
		if (attributes.FindBool("attr:viewable", i, &visible) != B_OK)
			visible = false;
		bool extra;
		if (attributes.FindBool("attr:extra", i, &extra) != B_OK)
			extra = false;

		int32 alignment;
		if (attributes.FindInt32("attr:alignment", i, &alignment) != B_OK)
			alignment = B_ALIGN_LEFT;

		int32 width;
		if (attributes.FindInt32("attr:width", i, &width) != B_OK)
			width = 50;

		AddItem(new AttributeItem(name, publicName, type, alignment, width,
			visible, editable));

		i++;
	}
}


void
AttributeListView::Draw(BRect updateRect)
{
	BListView::Draw(updateRect);

	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));

	float middle = Bounds().Width() / 2.0f;
	StrokeLine(BPoint(middle, 0.0f), BPoint(middle, Bounds().bottom));
}


//	#pragma mark -


FileTypesWindow::FileTypesWindow(BRect frame)
	: BWindow(frame, "FileTypes", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// add the menu

	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New Resource File" B_UTF8_ELLIPSIS,
		NULL, 'N', B_COMMAND_KEY));

	BMenu* recentsMenu = BRecentFilesList::NewFileListMenu("Open" B_UTF8_ELLIPSIS,
		NULL, NULL, be_app, 10, false, NULL, kSignature);
	BMenuItem* item = new BMenuItem(recentsMenu, new BMessage(kMsgOpenFilePanel));
	item->SetShortcut('O', B_COMMAND_KEY);
	menu->AddItem(item);
	menu->AddItem(new BMenuItem("Application Types" B_UTF8_ELLIPSIS,
		new BMessage));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("About FileTypes" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q', B_COMMAND_KEY));
	menu->SetTargetForItems(be_app);
	item->SetTarget(this);
	menuBar->AddItem(menu);

	// MIME Types list

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1.0f;
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);

	BButton* button = new BButton(rect, "add", "Add" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->MoveTo(8.0f, topView->Bounds().bottom - 8.0f - button->Bounds().Height());
	topView->AddChild(button);

	rect = button->Frame();
	rect.OffsetBy(rect.Width() + 8.0f, 0.0f);
	fRemoveTypeButton = new BButton(rect, "remove", "Remove", NULL, B_FOLLOW_BOTTOM);
	fRemoveTypeButton->ResizeToPreferred();
	topView->AddChild(fRemoveTypeButton);

	rect.bottom = rect.top - 10.0f;
	rect.top = 10.0f;
	rect.left = 10.0f;
	rect.right -= B_V_SCROLL_BAR_WIDTH + 2.0f;
	if (rect.right < 180)
		rect.right = 180;

	fTypeListView = new MimeTypeListView(rect, "listview",
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	fTypeListView->SetSelectionMessage(new BMessage(kMsgTypeSelected));

	BScrollView* scrollView = new BScrollView("scrollview", fTypeListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	topView->AddChild(scrollView);

	// "Icon" group

	BFont font(be_bold_font);
	float labelWidth = font.StringWidth("Icon");
	font_height fontHeight;
	font.GetHeight(&fontHeight);

	rect.left = rect.right + 12.0f + B_V_SCROLL_BAR_WIDTH;
	rect.right = rect.left + max_c(B_LARGE_ICON, labelWidth) + 32.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent)
		+ max_c(B_LARGE_ICON, button->Bounds().Height() * 2.0f + 4.0f) + 12.0f;
	rect.top -= 2.0f;
	BBox* box = new BBox(rect);
	box->SetLabel("Icon");
	topView->AddChild(box);

	BRect innerRect = BRect(0.0f, 0.0f, B_LARGE_ICON - 1.0f, B_LARGE_ICON - 1.0f);
	innerRect.OffsetTo((rect.Width() - innerRect.Width()) / 2.0f,
		fontHeight.ascent / 2.0f
		+ (rect.Height() - fontHeight.ascent / 2.0f - innerRect.Height()) / 2.0f);
	fIconView = new IconView(innerRect, "icon box", NULL);
	box->AddChild(fIconView);

	// "File Extensions" group

	BRect rightRect(rect);
	rightRect.left = rect.right + 8.0f;
	rightRect.right = topView->Bounds().Width() - 8.0f;
	box = new BBox(rightRect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("File Extensions");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Remove") - 16.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fAddExtensionButton = new BButton(innerRect, "add ext", "Add" B_UTF8_ELLIPSIS,
		NULL, B_FOLLOW_RIGHT);
	box->AddChild(fAddExtensionButton);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	fRemoveExtensionButton = new BButton(innerRect, "remove ext", "Remove", NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(fRemoveExtensionButton);

	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = 8.0f + ceilf(fontHeight.ascent);
	innerRect.bottom -= 2.0f;
		// take scrollview border into account
	fExtensionListView = new BListView(innerRect, "listview ext",
		B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT_RIGHT);
	fExtensionListView->SetSelectionMessage(new BMessage(kMsgExtensionSelected));
	scrollView = new BScrollView("scrollview ext", fExtensionListView,
		B_FOLLOW_LEFT_RIGHT, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	box->AddChild(scrollView);

	// "Description" group

	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent) + 19.0f;
	rect.right = rightRect.right;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Description");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fInternalNameControl = new BTextControl(innerRect, "internal", "Internal Name:", "",
		NULL, B_FOLLOW_LEFT_RIGHT);
	labelWidth = fInternalNameControl->StringWidth(fInternalNameControl->Label()) + 2.0f;
	fInternalNameControl->SetDivider(labelWidth);
	fInternalNameControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fInternalNameControl->SetEnabled(false);
	box->ResizeBy(0, fInternalNameControl->Bounds().Height() * 2.0f);
	box->AddChild(fInternalNameControl);

	innerRect.OffsetBy(0, fInternalNameControl->Bounds().Height() + 5.0f);
	fTypeNameControl = new BTextControl(innerRect, "type", "Type Name:", "",
		NULL, B_FOLLOW_LEFT_RIGHT);
	fTypeNameControl->SetDivider(labelWidth);
	fTypeNameControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	box->AddChild(fTypeNameControl);

	// "Preferred Application" group

	rect = box->Frame();
	rect.top = rect.bottom + 8.0f;
	rect.bottom = rect.top + ceilf(fontHeight.ascent)
		+ button->Bounds().Height() + 14.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT);
	box->SetLabel("Preferred Application");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Same As" B_UTF8_ELLIPSIS) - 24.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fSameAsButton = new BButton(innerRect, "same as", "Same As" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(fSameAsButton);

	innerRect.OffsetBy(-innerRect.Width() - 6.0f, 0.0f);
	fSelectButton = new BButton(innerRect, "select", "Select" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(fSelectButton);

	menu = new BPopUpMenu("preferred");
	menu->AddItem(item = new BMenuItem("None", NULL));
	item->SetMarked(true);

	innerRect.right = innerRect.left - 6.0f;
	innerRect.left = 8.0f;
	BView* constrainingView = new BView(innerRect, NULL, B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
	constrainingView->SetViewColor(topView->ViewColor());

	fPreferredField = new BMenuField(innerRect.OffsetToCopy(B_ORIGIN), "preferred",
		NULL, menu);
	float width, height;
	fPreferredField->GetPreferredSize(&width, &height);
	fPreferredField->ResizeTo(innerRect.Width(), height);
	fPreferredField->MoveBy(0.0f, (innerRect.Height() - height) / 2.0f);
	constrainingView->AddChild(fPreferredField);
		// we embed the menu field in another view to make it behave like
		// we want so that it can't obscure other elements with larger
		// labels

	box->AddChild(constrainingView);

	// "Extra Attributes" group

	rect.top = rect.bottom + 8.0f;
	rect.bottom = topView->Bounds().Height() - 8.0f;
	box = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM);
	box->SetLabel("Extra Attributes");
	topView->AddChild(box);

	innerRect = box->Bounds().InsetByCopy(8.0f, 6.0f);
	innerRect.top += ceilf(fontHeight.ascent);
	innerRect.left = innerRect.right - button->StringWidth("Remove") - 16.0f;
	innerRect.bottom = innerRect.top + button->Bounds().Height();
	fAddAttributeButton = new BButton(innerRect, "add attr", "Add" B_UTF8_ELLIPSIS, NULL,
		B_FOLLOW_RIGHT);
	box->AddChild(fAddAttributeButton);

	innerRect.OffsetBy(0, innerRect.Height() + 4.0f);
	fRemoveAttributeButton = new BButton(innerRect, "remove attr", "Remove",
		NULL, B_FOLLOW_RIGHT);
	box->AddChild(fRemoveAttributeButton);

	innerRect.right = innerRect.left - 10.0f - B_V_SCROLL_BAR_WIDTH;
	innerRect.left = 10.0f;
	innerRect.top = 8.0f + ceilf(fontHeight.ascent);
	innerRect.bottom = box->Bounds().bottom - 10.0f;
		// take scrollview border into account
	fAttributeListView = new AttributeListView(innerRect, "listview attr",
		B_FOLLOW_ALL);
	fAttributeListView->SetSelectionMessage(new BMessage(kMsgAttributeSelected));
	scrollView = new BScrollView("scrollview attr", fAttributeListView,
		B_FOLLOW_ALL, B_FRAME_EVENTS | B_WILL_DRAW, false, true);
	box->AddChild(scrollView);

	SetSizeLimits(rightRect.left + 72.0f + font.StringWidth("File Extensions"), 32767.0f,
		rect.top + 2.0f * button->Bounds().Height() + fontHeight.ascent + 32.0f
		+ menuBar->Bounds().Height(), 32767.0f);

	_SetType(NULL);
}


FileTypesWindow::~FileTypesWindow()
{
}


void
FileTypesWindow::_UpdateExtensions(BMimeType* type)
{
	// clear list

	for (int32 i = fExtensionListView->CountItems(); i-- > 0;) {
		delete fExtensionListView->ItemAt(i);
	}
	fExtensionListView->MakeEmpty();

	// fill it again
	
	if (type == NULL)
		return;

	BMessage extensions;
	if (type->GetFileExtensions(&extensions) != B_OK)
		return;

	const char* extension;
	int32 i = 0;
	while (extensions.FindString("extensions", i++, &extension) == B_OK) {
		char dotExtension[B_FILE_NAME_LENGTH];
		snprintf(dotExtension, B_FILE_NAME_LENGTH, ".%s", extension);

		fExtensionListView->AddItem(new BStringItem(dotExtension));
	}
}


void
FileTypesWindow::_AddSignature(BMenuItem* item, const char* signature)
{
	const char* subType = strchr(signature, '/');
	if (subType == NULL)
		return;

	char label[B_MIME_TYPE_LENGTH];
	snprintf(label, sizeof(label), "%s (%s)", item->Label(), subType + 1);

	item->SetLabel(label);
}


void
FileTypesWindow::_UpdatePreferredApps(BMimeType* type)
{
	// clear menu

	BMenu* menu = fPreferredField->Menu();
	for (int32 i = menu->CountItems(); i-- > 1;) {
		delete menu->RemoveItem(i);
	}

	// fill it again

	menu->ItemAt(0)->SetMarked(true);

	BMessage applications;
	if (type == NULL || type->GetSupportingApps(&applications) != B_OK)
		return;

	char preferred[B_MIME_TYPE_LENGTH];
	if (type->GetPreferredApp(preferred) != B_OK)
		preferred[0] = '\0';

	int32 lastFullSupport;
	if (applications.FindInt32("be:sub", &lastFullSupport) != B_OK)
		lastFullSupport = -1;

	BList subList;
	BList superList;

	const char* signature;
	int32 i = 0;
	while (applications.FindString("applications", i, &signature) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		BMenuItem *item;

		BMessage* message = new BMessage(kMsgPreferredAppChosen);
		message->AddString("signature", signature);

		BMimeType applicationType(signature);
		if (applicationType.GetShortDescription(name) == B_OK)
			item = new BMenuItem(name, message);
		else
			item = new BMenuItem(signature, message);

		if (i < lastFullSupport)
			subList.AddItem(item);
		else
			superList.AddItem(item);

		i++;
	}

	// sort lists
	
	subList.SortItems(compare_menu_items);
	superList.SortItems(compare_menu_items);

	// add lists to the menu

	if (subList.CountItems() != 0 || superList.CountItems() != 0)
		menu->AddSeparatorItem();

	for (int32 i = 0; i < subList.CountItems(); i++) {
		menu->AddItem((BMenuItem*)subList.ItemAt(i));
	}

	// Add type separator
	if (superList.CountItems() != 0 && subList.CountItems() != 0)
		menu->AddSeparatorItem();

	for (int32 i = 0; i < superList.CountItems(); i++) {
		menu->AddItem((BMenuItem*)superList.ItemAt(i));
	}

	// make items unique and select current choice

	bool lastItemSame = false;
	const char* lastSignature = NULL;
	BMenuItem* last = NULL;

	for (int32 index = 0; index < menu->CountItems(); index++) {
		BMenuItem* item = menu->ItemAt(index);
		if (item == NULL)
			continue;

		if (item->Message() == NULL
			|| item->Message()->FindString("signature", &signature) != B_OK)
			continue;

		if (!strcasecmp(signature, preferred))
			item->SetMarked(true);

		if (last == NULL || strcmp(last->Label(), item->Label())) {
			if (lastItemSame)
				_AddSignature(last, lastSignature);

			lastItemSame = false;
			last = item;
			lastSignature = signature;
			continue;
		}

		lastItemSame = true;
		_AddSignature(last, lastSignature);

		last = item;
		lastSignature = signature;
	}

	if (lastItemSame)
		_AddSignature(last, lastSignature);
}


void
FileTypesWindow::_SetType(BMimeType* type)
{
	bool enabled = type != NULL;

	if (type != NULL) {
		fInternalNameControl->SetText(type->Type());
		
		char description[B_MIME_TYPE_LENGTH];
		if (type->GetShortDescription(description) != B_OK)
			description[0] = '\0';
		fTypeNameControl->SetText(description);
	} else {
		fInternalNameControl->SetText(NULL);
		fTypeNameControl->SetText(NULL);
	}

	_UpdateExtensions(type);
	_UpdatePreferredApps(type);
	fIconView->SetTo(type);
	fAttributeListView->SetTo(type);

	fIconView->SetEnabled(enabled);

	fTypeNameControl->SetEnabled(enabled);
	fPreferredField->SetEnabled(enabled);

	fRemoveTypeButton->SetEnabled(enabled);

	fSelectButton->SetEnabled(enabled);
	fSameAsButton->SetEnabled(enabled);

	fAddExtensionButton->SetEnabled(enabled);
	fRemoveExtensionButton->SetEnabled(false);

	fAddAttributeButton->SetEnabled(enabled);
	fRemoveAttributeButton->SetEnabled(false);
}


void
FileTypesWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgTypeSelected:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				MimeTypeItem* item = (MimeTypeItem*)fTypeListView->ItemAt(index);
				if (item != NULL) {
					BMimeType type(item->Type());
					_SetType(&type);
				} else
					_SetType(NULL);
			}
			break;
		}

		case kMsgExtensionSelected:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				BStringItem* item = (BStringItem*)fExtensionListView->ItemAt(index);
				fRemoveExtensionButton->SetEnabled(item != NULL);
			}
			break;
		}

		case kMsgAttributeSelected:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				AttributeItem* item = (AttributeItem*)fAttributeListView->ItemAt(index);
				fRemoveAttributeButton->SetEnabled(item != NULL);
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


bool
FileTypesWindow::QuitRequested()
{
	be_app->PostMessage(kMsgTypesWindowClosed);
	return true;
}


