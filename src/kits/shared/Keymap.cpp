/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Axel Dörfler, axeld@pinc-software.de.
 */


#include <Keymap.h>

#include <new>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ByteOrder.h>
#include <File.h>

#ifdef HAIKU_TARGET_PLATFORM_HAIKU
#	include "SystemKeymap.h"
	// generated by the build system
#endif


// Private only at this point, as we might want to improve the dead key
// implementation in the future
enum dead_key_index {
	kDeadKeyAcute = 1,
	kDeadKeyGrave,
	kDeadKeyCircumflex,
	kDeadKeyDiaeresis,
	kDeadKeyTilde
};


static const uint32 kModifierKeys = B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY
	| B_CAPS_LOCK | B_OPTION_KEY | B_MENU_KEY;


BKeymap::BKeymap()
	:
	fChars(NULL),
	fCharsSize(0)
{
	Unset();
}


BKeymap::~BKeymap()
{
	delete[] fChars;
}


/*!	Load a map from a file.
	File format in big endian:
		struct key_map
		uint32 size of following charset
		charset (offsets go into this with size of character followed by
		  character)
*/
status_t
BKeymap::SetTo(const char* path)
{
	BFile file;
	status_t status = file.SetTo(path, B_READ_ONLY);
	if (status != B_OK)
		return status;

	return SetTo(file);
}


status_t
BKeymap::SetTo(BDataIO& stream)
{
	if (stream.Read(&fKeys, sizeof(fKeys)) < 1)
		return B_IO_ERROR;

	// convert from big-endian
	for (uint32 i = 0; i < sizeof(fKeys) / 4; i++) {
		((uint32*)&fKeys)[i] = B_BENDIAN_TO_HOST_INT32(((uint32*)&fKeys)[i]);
	}

	if (fKeys.version != 3)
		return B_BAD_DATA;

	if (stream.Read(&fCharsSize, sizeof(uint32)) < 1)
		return B_IO_ERROR;

	fCharsSize = B_BENDIAN_TO_HOST_INT32(fCharsSize);
	if (fCharsSize > 16 * 1024) {
		Unset();
		return B_BAD_DATA;
	}

	delete[] fChars;
	fChars = new char[fCharsSize];

	if (stream.Read(fChars, fCharsSize) != (ssize_t)fCharsSize) {
		Unset();
		return B_IO_ERROR;
	}

	return B_OK;
}


status_t
BKeymap::SetToCurrent()
{
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	key_map* keys = NULL;
	get_key_map(&keys, &fChars);
	if (!keys)
		return B_ERROR;

	memcpy(&fKeys, keys, sizeof(fKeys));
	free(keys);
	return B_OK;
#else	// ! __BEOS__
	fprintf(stderr, "Unsupported operation on this platform!\n");
	exit(1);
#endif	// ! __BEOS__
}


status_t
BKeymap::SetToDefault()
{
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	fKeys = kSystemKeymap;
	fCharsSize = kSystemKeyCharsSize;

	fChars = new (std::nothrow) char[fCharsSize];
	if (fChars == NULL) {
		Unset();
		return B_NO_MEMORY;
	}

	memcpy(fChars, kSystemKeyChars, fCharsSize);
	return B_OK;
#else	// ! __BEOS__
	fprintf(stderr, "Unsupported operation on this platform!\n");
	exit(1);
#endif	// ! __BEOS__
}


void
BKeymap::Unset()
{
	delete[] fChars;
	fChars = NULL;
	fCharsSize = 0;

	memset(&fKeys, 0, sizeof(fKeys));
}


/*!	We need to know if a key is a modifier key to choose
	a valid key when several are pressed together
*/
bool
BKeymap::IsModifierKey(uint32 keyCode) const
{
	return keyCode == fKeys.caps_key
		|| keyCode == fKeys.num_key
		|| keyCode == fKeys.scroll_key
		|| keyCode == fKeys.left_shift_key
		|| keyCode == fKeys.right_shift_key
		|| keyCode == fKeys.left_command_key
		|| keyCode == fKeys.right_command_key
		|| keyCode == fKeys.left_control_key
		|| keyCode == fKeys.right_control_key
		|| keyCode == fKeys.left_option_key
		|| keyCode == fKeys.right_option_key
		|| keyCode == fKeys.menu_key;
}


