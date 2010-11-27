/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef STACKING_H
#define STACKING_H

#include "ObjectList.h"
#include "StackAndTile.h"


class SATWindow;


class StackingEventHandler
{
public:
	static bool				HandleMessage(SATWindow* sender,
								BPrivate::LinkReceiver& link,
								BPrivate::LinkSender& reply);
};


class SATStacking : public SATSnappingBehaviour {
public:
							SATStacking(SATWindow* window);
							~SATStacking();

		bool				FindSnappingCandidates(SATGroup* group);
		bool				JoinCandidates();
		void				DoWindowLayout();

		void				RemovedFromArea(WindowArea* area);
		void				TabLocationMoved(float location, bool shifting);

private:
		void				_ClearSearchResult();
		void				_HighlightWindows(bool highlight = true);
		bool				_AdjustWindowTabs();

		SATWindow*			fSATWindow;

		SATWindow*			fStackingParent;
};

#endif
