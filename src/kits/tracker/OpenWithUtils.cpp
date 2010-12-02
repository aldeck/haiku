/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/


#include "OpenWithUtils.h"

#include "FSUtils.h"
#include "MimeTypes.h"
#include "Model.h"
#include "Tracker.h"

#include <Catalog.h>
#include <Locale.h>
#include <Roster.h>


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "OpenWithUtils"


//	#pragma mark - OpenWithUtils


void
OpenWithUtils::AddSupportingAppForTypeToQuery(
	SearchForSignatureEntryList* queryIterator,
	const char* type)
{
	// get supporting apps for type
	BMimeType mime(type);
	if (!mime.IsInstalled())
		return;

	BMessage message;
	mime.GetSupportingApps(&message);

	// push each of the supporting apps signature uniquely

	const char *signature;
	for (int32 index = 0; message.FindString("applications", index,
			&signature) == B_OK; index++) {
		queryIterator->PushUniqueSignature(signature);
	}
}


const entry_ref*
OpenWithUtils::AddOneRefSignatures(const entry_ref* ref, void* castToIterator)
{
	// TODO: resolve cases where each entry has a different type and
	// their supporting apps are disjoint sets

	SearchForSignatureEntryList *queryIterator =
		(SearchForSignatureEntryList *)castToIterator;

	Model model(ref, true, true);
	if (model.InitCheck() != B_OK)
		return NULL;

	BString mimeType(model.MimeType());

	if (!mimeType.Length() || mimeType.ICompare(B_FILE_MIMETYPE) == 0)
		// if model is of unknown type, try mimeseting it first
		model.Mimeset(true);

	bool preferredAppFromNode = false;
	entry_ref preferredRef;

	// add preferred app for file, if any
	if (model.PreferredAppSignature()[0]) {
		// got one, mark it as preferred for this node
		if (be_roster->FindApp(model.PreferredAppSignature(), &preferredRef) == B_OK) {
			queryIterator->PushUniqueSignature(model.PreferredAppSignature());
			preferredAppFromNode = true;
			queryIterator->TrySettingPreferredAppForFile(&preferredRef);
		}
	}

	mimeType = model.MimeType();
	mimeType.ToLower();

	if (mimeType.Length() && !mimeType.ICompare(B_FILE_MIMETYPE) == 0)
		queryIterator->NonGenericFileFound();

	// get supporting apps for type
	AddSupportingAppForTypeToQuery(queryIterator, mimeType.String());

	// find the preferred app for this type
	if (be_roster->FindApp(mimeType.String(), &preferredRef) == B_OK)
		queryIterator->TrySettingPreferredApp(&preferredRef);

	return NULL;
}


const BString*
OpenWithUtils::FindOne(const BString *element, void *castToString)
{
	if (strcasecmp(element->String(), (const char *)castToString) == 0)
		return element;

	return 0;
}


const BString*
OpenWithUtils::AddOnePredicateTerm(const BString* item, void* castToParams)
{
	AddOneTermParams* params = (AddOneTermParams*)castToParams;
	if (!params->first)
		(*params->result) << " || ";
	(*params->result) << kAttrAppSignature << " = " << item->String();

	params->first = false;

	return 0;
}


//	#pragma mark - SearchForSignatureEntryList


SearchForSignatureEntryList::SearchForSignatureEntryList(bool canAddAllApps)
	:
	fIteratorList(NULL),
	fSignatures(20, true),
	fPreferredAppCount(0),
	fPreferredAppForFileCount(0),
	fGenericFilesOnly(true),
	fCanAddAllApps(canAddAllApps),
	fFoundOneNonSuperHandler(false)
{
}


SearchForSignatureEntryList::~SearchForSignatureEntryList()
{
	delete fIteratorList;
}


void
SearchForSignatureEntryList::PushUniqueSignature(const char *str)
{
	// do a unique add
	if (fSignatures.EachElement(OpenWithUtils::FindOne, (void *)str))
		return;

	fSignatures.AddItem(new BString(str));
}


status_t
SearchForSignatureEntryList::GetNextEntry(BEntry *entry, bool)
{
	return fIteratorList->GetNextEntry(entry);
}


