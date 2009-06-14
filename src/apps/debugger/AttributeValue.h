/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_VALUE_H
#define ATTRIBUTE_VALUE_H

#include "attribute_classes.h"
#include "types.h"


class DebugInfoEntry;


struct AttributeValue {
	union {
		dwarf_addr_t		address;
		struct {
			const void*		data;
			dwarf_size_t	length;
		}					block;
		uint64				constant;
		bool				flag;
		dwarf_off_t			pointer;
		DebugInfoEntry*		reference;
		const char*			string;
	};

	uint16				attributeForm;
	uint8				attributeClass;
	bool				isSigned;

	const char* ToString(char* buffer, size_t size);
};


struct DynamicAttributeValue {
	union {
		uint64				constant;
		DebugInfoEntry*		reference;
		struct {
			const void*		data;
			dwarf_size_t	length;
		}					block;
	};
	uint8				attributeClass;

	DynamicAttributeValue()
		:
		attributeClass(ATTRIBUTE_CLASS_CONSTANT)
	{
		this->constant = 0;
	}

	void SetTo(uint64 constant)
	{
		this->constant = constant;
		attributeClass = ATTRIBUTE_CLASS_CONSTANT;
	}

	void SetTo(DebugInfoEntry* reference)
	{
		this->reference = reference;
		attributeClass = ATTRIBUTE_CLASS_REFERENCE;
	}

	void SetTo(const void* data, dwarf_size_t length)
	{
		block.data = data;
		block.length = length;
		attributeClass = ATTRIBUTE_CLASS_BLOCK;
	}
};


struct DeclarationLocation {
	uint32	file;
	uint32	line;
	uint32	column;

	DeclarationLocation()
		:
		file(0),
		line(0),
		column(0)
	{
	}

	void SetFile(uint32 file)
	{
		this->file = file;
	}

	void SetLine(uint32 line)
	{
		this->line = line;
	}

	void SetColumn(uint32 column)
	{
		this->column = column;
	}
};

#endif	// ATTRIBUTE_VALUE_H
