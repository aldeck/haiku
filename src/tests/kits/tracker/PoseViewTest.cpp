/*
 * Copyright 2011, Alexandre Deckner. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Application.h>
#include <LayoutBuilder.h>
#include <Window.h>

#include "IconCache.h"
#include "PoseView.h"
#include "PoseViewController.h"

#include "CountView.h"
#include "Navigator.h"
#include <MenuBar.h>

#include <stdio.h>


class Window : public BWindow {
	public:
		Window();

		virtual bool QuitRequested();
};


Window::Window()
	: BWindow(BRect(100, 100, 520, 430), "PoseViewTest /boot/home",
			B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	BEntry entry("/boot/home/");
	entry_ref ref;
   	entry.GetRef(&ref);
	Model* model = new Model(&ref, true);
	if (model->InitCheck() != B_OK || !model->IsDirectory()) {
		printf("Error initializing Model, ref = '%s'\n", ref.name);
		delete model;
		return;
	}

	IconCache::sIconCache = new IconCache();

	BPoseView* poseView = new BPoseView(model, kListMode);
	PoseViewController* controller = new PoseViewController();
	controller->SetPoseView(poseView);
	controller->CreateControls(model);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0.0f)
		//.Add(poseView)
		.Add(controller->MenuBar())
		.Add(controller->Navigator())
		.Add(controller->TitleView())
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(controller->PoseView())
			.Add(controller->VerticalScrollBar())
		.End()
		.AddGroup(B_HORIZONTAL, 0.0f)
			.Add(controller->CountView())
			.Add(controller->HorizontalScrollBar(), 3.0f)
			.SetInsets(0, 0, B_V_SCROLL_BAR_WIDTH, 0)
		.End();

	bool forWriting = true;
	ModelNodeLazyOpener *modelOpener =
		new ModelNodeLazyOpener(model, forWriting, false);
	AttributeStreamFileNode* streamNode = NULL;
	if (modelOpener->IsOpen(forWriting)) {
		streamNode = new AttributeStreamFileNode(modelOpener->TargetModel()->Node());
		poseView->Init(streamNode);
	}

	//RestoreState();			// TODO shouldn't be part of BContainerWindow, check if it was restoring the poseview

	controller->CreateMenus();
	//AddContextMenus();		// TODO shouldn't be part of BContainerWindow
	//AddCommonShortcuts();

	//CheckScreenIntersect();
		// check window frame TODO: should be done after/in restorewindowstate

	// TODO: this kind of call could come from a ViewMode object hook that
	// customizes the ui
	if (poseView->ViewMode() == kListMode)
		controller->ShowAttributesMenu();

	controller->SetControlVisible(controller->Navigator(), true);
	controller->CreateMoveCopyMenus();

	controller->TitleView()->Reset();
}


bool
Window::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


//	#pragma mark -


class Application : public BApplication {
	public:
		Application();

		virtual void ReadyToRun(void);
};


Application::Application()
	: BApplication("application/x-vnd.Haiku-poseview-test")
{
}


void
Application::ReadyToRun(void)
{
	BWindow *window = new Window();
	window->Show();
}


//	#pragma mark -


int
main(int argc, char **argv)
{
	Application app;

	app.Run();
	return 0;
}

