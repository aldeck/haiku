/*
** Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

#include <wchar_private.h>


int
__wcsncmp(const wchar_t* wcs1, const wchar_t* wcs2, size_t count)
{
	int cmp = 0;

	while (count-- > 0) {
		cmp = *wcs1 - *wcs2++;
			/* note: won't overflow, since our wchar_t is guaranteed to never
			   have the highest bit set */

		if (cmp != 0 || *wcs1++ == L'\0')
			break;
	}

	return cmp;
}


B_DEFINE_WEAK_ALIAS(__wcsncmp, wcsncmp);