//! We need to know a modifier for a key
uint32
BKeymap::Modifier(uint32 keyCode) const
{
	if (keyCode == fKeys.caps_key)
		return B_CAPS_LOCK;
	if (keyCode == fKeys.num_key)
		return B_NUM_LOCK;
	if (keyCode == fKeys.scroll_key)
		return B_SCROLL_LOCK;
	if (keyCode == fKeys.left_shift_key)
		return B_LEFT_SHIFT_KEY | B_SHIFT_KEY;
	if (keyCode == fKeys.right_shift_key)
		return B_RIGHT_SHIFT_KEY | B_SHIFT_KEY;
	if (keyCode == fKeys.left_command_key)
		return B_LEFT_COMMAND_KEY | B_COMMAND_KEY;
	if (keyCode == fKeys.right_command_key)
		return B_RIGHT_COMMAND_KEY | B_COMMAND_KEY;
	if (keyCode == fKeys.left_control_key)
		return B_LEFT_CONTROL_KEY | B_CONTROL_KEY;
	if (keyCode == fKeys.right_control_key)
		return B_RIGHT_CONTROL_KEY | B_CONTROL_KEY;
	if (keyCode == fKeys.left_option_key)
		return B_LEFT_OPTION_KEY | B_OPTION_KEY;
	if (keyCode == fKeys.right_option_key)
		return B_RIGHT_OPTION_KEY | B_OPTION_KEY;
	if (keyCode == fKeys.menu_key)
		return B_MENU_KEY;

	return 0;
}


uint32
BKeymap::KeyForModifier(uint32 modifier) const
{
	if (modifier == B_CAPS_LOCK)
		return fKeys.caps_key;
	if (modifier == B_NUM_LOCK)
		return fKeys.num_key;
	if (modifier == B_SCROLL_LOCK)
		return fKeys.scroll_key;
	if (modifier == B_LEFT_SHIFT_KEY || modifier == B_SHIFT_KEY)
		return fKeys.left_shift_key;
	if (modifier == B_RIGHT_SHIFT_KEY)
		return fKeys.right_shift_key;
	if (modifier == B_LEFT_COMMAND_KEY || modifier == B_COMMAND_KEY)
		return fKeys.left_command_key;
	if (modifier == B_RIGHT_COMMAND_KEY)
		return fKeys.right_command_key;
	if (modifier == B_LEFT_CONTROL_KEY || modifier == B_CONTROL_KEY)
		return fKeys.left_control_key;
	if (modifier == B_RIGHT_CONTROL_KEY)
		return fKeys.right_control_key;
	if (modifier == B_LEFT_OPTION_KEY || modifier == B_OPTION_KEY)
		return fKeys.left_option_key;
	if (modifier == B_RIGHT_OPTION_KEY)
		return fKeys.right_option_key;
	if (modifier == B_MENU_KEY)
		return fKeys.menu_key;

	return 0;
}


/*! Checks whether a key is an active dead key.
*/
uint8
BKeymap::ActiveDeadKey(uint32 keyCode, uint32 modifiers) const
{
	bool enabled;
	uint8 deadKey = DeadKey(keyCode, modifiers, &enabled);
	if (deadKey == 0 || !enabled)
		return 0;

	return deadKey;
}


