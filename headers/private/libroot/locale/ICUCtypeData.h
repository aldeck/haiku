/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_CTYPE_DATA_H
#define _ICU_CTYPE_DATA_H


#include "ICUCategoryData.h"
#include "LocaleBackend.h"


namespace BPrivate {
namespace Libroot {


class ICUCtypeData : public ICUCategoryData {
	typedef	ICUCategoryData		inherited;
public:
								ICUCtypeData();
	virtual 					~ICUCtypeData();

			void				Initialize(LocaleCtypeDataBridge* dataBridge);

	virtual	status_t			SetTo(const Locale& locale,
									const char* posixLocaleName);
	virtual	status_t			SetToPosix();

			int					IsWCType(wint_t wc, wctype_t charClass);
			status_t			ToWCTrans(wint_t wc, wctrans_t transition,
									wint_t& result);

			const char*			GetLanginfo(int index);

private:
			/*
			 * The following arrays have 384 elements where the elements at
			 * index -128..-2 mirror the elements at index 128..255 (to protect
			 * against invocations of ctype macros with negative character
			 * values).
			 * The element at index -1 is a dummy element containing the
			 * neutral/identity value used when the array is accessed as in
			 * 'isblank(EOF)' (i.e. with index -1).
			 */
			unsigned short		fClassInfo[384];
			int					fToLowerMap[384];
			int					fToUpperMap[384];

			LocaleCtypeDataBridge*	fDataBridge;
};


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _ICU_CTYPE_DATA_H
