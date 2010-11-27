/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "Stacking.h"

#include <Debug.h>

#include "StackAndTilePrivate.h"

#include "SATWindow.h"
#include "Window.h"


#define DEBUG_STACKING

#ifdef DEBUG_STACKING
#	define STRACE_STACKING(x...) debug_printf("SAT Stacking: "x)
#else
#	define STRACE_STACKING(x...) ;
#endif


using namespace BPrivate;


const float kMaxTabWidth = 165.;


bool
StackingEventHandler::HandleMessage(SATWindow* sender,
	BPrivate::LinkReceiver& link, BPrivate::LinkSender& reply)
{
	Desktop* desktop = sender->GetDesktop();
	StackAndTile* stackAndTile = sender->GetStackAndTile();

	int32 what;
	link.Read<int32>(&what);

	switch (what) {
		case kAddWindowToStack:
		{
			port_id port;
			int32 token;
			team_id team;
			link.Read<port_id>(&port);
			link.Read<int32>(&token);
			link.Read<team_id>(&team);
			int32 position;
			if (link.Read<int32>(&position) != B_OK)
				return false;

			WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			if (position < 0)
				position = area->WindowList().CountItems() - 1;

			SATWindow* parent = area->WindowList().ItemAt(position);
			Window* window = desktop->WindowForClientLooperPort(port);
			if (!parent || !window) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}

			SATWindow* candidate = stackAndTile->GetSATWindow(window);
			if (!candidate)
				return false;
			if (!parent->StackWindow(candidate))
				return false;

			reply.StartMessage(B_OK);
			reply.Flush();
			break;
		}
		case kRemoveWindowFromStack:
		{
			port_id port;
			int32 token;
			team_id team;
			link.Read<port_id>(&port);
			link.Read<int32>(&token);
			if (link.Read<team_id>(&team) != B_OK)
				return false;

			SATGroup* group = sender->GetGroup();
			if (!group)
				return false;

			Window* window = desktop->WindowForClientLooperPort(port);
			if (!window) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}
			SATWindow* candidate = stackAndTile->GetSATWindow(window);
			if (!candidate)
				return false;
			if (!group->RemoveWindow(candidate))
				return false;
			break;
		}
		case kRemoveWindowFromStackAt:
		{
			int32 position;
			if (link.Read<int32>(&position) != B_OK)
				return false;
			SATGroup* group = sender->GetGroup();
			WindowArea* area = sender->GetWindowArea();
			if (!area || !group)
				return false;
			SATWindow* removeWindow = area->WindowList().ItemAt(position);
			if (!removeWindow) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}

			if (!group->RemoveWindow(removeWindow))
				return false;

			ServerWindow* window = removeWindow->GetWindow()->ServerWindow();
			reply.StartMessage(B_OK);
			reply.Attach<port_id>(window->ClientLooperPort());
			reply.Attach<int32>(window->ClientToken());
			reply.Attach<team_id>(window->ClientTeam());
			reply.Flush();
			break;
		}
		case kCountWindowsOnStack:
		{
			WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			reply.StartMessage(B_OK);
			reply.Attach<int32>(area->WindowList().CountItems());
			reply.Flush();
			break;
		}
		case kWindowOnStackAt:
		{
			int32 position;
			if (link.Read<int32>(&position) != B_OK)
				return false;
			WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			SATWindow* satWindow = area->WindowList().ItemAt(position);
			if (!satWindow) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}

			ServerWindow* window = satWindow->GetWindow()->ServerWindow();
			reply.StartMessage(B_OK);
			reply.Attach<port_id>(window->ClientLooperPort());
			reply.Attach<int32>(window->ClientToken());
			reply.Attach<team_id>(window->ClientTeam());
			reply.Flush();
			break;
		}
		case kStackHasWindow:
		{
			port_id port;
			int32 token;
			team_id team;
			link.Read<port_id>(&port);
			link.Read<int32>(&token);
			if (link.Read<team_id>(&team) != B_OK)
				return false;

			Window* window = desktop->WindowForClientLooperPort(port);
			if (!window) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}
			SATWindow* candidate = stackAndTile->GetSATWindow(window);
			if (!candidate)
				return false;

			WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			reply.StartMessage(B_OK);
			reply.Attach<bool>(area->WindowList().HasItem(candidate));
			reply.Flush();
			break;
		}
		default:
			return false;
	}
	return true;
}


SATStacking::SATStacking(SATWindow* window)
	:
	fSATWindow(window),
	fStackingParent(NULL)
{
	
}


SATStacking::~SATStacking()
{
	
}