/*! Checks whether a key is a dead key.
	If it is, the enabled/disabled state of that dead key will be passed
	out via isEnabled (isEnabled is not touched for non-dead keys).
*/
uint8
BKeymap::DeadKey(uint32 keyCode, uint32 modifiers, bool* _isEnabled) const
{
	uint32 tableMask = 0;
	int32 offset = Offset(keyCode, modifiers, &tableMask);
	uint8 deadKeyIndex = DeadKeyIndex(offset);
	if (deadKeyIndex > 0 && _isEnabled != NULL) {
		uint32 deadTables[] = {
			fKeys.acute_tables,
			fKeys.grave_tables,
			fKeys.circumflex_tables,
			fKeys.dieresis_tables,
			fKeys.tilde_tables
		};
		*_isEnabled = (deadTables[deadKeyIndex - 1] & tableMask) != 0;
	}

	return deadKeyIndex;
}


//! Tell if a key is a dead second key.
bool
BKeymap::IsDeadSecondKey(uint32 keyCode, uint32 modifiers,
	uint8 activeDeadKey) const
{
	if (!activeDeadKey)
		return false;

	int32 offset = Offset(keyCode, modifiers);
	if (offset < 0)
		return false;

	uint32 numBytes = fChars[offset];
	if (!numBytes)
		return false;

	const int32* deadOffsets[] = {
		fKeys.acute_dead_key,
		fKeys.grave_dead_key,
		fKeys.circumflex_dead_key,
		fKeys.dieresis_dead_key,
		fKeys.tilde_dead_key
	};

	const int32* deadOffset = deadOffsets[activeDeadKey - 1];

	for (int32 i = 0; i < 32; i++) {
		if (offset == deadOffset[i])
			return true;

		uint32 deadNumBytes = fChars[deadOffset[i]];

		if (!deadNumBytes)
			continue;

		if (strncmp(&fChars[offset + 1], &fChars[deadOffset[i] + 1],
				deadNumBytes) == 0)
			return true;
		i++;
	}
	return false;
}


//! Get the char for a key given modifiers and active dead key
void
BKeymap::GetChars(uint32 keyCode, uint32 modifiers, uint8 activeDeadKey,
	char** chars, int32* numBytes) const
{
	*numBytes = 0;
	*chars = NULL;

	if (keyCode > 128 || fChars == NULL)
		return;

	// here we take NUMLOCK into account
	if ((modifiers & B_NUM_LOCK) != 0) {
		switch (keyCode) {
			case 0x37:
			case 0x38:
			case 0x39:
			case 0x48:
			case 0x49:
			case 0x4a:
			case 0x58:
			case 0x59:
			case 0x5a:
			case 0x64:
			case 0x65:
				modifiers ^= B_SHIFT_KEY;
		}
	}

	int32 offset = Offset(keyCode, modifiers);
	if (offset < 0)
		return;

	// here we get the char size
	*numBytes = fChars[offset];
	if (!*numBytes)
		return;

	// here we take an potential active dead key
	const int32* deadKey;
	switch (activeDeadKey) {
		case kDeadKeyAcute:
			deadKey = fKeys.acute_dead_key;
			break;
		case kDeadKeyGrave:
			deadKey = fKeys.grave_dead_key;
			break;
		case kDeadKeyCircumflex:
			deadKey = fKeys.circumflex_dead_key;
			break;
		case kDeadKeyDiaeresis:
			deadKey = fKeys.dieresis_dead_key;
			break;
		case kDeadKeyTilde:
			deadKey = fKeys.tilde_dead_key;
			break;
		default:
		{
			// if not dead, we copy and return the char
			char* str = *chars = new char[*numBytes + 1];
			strncpy(str, &fChars[offset + 1], *numBytes);
			str[*numBytes] = 0;
			return;
		}
	}

	// if dead key, we search for our current offset char in the dead key
	// offset table string comparison is needed
	for (int32 i = 0; i < 32; i++) {
		if (strncmp(&fChars[offset + 1], &fChars[deadKey[i] + 1], *numBytes)
				== 0) {
			*numBytes = fChars[deadKey[i + 1]];

			switch (*numBytes) {
				case 0:
					// Not mapped
					*chars = NULL;
					break;
				default:
				{
					// 1-, 2-, 3-, or 4-byte UTF-8 character
					char *str = *chars = new char[*numBytes + 1];
					strncpy(str, &fChars[deadKey[i + 1] + 1], *numBytes);
					str[*numBytes] = 0;
					break;
				}
			}
			return;
		}
		i++;
	}

	// if not found we return the current char mapped
	*chars = new char[*numBytes + 1];
	strncpy(*chars, &fChars[offset + 1], *numBytes);
	(*chars)[*numBytes] = 0;
}


