/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STACK_FRAME_DEBUG_INFO_H
#define STACK_FRAME_DEBUG_INFO_H


#include <Referenceable.h>

#include "Types.h"


class BaseType;
class DataMember;
class StackFrame;
class Type;
class ValueLocation;


class StackFrameDebugInfo : public Referenceable {
public:
								StackFrameDebugInfo();
	virtual						~StackFrameDebugInfo();

	virtual	status_t			ResolveObjectDataLocation(
									StackFrame* stackFrame, Type* type,
									target_addr_t objectAddress,
									ValueLocation*& _location) = 0;
									// returns a reference
	virtual	status_t			ResolveBaseTypeLocation(
									StackFrame* stackFrame, Type* type,
									BaseType* baseType,
									const ValueLocation& parentLocation,
									ValueLocation*& _location) = 0;
									// returns a reference
	virtual	status_t			ResolveDataMemberLocation(
									StackFrame* stackFrame, Type* type,
									DataMember* member,
									const ValueLocation& parentLocation,
									ValueLocation*& _location) = 0;
									// returns a reference
};


#endif	// STACK_FRAME_DEBUG_INFO_H