status_t
SearchForSignatureEntryList::GetNextRef(entry_ref *ref)
{
	return fIteratorList->GetNextRef(ref);
}


int32
SearchForSignatureEntryList::GetNextDirents(struct dirent *buffer,
	size_t length, int32 count)
{
	return fIteratorList->GetNextDirents(buffer, length, count);
}


status_t
SearchForSignatureEntryList::Rewind()
{
	if (fIteratorList)
		return fIteratorList->Rewind();

	if (!fSignatures.CountItems())
		return ENOENT;

	// build up the iterator
	fIteratorList = new CachedEntryIteratorList(false);
		// We cannot sort the cached inodes, as CanOpenWithFilter() relies
		// on the fact that ConditionalAllAppsIterator results come last.

	// build the predicate string by oring queries for the individual
	// signatures
	BString predicateString;

	OpenWithUtils::AddOneTermParams params;
	params.result = &predicateString;
	params.first = true;

	fSignatures.EachElement(OpenWithUtils::AddOnePredicateTerm, &params);

	ASSERT(predicateString.Length());
// 	PRINT(("query predicate %s\n", predicateString.String()));
	fIteratorList->AddItem(new TWalkerWrapper(
		new BTrackerPrivate::TQueryWalker(predicateString.String())));
	fIteratorList->AddItem(new ConditionalAllAppsIterator(this));

	return fIteratorList->Rewind();
}


int32
SearchForSignatureEntryList::CountEntries()
{
	return 0;
}


bool
SearchForSignatureEntryList::GetPreferredApp(entry_ref *ref) const
{
	if (fPreferredAppCount == 1)
		*ref = fPreferredRef;

	return fPreferredAppCount == 1;
}


void
SearchForSignatureEntryList::TrySettingPreferredApp(const entry_ref *ref)
{
	if (!fPreferredAppCount) {
		fPreferredRef = *ref;
		fPreferredAppCount++;
	} else if (fPreferredRef != *ref)
		// if more than one, will not return any
		fPreferredAppCount++;
}


void
SearchForSignatureEntryList::TrySettingPreferredAppForFile(const entry_ref *ref)
{
	if (!fPreferredAppForFileCount) {
		fPreferredRefForFile = *ref;
		fPreferredAppForFileCount++;
	} else if (fPreferredRefForFile != *ref) {
		// if more than one, will not return any
		fPreferredAppForFileCount++;
	}
}


void
SearchForSignatureEntryList::NonGenericFileFound()
{
	fGenericFilesOnly = false;
}


bool
SearchForSignatureEntryList::GenericFilesOnly() const
{
	return fGenericFilesOnly;
}


bool
SearchForSignatureEntryList::ShowAllApplications() const
{
	return fCanAddAllApps && !fFoundOneNonSuperHandler;
}


int32
SearchForSignatureEntryList::Relation(const Model *nodeModel,
	const Model *applicationModel)
{
 	switch (applicationModel->SupportsMimeType(nodeModel->MimeType(), 0, true)) {
		case kDoesNotSupportType:
			return kNoRelation;

		case kSuperhandlerModel:
			return kSuperhandler;

		case kModelSupportsSupertype:
			return kSupportsSupertype;

		case kModelSupportsType:
			return kSupportsType;
	}

	TRESPASS();
	return kNoRelation;
}


int32
SearchForSignatureEntryList::Relation(const BMessage *entriesToOpen,
	const Model *model) const
{
	return Relation(entriesToOpen, model,
		fPreferredAppCount == 1 ? &fPreferredRef : 0,
		fPreferredAppForFileCount == 1 ? &fPreferredRefForFile : 0);
}


void
SearchForSignatureEntryList::RelationDescription(const BMessage *entriesToOpen,
	const Model *model, BString *description) const
{
	RelationDescription(entriesToOpen, model, description,
		fPreferredAppCount == 1 ? &fPreferredRef : 0,
		fPreferredAppForFileCount == 1 ? &fPreferredRefForFile : 0);
}


