/*
 * Copyright 2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Alexandre Deckner <alex@zappotek.com>
 */
#ifndef _POSEVIEW_LISTENER_H
#define _POSEVIEW_LISTENER_H


namespace BPrivate {


class Model;


class PoseViewListener {
public:
	virtual						~PoseViewListener();

	virtual void				TargetModelChanged() = 0;
	virtual void				SelectionChanged() = 0;
	virtual	void				MimeTypesChanged() = 0;
	virtual	void				ColumnsChanged() = 0;
};


} // namespace BPrivate


using namespace BPrivate;


#endif	// _POSEVIEW_LISTENER_H
