/*
 * Copyright 2010, Haiku, Inc. All Rights Reserved.
 * Copyright 2008-2009, Pier Luigi Fiorini. All Rights Reserved.
 * Copyright 2004-2008, Michael Davidson. All Rights Reserved.
 * Copyright 2004-2007, Mikael Eiman. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Davidson, slaad@bong.com.au
 *		Mikael Eiman, mikael@eiman.tv
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <algorithm>

#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupView.h>

#include "AppGroupView.h"

#include "NotificationWindow.h"
#include "NotificationView.h"


static const int kHeaderSize = 20;


AppGroupView::AppGroupView(NotificationWindow* win, const char* label)
	:
	BGroupView("appGroup", B_VERTICAL, 0),
	fLabel(label),
	fParent(win),
	fCollapsed(false)
{
	SetFlags(Flags() | B_WILL_DRAW);

	static_cast<BGroupLayout*>(GetLayout())->SetInsets(0, kHeaderSize, 0, 0);
}


void
AppGroupView::Draw(BRect updateRect)
{
	rgb_color menuColor = ViewColor();
	BRect bounds = Bounds();
	rgb_color hilite = tint_color(menuColor, B_DARKEN_1_TINT);
	rgb_color dark = tint_color(menuColor, B_DARKEN_2_TINT);
	rgb_color vlight = tint_color(menuColor, B_LIGHTEN_2_TINT);
	bounds.bottom = bounds.top + kHeaderSize;

	// Draw the header background
	if (be_control_look != NULL) {
		SetHighColor(tint_color(menuColor, 1.22));
		StrokeLine(bounds.LeftTop(), bounds.LeftBottom());
		uint32 borders = BControlLook::B_TOP_BORDER
			| BControlLook::B_BOTTOM_BORDER | BControlLook::B_RIGHT_BORDER;

		be_control_look->DrawButtonBackground(this, bounds, bounds, menuColor,
			0, borders);
	} else {
		SetHighColor(vlight);
		StrokeLine(bounds.LeftTop(), bounds.RightTop());
		StrokeLine(BPoint(bounds.left, bounds.top + 1), bounds.LeftBottom());
		SetHighColor(hilite);
		StrokeLine(BPoint(bounds.left + 1, bounds.bottom),
			bounds.RightBottom());
	}

	// Draw the buttons
	fCollapseRect.top = (kHeaderSize - kExpandSize) / 2;
	fCollapseRect.left = kEdgePadding * 2;
	fCollapseRect.right = fCollapseRect.left + 1.5 * kExpandSize;
	fCollapseRect.bottom = fCollapseRect.top + kExpandSize;

	fCloseRect = bounds;
	fCloseRect.top = (kHeaderSize - kExpandSize) / 2;
	fCloseRect.right -= kEdgePadding * 2;
	fCloseRect.left = fCloseRect.right - kCloseSize;
	fCloseRect.bottom = fCloseRect.top + kCloseSize;

	if (be_control_look != NULL) {
		uint32 arrowDirection = fCollapsed
			? BControlLook::B_DOWN_ARROW : BControlLook::B_UP_ARROW;
		be_control_look->DrawArrowShape(this, fCollapseRect, fCollapseRect,
			LowColor(), arrowDirection, 0, B_DARKEN_3_TINT);
	} else {
		rgb_color outlineColor = {80, 80, 80, 255};
		rgb_color middleColor = {200, 200, 200, 255};

		SetDrawingMode(B_OP_OVER);

		if (fCollapsed) {
			BeginLineArray(6);

			AddLine(BPoint(fCollapseRect.left + 3, fCollapseRect.top + 1),
					BPoint(fCollapseRect.left + 3, fCollapseRect.bottom - 1), outlineColor);
			AddLine(BPoint(fCollapseRect.left + 3, fCollapseRect.top + 1),
					BPoint(fCollapseRect.left + 7, fCollapseRect.top + 5), outlineColor);
			AddLine(BPoint(fCollapseRect.left + 7, fCollapseRect.top + 5),
					BPoint(fCollapseRect.left + 3, fCollapseRect.bottom - 1), outlineColor);

			AddLine(BPoint(fCollapseRect.left + 4, fCollapseRect.top + 3),
					BPoint(fCollapseRect.left + 4, fCollapseRect.bottom - 3), middleColor);
			AddLine(BPoint(fCollapseRect.left + 5, fCollapseRect.top + 4),
					BPoint(fCollapseRect.left + 5, fCollapseRect.bottom - 4), middleColor);
			AddLine(BPoint(fCollapseRect.left + 5, fCollapseRect.top + 5),
					BPoint(fCollapseRect.left + 6, fCollapseRect.top + 5), middleColor);
			EndLineArray();
		} else {
			// expanded state

			BeginLineArray(6);
			AddLine(BPoint(fCollapseRect.left + 1, fCollapseRect.top + 3),
					BPoint(fCollapseRect.right - 3, fCollapseRect.top + 3), outlineColor);
			AddLine(BPoint(fCollapseRect.left + 1, fCollapseRect.top + 3),
					BPoint(fCollapseRect.left + 5, fCollapseRect.top + 7), outlineColor);
			AddLine(BPoint(fCollapseRect.left + 5, fCollapseRect.top + 7),
					BPoint(fCollapseRect.right - 3, fCollapseRect.top + 3), outlineColor);

			AddLine(BPoint(fCollapseRect.left + 3, fCollapseRect.top + 4),
					BPoint(fCollapseRect.right - 5, fCollapseRect.top + 4), middleColor);
			AddLine(BPoint(fCollapseRect.left + 4, fCollapseRect.top + 5),
					BPoint(fCollapseRect.right - 6, fCollapseRect.top + 5), middleColor);
			AddLine(BPoint(fCollapseRect.left + 5, fCollapseRect.top + 5),
					BPoint(fCollapseRect.left + 5, fCollapseRect.top + 6), middleColor);
			EndLineArray();
		}
	}

	SetPenSize(kPenSize);

	// Draw the dismiss widget
	BRect closeCross = fCloseRect;
	closeCross.InsetBy(kSmallPadding, kSmallPadding);
	rgb_color detailCol = ui_color(B_CONTROL_BORDER_COLOR);
	detailCol = tint_color(detailCol, B_LIGHTEN_2_TINT);

	StrokeRoundRect(fCloseRect, kSmallPadding, kSmallPadding);
	StrokeLine(closeCross.LeftTop(), closeCross.RightBottom());
	StrokeLine(closeCross.RightTop(), closeCross.LeftBottom());

	// Draw the label
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
	BString label = fLabel;
	if (fCollapsed)
		label << " (" << fInfo.size() << ")";

	SetFont(be_bold_font);

	DrawString(label.String(), BPoint(fCollapseRect.right + 2 * kEdgePadding,
				fCloseRect.bottom));
}


	void
AppGroupView::MouseDown(BPoint point)
{
	bool changed = false;
	if (fCloseRect.Contains(point)) {
		changed = true;

		int32 children = fInfo.size();
		for (int32 i = 0; i < children; i++) {
			GetLayout()->RemoveView(fInfo[i]);
			delete fInfo[i];
		}

		fInfo.clear();

		// Remove ourselves from the parent view
		BMessage message(kRemoveView);
		message.AddPointer("view", this);
		fParent->PostMessage(&message);
	}

	if (fCollapseRect.Contains(point)) {
		fCollapsed = !fCollapsed;
		int32 children = fInfo.size();
		if (fCollapsed) {
			for (int32 i = 0; i < children; i++) {
				if (!fInfo[i]->IsHidden())
					fInfo[i]->Hide();
			}
		} else {
			for (int32 i = 0; i < children; i++) {
				if (fInfo[i]->IsHidden())
					fInfo[i]->Show();
			}
		}
		changed = true;
		Invalidate(); // Need to redraw the collapse indicator and title

		BMessage message(kRemoveView);
			// Do not actually remive anything, but update size
		fParent->PostMessage(&message);
	}

	if (changed) {
		_ResizeViews();
	}
}


void
AppGroupView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kRemoveView:
		{
			NotificationView* view = NULL;
			if (msg->FindPointer("view", (void**)&view) != B_OK)
				return;

			infoview_t::iterator vIt = find(fInfo.begin(), fInfo.end(), view);

			if (vIt != fInfo.end()) {
				fInfo.erase(vIt);
				GetLayout()->RemoveView(view);
				delete view;
			}

			_ResizeViews();

			if (Window() != NULL)
				Window()->PostMessage(msg);
			break;
		}
		default:
			BView::MessageReceived(msg);
	}
}


void
AppGroupView::AddInfo(NotificationView* view)
{
	BString id = view->MessageID();
	bool found = false;

	if (id.Length() > 0) {
		int32 children = fInfo.size();

		for (int32 i = 0; i < children; i++) {
			if (id == fInfo[i]->MessageID()) {
				GetLayout()->RemoveView(fInfo[i]);
				delete fInfo[i];
				
				fInfo[i] = view;
				found = true;
				
				break;
			}
		}
	}

	if (!found) {
		fInfo.push_back(view);
	}
	GetLayout()->AddView(view);

	if (IsHidden())
		Show();
	if (view->IsHidden() && !fCollapsed)
		view->Show();
}


void
AppGroupView::_ResizeViews()
{
	int32 children = fInfo.size();

	if (!fCollapsed) {
		for (int32 i = 0; i < children; i++) {
			if (fInfo[i]->IsHidden())
				fInfo[i]->Show();
		}
	} else {
		for (int32 i = 0; i < children; i++)
			if (!fInfo[i]->IsHidden())
				fInfo[i]->Hide();
	}

	if (!IsHidden() && !HasChildren())
		Hide();
	fParent->Layout(true);
}


bool
AppGroupView::HasChildren()
{
	return !fInfo.empty();
}