bool
SATStacking::FindSnappingCandidates(SATGroup* group)
{
	_ClearSearchResult();

	Window* window = fSATWindow->GetWindow();
	if (!window->Decorator())
		return false;

	BPoint mousePosition;
	int32 buttons;
	fSATWindow->GetDesktop()->GetLastMouseState(&mousePosition, &buttons);
	if (!window->Decorator()->TabRect().Contains(mousePosition))
		return false;

	// use the upper edge of the candidate window to find the parent window
	mousePosition.y = window->Decorator()->TabRect().top;

	for (int i = 0; i < group->CountItems(); i++) {
		SATWindow* satWindow = group->WindowAt(i);
		// search for stacking parent
		Window* win = satWindow->GetWindow();
		if (win == window || !win->Decorator())
			continue;
		if (win->Decorator()->TabRect().Contains(mousePosition)) {
			// remember window as the parent for stacking
			fStackingParent = satWindow;
			_HighlightWindows(true);
			return true;
		}
	}

	return false;
}


bool
SATStacking::JoinCandidates()
{
	if (!fStackingParent)
		return false;

	bool result = fStackingParent->StackWindow(fSATWindow);

	_ClearSearchResult();
	return result;
}


void
SATStacking::DoWindowLayout()
{
	_AdjustWindowTabs();
}


void
SATStacking::RemovedFromArea(WindowArea* area)
{
	const SATWindowList& list = area->WindowList();
	if (list.CountItems() == 1)
		list.ItemAt(0)->SetStackedMode(false);
	else if (list.CountItems() > 0)
		list.ItemAt(0)->DoGroupLayout();

	fSATWindow->SetStackedMode(false);
}


void
SATStacking::TabLocationMoved(float location, bool shifting)
{
	if (!shifting) {
		_AdjustWindowTabs();
		return;
	}

	SATDecorator* decorator = fSATWindow->GetDecorator();
	Desktop* desktop = fSATWindow->GetWindow()->Desktop();
	WindowArea* area = fSATWindow->GetWindowArea();
	if (!desktop || !area || ! decorator)
		return;

	const SATWindowList& stackedWindows = area->WindowList();
	ASSERT(stackedWindows.CountItems() > 0);
	int32 windowIndex = stackedWindows.IndexOf(fSATWindow);
	ASSERT(windowIndex >= 0);
	float tabLength = stackedWindows.ItemAt(0)->GetDecorator()
		->StackedTabLength();

	float oldTabPosition = windowIndex * (tabLength + 1);
	if (fabs(oldTabPosition - location) < tabLength / 2)
		return;

	int32 neighbourIndex = windowIndex;
	if (oldTabPosition > location)
		neighbourIndex--;
	else
		neighbourIndex++;

	SATWindow* neighbour = stackedWindows.ItemAt(neighbourIndex);
	if (!neighbour)
		return;

	float newNeighbourPosition = windowIndex * (tabLength + 1);
	area->MoveWindowToPosition(fSATWindow, neighbourIndex);
	desktop->SetWindowTabLocation(neighbour->GetWindow(), newNeighbourPosition);
}


void
SATStacking::_ClearSearchResult()
{
	if (!fStackingParent)
		return;

	_HighlightWindows(false);
	fStackingParent = NULL;
}


void
SATStacking::_HighlightWindows(bool highlight)
{
	Desktop* desktop = fSATWindow->GetWindow()->Desktop();
	if (!desktop)
		return;
	fStackingParent->HighlightTab(highlight);
	fSATWindow->HighlightTab(highlight);
}


bool
SATStacking::_AdjustWindowTabs()
{
	SATDecorator* decorator = fSATWindow->GetDecorator();
	Desktop* desktop = fSATWindow->GetWindow()->Desktop();
	WindowArea* area = fSATWindow->GetWindowArea();
	if (!desktop || !area || ! decorator)
		return false;

	if (!decorator->StackedMode())
		return false;

	BRect frame = fSATWindow->CompleteWindowFrame();

	const SATWindowList& stackedWindows = area->WindowList();

	int stackCount = stackedWindows.CountItems();
	float titleBarLength = frame.Width();
	ASSERT(titleBarLength > 0);
	// floor to avoid drawing issues
	float tabLength = floorf(titleBarLength / stackCount);
	// the part that we lost due to the floor
	float roundingError = 0;
	if (tabLength > kMaxTabWidth)
		tabLength = kMaxTabWidth;
	else
		roundingError = titleBarLength - stackCount * tabLength;

	float location = 0;
	for (int i = 0; i < stackCount; i++) {
		SATWindow* window = stackedWindows.ItemAt(i);
		if (i == stackCount - 1)
			window->SetStackedTabLength(tabLength - 1 + roundingError);
		else
			window->SetStackedTabLength(tabLength - 1);

		desktop->SetWindowTabLocation(window->GetWindow(), location);
		location += tabLength;
	}
	return true;
}
