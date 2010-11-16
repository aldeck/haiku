/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _DEFAULT_CONTROLS_H
#define _DEFAULT_CONTROLS_H


#include <Menu.h>
#include <PopUpMenu.h>

#include "NavMenu.h"
#include "PoseViewListener.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "libtracker"


class BMenuItem;
class BMimeType;


namespace BPrivate {

class Model;
class PoseViewController;


class DefaultFileContextMenu : public BPopUpMenu {
public:
								DefaultFileContextMenu(PoseViewController* controller);
};


class DefaultFileMenu : public BMenu, public PoseViewListener {
public:
								DefaultFileMenu(PoseViewController* controller);
	virtual	void				AttachedToWindow();
	virtual	void				TargetModelChanged();
	virtual	void				SelectionChanged();
	virtual	void				MimeTypesChanged() {};
	virtual	void				ColumnsChanged() {};

protected:
			PoseViewController* fController;
};


class DefaultWindowMenu : public BMenu, public PoseViewListener {
public:
								DefaultWindowMenu(PoseViewController* controller);
	virtual	void				TargetModelChanged() {}; // TODO
	virtual	void				SelectionChanged() {};
	virtual	void				MimeTypesChanged() {};
	virtual	void				ColumnsChanged() {};
};


class DefaultMoveMenu : public BNavMenu, public PoseViewListener {
public:
								DefaultMoveMenu(const char* itemName,
									uint32 messageWhat,
									PoseViewController* controller);

	virtual	void				TargetModelChanged();
	virtual	void				SelectionChanged();
	virtual	void				MimeTypesChanged() {};
	virtual	void				ColumnsChanged() {};

protected:
			void 				_Populate(const entry_ref *, bool addLocalOnly);

			PoseViewController*	fController;
			uint32				fMessageWhat;
};


class DefaultAttributeMenu : public BPopUpMenu, public PoseViewListener {
public:
								DefaultAttributeMenu(PoseViewController* controller);
	virtual	void				TargetModelChanged() {};
	virtual	void				SelectionChanged() {};
	virtual	void				MimeTypesChanged();
	virtual	void				ColumnsChanged();

protected:
			void				_MarkItems();

			BMenuItem*			_NewItem(const char *label,
									const char *name, int32 type, float width,
									int32 align, bool editable, bool statField);
			BMenuItem*			_NewItem(const char *label,
									const char *name, int32 type,
									const char* displayAs, float width, int32 align,
									bool editable, bool statField);
			BMenu*				_AddMimeMenu(const BMimeType& mimeType,
									bool isSuperType,
									BMenu* menu, int32 start);

			PoseViewController*	fController;
};


} // namespace BPrivate


using namespace BPrivate;


#endif 	// _DEFAULT_CONTROLS_H
