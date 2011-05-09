/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef SAT_WINDOW_H
#define SAT_WINDOW_H


#include <Region.h>
#include "SATDecorator.h"
#include "Stacking.h"
#include "Tiling.h"


class Desktop;
class SATGroup;
class StackAndTile;
class Window;


class GroupCookie
{
public:
								GroupCookie(SATWindow* satWindow);
								~GroupCookie();

			bool				Init(SATGroup* group, WindowArea* area);
			void				Uninit();

			void				DoGroupLayout(SATWindow* triggerWindow);
			void				MoveWindow(int32 workspace);
			void				SetSizeLimits(int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);

			SATGroup*			GetGroup() { return fSATGroup.Get(); }

			WindowArea*			GetWindowArea() { return fWindowArea; }

			bool				PropagateToGroup(SATGroup* group,
									WindowArea* area);

private:
			void				_UpdateWindowSize(const BRect& frame);

			SATWindow*			fSATWindow;

			BReference<SATGroup>	fSATGroup;

			WindowArea*			fWindowArea;

			Variable*			fLeftBorder;
			Variable*			fTopBorder;
			Variable*			fRightBorder;
			Variable*			fBottomBorder;

			Constraint*			fLeftBorderConstraint;
			Constraint*			fTopBorderConstraint;
			Constraint*			fRightBorderConstraint;
			Constraint*			fBottomBorderConstraint;

			Constraint*			fLeftConstraint;
			Constraint*			fTopConstraint;
			Constraint*			fMinWidthConstraint;
			Constraint*			fMinHeightConstraint;
			Constraint*			fWidthConstraint;
			Constraint*			fHeightConstraint;
};


class SATWindow {
public:
								SATWindow(StackAndTile* sat, Window* window);
								~SATWindow();

			Window*				GetWindow() { return fWindow; }
			SATDecorator*		GetDecorator() const;
			StackAndTile*		GetStackAndTile() { return fStackAndTile; }
			Desktop*			GetDesktop() { return fDesktop; }
			//! Can be NULL if memory allocation failed!
			SATGroup*			GetGroup();
			WindowArea*			GetWindowArea() {
									return fGroupCookie->GetWindowArea(); }

			bool				HandleMessage(SATWindow* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply);

			bool				PropagateToGroup(SATGroup* group,
									WindowArea* area);

			//! Move the window to the tab's position. 
			void				MoveWindowToSAT(int32 workspace);

			// hook function called from SATGroup
			bool				AddedToGroup(SATGroup* group, WindowArea* area);
			bool				RemovedFromGroup(SATGroup* group,
									bool stayBelowMouse);
			void				RemovedFromArea(WindowArea* area);

			bool				StackWindow(SATWindow* child);

			void				FindSnappingCandidates();
			bool				JoinCandidates();
			void				DoWindowLayout();
			void				DoGroupLayout();

			void				SetSizeLimits(int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);
			// hook called when window has been resized form the outside
			void				Resized();

			//! \return the complete window frame including the Decorator
			BRect				CompleteWindowFrame();
			void				GetSizeLimits(int32* minWidth, int32* maxWidth,
									int32* minHeight, int32* maxHeight) const;

			//! \return true if window is in a group with a least another window
			bool				PositionManagedBySAT();

			bool				HighlightTab(bool active);
			bool				HighlightBorders(Decorator::Region region,
									bool active);
			bool				IsTabHighlighted();
			bool				IsBordersHighlighted();

			bool				SetStackedMode(bool stacked = true);
			bool				SetStackedTabLength(float length);
			bool				SetStackedTabMoving(bool moving = true);
			void				TabLocationMoved(float location, bool shifting);

			uint64				Id();

			bool				SetSettings(const BMessage& message);
			void				GetSettings(BMessage& message);
private:
			void				_InitGroup();
			void				_UpdateSizeLimits();
			uint64				_GenerateId();

			void				_RestoreOriginalSize(
									bool stayBelowMouse = true);

			Window*				fWindow;
			StackAndTile*		fStackAndTile;
			Desktop*			fDesktop;

			//! Current group.
			GroupCookie*		fGroupCookie;
			/*! If the window is added to another group the own group is cached
			here. */
			GroupCookie			fOwnGroupCookie;
			GroupCookie			fForeignGroupCookie;

			SATSnappingBehaviour*	fOngoingSnapping;
			SATStacking			fSATStacking;
			SATTiling			fSATTiling;

			SATSnappingBehaviourList	fSATSnappingBehaviourList;

			bool				fShutdown;

			int32				fOriginalMinWidth;
			int32				fOriginalMaxWidth;
			int32				fOriginalMinHeight;
			int32				fOriginalMaxHeight;

			float				fOriginalWidth;
			float				fOriginalHeight;

			uint64				fId;
};


#endif
