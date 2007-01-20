/*
 * Copyright 2003-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin	
 *		Michael Pfeiffer
 */

#include <InterfaceKit.h>
#include <StorageKit.h>
#include <SupportKit.h>

#include "PrinterSetupWindow.h"

// --------------------------------------------------
PrinterSetupWindow::PrinterSetupWindow(char *printerName)
	:	BlockingWindow(BRect(0,0,300,300), printerName, B_TITLED_WINDOW_LOOK,
			B_MODAL_APP_WINDOW_FEEL, B_NOT_ZOOMABLE)
{
	MoveTo(300, 300);

	fPrinterName 	= printerName;

	if (printerName) {
		BString	title;
		title << printerName << " Printer Setup";
		SetTitle(title.String());
	} else
		SetTitle("Printer Setup");
	
	// ---- Ok, build a default job setup user interface
	BRect			r;
	BButton 		*button;
	float			x, y, w, h;
	font_height		fh;
	
	r = Bounds();

	// add a *dialog* background
	BBox *panel = new BBox(r, "top_panel", B_FOLLOW_ALL, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP,
		B_PLAIN_BORDER);


	const int kInterSpace = 8;
	const int kHorzMargin = 10;
	const int kVertMargin = 10;

	x = kHorzMargin;
	y = kVertMargin;

	// add a label before the list
	const char *kModelLabel = "Printer model";

	be_plain_font->GetHeight(&fh);

	w = Bounds().Width();
	w -= 2 * kHorzMargin;
	h = 150;
	
	BBox * model_group = new BBox(BRect(x, y, x+w, y+h), "model_group", B_FOLLOW_ALL_SIDES);
	model_group->SetLabel(kModelLabel);
	
	BRect rlv = model_group->Bounds();
	
	rlv.InsetBy(kHorzMargin, kVertMargin);
	rlv.top 	+= fh.ascent + fh.descent + fh.leading;
	rlv.right	-= B_V_SCROLL_BAR_WIDTH;
	fModelList		= new BListView(rlv, "model_list",
											B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES );
										
	BScrollView * sv	= new BScrollView( "model_list_scrollview", fModelList,
											B_FOLLOW_ALL_SIDES,	B_WILL_DRAW | B_FRAME_EVENTS, false, true );
	model_group->AddChild(sv);
	
	panel->AddChild(model_group);

	y += (h + kInterSpace);

	x = r.right - kHorzMargin;

	// add a "OK" button, and make it default
	fOkButton 	= new BButton(BRect(x, y, x + 400, y), NULL, "OK", new BMessage(OK_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	fOkButton->ResizeToPreferred();
	fOkButton->GetPreferredSize(&w, &h);
	x -= w;
	fOkButton->MoveTo(x, y);
	fOkButton->MakeDefault(true);
	fOkButton->SetEnabled(false);

	panel->AddChild(fOkButton);

	x -= kInterSpace;

	// add a "Cancel" button	
	button 	= new BButton(BRect(x, y, x + 400, y), NULL, "Cancel", new BMessage(CANCEL_MSG), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	button->ResizeToPreferred();
	button->GetPreferredSize(&w, &h);
	x -= w;
	button->MoveTo(x, y);
	panel->AddChild(button);

	y += (h + kInterSpace);

	panel->ResizeTo(Bounds().Width(), y);
	ResizeTo(Bounds().Width(), y);
	
	float minWidth, maxWidth, minHeight, maxHeight;

	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	SetSizeLimits(panel->Frame().Width(), panel->Frame().Width(),
		panel->Frame().Height(), maxHeight);

	// Finally, add our panel to window
	AddChild(panel);

	BDirectory	Folder;
	BEntry		entry;
	
	Folder.SetTo ("/boot/beos/etc/bubblejet");
	if (Folder.InitCheck() != B_OK)
		return;
		
	while (Folder.GetNextEntry(&entry) != B_ENTRY_NOT_FOUND) {
		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) == B_NO_ERROR)
			fModelList->AddItem (new BStringItem(name));
	}

	fModelList->SetSelectionMessage(new BMessage(MODEL_MSG));
	fModelList->SetInvocationMessage(new BMessage(OK_MSG));
}


// --------------------------------------------------
void 
PrinterSetupWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case OK_MSG:
			{
				// Test model selection (if any), save it in printerName node and return
				BNode  	spoolDir;
				BPath *	path;
				status_t result = B_ERROR;
	
				if (fModelList->CurrentSelection() < 0)
					break;
	
				BStringItem * item = dynamic_cast<BStringItem*>
					(fModelList->ItemAt(fModelList->CurrentSelection()));

				if (!item)
					break;
	
				path = new BPath();
	
				find_directory(B_USER_SETTINGS_DIRECTORY, path);
				path->Append("printers");
				path->Append(fPrinterName);
	
				spoolDir.SetTo(path->Path());
				delete path;
	
				if (spoolDir.InitCheck() != B_OK) {
					BAlert * alert = new BAlert("Uh oh!", 
						"Couldn't find printer spool directory.", "OK");
					alert->Go();
				} else {			
					spoolDir.WriteAttr("printer_model", B_STRING_TYPE, 0, item->Text(), 
						strlen(item->Text()));
					result = B_OK;
				}
				
				Quit(result);
				break;
			}
		
		case CANCEL_MSG:
			Quit(B_ERROR);
			break;
		
		case MODEL_MSG:
			fOkButton->SetEnabled((fModelList->CurrentSelection() >= 0));
			break;
		
		default:
			inherited::MessageReceived(msg);
			break;
		};
}

