/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _POSEVIEW_CONTROLLER_H
#define _POSEVIEW_CONTROLLER_H


#include <ScrollBar.h>

#include <vector>


class BGridLayout;
class BGroupLayout;
class BLayoutItem;
class BMenu;
class BMenuBar;
class BMenuItem;
class BMimeType;


namespace BPrivate {

class BNavigator;
class BTitleView;
class BCountView;
class BHScrollBar;
class BPoseView;
class Model;
class PoseViewListener;


class PoseViewController { // TODO: more abstraction
public:
								PoseViewController();
	virtual						~PoseViewController();

			void				SetPoseView(BPoseView* poseView);

	virtual	void				CreateControls(Model* model); // temporary helper method
									// model is needed for BNavigator.
									// TODO: Just get the model out of fPoseView
			void				CreateMenus();	// temporary helper method
			void				CreateMoveCopyMenus();	// temporary helper method
			void				ReparentSharedMenus(BMenu* newParent);

	virtual	void				SetControlVisible(BView* control, bool visible);
			void				ShowAttributeMenu();
			void				HideAttributeMenu();

			// Scrollbar management
			// TODO: to be simplified, make it "event" based just like countview and
			// barberpole. Through a ExtentChanged hook.

			void				SetScrollBarsEnabled(bool enable);
			void				SetScrollBarsTo(const BPoint& point);
			void				UpdateScrollRange();

			void				ItemCountChanged(uint32 count);
			void				SlowOperationStarted();
			void				SlowOperationEnded();
			void				AddPosesCompleted();

			// TODO: Not sure we need to expose everything, there's no use for it
			// anymore with the listener mechanism
			BMenuBar*	 		MenuBar()		{ return fMenuBar; };
			BMenu*				FileMenu()		{ return fFileMenu; }; 
				// still needed by containerwindow to dynamically change the
				// kMoveToTrash item's label 
			BNavigator* 		Navigator()		{ return fNavigator; };
			BTitleView* 		TitleView()		{ return fTitleView; };
			BPoseView*	 		PoseView()		{ return fPoseView; };
			BCountView* 		CountView()		{ return fCountView; };
			BScrollBar* 		HorizontalScrollBar()
									{ return fHorizontalScrollBar; };
			BScrollBar* 		VerticalScrollBar()
									{ return fVerticalScrollBar; };

protected:
			BMenuBar*			fMenuBar;

			BMenu*				fAttributesMenu;
			BMenu*				fWindowMenu;
			BMenu*				fFileMenu;
			BMenu*				fMoveToMenu;
			BMenu*				fCopyToMenu;
			BMenu*				fCreateLinkMenu;
			BMenu*				fOpenWithMenu;

			BNavigator* 		fNavigator;
			BTitleView* 		fTitleView;
			BPoseView*			fPoseView;
			BCountView* 		fCountView;
			BScrollBar* 		fVerticalScrollBar;
			BScrollBar* 		fHorizontalScrollBar;
};


class BHScrollBar : public BScrollBar {
public:
								BHScrollBar(const char* name, BView* target, BTitleView* titleView);

	virtual	void				ValueChanged(float);

private:
			BTitleView*			fTitleView;
};


} // namespace BPrivate


using namespace BPrivate;


#endif /* _POSEVIEW_CONTROLLER_H */
