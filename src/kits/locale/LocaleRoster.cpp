/*
 * Copyright 2003-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <LocaleRoster.h>

#include <ctype.h>
#include <set>

#include <assert.h>
#include <syslog.h>

#include <Autolock.h>
#include <AppFileInfo.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Collator.h>
#include <DefaultCatalog.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FormattingConventions.h>
#include <IconUtils.h>
#include <Language.h>
#include <Locale.h>
#include <MutableLocaleRoster.h>
#include <Node.h>
#include <Path.h>
#include <String.h>
#include <TimeZone.h>

#include <ICUWrapper.h>

// ICU includes
#include <unicode/locdspnm.h>
#include <unicode/locid.h>
#include <unicode/timezone.h>


using BPrivate::CatalogAddOnInfo;


/*
 * several attributes/resource-IDs used within the Locale Kit:
 */
const char* BLocaleRoster::kCatLangAttr = "BEOS:LOCALE_LANGUAGE";
	// name of catalog language, lives in every catalog file
const char* BLocaleRoster::kCatSigAttr = "BEOS:LOCALE_SIGNATURE";
	// catalog signature, lives in every catalog file
const char* BLocaleRoster::kCatFingerprintAttr = "BEOS:LOCALE_FINGERPRINT";
	// catalog fingerprint, may live in catalog file

const char* BLocaleRoster::kEmbeddedCatAttr = "BEOS:LOCALE_EMBEDDED_CATALOG";
	// attribute which contains flattened data of embedded catalog
	// this may live in an app- or add-on-file
int32 BLocaleRoster::kEmbeddedCatResId = 0xCADA;
	// a unique value used to identify the resource (=> embedded CAtalog DAta)
	// which contains flattened data of embedded catalog.
	// this may live in an app- or add-on-file


BLocaleRoster::BLocaleRoster()
{
}


BLocaleRoster::~BLocaleRoster()
{
}


/*static*/ BLocaleRoster*
BLocaleRoster::Default()
{
	return MutableLocaleRoster::Default();
}


status_t
BLocaleRoster::Refresh()
{
	return RosterData::Default()->Refresh();
}


status_t
BLocaleRoster::GetDefaultTimeZone(BTimeZone* timezone) const
{
	if (!timezone)
		return B_BAD_VALUE;

	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*timezone = rosterData->fDefaultTimeZone;

	return B_OK;
}


status_t
BLocaleRoster::GetLanguage(const char* languageCode,
	BLanguage** _language) const
{
	if (_language == NULL || languageCode == NULL || languageCode[0] == '\0')
		return B_BAD_VALUE;

	BLanguage* language = new(std::nothrow) BLanguage(languageCode);
	if (language == NULL)
		return B_NO_MEMORY;

	*_language = language;
	return B_OK;
}


status_t
BLocaleRoster::GetPreferredLanguages(BMessage* languages) const
{
	if (!languages)
		return B_BAD_VALUE;

	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	*languages = rosterData->fPreferredLanguages;

	return B_OK;
}


/**
 * \brief Fills \c message with 'language'-fields containing the language-
 * ID(s) of all available languages.
 */
status_t
BLocaleRoster::GetAvailableLanguages(BMessage* languages) const
{
	if (!languages)
		return B_BAD_VALUE;

	int32_t localeCount;
	const Locale* icuLocaleList = Locale::getAvailableLocales(localeCount);

	for (int i = 0; i < localeCount; i++)
		languages->AddString("language", icuLocaleList[i].getName());

	return B_OK;
}


status_t
BLocaleRoster::GetAvailableCountries(BMessage* countries) const
{
	if (!countries)
		return B_BAD_VALUE;

	int32 i;
	const char* const* countryList = uloc_getISOCountries();

	for (i = 0; countryList[i] != NULL; i++)
		countries->AddString("country", countryList[i]);

	return B_OK;
}


status_t
BLocaleRoster::GetAvailableTimeZones(BMessage* timeZones) const
{
	if (!timeZones)
		return B_BAD_VALUE;

	status_t status = B_OK;

	StringEnumeration* zoneList = TimeZone::createEnumeration();

	UErrorCode icuStatus = U_ZERO_ERROR;
	int32 count = zoneList->count(icuStatus);
	if (U_SUCCESS(icuStatus)) {
		for (int i = 0; i < count; ++i) {
			const char* zoneID = zoneList->next(NULL, icuStatus);
			if (zoneID == NULL || !U_SUCCESS(icuStatus)) {
				status = B_ERROR;
				break;
			}
 			timeZones->AddString("timeZone", zoneID);
		}
	} else
		status = B_ERROR;

	delete zoneList;

	return status;
}


