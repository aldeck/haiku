//****************************************************************************************
//
//	File:		NormalPulseView.h
//
//	Written by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************
#ifndef NORMALPULSEVIEW_H
#define NORMALPULSEVIEW_H


#include "PulseView.h"
#include "ProgressBar.h"
#include "CPUButton.h"


class NormalPulseView : public PulseView {
	public:
		NormalPulseView(BRect rect);
		~NormalPulseView();
		void Draw(BRect rect);
		void Pulse();
		void AttachedToWindow();
		void UpdateColors(BMessage *message);

	private:
		int CalculateCPUSpeed();
		void DetermineVendorAndProcessor();
		
		char fVendor[32], fProcessor[32];
		bigtime_t fPreviousTime;
		ProgressBar **fProgressBars;
		CPUButton **fCpuButtons;
		BBitmap *fCpuLogo;
		bool fHasBrandLogo;
};

#endif
