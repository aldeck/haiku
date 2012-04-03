/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLinkDirectory.h"

#include <new>

#include <NodeMonitor.h>

#include <AutoDeleter.h>

#include "DebugSupport.h"
#include "PackageLinksListener.h"
#include "Utils.h"
#include "Version.h"
#include "Volume.h"


static const char* const kSelfLinkName = ".self";


PackageLinkDirectory::PackageLinkDirectory()
	:
	Directory(0),
		// the ID needs to be assigned later, when added to a volume
	fSelfLink(NULL)
{
	get_real_time(fModifiedTime);
}


PackageLinkDirectory::~PackageLinkDirectory()
{
	if (fSelfLink != NULL)
		fSelfLink->ReleaseReference();

	while (DependencyLink* link = fDependencyLinks.RemoveHead())
		link->ReleaseReference();
}


status_t
PackageLinkDirectory::Init(Directory* parent, Package* package)
{
	// compute the allocation size needed for the versioned name
	size_t nameLength = strlen(package->Name());
	size_t size = nameLength + 1;

	Version* version = package->Version();
	if (version != NULL) {
		size += 1 + version->ToString(NULL, 0);
			// + 1 for the '-'
	}

	// allocate the name and compose it
	char* name = (char*)malloc(size);
	if (name == NULL)
		return B_NO_MEMORY;

	memcpy(name, package->Name(), nameLength + 1);
	if (version != NULL) {
		name[nameLength] = '-';
		version->ToString(name + nameLength + 1, size - nameLength - 1);
	}

	// init the directory/node
	status_t error = Init(parent, name, NODE_FLAG_KEEP_NAME);
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the package
	AddPackage(package, NULL);

	return B_OK;
}


status_t
PackageLinkDirectory::Init(Directory* parent, const char* name, uint32 flags)
{
	return Directory::Init(parent, name, flags);
}


timespec
PackageLinkDirectory::ModifiedTime() const
{
	return fModifiedTime;
}

void
PackageLinkDirectory::AddPackage(Package* package,
	PackageLinksListener* listener)
{
	NodeWriteLocker writeLocker(this);

	// Find the insertion point in the list. We sort by mount type -- the more
	// specific the higher the priority.
	MountType mountType = package->Domain()->Volume()->MountType();
	Package* otherPackage = NULL;
	for (PackageList::Iterator it = fPackages.GetIterator();
			(otherPackage = it.Next()) != NULL;) {
		if (otherPackage->Domain()->Volume()->MountType() <= mountType)
			break;
	}

	fPackages.InsertBefore(otherPackage, package);
	package->SetLinkDirectory(this);

	if (package == fPackages.Head())
		_Update(listener);
}


void
PackageLinkDirectory::RemovePackage(Package* package,
	PackageLinksListener* listener)
{
	ASSERT(package->LinkDirectory() == this);

	NodeWriteLocker writeLocker(this);

	bool firstPackage = package == fPackages.Head();

	package->SetLinkDirectory(NULL);
	fPackages.Remove(package);

	if (firstPackage)
		_Update(listener);
}


void
PackageLinkDirectory::UpdatePackageDependencies(Package* package,
	PackageLinksListener* listener)
{
	ASSERT(package->LinkDirectory() == this);

	NodeWriteLocker writeLocker(this);

	// We only need to update, if that head package is affected.
	if (package != fPackages.Head())
		return;

	_UpdateDependencies(listener);
}


void
PackageLinkDirectory::NotifyDirectoryAdded(PackageLinksListener* listener)
{
	NodeWriteLocker writeLocker(this);

	listener->PackageLinkNodeAdded(this);

	if (fSelfLink != NULL) {
		NodeWriteLocker selfLinkLocker(fSelfLink);
		listener->PackageLinkNodeAdded(fSelfLink);
	}

	for (FamilyDependencyList::Iterator it = fDependencyLinks.GetIterator();
			DependencyLink* link = it.Next();) {
		NodeWriteLocker linkLocker(link);
		listener->PackageLinkNodeAdded(link);
	}
}


status_t
PackageLinkDirectory::_Update(PackageLinksListener* listener)
{
	// Always remove all dependency links -- if there's still a package, they
	// will be re-created below.
	while (DependencyLink* link = fDependencyLinks.RemoveHead()) {
		NodeWriteLocker linkLocker(link);
		if (listener != NULL)
			listener->PackageLinkNodeRemoved(link);

		RemoveChild(link);
		linkLocker.Unlock();
		link->ReleaseReference();
	}

	// check, if empty
	Package* package = fPackages.Head();
	if (package == NULL) {
		// remove self link, if any
		if (fSelfLink != NULL) {
			NodeWriteLocker selfLinkLocker(fSelfLink);
			if (listener != NULL)
				listener->PackageLinkNodeRemoved(fSelfLink);

			RemoveChild(fSelfLink);
			selfLinkLocker.Unlock();
			fSelfLink->ReleaseReference();
			fSelfLink = NULL;
		}

		return B_OK;
	}

	// create/update self link
	if (fSelfLink == NULL) {
		fSelfLink = new(std::nothrow) Link(package);
		if (fSelfLink == NULL)
			return B_NO_MEMORY;

		status_t error = fSelfLink->Init(this, kSelfLinkName,
			NODE_FLAG_CONST_NAME);
		if (error != B_OK)
			RETURN_ERROR(error);

		AddChild(fSelfLink);

		if (listener != NULL) {
			NodeWriteLocker selfLinkLocker(fSelfLink);
			listener->PackageLinkNodeAdded(fSelfLink);
		}
	} else {
		NodeWriteLocker selfLinkLocker(fSelfLink);
		fSelfLink->Update(package, listener);
	}

	// update the dependency links
	return _UpdateDependencies(listener);
}


status_t
PackageLinkDirectory::_UpdateDependencies(PackageLinksListener* listener)
{
	Package* package = fPackages.Head();
	if (package == NULL)
		return B_OK;

	// Iterate through the package's dependencies
	for (DependencyList::ConstIterator it
				= package->Dependencies().GetIterator();
			Dependency* dependency = it.Next();) {
		Resolvable* resolvable = dependency->Resolvable();
		Package* resolvablePackage = resolvable != NULL
			? resolvable->Package() : NULL;

		Node* node = FindChild(dependency->Name());
		if (node != NULL) {
			// link already exists -- update
			DependencyLink* link = static_cast<DependencyLink*>(node);

			NodeWriteLocker linkLocker(link);
			link->Update(resolvablePackage, listener);
		} else {
			// no link for the dependency yet -- create one
			DependencyLink* link = new(std::nothrow) DependencyLink(
				resolvablePackage);
			if (link == NULL)
				return B_NO_MEMORY;

			status_t error = link->Init(this, dependency->Name(), 0);
			if (error != B_OK) {
				delete link;
				RETURN_ERROR(error);
			}

			AddChild(link);
			fDependencyLinks.Add(link);

			if (listener != NULL) {
				NodeWriteLocker linkLocker(link);
				listener->PackageLinkNodeAdded(link);
			}
		}
	}

	return B_OK;
}