status_t
BLocaleRoster::GetAvailableTimeZonesForCountry(BMessage* timeZones,
	const char* countryCode) const
{
	if (!timeZones)
		return B_BAD_VALUE;

	status_t status = B_OK;

	StringEnumeration* zoneList = TimeZone::createEnumeration(countryCode);
		// countryCode == NULL will yield all timezones not bound to a country

	UErrorCode icuStatus = U_ZERO_ERROR;
	int32 count = zoneList->count(icuStatus);
	if (U_SUCCESS(icuStatus)) {
		for (int i = 0; i < count; ++i) {
			const char* zoneID = zoneList->next(NULL, icuStatus);
			if (zoneID == NULL || !U_SUCCESS(icuStatus)) {
				status = B_ERROR;
				break;
			}
			timeZones->AddString("timeZone", zoneID);
		}
	} else
		status = B_ERROR;

	delete zoneList;

	return status;
}


status_t
BLocaleRoster::GetFlagIconForCountry(BBitmap* flagIcon, const char* countryCode)
{
	if (countryCode == NULL)
		return B_BAD_DATA;

	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	if (!rosterData->fAreResourcesLoaded) {
		status_t result = rosterData->fResources.SetToImage(rosterData);
		if (result != B_OK)
			return result;

		result = rosterData->fResources.PreloadResourceType();
		if (result != B_OK)
			return result;

		rosterData->fAreResourcesLoaded = true;
	}

	size_t size;

	// normalize the country code : 2 letters uparcase
	// filter things out so that "pt_BR" gived the flag for brazil
	char normalizedCode[3];
	normalizedCode[2] = '\0';

	int codeLength = strlen(countryCode);

	normalizedCode[0] = toupper(countryCode[codeLength - 2]);
	normalizedCode[1] = toupper(countryCode[codeLength - 1]);

	const void* buffer = rosterData->fResources.LoadResource(
		B_VECTOR_ICON_TYPE, normalizedCode, &size);
	if (buffer == NULL || size == 0)
		return B_NAME_NOT_FOUND;

	return BIconUtils::GetVectorIcon(static_cast<const uint8*>(buffer), size,
		flagIcon);
}


status_t
BLocaleRoster::GetAvailableCatalogs(BMessage*  languageList,
	const char* sigPattern,	const char* langPattern, int32 fingerprint) const
{
	if (languageList == NULL)
		return B_BAD_VALUE;

	RosterData* rosterData = RosterData::Default();
	BAutolock lock(rosterData->fLock);
	if (!lock.IsLocked())
		return B_ERROR;

	int32 count = rosterData->fCatalogAddOnInfos.CountItems();
	for (int32 i = 0; i < count; ++i) {
		CatalogAddOnInfo* info
			= (CatalogAddOnInfo*)rosterData->fCatalogAddOnInfos.ItemAt(i);

		if (!info->MakeSureItsLoaded() || !info->fLanguagesFunc)
			continue;

		info->fLanguagesFunc(languageList, sigPattern, langPattern,
			fingerprint);
	}

	return B_OK;
}


BCatalog*
BLocaleRoster::_GetCatalog(BCatalog* catalog, vint32* catalogInitStatus)
{
	// This function is used in the translation macros, so it can't return a
	// status_t. Maybe it could throw exceptions ?

	if (*catalogInitStatus == true) {
		// Catalog already loaded - nothing else to do
		return catalog;
	}

	// figure out image (shared object) from catalog address
	image_info info;
	int32 cookie = 0;
	bool found = false;

	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		if ((char*)info.data < (char*)catalog && (char*)info.data
				+ info.data_size > (char*)catalog) {
			found = true;
			break;
		}
	}

	if (!found) {
		log_team(LOG_DEBUG, "Catalog %x doesn't belong to any image !",
			catalog);
		return catalog;
	}
	// figure out mimetype from image
	BFile objectFile(info.name, B_READ_ONLY);
	BAppFileInfo objectInfo(&objectFile);
	char objectSignature[B_MIME_TYPE_LENGTH];
	if (objectInfo.GetSignature(objectSignature) != B_OK) {
		log_team(LOG_ERR, "File %s has no mimesignature, so it can't use"
			"localization.", info.name);
		return catalog;
	}

	// drop supertype from mimetype (should be "application/"):
	char* stripSignature = objectSignature;
	while (*stripSignature != '/')
		stripSignature ++;
	stripSignature ++;

	log_team(LOG_DEBUG,
		"Image %s (address %x) requested catalog with mimetype %s",
		info.name, catalog, stripSignature);

	// load the catalog for this mimetype and return it to the app
	catalog->SetCatalog(stripSignature, 0);
	*catalogInitStatus = true;

	return catalog;
}
