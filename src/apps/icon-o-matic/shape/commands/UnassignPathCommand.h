/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef UNASSIGN_PATH_COMMAND_H
#define UNASSIGN_PATH_COMMAND_H

#include "Command.h"

class Shape;
class VectorPath;

class UnassignPathCommand : public Command {
 public:
								UnassignPathCommand(Shape* shape,
													VectorPath* path);
	virtual						~UnassignPathCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Shape*				fShape;
			VectorPath*			fPath;
			bool				fPathRemoved;
};

#endif // UNASSIGN_PATH_COMMAND_H
