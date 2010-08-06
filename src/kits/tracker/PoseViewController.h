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
	virtual	void				SetControlVisible(BView* control, bool visible);

			// TODO: rework menu system. Menu management migrated "as is"
			//	from ContainerWindow			
			void				AddMenus();
			void				AddFileMenu(BMenu *menu);
			void				AddWindowMenu(BMenu *menu);
			
			// TODO: move that in an separate AttributeMenu class. And maybe do the
			// 	same for the other (context)menus.
			BMenuItem*			NewAttributeMenuItem(const char *label,
									const char *name, int32 type, float width,
									int32 align, bool editable, bool statField);	
			BMenuItem*			NewAttributeMenuItem(const char *label,
									const char *name, int32 type,
									const char* displayAs, float width,
									int32 align, bool editable, bool statField);	
			void				NewAttributeMenu(BMenu *menu);
			void				ShowAttributeMenu();
			void				HideAttributeMenu();
			void				MarkAttributeMenu();
			void				MarkAttributeMenu(BMenu *menu);
			void				AddMimeTypesToMenu();
			void				AddMimeTypesToMenu(BMenu *menu);
			BMenu*				AddMimeMenu(const BMimeType& mimeType,
									bool isSuperType,
									BMenu* menu, int32 start);

			// Scrollbar management
			// TODO: to be simplified, make it "event" based just like countview and 
			// barberpole. Through a ExtentChanged hook.

			void				SetScrollBarsEnabled(bool enable);
			void				SetScrollBarsTo(const BPoint& point);
			void				UpdateScrollRange();

			void				ItemCountChanged(uint32 count);
			void				SlowOperationStarted();
			void				SlowOperationEnded();
		
			BMenuBar*	 		MenuBar()		{ return fMenuBar; };
			BMenu*	 			AttributeMenu() { return fAttrMenu; };			
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
			BMenu*				fAttrMenu;
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
