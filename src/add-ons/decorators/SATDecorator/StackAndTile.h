/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef STACK_AND_TILE_H
#define STACK_AND_TILE_H


#include <map>

#include <Message.h>
#include <MessageFilter.h>

#include "Desktop.h"
#include "ObjectList.h"
#include "SATGroup.h"
#include "WindowList.h"


//#define DEBUG_STACK_AND_TILE

#ifdef DEBUG_STACK_AND_TILE
#	define STRACE_SAT(x...) debug_printf("SAT: "x)
#else
#	define STRACE_SAT(x...) ;
#endif


class SATWindow;
class Window;


typedef std::map<Window*, SATWindow*> SATWindowMap;


class StackAndTile : public DesktopListener {
public:
								StackAndTile();
	virtual						~StackAndTile();

	virtual int32				Identifier();

	// DesktopListener hooks
	virtual void				ListenerRegistered(Desktop* desktop);
	virtual	void				ListenerUnregistered();

	virtual bool				HandleMessage(Window* sender,
									BPrivate::ServerLink& link);

	virtual void				WindowAdded(Window* window);
	virtual void				WindowRemoved(Window* window);
	
	virtual void				KeyPressed(uint32 what, int32 key,
									int32 modifiers);
	virtual void				MouseEvent(BMessage* message) {}
	virtual void				MouseDown(Window* window, BMessage* message,
									const BPoint& where);
	virtual void				MouseUp(Window* window, BMessage* message,
									const BPoint& where);
	virtual void				MouseMoved(Window* window, BMessage* message,
									const BPoint& where) {}

	virtual void				WindowMoved(Window* window);
	virtual void				WindowResized(Window* window);
	virtual void				WindowActitvated(Window* window);
	virtual void				WindowSentBehind(Window* window,
									Window* behindOf);
	virtual void				WindowWorkspacesChanged(Window* window,
									uint32 workspaces);
	virtual void				WindowMinimized(Window* window, bool minimize);

	virtual void				WindowTabLocationChanged(Window* window,
									float location);
	virtual void				SizeLimitsChanged(Window* window,
									int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);
	virtual void				WindowLookChanged(Window* window,
									window_look look);

	virtual bool				SetDecoratorSettings(Window* window,
									const BMessage& settings);
	virtual void				GetDecoratorSettings(Window* window,
									BMessage& settings);

			bool				SATKeyPressed()
									{ return fSATKeyPressed; }

		SATWindow*				GetSATWindow(Window* window);

private:
			void				_StartSAT();
			void				_StopSAT();
			void				_ActivateWindow(SATWindow* window);

			bool				fSATKeyPressed;
		
			SATWindowMap		fSATWindowMap;
			SATWindowList		fGrouplessWindows;

			SATWindow*			fCurrentSATWindow;

			bool				fTabIsShifting;
};


class GroupIterator {
public:
								GroupIterator(StackAndTile* sat,
									Desktop* desktop);

			void				RewindToFront();
			SATGroup*			NextGroup();

private:
			StackAndTile*		fStackAndTile;
			Desktop*			fDesktop;
			Window*				fCurrentWindow;
			SATGroup*			fCurrentGroup;
};


class WindowIterator {
public:
								WindowIterator(SATGroup* group,
									bool reverseLayerOrder = false);

			void				Rewind();
			/*! Iterates over all areas in the group and return the windows in
			the areas. Within one area the windows are ordered by their layer
			position. If reverseLayerOrder is false the bottommost window comes
			first. */
			SATWindow*			NextWindow();
		

private:
			SATWindow*			_ReverseNextWindow();
			void				_ReverseRewind();

			SATGroup*			fGroup;
			bool				fReverseLayerOrder;

			WindowArea*			fCurrentArea;
			int32				fAreaIndex;
			int32				fWindowIndex;
};


class SATSnappingBehaviour {
public:
	virtual						~SATSnappingBehaviour();

	/*! Find all window candidates which possibly can join the group. Found
	candidates are marked here visual. */
	virtual bool				FindSnappingCandidates(SATGroup* group) = 0;
	/*! Join all candidates found in FindSnappingCandidates to the group.
	Previously visually mark should be removed here. \return true if
	integration has been succeed. */
	virtual bool				JoinCandidates() = 0;
	/*! Update the window tab values, solve the layout and move all windows in
	the group accordantly. */
	virtual void				DoWindowLayout() = 0;
	virtual void				RemovedFromArea(WindowArea* area) {}
	virtual void				TabLocationMoved(float location, bool shifting)
									{}
};


typedef BObjectList<SATSnappingBehaviour> SATSnappingBehaviourList;


#endif