int32
SearchForSignatureEntryList::Relation(const BMessage *entriesToOpen,
	const Model *applicationModel, const entry_ref *preferredApp,
	const entry_ref *preferredAppForFile)
{
	for (int32 index = 0; ; index++) {
		entry_ref ref;
		if (entriesToOpen->FindRef("refs", index, &ref) != B_OK)
			break;

		// need to init a model so that typeless folders etc. will still appear to
		// have a mime type

		Model model(&ref, true, true);
		if (model.InitCheck())
			continue;

		int32 result = Relation(&model, applicationModel);
		if (result != kNoRelation) {
			if (preferredAppForFile
				&& *applicationModel->EntryRef() == *preferredAppForFile)
				return kPreferredForFile;

			if (result == kSupportsType && preferredApp
				&& *applicationModel->EntryRef() == *preferredApp)
				// application matches cached preferred app, we are done
				return kPreferredForType;

			return result;
		}
	}

	return kNoRelation;
}


void
SearchForSignatureEntryList::RelationDescription(const BMessage *entriesToOpen,
	const Model *applicationModel, BString *description, const entry_ref *preferredApp,
	const entry_ref *preferredAppForFile)
{
	for (int32 index = 0; ;index++) {
		entry_ref ref;
		if (entriesToOpen->FindRef("refs", index, &ref) != B_OK)
			break;

		if (preferredAppForFile && ref == *preferredAppForFile) {
			description->SetTo(B_TRANSLATE("Preferred for file"));
			return;
		}

		Model model(&ref, true, true);
		if (model.InitCheck())
			continue;

		BMimeType mimeType;
		int32 result = Relation(&model, applicationModel);
		switch (result) {
			case kDoesNotSupportType:
				continue;

			case kSuperhandler:
				description->SetTo(B_TRANSLATE("Handles any file"));
				return;

			case kSupportsSupertype:
			{
				mimeType.SetTo(model.MimeType());
				// status_t result = mimeType.GetSupertype(&mimeType);

				char *type = (char *)mimeType.Type();
				char *tmp = strchr(type, '/');
				if (tmp)
					*tmp = '\0';

				//PRINT(("getting supertype for %s, result %s, got %s\n",
				//	model.MimeType(), strerror(result), mimeType.Type()));
				description->SetTo(B_TRANSLATE("Handles any %type"));
				// *description += mimeType.Type();
				description->ReplaceFirst("%type", type);
				return;
			}

			case kSupportsType:
			{
				mimeType.SetTo(model.MimeType());

				if (preferredApp
					&& *applicationModel->EntryRef() == *preferredApp) {
					// application matches cached preferred app, we are done
					description->SetTo(B_TRANSLATE("Preferred for %type"));
				} else
					description->SetTo(B_TRANSLATE("Handles %type"));

				char shortDescription[256];
				if (mimeType.GetShortDescription(shortDescription) == B_OK)
					description->ReplaceFirst("%type", shortDescription);
				else
					description->ReplaceFirst("%type", mimeType.Type());
				return;
			}
		}
	}

	description->SetTo(B_TRANSLATE("Does not handle file"));
}


