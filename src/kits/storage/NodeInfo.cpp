//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file NodeInfo.cpp
	BNodeInfo implementation.
*/

#include <NodeInfo.h>

#include <new>

#include <Bitmap.h>
#include <Entry.h>
#include <Node.h>
#include <Path.h>
#include <Rect.h>

#include <fs_attr.h>

// attribute names
#define NI_BEOS "BEOS"
static const char *kNITypeAttribute			= NI_BEOS ":TYPE";
static const char *kNIPreferredAppAttribute	= NI_BEOS ":PREF_APP";
static const char *kNIAppHintAttribute		= NI_BEOS ":PPATH";
static const char *kNIMiniIconAttribute		= NI_BEOS ":M:STD_ICON";
static const char *kNILargeIconAttribute	= NI_BEOS ":L:STD_ICON";

// icon types
enum {
	B_MINI_ICON_TYPE	= 'MICN',
	B_LARGE_ICON_TYPE	= 'ICON',
};

// constructor
/*!	\brief Creates an uninitialized BNodeInfo object.

	After created a BNodeInfo with this, you should call SetTo().

	\see SetTo(BNode *node)
*/
BNodeInfo::BNodeInfo()
		 : fNode(NULL),
		   fCStatus(B_NO_INIT)
{
}

// constructor
/*!	\brief Creates a BNodeInfo object and initializes it to the supplied node.

	\param node The node to gather information on. Can be any flavor.

	\see SetTo(BNode *node)
*/
BNodeInfo::BNodeInfo(BNode *node)
		 : fNode(NULL),
		   fCStatus(B_NO_INIT)
{
	fCStatus = SetTo(node);
}

// destructor
/*!	\brief Frees all resources associated with this object.

	The BNode object passed to the constructor or to SetTo() is not deleted.
*/
BNodeInfo::~BNodeInfo()
{
}

// SetTo
/*!	\brief Initializes the BNodeInfo to the supplied node.

	The BNodeInfo object does not copy the supplied object, but uses it
	directly. You must not delete the object you supply while the BNodeInfo
	does exist. The BNodeInfo does not take over ownership of the BNode and
	it doesn't delete it on destruction.

	\param node The node to play with

	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: The node was bad.
*/
status_t
BNodeInfo::SetTo(BNode *node)
{
	fNode = NULL;
	// check parameter
	fCStatus = (node && node->InitCheck() == B_OK ? B_OK : B_BAD_VALUE);
	if (fCStatus == B_OK)
		fNode = node;
	return fCStatus;
}

// InitCheck
/*!	\brief returns whether the object has been properly initialized.

	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The node is not properly initialized.
*/
status_t 
BNodeInfo::InitCheck() const
{
	return fCStatus;
}

