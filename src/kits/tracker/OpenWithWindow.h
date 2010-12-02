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

#ifndef	_OPEN_WITH_WINDOW_H
#define _OPEN_WITH_WINDOW_H

#include <String.h>

#include "ContainerWindow.h"
#include "EntryIterator.h"
#include "NodeWalker.h"
#include "OpenWithUtils.h"
#include "PoseView.h"
#include "Query.h"
#include "Utilities.h"

namespace BPrivate {

class OpenWithPoseView;
class PoseViewController;


class OpenWithContainerWindow : public BContainerWindow {
	public:
		OpenWithContainerWindow(BMessage *entriesToOpen,
			LockingList<BWindow> *windowList,
			window_look look = B_DOCUMENT_WINDOW_LOOK,
			window_feel feel = B_NORMAL_WINDOW_FEEL,
			uint32 flags = 0, uint32 workspace = B_CURRENT_WORKSPACE);
			// <entriesToOpen> eventually get opened by the selected app
		virtual ~OpenWithContainerWindow();

		const BMessage *EntryList() const;
			// return the list of the entries we are supposed to open

		void SetCanSetAppAsDefault(bool);
		void SetCanOpen(bool);

		OpenWithPoseView *PoseView() const;

	protected:
		virtual	void _Init(const BMessage *message);
		virtual	void ShowContextMenu(BPoint, const entry_ref *, BView *);
		virtual void AddShortcuts();
		virtual void NewAttributeMenu(BMenu *);

		virtual void RestoreState();
		virtual	void RestoreState(const BMessage &);
		virtual void RestoreWindowState(AttributeStreamNode *);
		virtual void RestoreWindowState(const BMessage &);
		virtual bool NeedsDefaultStateSetup();
		virtual	void SaveState(bool hide = true);
		virtual	void SaveState(BMessage &) const;
		virtual void SetUpDefaultState();

		virtual bool IsShowing(const node_ref *) const;
		virtual bool IsShowing(const entry_ref *) const;

		virtual void MessageReceived(BMessage *);

		void OpenWithSelection();
			// open entries with the selected app
		void MakeDefaultAndOpen();
			// open entries with the selected app and make it the default handler

	private:
		static filter_result KeyDownFilter(BMessage *, BHandler **, BMessageFilter *);

		BMessage *fEntriesToOpen;
		BButton *fLaunchButton;
		BButton *fLaunchAndMakeDefaultButton;
		float fMinimalWidth;

		typedef BContainerWindow _inherited;
};


class OpenWithPoseView : public BPoseView {
	public:
		OpenWithPoseView();

		virtual void OpenSelection(BPose *, int32 *);
			// open entries with the selected app

		int32 OpenWithRelation(const Model *) const;
			// returns the reason why an application is shown in Open With window
		void OpenWithRelationDescription(const Model *, BString *) const;
			// returns a string describing why application handles files to open

		OpenWithContainerWindow *ContainerWindow() const;

		virtual bool AddPosesThreadValid(const entry_ref *) const;

	protected:
		// don't do any volume watching and memtamime watching in open with panels for now
		virtual void InitialStartWatching() {}
		virtual void FinalStopWatching() {}

		virtual void AttachedToWindow();
		EntryListBase *InitDirentIterator(const entry_ref *ref);

		virtual void SetUpDefaultColumnsIfNeeded();
			// show launch window specific columns

		// empty overrides for functions that depend on having an fModel
		virtual	void SaveState(AttributeStreamNode *);
		virtual void RestoreState(AttributeStreamNode *);
		virtual	void SaveState(BMessage &) const;
		virtual void RestoreState(const BMessage &);
		virtual void SavePoseLocations(BRect * = NULL);
		virtual void MoveSelectionToTrash(bool selectNext = true);
		virtual void MoveSelectionTo(BPoint, BPoint, BContainerWindow*);
		virtual void MoveSelectionInto(Model* destFolder, BContainerWindow *srcWindow,
			bool forceCopy, bool create_link = false);
		virtual bool HandleMessageDropped(BMessage *);
		virtual bool CanHandleDragSelection(const Model *, const BMessage *, bool);

		virtual bool Represents(const node_ref *) const;
		virtual bool Represents(const entry_ref *) const;

		virtual void CreatePoses(Model **models, PoseInfo *poseInfoArray, int32 count,
			BPose **resultingPoses, bool insertionSort = true, int32 *lastPoseIndexPtr = NULL,
			BRect *boundsPtr = NULL, bool forceDraw = false);
			// override to add selecting the default handling app for selection

		virtual bool ShouldShowPose(const Model *, const PoseInfo *);

		virtual void Pulse();

		virtual void KeyDown(const char *bytes, int32 count);

	private:
		entry_ref fPreferredRef;
		bool fHaveCommonPreferredApp;

		SearchForSignatureEntryList *fIterator;
			// private copy of the iterator pointer

		typedef BPoseView _inherited;
};


} // namespace BPrivate

using namespace BPrivate;

#endif	// _OPEN_WITH_WINDOW_H
