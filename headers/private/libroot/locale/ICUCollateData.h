/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_COLLATE_DATA_H
#define _ICU_COLLATE_DATA_H


#include "ICUCategoryData.h"

#include <unicode/coll.h>


namespace BPrivate {
namespace Libroot {


class ICUCollateData : public ICUCategoryData {
	typedef	ICUCategoryData		inherited;

public:
								ICUCollateData();
	virtual						~ICUCollateData();

	virtual	status_t			SetTo(const Locale& locale,
										const char* posixLocaleName);
	virtual	status_t			SetToPosix();

			status_t			Strcoll(const char* a, const char* b, int& out);
			status_t			Strxfrm(char* out, const char* in, size_t size,
									size_t& outSize);

private:
			status_t			_ToUnicodeString(const char* in,
									UnicodeString& out);

			Collator*			fCollator;
};


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _ICU_COLLATE_DATA_H
