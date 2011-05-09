/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_ROSTER_H_
#define _PACKAGE__PACKAGE_ROSTER_H_


#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>


template <class T> class BObjectList;


namespace BPackageKit {


struct BRepositoryConfigVisitor {
	virtual ~BRepositoryConfigVisitor()
	{
	}

	virtual status_t operator()(const BEntry& entry) = 0;
};


class BRepositoryCache;
class BRepositoryConfig;


class BPackageRoster {
public:
								BPackageRoster();
								~BPackageRoster();

			status_t			GetCommonRepositoryCachePath(BPath* path,
									bool create = false) const;
			status_t			GetUserRepositoryCachePath(BPath* path,
									bool create = false) const;

			status_t			GetCommonRepositoryConfigPath(BPath* path,
									bool create = false) const;
			status_t			GetUserRepositoryConfigPath(BPath* path,
									bool create = false) const;

			status_t			GetRepositoryNames(BObjectList<BString>& names);

			status_t			VisitCommonRepositoryConfigs(
									BRepositoryConfigVisitor& visitor);
			status_t			VisitUserRepositoryConfigs(
									BRepositoryConfigVisitor& visitor);

			status_t			GetRepositoryCache(const BString& name,
									BRepositoryCache* repositoryCache);
			status_t			GetRepositoryConfig(const BString& name,
									BRepositoryConfig* repositoryConfig);
private:
			status_t			_GetRepositoryPath(BPath* path, bool create,
									directory_which whichDir) const;
			status_t			_VisitRepositoryConfigs(const BPath& path,
									BRepositoryConfigVisitor& visitor);

};


}	// namespace BPackageKit


#endif // _PACKAGE__PACKAGE_ROSTER_H_
