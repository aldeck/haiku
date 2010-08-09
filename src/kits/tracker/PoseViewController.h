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


class BGridLayout;
class BGroupLayout;
class BLayoutItem;
class BMenu;
class BMenuBar;
class BMenuItem;
class BMimeType;


namespace BPrivate {

class DefaultAttributeMenu;	
class BNavigator;
class BTitleView;
class BCountView;
class BHScrollBar;
class BPoseView;
class Model;


class PoseViewController { // TODO: more abstraction
public:
								PoseViewController();
	virtual						~PoseViewController();

			void				SetPoseView(BPoseView* poseView);
			
	virtual	void				CreateControls(Model* model); // temporary helper method
									// model is needed for BNavigator.
									// TODO: Just get the model out of fPoseView
			void				CreateMenus();	// temporary helper method
			
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
		
			BMenuBar*	 		MenuBar()		{ return fMenuBar; };
			DefaultAttributeMenu*	AttributeMenu() { return fAttributesMenu; };			
			BMenu*	 			WindowMenu()	{ return fWindowMenu; };
			BMenu*	 			FileMenu()		{ return fFileMenu; };
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
			DefaultAttributeMenu*	fAttributesMenu;
			BMenu*				fWindowMenu;
			BMenu*				fFileMenu;			
			BNavigator* 		fNavigator;
			BTitleView* 		fTitleView;
			BPoseView*			fPoseView;
			BCountView* 		fCountView;
			BScrollBar* 		fVerticalScrollBar;
			BScrollBar* 		fHorizontalScrollBar;
};


class BHScrollBar : public BScrollBar {
	public:
		BHScrollBar(const char *name, BView *target, BTitleView *titleView);

		// BScrollBar overrides
		virtual	void ValueChanged(float);

	private:
		BTitleView *fTitleView;

		typedef BScrollBar _inherited;
};


} // namespace BPrivate


using namespace BPrivate;


#endif /* _POSEVIEW_CONTROLLER_H */
