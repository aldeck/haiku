/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "RadioView.h"

#include <stdio.h>

#include <MessageRunner.h>


const uint32 kMsgPulse = 'puls';

const bigtime_t kMinPulseInterval = 100000;
const bigtime_t kMaxPulseInterval = 300000;
const float kMinStep = 3.f;


RadioView::RadioView(BRect frame, const char* name, int32 resizingMode)
	:
	BView(frame, name, resizingMode,
		B_FULL_UPDATE_ON_RESIZE | B_WILL_DRAW | B_FRAME_EVENTS),
	fPercent(0),
	fPulse(NULL),
	fPhase(0),
	fMax(7)
{
}


RadioView::~RadioView()
{
}


void
RadioView::SetPercent(int32 percent)
{
	if (percent < 0)
		percent = 0;
	if (percent > 100)
		percent = 100;

	if (percent == fPercent)
		return;

	fPercent = percent;
	Invalidate();
}


void
RadioView::SetMax(int32 max)
{
	if (max < 0)
		max = 0;
	if (max > 100)
		max = 100;
	if (max == fMax)
		return;

	fMax = max;
	Invalidate();
}


void
RadioView::StartPulsing()
{
	fPhase = 0;
	_RestartPulsing();
}


void
RadioView::StopPulsing()
{
	if (!IsPulsing())
		return;

	delete fPulse;
	fPulse = NULL;
	fPhase = 0;
	Invalidate();
}


void
RadioView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void
RadioView::DetachedFromWindow()
{
	StopPulsing();
}


void
RadioView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgPulse:
			fPhase++;
			Invalidate();
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
RadioView::Draw(BRect updateRect)
{
	SetLowColor(ViewColor());

	BPoint center;
	int32 count;
	float step;
	_Compute(Bounds(), center, count, step);

	for (int32 i = 0; i < count; i++) {
		_SetColor(i, count);
		if (step == kMinStep && _IsDisabled(i, count))
			continue;

		_DrawBow(i, center, count, step);
	}
}


void
RadioView::FrameResized(float /*width*/, float /*height*/)
{
	if (IsPulsing())
		_RestartPulsing();
}


void
RadioView::_RestartPulsing()
{
	delete fPulse;

	// The pulse speed depends on the size of the view
	BPoint center;
	int32 count;
	float step;
	_Compute(Bounds(), center, count, step);

	BMessage message(kMsgPulse);
	fPulse = new BMessageRunner(this, &message, (bigtime_t)(kMinPulseInterval
			+ (kMaxPulseInterval - kMinPulseInterval) / step), -1);
}


void
RadioView::_Compute(const BRect& bounds, BPoint& center, int32& count,
	float& step) const
{
	center.Set(roundf(bounds.Width() / 2), bounds.bottom);
	float size = min_c(center.x * 3 / 2, center.y);
	step = floorf(size / fMax);
	if (step < kMinStep) {
		count = (int32)(size / kMinStep);
		step = kMinStep;
	} else
		count = fMax;
}


void
RadioView::_DrawBow(int32 index, const BPoint& center, int32 count, float step)
{
	float radius = step * index + 1;

	if (step < 4)
		SetPenSize(step / 2);
	else
		SetPenSize(step * 2 / 3);

	SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
	StrokeArc(center, radius, radius, 50, 80);
}


void
RadioView::_SetColor(int32 index, int32 count)
{
	if (_IsDisabled(index, count)) {
		// disabled
		SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	} else if (fPhase == 0 || fPhase % count != index) {
		// enabled
		SetHighColor(tint_color(ViewColor(), B_DARKEN_3_TINT));
	} else {
		// pulsing
		SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
	}
}


bool
RadioView::_IsDisabled(int32 index, int32 count) const
{
	return fPercent < 100 * index / count;
}