bool
BKeymap::operator==(const BKeymap& other) const
{
	return fCharsSize == other.fCharsSize
		&& !memcmp(&fKeys, &other.fKeys, sizeof(fKeys))
		&& !memcmp(fChars, other.fChars, sizeof(fChars));
}


bool
BKeymap::operator!=(const BKeymap& other) const
{
	return !(*this == other);
}


BKeymap&
BKeymap::operator=(const BKeymap& other)
{
	Unset();

	fChars = new char[fCharsSize];
	fCharsSize = other.fCharsSize;
	memcpy(fChars, other.fChars, fCharsSize);
	memcpy(&fKeys, &other.fKeys, sizeof(fKeys));

	return *this;
}


int32
BKeymap::Offset(uint32 keyCode, uint32 modifiers, uint32* _table) const
{
	int32 offset;
	uint32 table;

	if (keyCode >= 128)
		return -1;

	switch (modifiers & kModifierKeys) {
		case B_SHIFT_KEY:
			offset = fKeys.shift_map[keyCode];
			table = B_SHIFT_TABLE;
			break;
		case B_CAPS_LOCK:
			offset = fKeys.caps_map[keyCode];
			table = B_CAPS_TABLE;
			break;
		case B_CAPS_LOCK | B_SHIFT_KEY:
			offset = fKeys.caps_shift_map[keyCode];
			table = B_CAPS_SHIFT_TABLE;
			break;
		case B_OPTION_KEY:
			offset = fKeys.option_map[keyCode];
			table = B_OPTION_TABLE;
			break;
		case B_OPTION_KEY | B_SHIFT_KEY:
			offset = fKeys.option_shift_map[keyCode];
			table = B_OPTION_SHIFT_TABLE;
			break;
		case B_OPTION_KEY | B_CAPS_LOCK:
			offset = fKeys.option_caps_map[keyCode];
			table = B_OPTION_CAPS_TABLE;
			break;
		case B_OPTION_KEY | B_SHIFT_KEY | B_CAPS_LOCK:
			offset = fKeys.option_caps_shift_map[keyCode];
			table = B_OPTION_CAPS_SHIFT_TABLE;
			break;
		case B_CONTROL_KEY:
			offset = fKeys.control_map[keyCode];
			table = B_CONTROL_TABLE;
			break;
		default:
			offset = fKeys.normal_map[keyCode];
			table = B_NORMAL_TABLE;
			break;
	}

	if (_table != NULL)
		*_table = table;

	if (offset >= (int32)fCharsSize)
		return -1;

	return offset;
}


uint8
BKeymap::DeadKeyIndex(int32 offset) const
{
	if (fChars == NULL || offset <= 0)
		return 0;

	uint32 numBytes = fChars[offset];
	if (!numBytes || numBytes > 4)
		return 0;

	char chars[5];
	strncpy(chars, &fChars[offset + 1], numBytes);
	chars[numBytes] = 0;

	const int32 deadOffsets[] = {
		fKeys.acute_dead_key[1],
		fKeys.grave_dead_key[1],
		fKeys.circumflex_dead_key[1],
		fKeys.dieresis_dead_key[1],
		fKeys.tilde_dead_key[1]
	};

	uint8 result = 0;
	for (int32 i = 0; i < 5; i++) {
		if (offset == deadOffsets[i])
			return i + 1;

		uint32 deadNumBytes = fChars[deadOffsets[i]];
		if (!deadNumBytes)
			continue;

		if (strncmp(chars, &fChars[deadOffsets[i] + 1], deadNumBytes) == 0)
			return i + 1;
	}

	return result;
}
