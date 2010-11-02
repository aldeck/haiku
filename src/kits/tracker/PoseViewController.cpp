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
#include "DefaultControls.h"
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
	fAttributesMenu(NULL),
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


// TODO: remove this, containerwindows will just create what they need.
void
PoseViewController::CreateMenus()
{
	fFileMenu = new DefaultFileMenu(this);
	fWindowMenu = new DefaultWindowMenu(this);
	fAttributesMenu = new DefaultAttributeMenu(this);

	fMenuBar->AddItem(fFileMenu);
	fMenuBar->AddItem(fWindowMenu);
}


BLayoutItem*
LayoutItemForView(BView* view)
{
	if (view != NULL) {
		BLayout* layout = NULL;

		if (view->Parent() != NULL)			
			layout = view->Parent()->GetLayout();
		else if (view->Window() != NULL)
			layout = view->Window()->GetLayout();
		else			
			return NULL;

		if (layout != NULL)
			return layout->ItemAt(layout->IndexOfView(view));
	}
	return NULL;
}


void
PoseViewController::SetControlVisible(BView* control, bool visible)
{
	BLayoutItem* layoutItem = LayoutItemForView(control);
	if (layoutItem != NULL)
		layoutItem->SetVisible(visible);
}


// poseview events


void
PoseViewController::ItemCountChanged(uint32 itemCount)
{
	if (fCountView != NULL) {
		fCountView->SetCount(itemCount);
	}
}


void
PoseViewController::SlowOperationStarted()
{
	if (fCountView != NULL) {
		fCountView->ShowBarberPole();
	}
}


void
PoseViewController::SlowOperationEnded()
{
	if (fCountView != NULL) {
		fCountView->HideBarberPole();
	}
}


void
PoseViewController::AddPosesCompleted()
{
	if (fAttributesMenu != NULL) {
		fAttributesMenu->MimeTypesChanged();
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
PoseViewController::ShowAttributeMenu()
{
	ASSERT(fAttributesMenu);
	fMenuBar->AddItem(fAttributesMenu);
}


void
PoseViewController::HideAttributeMenu()
{
	ASSERT(fAttributesMenu);
	fMenuBar->RemoveItem(fAttributesMenu);
}



//	#pragma mark -


BHScrollBar::BHScrollBar(const char* name, BView* target,
	BTitleView* titleView)
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

	BScrollBar::ValueChanged(value);
}