bool
SearchForSignatureEntryList::CanOpenWithFilter(const Model *appModel,
	const BMessage *entriesToOpen, const entry_ref *preferredApp)
{
	if (!appModel->IsExecutable() || !appModel->Node()) {
		// weed out non-executable
#if xDEBUG
		BPath path;
		BEntry entry(appModel->EntryRef());
		entry.GetPath(&path);
		PRINT(("filtering out %s- not executable \n", path.Path()));
#endif
		return false;
	}

	if (strcasecmp(appModel->MimeType(), B_APP_MIME_TYPE) != 0) {
		// filter out pe containers on PPC etc.
		return false;
	}

	ASSERT(dynamic_cast<BFile *>(appModel->Node()));
	char signature[B_MIME_TYPE_LENGTH];
	status_t result = GetAppSignatureFromAttr(
		dynamic_cast<BFile *>(appModel->Node()), signature);

	if (result == B_OK && strcasecmp(signature, kTrackerSignature) == 0) {
		// special case the Tracker - make sure only the running copy is
		// in the list
		app_info trackerInfo;
		result = be_roster->GetActiveAppInfo(&trackerInfo);
		if (*appModel->EntryRef() != trackerInfo.ref) {
			// this is an inactive copy of the Tracker, remove it

#if xDEBUG
			BPath path, path2;
			BEntry entry(appModel->EntryRef());
			entry.GetPath(&path);

			BEntry entry2(&trackerInfo.ref);
			entry2.GetPath(&path2);

			PRINT(("filtering out %s, sig %s, active Tracker at %s, result %s, refName %s\n",
				path.Path(), signature, path2.Path(), strerror(result),
				trackerInfo.ref.name));
#endif
			return false;
		}
	}

	if (FSInTrashDir(appModel->EntryRef()))
		return false;

	if (ShowAllApplications()) {
		// don't check for these if we didn't look for every single app
		// to not slow filtering down
		uint32 flags;
		BAppFileInfo appFileInfo(dynamic_cast<BFile *>(appModel->Node()));
		if (appFileInfo.GetAppFlags(&flags) != B_OK)
			return false;

		if ((flags & B_BACKGROUND_APP) || (flags & B_ARGV_ONLY))
			return false;

		if (!signature[0])
			// weed out apps with empty signatures
			return false;
	}

	int32 relation = Relation(entriesToOpen, appModel, preferredApp, 0);
	if (relation == kNoRelation && !ShowAllApplications()) {
#if xDEBUG
		BPath path;
		BEntry entry(appModel->EntryRef());
		entry.GetPath(&path);

		PRINT(("filtering out %s, does not handle any of opened files\n",
			path.Path()));
#endif
		return false;
	}

	if (relation != kNoRelation && relation != kSuperhandler && !fGenericFilesOnly) {
		// we hit at least one app that is not a superhandler and
		// handles the document
		fFoundOneNonSuperHandler = true;
	}

	return true;
}


//	#pragma mark - ConditionalAllAppsIterator


ConditionalAllAppsIterator::ConditionalAllAppsIterator(
		SearchForSignatureEntryList *parent)
	:
	fParent(parent),
	fWalker(NULL)
{
}


void
ConditionalAllAppsIterator::Instantiate()
{
	if (fWalker)
		return;

	BString lookForAppsPredicate;
	lookForAppsPredicate << "(" << kAttrAppSignature << " = \"*\" ) && ( "
		<< kAttrMIMEType << " = " << B_APP_MIME_TYPE << " ) ";
	fWalker = new BTrackerPrivate::TQueryWalker(lookForAppsPredicate.String());
}


ConditionalAllAppsIterator::~ConditionalAllAppsIterator()
{
	delete fWalker;
}


status_t
ConditionalAllAppsIterator::GetNextEntry(BEntry *entry, bool traverse)
{
	if (!Iterate())
		return B_ENTRY_NOT_FOUND;

	Instantiate();
	return fWalker->GetNextEntry(entry, traverse);
}


status_t
ConditionalAllAppsIterator::GetNextRef(entry_ref *ref)
{
	if (!Iterate())
		return B_ENTRY_NOT_FOUND;

	Instantiate();
	return fWalker->GetNextRef(ref);
}


int32
ConditionalAllAppsIterator::GetNextDirents(struct dirent *buffer, size_t length,
	int32 count)
{
	if (!Iterate())
		return 0;

	Instantiate();
	return fWalker->GetNextDirents(buffer, length, count);
}


status_t
ConditionalAllAppsIterator::Rewind()
{
	if (!Iterate())
		return B_OK;

	Instantiate();
	return fWalker->Rewind();
}


int32
ConditionalAllAppsIterator::CountEntries()
{
	if (!Iterate())
		return 0;

	Instantiate();
	return fWalker->CountEntries();
}


bool
ConditionalAllAppsIterator::Iterate() const
{
	return fParent->ShowAllApplications();
}

//	#pragma mark - RelationCachingModelProxy



RelationCachingModelProxy::RelationCachingModelProxy(Model *model)
	:
	fModel(model),
	fRelation(kUnknownRelation)
{
}


RelationCachingModelProxy::~RelationCachingModelProxy()
{
	delete fModel;
}


int32
RelationCachingModelProxy::Relation(SearchForSignatureEntryList *iterator,
	BMessage *entries) const
{
	if (fRelation == kUnknownRelation)
		fRelation = iterator->Relation(entries, fModel);

	return fRelation;
}
