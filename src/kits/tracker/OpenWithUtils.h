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

#ifndef _OPEN_WITH_UTILS_H
#define _OPEN_WITH_UTILS_H


#include "Attributes.h"
#include "EntryIterator.h"


class BMessage;


namespace BPrivate {


class Model;


enum {
	kUnknownRelation = -1,
	kNoRelation = 0,
	kSuperhandler,
	kSupportsSupertype,
	kSupportsType,
	kPreferredForType,
	kPreferredForFile
};


// pass in a predicate; a query will search for matches
// matches will be returned in iteration
class SearchForSignatureEntryList : public EntryListBase {
	public:
		SearchForSignatureEntryList(bool canAddAllApps);
		virtual ~SearchForSignatureEntryList();

		void PushUniqueSignature(const char *);
			// add one signature to search for

		// entry list iterators
		virtual status_t GetNextEntry(BEntry *entry, bool traverse = false);
		virtual status_t GetNextRef(entry_ref *ref);
		virtual int32 GetNextDirents(struct dirent *buffer, size_t length,
			int32 count = INT_MAX);

		virtual status_t Rewind();
		virtual int32 CountEntries();

		bool GetPreferredApp(entry_ref *ref) const;
			// gets the preferred app for all the files it was asked to
			// find supporting apps for, returns false if no preferred app
			// found or if more than one found
		void TrySettingPreferredApp(const entry_ref *);
		void TrySettingPreferredAppForFile(const entry_ref *);

		int32 Relation(const BMessage *entriesToOpen, const Model *) const;
			// returns the reason why an application is shown in Open With window
		void RelationDescription(const BMessage *entriesToOpen, const Model *,
			BString *) const;
			// returns a string describing why application handles files to open

		static int32 Relation(const BMessage *entriesToOpen,
			const Model *, const entry_ref *preferredApp,
			const entry_ref *preferredAppForFile);
			// returns the reason why an application is shown in Open With window
			// static version, needs the preferred app for preformance
		static void RelationDescription(const BMessage *entriesToOpen,
			const Model *, BString *, const entry_ref *preferredApp,
			const entry_ref *preferredAppForFile);
			// returns a string describing why application handles files to open

		bool CanOpenWithFilter(const Model *appModel, const BMessage *entriesToOpen,
			const entry_ref *preferredApp);

		void NonGenericFileFound();
		bool GenericFilesOnly() const;

		bool ShowAllApplications() const;

	private:
		static int32 Relation(const Model *node, const Model *app);
			// returns the reason why an application is shown in Open With window

		CachedEntryIteratorList *fIteratorList;
		BObjectList<BString> fSignatures;

		entry_ref fPreferredRef;
		int32 fPreferredAppCount;
		entry_ref fPreferredRefForFile;
		int32 fPreferredAppForFileCount;
		bool fGenericFilesOnly;
		bool fCanAddAllApps;
		bool fFoundOneNonSuperHandler;
};


class RelationCachingModelProxy {
	public:
		RelationCachingModelProxy(Model *model);
		~RelationCachingModelProxy();

		int32 Relation(SearchForSignatureEntryList *iterator, BMessage *entries) const;

		Model *fModel;
		mutable int32 fRelation;
};


// used for optionally showing the list of all apps. Do nothing
// until asked to iterate and only if supposed to do so
class ConditionalAllAppsIterator : public EntryListBase {
	public:
		ConditionalAllAppsIterator(SearchForSignatureEntryList *parent);
		~ConditionalAllAppsIterator();

		virtual status_t GetNextEntry(BEntry *entry, bool traverse = false);
		virtual status_t GetNextRef(entry_ref *ref);
		virtual int32 GetNextDirents(struct dirent *buffer, size_t length,
			int32 count = INT_MAX);

		virtual status_t Rewind();
		virtual int32 CountEntries();

	protected:
		bool Iterate() const;
		void Instantiate();

	private:
		SearchForSignatureEntryList *fParent;
		BTrackerPrivate::TWalker *fWalker;
};


class OpenWithUtils {
public:
	static const entry_ref* 	AddOneRefSignatures(
									const entry_ref* ref,
									void* castToIterator);
	static void					AddSupportingAppForTypeToQuery(
									SearchForSignatureEntryList* queryIterator,
									const char* type);	
	static const BString*		FindOne(const BString* element,
									void* castToString);
	static const BString*		AddOnePredicateTerm(const BString* item,
									void* castToParams);
									
	struct 						AddOneTermParams {
									BString *result;
									bool first;
								};

};


} // namespace BPrivate


using namespace BPrivate;


#endif	// _OPEN_WITH_UTILS_H