// GetType
/*!	\brief Gets the node's MIME type.

	Writes the contents of the "BEOS:TYPE" attribute into the supplied buffer
	\a type.

	\param type A pointer to a pre-allocated character buffer of size
		   \c B_MIME_TYPE_LENGTH or larger into which the MIME type of the
		   node shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a type or the type string stored in the
	  attribute is longer than \c B_MIME_TYPE_LENGTH.
	- \c B_BAD_TYPE: The attribute the type string is stored in has the wrong
	   type.
	- \c B_ENTRY_NOT_FOUND: No type is set on the node.
	- other error codes
*/
status_t 
BNodeInfo::GetType(char *type) const
{
	// check parameter and initialization
	status_t error = (type ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// get the attribute info and check type and length of the attr contents
	attr_info attrInfo;
	if (error == B_OK)
		error = fNode->GetAttrInfo(kNITypeAttribute, &attrInfo);
	if (error == B_OK && attrInfo.type != B_MIME_STRING_TYPE)
		error = B_BAD_TYPE;
	if (error == B_OK && attrInfo.size > B_MIME_TYPE_LENGTH)
		error = B_BAD_VALUE;	// TODO: B_BAD_DATA?
	// read the data
	if (error == B_OK) {
		ssize_t read = fNode->ReadAttr(kNITypeAttribute, attrInfo.type, 0,
									   type, attrInfo.size);
		if (read < 0)
			error = read;
		else if (read != attrInfo.size)
			error = B_ERROR;
		// to be save, null terminate the string at the very end
		if (error == B_OK)
			type[B_MIME_TYPE_LENGTH - 1] = '\0';
	}
	return error;
}

// SetType
/*!	\brief Sets the node's MIME type.

	The supplied string is written into the node's "BEOS:TYPE" attribute.

	If \a type is \c NULL, the respective attribute is removed.

	\param type The MIME type to be assigned to the node. Must not be longer
		   than \c B_MIME_TYPE_LENGTH (including the terminating null).
		   May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \a type is longer than \c B_MIME_TYPE_LENGTH.
	- other error codes
*/
status_t
BNodeInfo::SetType(const char *type)
{
	// check parameter and initialization
	status_t error = B_OK;
	if (error == B_OK && type && strlen(type) >= B_MIME_TYPE_LENGTH)
		error = B_BAD_VALUE;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// write/remove the attribute
	if (error == B_OK) {
		if (type) {
			size_t toWrite = strlen(type) + 1;
			ssize_t written = fNode->WriteAttr(kNITypeAttribute,
											   B_MIME_STRING_TYPE, 0, type,
											   toWrite);
			if (written < 0)
				error = written;
			else if (written != (ssize_t)toWrite)
				error = B_ERROR;
		} else
			error = fNode->RemoveAttr(kNITypeAttribute);
	}
	return error;
}

// GetIcon
/*!	\brief Gets the node's icon.

	The icon stored in the node's "BEOS:L:STD_ICON" (large) or
	"BEOS:M:STD_ICON" (mini) attribute is retrieved.

	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param k Specifies the size of the icon to be retrieved: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a icon, unsupported icon size \a k or bitmap
		 dimensions (\a icon) and icon size (\a k) do not match.
	- other error codes
*/
status_t
BNodeInfo::GetIcon(BBitmap *icon, icon_size k) const
{
	status_t error = B_OK;
	// set some icon size related variables
	const char *attribute = NULL;
	BRect bounds;
	uint32 attrType;
	size_t attrSize;
	switch (k) {
		case B_MINI_ICON:
			attribute = kNIMiniIconAttribute;
			bounds.Set(0, 0, 15, 15);
			attrType = B_MINI_ICON_TYPE;
			attrSize = 16 * 16;
			break;
		case B_LARGE_ICON:
			attribute = kNILargeIconAttribute;
			bounds.Set(0, 0, 31, 31);
			attrType = B_LARGE_ICON_TYPE;
			attrSize = 32 * 32;
			break;
		default:
			error = B_BAD_VALUE;
			break;
	}
	// check parameter and initialization
	if (error == B_OK
		&& (!icon || icon->InitCheck() != B_OK || icon->Bounds() != bounds)) {
		error = B_BAD_VALUE;
	}
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// get the attribute info and check type and size of the attr contents
	attr_info attrInfo;
	if (error == B_OK)
		error = fNode->GetAttrInfo(attribute, &attrInfo);
	if (error == B_OK && attrInfo.type != attrType)
		error = B_BAD_TYPE;
	if (error == B_OK && attrInfo.size != attrSize)
		error = B_BAD_VALUE;	// TODO: B_BAD_DATA?
	// read the attribute
	if (error == B_OK) {
		bool otherColorSpace = (icon->ColorSpace() != B_CMAP8);
		char *buffer = NULL;
		ssize_t read;
		if (otherColorSpace) {
			// other color space than stored in attribute
			buffer = new(nothrow) char[attrSize];
			if (!buffer)
				error = B_NO_MEMORY;
			if (error == B_OK) {
				read = fNode->ReadAttr(attribute, attrType, 0, buffer,
									   attrSize);
			}
		} else {
			read = fNode->ReadAttr(attribute, attrType, 0, icon->Bits(), 
								   attrSize);
		}
		if (error == B_OK) {
			if (read < 0)
				error = read;
			else if (read != attrInfo.size)
				error = B_ERROR;
		}
		if (otherColorSpace) {
			// other color space than stored in attribute
			if (error == B_OK)
				icon->SetBits(buffer, attrSize, 0, B_CMAP8);
			delete[] buffer;
		}
	}
	return error;
}

// SetIcon
/*!	\brief Sets the node's icon.

	The icon is stored in the node's "BEOS:L:STD_ICON" (large) or
	"BEOS:M:STD_ICON" (mini) attribute.

	If \a icon is \c NULL, the respective attribute is removed.

	\param icon A pointer to the BBitmap containing the icon to be set.
		   May be \c NULL.
	\param k Specifies the size of the icon to be set: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: Unknown icon size \a k or bitmap dimensions (\a icon)
		 and icon size (\a k) do not match.
	- other error codes
*/
status_t 
BNodeInfo::SetIcon(const BBitmap *icon, icon_size k)
{
	status_t error = B_OK;
	// set some icon size related variables
	const char *attribute = NULL;
	BRect bounds;
	uint32 attrType;
	size_t attrSize;
	switch (k) {
		case B_MINI_ICON:
			attribute = kNIMiniIconAttribute;
			bounds.Set(0, 0, 15, 15);
			attrType = B_MINI_ICON_TYPE;
			attrSize = 16 * 16;
			break;
		case B_LARGE_ICON:
			attribute = kNILargeIconAttribute;
			bounds.Set(0, 0, 31, 31);
			attrType = B_LARGE_ICON_TYPE;
			attrSize = 32 * 32;
			break;
		default:
			error = B_BAD_VALUE;
			break;
	}
	// check parameter and initialization
	if (error == B_OK && icon
		&& (icon->InitCheck() != B_OK || icon->Bounds() != bounds)) {
		error = B_BAD_VALUE;
	}
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// write/remove the attribute
	if (error == B_OK) {
		if (icon) {
			bool otherColorSpace = (icon->ColorSpace() != B_CMAP8);
			ssize_t written;
			if (otherColorSpace) {
				BBitmap bitmap(bounds, B_CMAP8);
				error = bitmap.InitCheck();
				if (error == B_OK) {
					bitmap.SetBits(icon->Bits(), 0, attrSize, B_CMAP8);
					written = fNode->WriteAttr(attribute, attrType, 0,
											   bitmap.Bits(), attrSize);
				}
			} else {
				written = fNode->WriteAttr(attribute, attrType, 0,
										   icon->Bits(), attrSize);
			}
			if (error == B_OK) {
				if (written < 0)
					error = written;
				else if (written != (ssize_t)attrSize)
					error = B_ERROR;
			}
		} else	// no icon given => remove
			error = fNode->RemoveAttr(attribute);
	}
	return error;
}

// GetPreferredApp
/*!	\brief Gets the node's preferred application.

	Writes the contents of the "BEOS:PREF_APP" attribute into the supplied
	buffer \a signature. The preferred application is identifief by its
	signature.

	\param signature A pointer to a pre-allocated character buffer of size
		   \c B_MIME_TYPE_LENGTH or larger into which the MIME type of the
		   preferred application shall be written.
	\param verb Specifies the type of access the preferred application is
		   requested for. Currently only \c B_OPEN is meaningful.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a signature or bad app_verb \a verb.
	- other error codes
*/
status_t 
BNodeInfo::GetPreferredApp(char *signature, app_verb verb) const
{
	// check parameter and initialization
	status_t error = (signature && verb == B_OPEN ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// get the attribute info and check type and length of the attr contents
	attr_info attrInfo;
	if (error == B_OK)
		error = fNode->GetAttrInfo(kNIPreferredAppAttribute, &attrInfo);
	if (error == B_OK && attrInfo.type != B_MIME_STRING_TYPE)
		error = B_BAD_TYPE;
	if (error == B_OK && attrInfo.size > B_MIME_TYPE_LENGTH)
		error = B_BAD_VALUE;	// TODO: B_BAD_DATA?
	// read the data
	if (error == B_OK) {
		ssize_t read = fNode->ReadAttr(kNIPreferredAppAttribute, attrInfo.type,
									   0, signature, attrInfo.size);
		if (read < 0)
			error = read;
		else if (read != attrInfo.size)
			error = B_ERROR;
		// to be save, null terminate the string at the very end
		if (error == B_OK)
			signature[B_MIME_TYPE_LENGTH - 1] = '\0';
	}
	return error;
}

// SetPreferredApp
/*!	\brief Sets the node's preferred application.

	The supplied string is written into the node's "BEOS:PREF_APP" attribute.

	If \a signature is \c NULL, the respective attribute is removed.

	\param signature The signature of the preferred application to be set.
		   Must not be longer than \c B_MIME_TYPE_LENGTH (including the
		   terminating null). May be \c NULL.
	\param verb Specifies the type of access the preferred application shall
		   be set for. Currently only \c B_OPEN is meaningful.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a signature, \a signature is longer than
	  \c B_MIME_TYPE_LENGTH or bad app_verb \a verb.
	- other error codes
*/
status_t 
BNodeInfo::SetPreferredApp(const char *signature, app_verb verb)
{
	// check parameters and initialization
	status_t error = (verb == B_OPEN ? B_OK : B_BAD_VALUE);
	if (error == B_OK && signature && strlen(signature) >= B_MIME_TYPE_LENGTH)
		error = B_BAD_VALUE;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// write/remove the attribute
	if (error == B_OK) {
		if (signature) {
			size_t toWrite = strlen(signature) + 1;
			ssize_t written = fNode->WriteAttr(kNIPreferredAppAttribute,
											   B_MIME_STRING_TYPE, 0,
											   signature, toWrite);
			if (written < 0)
				error = written;
			else if (written != (ssize_t)toWrite)
				error = B_ERROR;
		} else
			error = fNode->RemoveAttr(kNIPreferredAppAttribute);
	}
	return error;
}

// GetAppHint
/*!	\brief Returns a hint in form of and entry_ref to the application that
		   shall be used to open this node.

	The path contained in the node's "BEOS:PPATH" attribute is converted into
	an entry_ref and returned in \a ref.

	\param ref A pointer to a pre-allocated entry_ref into which the requested
		   app hint shall be written.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- other error codes
*/
status_t 
BNodeInfo::GetAppHint(entry_ref *ref) const
{
	// check parameter and initialization
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// get the attribute info and check type and length of the attr contents
	attr_info attrInfo;
	if (error == B_OK)
		error = fNode->GetAttrInfo(kNIAppHintAttribute, &attrInfo);
	// NOTE: The attribute type should be B_STRING_TYPE, but R5 uses
	// B_MIME_STRING_TYPE.
	if (error == B_OK && attrInfo.type != B_MIME_STRING_TYPE)
		error = B_BAD_TYPE;
	if (error == B_OK && attrInfo.size > B_PATH_NAME_LENGTH)
		error = B_BAD_VALUE;	// TODO: B_BAD_DATA?
	// read the data
	if (error == B_OK) {
		char path[B_PATH_NAME_LENGTH];
		ssize_t read = fNode->ReadAttr(kNIAppHintAttribute, attrInfo.type, 0,
									   path, attrInfo.size);
		if (read < 0)
			error = read;
		else if (read != attrInfo.size)
			error = B_ERROR;
		// get the entry_ref for the path
		if (error == B_OK) {
			// to be save, null terminate the path at the very end
			path[B_PATH_NAME_LENGTH - 1] = '\0';
			error = get_ref_for_path(path, ref);
		}
	}
	return error;
}

// SetAppHint
/*!	\brief Sets the node's app hint.

	The supplied entry_ref is converted into a path and stored in the node's
	"BEOS:PPATH" attribute.

	If \a ref is \c NULL, the respective attribute is removed.

	\param ref A pointer to an entry_ref referring to the application.
		   May be \c NULL.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a ref.
	- other error codes
*/
status_t 
BNodeInfo::SetAppHint(const entry_ref *ref)
{
	// check parameter and initialization
	status_t error = B_OK;
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	// write/remove the attribute
	if (error == B_OK) {
		if (ref) {
			BPath path;
			error = path.SetTo(ref);
			if (error == B_OK) {
				size_t toWrite = strlen(path.Path()) + 1;
				ssize_t written = fNode->WriteAttr(kNIAppHintAttribute,
												   B_MIME_STRING_TYPE, 0,
												   path.Path(), toWrite);
				if (written < 0)
					error = written;
				else if (written != (ssize_t)toWrite)
					error = B_ERROR;
			}
		} else
			error = fNode->RemoveAttr(kNIAppHintAttribute);
	}
	return error;
}

// GetTrackerIcon
/*!	\brief Gets the icon which tracker displays.

	This method tries real hard to find an icon for the node:
	- If the node has no type, return the icon for "application/octet-stream"
	  from the MIME database. Even, if the node has an own icon!
	- Ask GetIcon().
	- Get the preferred application and ask the MIME database, if that
	  application has a special icon for the node's file type.
	- Ask the MIME database whether there is an icon for the node's file type.
	- Ask the MIME database for the preferred application for the node's
	  file type and whether this application has a special icon for the type.
	- Return the icon for "application/octet-stream" from the MIME database.
	This list is processed in the given order and the icon the first
	successful attempt provides is returned. In case none of them yields an
	icon, this method fails. This is very unlikely though.

	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param k Specifies the size of the icon to be retrieved: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL \a icon, unsupported icon size \a k or bitmap
		 dimensions (\a icon) and icon size (\a k) do not match.
	- other error codes
*/
status_t 
BNodeInfo::GetTrackerIcon(BBitmap *icon, icon_size k) const
{
	// set some icon size related variables
	status_t error = B_OK;
	BRect bounds;
	switch (k) {
		case B_MINI_ICON:
			bounds.Set(0, 0, 15, 15);
			break;
		case B_LARGE_ICON:
			bounds.Set(0, 0, 31, 31);
			break;
		default:
			error = B_BAD_VALUE;
			break;
	}
	// check parameters and initialization
	if (error == B_OK
		&& (!icon || icon->InitCheck() != B_OK || icon->Bounds() != bounds)) {
		error = B_BAD_VALUE;
	}
	if (error == B_OK && InitCheck() != B_OK)
		error = B_NO_INIT;
	bool success = false;
	// get node MIME type, and, if that fails, the generic icon
	char mimeString[B_MIME_TYPE_LENGTH];
	if (error == B_OK) {
		if (GetType(mimeString) != B_OK) {
			// no type available -- get the "application/octet-stream" icon
			BMimeType type("application/octet-stream");
			error = type.GetIcon(icon, k);
			success = (error == B_OK);
		}
	}
	// Ask GetIcon().
	if (error == B_OK && !success)
		success = (GetIcon(icon, k) == B_OK);
	// Get the preferred application and ask the MIME database, if that
	// application has a special icon for the node's file type.
	if (error == B_OK && !success) {
		char signature[B_MIME_TYPE_LENGTH];
		if (GetPreferredApp(signature) == B_OK) {
			BMimeType type(signature);
			success = (type.GetIconForType(mimeString, icon, k) == B_OK);
		}
	}
	// Ask the MIME database whether there is an icon for the node's file type.
	BMimeType nodeType;
	if (error == B_OK && !success) {
		nodeType.SetTo(mimeString);
		success = (nodeType.GetIcon(icon, k) == B_OK);
	}
	// Ask the MIME database for the preferred application for the node's
	// file type and whether this application has a special icon for the type.
	if (error == B_OK && !success) {
		char signature[B_MIME_TYPE_LENGTH];
		if (nodeType.GetPreferredApp(signature) == B_OK) {
			BMimeType type(signature);
			success = (type.GetIconForType(mimeString, icon, k) == B_OK);
		}
	}
	// Return the icon for "application/octet-stream" from the MIME database.
	if (error == B_OK && !success) {
		// get the "application/octet-stream" icon
		BMimeType type("application/octet-stream");
		error = type.GetIcon(icon, k);
		success = (error == B_OK);
	}
	return error;
}

// GetTrackerIcon
/*!	\brief Gets the icon which tracker displays for the node referred to by
		   the supplied entry_ref.

	This methods works similar to the non-static version. The first argument
	\a ref identifies the node in question.

	\param ref An entry_ref referring to the node for which the icon shall be
		   retrieved.
	\param icon A pointer to a pre-allocated BBitmap of the correct dimension
		   to store the requested icon (16x16 for the mini and 32x32 for the
		   large icon).
	\param k Specifies the size of the icon to be retrieved: \c B_MINI_ICON
		   for the mini and \c B_LARGE_ICON for the large icon.
	\return
	- \c B_OK: Everything went fine.
	- \c B_NO_INIT: The object is not properly initialized.
	- \c B_BAD_VALUE: \c NULL ref or \a icon, unsupported icon size \a k or
		 bitmap dimensions (\a icon) and icon size (\a k) do not match.
	- other error codes
*/
status_t 
BNodeInfo::GetTrackerIcon(const entry_ref *ref, BBitmap *icon, icon_size k)
{
	// check ref param
	status_t error = (ref ? B_OK : B_BAD_VALUE);
	// init a BNode
	BNode node;
	if (error == B_OK)
		error = node.SetTo(ref);
	// init a BNodeInfo
	BNodeInfo nodeInfo;
	if (error == B_OK)
		error = nodeInfo.SetTo(&node);
	// let the non-static GetTrackerIcon() do the dirty work
	if (error == B_OK)
		error = nodeInfo.GetTrackerIcon(icon, k);
	return error;
}

void 
BNodeInfo::_ReservedNodeInfo1()
{
}

void 
BNodeInfo::_ReservedNodeInfo2()
{
}

void 
BNodeInfo::_ReservedNodeInfo3()
{
}

// =
/*!	\brief Privatized assignment operator to prevent usage.
*/
BNodeInfo &
BNodeInfo::operator=(const BNodeInfo &nodeInfo)
{
	return *this;
}

// copy constructor
/*!	\brief Privatized copy constructor to prevent usage.
*/
BNodeInfo::BNodeInfo(const BNodeInfo &)
{
}

