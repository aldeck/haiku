/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ConfigView.h"
#include "ICOTranslator.h"

#include <CheckBox.h>
#include <ControlLook.h>
#include <SpaceLayoutItem.h>
#include <StringView.h>

#include <stdio.h>
#include <string.h>


ConfigView::ConfigView()
	:
	BGroupView("ICOTranslator Settings", B_VERTICAL, 0)
{
	BAlignment leftAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_UNSET);	

	BStringView* stringView = new BStringView("title", "Windows icon images");
	stringView->SetFont(be_bold_font);
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	float spacing = be_control_look->DefaultItemSpacing();
	AddChild(BSpaceLayoutItem::CreateVerticalStrut(spacing));

	char version[256];
	sprintf(version, "Version %d.%d.%d, %s",
		int(B_TRANSLATION_MAJOR_VERSION(ICO_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_MINOR_VERSION(ICO_TRANSLATOR_VERSION)),
		int(B_TRANSLATION_REVISION_VERSION(ICO_TRANSLATOR_VERSION)),
		__DATE__);
	stringView = new BStringView("version", version);
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	stringView = new BStringView("copyright",
		B_UTF8_COPYRIGHT "2005-2006 Haiku Inc.");
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	AddChild(BSpaceLayoutItem::CreateVerticalStrut(spacing));

	BCheckBox *checkBox = new BCheckBox("color", "Write 32 bit images on"
		" true color input", NULL);
	checkBox->SetExplicitAlignment(leftAlignment);
	AddChild(checkBox);

	checkBox = new BCheckBox("size", "Enforce valid icon sizes", NULL);
	checkBox->SetValue(1);
	checkBox->SetExplicitAlignment(leftAlignment);
	AddChild(checkBox);

	AddChild(BSpaceLayoutItem::CreateVerticalStrut(spacing));

	stringView = new BStringView("valid1", "Valid icon sizes are"
		" 16, 32, or 48");
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	stringView = new BStringView("valid2", "pixel in either direction.");
	stringView->SetExplicitAlignment(leftAlignment);
	AddChild(stringView);

	AddChild(BSpaceLayoutItem::CreateGlue());
	GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);

	SetExplicitPreferredSize(GroupLayout()->MinSize());
}


ConfigView::~ConfigView()
{
}

