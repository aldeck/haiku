/*****************************************************************************/
// InfoWindow
// Written by Michael Wilber, OBOS Translation Kit Team
//
// InfoWindow.h
//
// BWindow class for displaying information about the currently open
// document
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef INFOWINDOW_H
#define INFOWINDOW_H

#include <Window.h>
#include <TextView.h>

class InfoWindow : public BWindow {
public:
	InfoWindow(BRect rect, const char *name, const char *text =
		"This space for rent");
	~InfoWindow();
	void FrameResized(float width, float height);
	void MessageReceived(BMessage *pmsg);
	void Quit();

private:
	BTextView *fptextView;
};

#endif // #ifndef INFOWINDOW_H
