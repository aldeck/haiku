/*
 * Copyright 2001-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 *		Michael Lotz <mmlr@mlotz.ch>
 */

/**	This file contains colorspace conversion functions
 *	and a palette <-> true color conversion class.
 */

#include "ColorConversion.h"
#include <Locker.h>
#include <new.h>
#include <Point.h>

namespace BPrivate {

// TODO: system palette -- hard-coded for now, when the app server is ready
// we should use system_colors() or BScreen::ColorMap().
const rgb_color kSystemPalette[] = {
 {   0,   0,   0, 255 }, {   8,   8,   8, 255 }, {  16,  16,  16, 255 },
 {  24,  24,  24, 255 }, {  32,  32,  32, 255 }, {  40,  40,  40, 255 },
 {  48,  48,  48, 255 }, {  56,  56,  56, 255 }, {  64,  64,  64, 255 },
 {  72,  72,  72, 255 }, {  80,  80,  80, 255 }, {  88,  88,  88, 255 },
 {  96,  96,  96, 255 }, { 104, 104, 104, 255 }, { 112, 112, 112, 255 },
 { 120, 120, 120, 255 }, { 128, 128, 128, 255 }, { 136, 136, 136, 255 },
 { 144, 144, 144, 255 }, { 152, 152, 152, 255 }, { 160, 160, 160, 255 },
 { 168, 168, 168, 255 }, { 176, 176, 176, 255 }, { 184, 184, 184, 255 },
 { 192, 192, 192, 255 }, { 200, 200, 200, 255 }, { 208, 208, 208, 255 },
 { 216, 216, 216, 255 }, { 224, 224, 224, 255 }, { 232, 232, 232, 255 },
 { 240, 240, 240, 255 }, { 248, 248, 248, 255 }, {   0,   0, 255, 255 },
 {   0,   0, 229, 255 }, {   0,   0, 204, 255 }, {   0,   0, 179, 255 },
 {   0,   0, 154, 255 }, {   0,   0, 129, 255 }, {   0,   0, 105, 255 },
 {   0,   0,  80, 255 }, {   0,   0,  55, 255 }, {   0,   0,  30, 255 },
 { 255,   0,   0, 255 }, { 228,   0,   0, 255 }, { 203,   0,   0, 255 },
 { 178,   0,   0, 255 }, { 153,   0,   0, 255 }, { 128,   0,   0, 255 },
 { 105,   0,   0, 255 }, {  80,   0,   0, 255 }, {  55,   0,   0, 255 },
 {  30,   0,   0, 255 }, {   0, 255,   0, 255 }, {   0, 228,   0, 255 },
 {   0, 203,   0, 255 }, {   0, 178,   0, 255 }, {   0, 153,   0, 255 },
 {   0, 128,   0, 255 }, {   0, 105,   0, 255 }, {   0,  80,   0, 255 },
 {   0,  55,   0, 255 }, {   0,  30,   0, 255 }, {   0, 152,  51, 255 },
 { 255, 255, 255, 255 }, { 203, 255, 255, 255 }, { 203, 255, 203, 255 },
 { 203, 255, 152, 255 }, { 203, 255, 102, 255 }, { 203, 255,  51, 255 },
 { 203, 255,   0, 255 }, { 152, 255, 255, 255 }, { 152, 255, 203, 255 },
 { 152, 255, 152, 255 }, { 152, 255, 102, 255 }, { 152, 255,  51, 255 },
 { 152, 255,   0, 255 }, { 102, 255, 255, 255 }, { 102, 255, 203, 255 },
 { 102, 255, 152, 255 }, { 102, 255, 102, 255 }, { 102, 255,  51, 255 },
 { 102, 255,   0, 255 }, {  51, 255, 255, 255 }, {  51, 255, 203, 255 },
 {  51, 255, 152, 255 }, {  51, 255, 102, 255 }, {  51, 255,  51, 255 },
 {  51, 255,   0, 255 }, { 255, 152, 255, 255 }, { 255, 152, 203, 255 },
 { 255, 152, 152, 255 }, { 255, 152, 102, 255 }, { 255, 152,  51, 255 },
 { 255, 152,   0, 255 }, {   0, 102, 255, 255 }, {   0, 102, 203, 255 },
 { 203, 203, 255, 255 }, { 203, 203, 203, 255 }, { 203, 203, 152, 255 },
 { 203, 203, 102, 255 }, { 203, 203,  51, 255 }, { 203, 203,   0, 255 },
 { 152, 203, 255, 255 }, { 152, 203, 203, 255 }, { 152, 203, 152, 255 },
 { 152, 203, 102, 255 }, { 152, 203,  51, 255 }, { 152, 203,   0, 255 },
 { 102, 203, 255, 255 }, { 102, 203, 203, 255 }, { 102, 203, 152, 255 },
 { 102, 203, 102, 255 }, { 102, 203,  51, 255 }, { 102, 203,   0, 255 },
 {  51, 203, 255, 255 }, {  51, 203, 203, 255 }, {  51, 203, 152, 255 },
 {  51, 203, 102, 255 }, {  51, 203,  51, 255 }, {  51, 203,   0, 255 },
 { 255, 102, 255, 255 }, { 255, 102, 203, 255 }, { 255, 102, 152, 255 },
 { 255, 102, 102, 255 }, { 255, 102,  51, 255 }, { 255, 102,   0, 255 },
 {   0, 102, 152, 255 }, {   0, 102, 102, 255 }, { 203, 152, 255, 255 },
 { 203, 152, 203, 255 }, { 203, 152, 152, 255 }, { 203, 152, 102, 255 },
 { 203, 152,  51, 255 }, { 203, 152,   0, 255 }, { 152, 152, 255, 255 },
 { 152, 152, 203, 255 }, { 152, 152, 152, 255 }, { 152, 152, 102, 255 },
 { 152, 152,  51, 255 }, { 152, 152,   0, 255 }, { 102, 152, 255, 255 },
 { 102, 152, 203, 255 }, { 102, 152, 152, 255 }, { 102, 152, 102, 255 },
 { 102, 152,  51, 255 }, { 102, 152,   0, 255 }, {  51, 152, 255, 255 },
 {  51, 152, 203, 255 }, {  51, 152, 152, 255 }, {  51, 152, 102, 255 },
 {  51, 152,  51, 255 }, {  51, 152,   0, 255 }, { 230, 134,   0, 255 },
 { 255,  51, 203, 255 }, { 255,  51, 152, 255 }, { 255,  51, 102, 255 },
 { 255,  51,  51, 255 }, { 255,  51,   0, 255 }, {   0, 102,  51, 255 },
 {   0, 102,   0, 255 }, { 203, 102, 255, 255 }, { 203, 102, 203, 255 },
 { 203, 102, 152, 255 }, { 203, 102, 102, 255 }, { 203, 102,  51, 255 },
 { 203, 102,   0, 255 }, { 152, 102, 255, 255 }, { 152, 102, 203, 255 },
 { 152, 102, 152, 255 }, { 152, 102, 102, 255 }, { 152, 102,  51, 255 },
 { 152, 102,   0, 255 }, { 102, 102, 255, 255 }, { 102, 102, 203, 255 },
 { 102, 102, 152, 255 }, { 102, 102, 102, 255 }, { 102, 102,  51, 255 },
 { 102, 102,   0, 255 }, {  51, 102, 255, 255 }, {  51, 102, 203, 255 },
 {  51, 102, 152, 255 }, {  51, 102, 102, 255 }, {  51, 102,  51, 255 },
 {  51, 102,   0, 255 }, { 255,   0, 255, 255 }, { 255,   0, 203, 255 },
 { 255,   0, 152, 255 }, { 255,   0, 102, 255 }, { 255,   0,  51, 255 },
 { 255, 175,  19, 255 }, {   0,  51, 255, 255 }, {   0,  51, 203, 255 },
 { 203,  51, 255, 255 }, { 203,  51, 203, 255 }, { 203,  51, 152, 255 },
 { 203,  51, 102, 255 }, { 203,  51,  51, 255 }, { 203,  51,   0, 255 },
 { 152,  51, 255, 255 }, { 152,  51, 203, 255 }, { 152,  51, 152, 255 },
 { 152,  51, 102, 255 }, { 152,  51,  51, 255 }, { 152,  51,   0, 255 },
 { 102,  51, 255, 255 }, { 102,  51, 203, 255 }, { 102,  51, 152, 255 },
 { 102,  51, 102, 255 }, { 102,  51,  51, 255 }, { 102,  51,   0, 255 },
 {  51,  51, 255, 255 }, {  51,  51, 203, 255 }, {  51,  51, 152, 255 },
 {  51,  51, 102, 255 }, {  51,  51,  51, 255 }, {  51,  51,   0, 255 },
 { 255, 203, 102, 255 }, { 255, 203, 152, 255 }, { 255, 203, 203, 255 },
 { 255, 203, 255, 255 }, {   0,  51, 152, 255 }, {   0,  51, 102, 255 },
 {   0,  51,  51, 255 }, {   0,  51,   0, 255 }, { 203,   0, 255, 255 },
 { 203,   0, 203, 255 }, { 203,   0, 152, 255 }, { 203,   0, 102, 255 },
 { 203,   0,  51, 255 }, { 255, 227,  70, 255 }, { 152,   0, 255, 255 },
 { 152,   0, 203, 255 }, { 152,   0, 152, 255 }, { 152,   0, 102, 255 },
 { 152,   0,  51, 255 }, { 152,   0,   0, 255 }, { 102,   0, 255, 255 },
 { 102,   0, 203, 255 }, { 102,   0, 152, 255 }, { 102,   0, 102, 255 },
 { 102,   0,  51, 255 }, { 102,   0,   0, 255 }, {  51,   0, 255, 255 },
 {  51,   0, 203, 255 }, {  51,   0, 152, 255 }, {  51,   0, 102, 255 },
 {  51,   0,  51, 255 }, {  51,   0,   0, 255 }, { 255, 203,  51, 255 },
 { 255, 203,   0, 255 }, { 255, 255,   0, 255 }, { 255, 255,  51, 255 },
 { 255, 255, 102, 255 }, { 255, 255, 152, 255 }, { 255, 255, 203, 255 },
 { 255, 255, 255, 0 } // B_TRANSPARENT_MAGIC_CMAP8
};


/*!	\brief Returns the brightness of an RGB 24 color.
	\param red Value of the red component.
	\param green Value of the green component.
	\param blue Value of the blue component.
	\return The brightness for the supplied RGB color as a value between 0
			and 255.
*/
static inline
uint8
brightness_for(uint8 red, uint8 green, uint8 blue)
{
	// brightness = 0.301 * red + 0.586 * green + 0.113 * blue
	// we use for performance reasons:
	// brightness = (308 * red + 600 * green + 116 * blue) / 1024
	return uint8((308 * red + 600 * green + 116 * blue) / 1024);
}


/*!	\brief Returns the "distance" between two RGB colors.

	This functions defines an metric on the RGB color space. The distance
	between two colors is 0, if and only if the colors are equal.

	\param red1 Red component of the first color.
	\param green1 Green component of the first color.
	\param blue1 Blue component of the first color.
	\param red2 Red component of the second color.
	\param green2 Green component of the second color.
	\param blue2 Blue component of the second color.
	\return The distance between the given colors.
*/
static inline
unsigned
color_distance(uint8 red1, uint8 green1, uint8 blue1,
			   uint8 red2, uint8 green2, uint8 blue2)
{
	// euklidian distance (its square actually)
	int rd = (int)red1 - (int)red2;
	int gd = (int)green1 - (int)green2;
	int bd = (int)blue1 - (int)blue2;
	//return rd * rd + gd * gd + bd * bd;

	// distance according to psycho-visual tests
	int rmean = ((int)red1 + (int)red2) / 2;
	return (((512 + rmean) * rd * rd) >> 8)
		   + 4 * gd * gd
		   + (((767 - rmean) * bd * bd) >> 8);
}


/*!	\brief Creates an uninitialized PaletteConverter.
*/
PaletteConverter::PaletteConverter()
	: fColorMap(NULL),
	  fOwnColorMap(NULL),
	  fCStatus(B_NO_INIT)
{
}


/*!	\brief Creates a PaletteConverter and initializes it to the supplied
		   palette.
	\param palette The palette being a 256 entry rgb_color array.
*/
PaletteConverter::PaletteConverter(const rgb_color *palette)
	: fColorMap(NULL),
	  fOwnColorMap(NULL),
	  fCStatus(B_NO_INIT)
{
	SetTo(palette);
}


/*!	\brief Creates a PaletteConverter and initializes it to the supplied
		   color map.
	\param colorMap The completely initialized color map.
*/
PaletteConverter::PaletteConverter(const color_map *colorMap)
	: fColorMap(NULL),
	  fOwnColorMap(NULL),
	  fCStatus(B_NO_INIT)
{
	SetTo(colorMap);
}


/*!	\brief Frees all resources associated with this object.
*/
PaletteConverter::~PaletteConverter()
{
	delete fOwnColorMap;
}


/*!	\brief Initializes the converter to the supplied palette.
	\param palette The palette being a 256 entry rgb_color array.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
PaletteConverter::SetTo(const rgb_color *palette)
{
	// cleanup
	SetTo((const color_map*)NULL);
	status_t error = (palette ? B_OK : B_BAD_VALUE);
	// alloc color map
	if (error == B_OK) {
		fOwnColorMap = new(nothrow) color_map;
		if (fOwnColorMap == NULL)
			error = B_NO_MEMORY;
	}
	// init color map
	if (error == B_OK) {
		fColorMap = fOwnColorMap;
		// init color list
		memcpy(fOwnColorMap->color_list, palette, sizeof(rgb_color) * 256);
		// init index map
// TODO: build this list takes about 2 seconds in qemu on my system
//		(because of color_distance())
		for (int32 color = 0; color < 32768; color++) {
			// get components
			uint8 red = (color & 0x7c00) >> 7;
			uint8 green = (color & 0x3e0) >> 2;
			uint8 blue = (color & 0x1f) << 3;
			red |= red >> 5;
			green |= green >> 5;
			blue |= blue >> 5;
			// find closest color
			uint8 closestIndex = 0;
			unsigned closestDistance = UINT_MAX;
			for (int32 i = 0; i < 256; i++) {
				const rgb_color &c = fOwnColorMap->color_list[i];
				unsigned distance = color_distance(red, green, blue,
												   c.red, c.green, c.blue);
				if (distance < closestDistance) {
					closestIndex = i;
					closestDistance = distance;
				}
			}
			fOwnColorMap->index_map[color] = closestIndex;
		}
		// no need to init inversion map
	}
	fCStatus = error;
	return error;
}


/*!	\brief Initializes the converter to the supplied color map.
	\param colorMap The completely initialized color map.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
PaletteConverter::SetTo(const color_map *colorMap)
{
	// cleanup
	if (fOwnColorMap) {
		delete fOwnColorMap;
		fOwnColorMap = NULL;
	}
	// set
	fColorMap = colorMap;
	fCStatus = (fColorMap ? B_OK : B_BAD_VALUE);
	return fCStatus;
}


/*!	\brief Returns the result of the last initialization via constructor or
		   SetTo().
	\return \c B_OK, if the converter is properly initialized, an error code
			otherwise.
*/
status_t
PaletteConverter::InitCheck() const
{
	return fCStatus;
}


/*!	\brief Returns the palette color index closest to a given RGB 15 color.

	The object must be properly initialized.

	\param rgb The RGB 15 color value (R[14:10]G[9:5]B[4:0]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB15(uint16 rgb) const
{
	return fColorMap->index_map[rgb];
}


/*!	\brief Returns the palette color index closest to a given RGB 15 color.

	The object must be properly initialized.

	\param red Red component of the color (R[4:0]).
	\param green Green component of the color (G[4:0]).
	\param blue Blue component of the color (B[4:0]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB15(uint8 red, uint8 green, uint8 blue) const
{
	// the 5 least significant bits are used
	return fColorMap->index_map[(red << 10) | (green << 5) | blue];
}


/*!	\brief Returns the palette color index closest to a given RGB 16 color.

	The object must be properly initialized.

	\param rgb The RGB 16 color value (R[15:11]G[10:5]B[4:0]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB16(uint16 rgb) const
{
	return fColorMap->index_map[(rgb >> 1) & 0x7fe0 | rgb & 0x1f];
}


/*!	\brief Returns the palette color index closest to a given RGB 16 color.

	The object must be properly initialized.

	\param red Red component of the color (R[4:0]).
	\param green Green component of the color (G[5:0]).
	\param blue Blue component of the color (B[4:0]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB16(uint8 red, uint8 green, uint8 blue) const
{
	// the 5 (for red, blue) / 6 (for green) least significant bits are used
	return fColorMap->index_map[(red << 10) | ((green & 0x3e) << 4) | blue];
}


/*!	\brief Returns the palette color index closest to a given RGB 32 color.

	The object must be properly initialized.

	\param rgb The RGB 32 color value (R[31:24]G[23:16]B[15:8]).
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB24(uint32 rgb) const
{
	return fColorMap->index_map[((rgb & 0xf8000000) >> 17)
								| ((rgb & 0xf80000) >> 14)
								| ((rgb & 0xf800) >> 11)];
}


/*!	\brief Returns the palette color index closest to a given RGB 24 color.

	The object must be properly initialized.

	\param red Red component of the color.
	\param green Green component of the color.
	\param blue Blue component of the color.
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForRGB24(uint8 red, uint8 green, uint8 blue) const
{
	return fColorMap->index_map[((red & 0xf8) << 7)
								| ((green & 0xf8) << 2)
								| (blue >> 3)];
}


/*!	\brief Returns the palette color index closest to a given Gray 8 color.

	The object must be properly initialized.

	\param gray The Gray 8 color value.
	\return The palette color index for the supplied color.
*/
inline
uint8
PaletteConverter::IndexForGray(uint8 gray) const
{
	return IndexForRGB24(gray, gray, gray);
}


/*!	\brief Returns the RGB color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index.
*/
inline
const rgb_color &
PaletteConverter::RGBColorForIndex(uint8 index) const
{
	return fColorMap->color_list[index];
}


/*!	\brief Returns the RGB 15 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index
			(R[14:10]G[9:5]B[4:0]).
*/
inline
uint16
PaletteConverter::RGB15ColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return ((color.red & 0xf8) << 7)
		   | ((color.green & 0xf8) << 2)
		   | (color.blue >> 3);
}


/*!	\brief Returns the RGB 16 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index
			(R[15:11]G[10:5]B[4:0]).
*/
inline
uint16
PaletteConverter::RGB16ColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return ((color.red & 0xf8) << 8)
		   | ((color.green & 0xfc) << 3)
		   | (color.blue >> 3);
}


/*!	\brief Returns the RGBA 32 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index
			(A[31:24]B[23:16]G[15:8]R[7:0]).
*/
inline
uint32
PaletteConverter::RGBA32ColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return (color.red << 16) | (color.green << 8) | color.blue
		| (color.alpha << 24);
}


/*!	\brief Returns the RGBA 32 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\param red Reference to the variable the red component shall be stored
		   into.
	\param green Reference to the variable the green component shall be stored
		   into.
	\param blue Reference to the variable the blue component shall be stored
		   into.
	\param alpha Reference to the variable the alpha component shall be stored
		   into.
*/
inline
void
PaletteConverter::RGBA32ColorForIndex(uint8 index, uint8 &red, uint8 &green,
									 uint8 &blue, uint8 &alpha) const
{
	const rgb_color &color = fColorMap->color_list[index];
	red = color.red;
	green = color.green;
	blue = color.blue;
	alpha = color.alpha;
}


/*!	\brief Returns the Gray 8 color for a given palette color index.

	The object must be properly initialized.

	\param index The palette color index.
	\return The color for the supplied palette color index.
*/
inline
uint8
PaletteConverter::GrayColorForIndex(uint8 index) const
{
	const rgb_color &color = fColorMap->color_list[index];
	return brightness_for(color.red, color.green, color.blue);
}


// TODO: Remove these and palette_converter() when BScreen is available.
static BLocker			gPaletteConverterLock("PalConvLock");
static PaletteConverter	gPaletteConverter;


/*!	\brief Returns a PaletteConverter using the system color palette.
	\return A PaletteConverter.
*/
static
const PaletteConverter*
palette_converter()
{
	if (gPaletteConverterLock.Lock()) {
		if (gPaletteConverter.InitCheck() != B_OK)
			gPaletteConverter.SetTo(kSystemPalette);
		gPaletteConverterLock.Unlock();
	}
	return &gPaletteConverter;
}


typedef uint32 (readFunc)(const uint8 **source, int32 index);
typedef void (writeFunc)(uint8 **dest, uint8 *data, int32 index);


void
WriteRGB24(uint8 **dest, uint8 *data, int32 index)
{
	(*dest)[0] = data[0];
	(*dest)[1] = data[1];
	(*dest)[2] = data[2];
	*dest += 3;
}


uint32
ReadRGB24(const uint8 **source, int32 index)
{
	uint32 result = (*source)[0] | ((*source)[1] << 8) | ((*source)[2] << 16);
	*source += 3;
	return result;
}


void
WriteGray8(uint8 **dest, uint8 *data, int32 index)
{
	**dest = data[2] * 308 + data[1] * 600 + data[0] * 116 >> 10;
	// this would boost the speed but is less accurate:
	//*dest = (data[2] << 8) + (data[1] << 9) + (data[0] << 8) >> 10;
	(*dest)++;
}


uint32
ReadGray8(const uint8 **source, int32 index)
{
	uint32 result = **source;
	(*source)++;
	return result;
}


void
WriteGray1(uint8 **dest, uint8 *data, int32 index)
{
	int32 shift = 7 - (index % 8);
	**dest &= ~(0x01 << shift);
	**dest |= (data[2] * 308 + data[1] * 600 + data[0] * 116) >> (17 - shift);
	if (shift == 0)
		(*dest)++;
}


uint32
ReadGray1(const uint8 **source, int32 index)
{
	int32 shift = 7 - (index % 8);
	uint32 result = ((**source >> shift) & 0x01) ? 0xff : 0x00;
	if (shift == 0)
		(*source)++;
	return result;
}


void
WriteCMAP8(uint8 **dest, uint8 *data, int32 index)
{
	**dest = gPaletteConverter.IndexForRGB15(*(uint16 *)data);
	(*dest)++;
}


uint32
ReadCMAP8(const uint8 **source, int32 index)
{
	uint32 result = gPaletteConverter.RGBA32ColorForIndex(**source);
	(*source)++;
	return result;
}


template<typename srcByte, typename dstByte>
status_t
ConvertBits(const srcByte *srcBits, dstByte *dstBits, int32 srcBitsLength,
	int32 dstBitsLength, int32 redShift, int32 greenShift, int32 blueShift,
	int32 alphaShift, int32 alphaBits, uint32 redMask, uint32 greenMask,
	uint32 blueMask, uint32 alphaMask, int32 srcBytesPerRow,
	int32 dstBytesPerRow, int32 srcBitsPerPixel, int32 dstBitsPerPixel,
	color_space srcColorSpace, color_space dstColorSpace, BPoint srcOffset,
	BPoint dstOffset, int32 width, int32 height, bool srcSwap, bool dstSwap,
	readFunc *srcFunc, writeFunc *dstFunc)
{
	char *srcBitsEnd = (char *)srcBits + srcBitsLength;
	char *dstBitsEnd = (char *)dstBits + dstBitsLength;

	int32 srcBitsPerRow = srcBytesPerRow << 3;
	int32 dstBitsPerRow = dstBytesPerRow << 3;

	// Advance the buffers to reach their offsets
	int32 srcOffsetX = (int32)srcOffset.x;
	int32 dstOffsetX = (int32)dstOffset.x;
	(char *)srcBits += ((int32)srcOffset.y * srcBitsPerRow + srcOffsetX
		* srcBitsPerPixel) >> 3;
	(char *)dstBits += ((int32)dstOffset.y * dstBitsPerRow + dstOffsetX
		* dstBitsPerPixel) >> 3;

	// Ensure that the width fits
	int32 srcWidth = (srcBitsPerRow - srcOffsetX * srcBitsPerPixel)
		/ srcBitsPerPixel;
	if (srcWidth < width)
		width = srcWidth;

	int32 dstWidth = (dstBitsPerRow - dstOffsetX * dstBitsPerPixel)
		/ dstBitsPerPixel;
	if (dstWidth < width)
		width = dstWidth;

	if (width < 0)
		return B_OK;

	// Catch the copy case
	if (srcColorSpace == dstColorSpace && srcBitsPerPixel % 8 == 0) {
		int32 copyCount = (width * srcBitsPerPixel) >> 3;
		for (int32 i = 0; i < height; i++) {
			memcpy(dstBits, srcBits, copyCount);

			(char *)srcBits += srcBytesPerRow;
			(char *)dstBits += dstBytesPerRow;

			if ((char *)srcBits > srcBitsEnd || (char *)dstBits > dstBitsEnd)
				return B_OK;
		}

		return B_OK;
	}

	int32 srcLinePad = (srcBitsPerRow - width * srcBitsPerPixel) >> 3;
	int32 dstLinePad = (dstBitsPerRow - width * dstBitsPerPixel) >> 3;

	register uint32 result;
	register uint32 source;
	for (int32 i = 0; i < height; i++) {
		for (int32 j = 0; j < width; j++) {
			if (srcFunc)
				source = srcFunc((const uint8 **)&srcBits, srcOffsetX++);
			else {
				source = *srcBits;
				srcBits++;
			}

			// This is valid, as only 16 bit modes will need to swap
			if (srcSwap)
				source = (source << 8) | (source >> 8);

			if (redShift > 0)
				result = ((source >> redShift) & redMask);
			else if (redShift < 0)
				result = ((source << -redShift) & redMask);
			else
				result = source & redMask;

			if (greenShift > 0)
				result |= ((source >> greenShift) & greenMask);
			else if (greenShift < 0)
				result |= ((source << -greenShift) & greenMask);
			else
				result |= source & greenMask;

			if (blueShift > 0)
				result |= ((source >> blueShift) & blueMask);
			else if (blueShift < 0)
				result |= ((source << -blueShift) & blueMask);
			else
				result |= source & blueMask;

			if (alphaBits > 0) {
				if (alphaShift > 0)
					result |= ((source >> alphaShift) & alphaMask);
				else if (alphaShift < 0)
					result |= ((source << -alphaShift) & alphaMask);
				else
					result |= source & alphaMask;

				// if we only had one alpha bit we want it to be 0/255
				if (alphaBits == 1 && result & alphaMask)
					result |= alphaMask;
			} else
				result |= alphaMask;

			// This is valid, as only 16 bit modes will need to swap
			if (dstSwap)
				result = (result << 8) | (result >> 8);

			if (dstFunc)
				dstFunc((uint8 **)&dstBits, (uint8 *)&result, dstOffsetX++);
			else {
				*dstBits = result;
				dstBits++;
			}

			if ((char *)srcBits > srcBitsEnd || (char *)dstBits > dstBitsEnd)
				return B_OK;
		}

		(char *)srcBits += srcLinePad;
		(char *)dstBits += dstLinePad;
		dstOffsetX -= width;
		srcOffsetX -= width;
	}

	return B_OK;
}


template<typename srcByte>
status_t
ConvertBits(const srcByte *srcBits, void *dstBits, int32 srcBitsLength,
	int32 dstBitsLength, int32 redShift, int32 greenShift, int32 blueShift,
	int32 alphaShift, int32 alphaBits, int32 srcBytesPerRow,
	int32 dstBytesPerRow, int32 srcBitsPerPixel, color_space srcColorSpace,
	color_space dstColorSpace, BPoint srcOffset, BPoint dstOffset, int32 width,
	int32 height, bool srcSwap,	readFunc *srcFunc)
{
	switch (dstColorSpace) {
		case B_RGBA32:
			ConvertBits(srcBits, (uint32 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 24, greenShift - 16, blueShift - 8,
				alphaShift - 32, alphaBits, 0x00ff0000, 0x0000ff00, 0x000000ff,
				0xff000000, srcBytesPerRow, dstBytesPerRow, srcBitsPerPixel,
				32, srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, srcSwap, false, srcFunc, NULL);
			break;

		case B_RGBA32_BIG:
			ConvertBits(srcBits, (uint32 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 16, greenShift - 24, blueShift - 32,
				alphaShift - 8, alphaBits, 0x0000ff00, 0x00ff0000, 0xff000000,
				0x00000ff, srcBytesPerRow, dstBytesPerRow, srcBitsPerPixel, 32,
				srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, srcSwap, false, srcFunc, NULL);
			break;

		/* Note:	we set the unused alpha to 255 here. This is because BeOS
					uses the unused alpha for B_OP_ALPHA even though it should
					not care about it. */
		case B_RGB32:
			ConvertBits(srcBits, (uint32 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 24, greenShift - 16, blueShift - 8,
				0, 0, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000,
				srcBytesPerRow, dstBytesPerRow, srcBitsPerPixel, 32,
				srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, srcSwap, false, srcFunc, NULL);
			break;

		case B_RGB32_BIG:
			ConvertBits(srcBits, (uint32 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 16, greenShift - 24, blueShift - 32,
				0, 0, 0x0000ff00, 0x00ff0000, 0xff000000, 0x000000ff,
				srcBytesPerRow, dstBytesPerRow, srcBitsPerPixel, 32,
				srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, srcSwap, false, srcFunc, NULL);
			break;

		case B_RGB24:
			ConvertBits(srcBits, (uint8 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 24, greenShift - 16, blueShift - 8,
				0, 0, 0xff0000, 0x00ff00, 0x0000ff, 0x000000, srcBytesPerRow,
				dstBytesPerRow, srcBitsPerPixel, 24, srcColorSpace,
				dstColorSpace, srcOffset, dstOffset, width, height, srcSwap,
				false, srcFunc, WriteRGB24);
			break;

		case B_RGB24_BIG:
			ConvertBits(srcBits, (uint8 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 8, greenShift - 16, blueShift - 24,
				0, 0, 0x0000ff, 0x00ff00, 0xff0000, 0x000000, srcBytesPerRow,
				dstBytesPerRow, srcBitsPerPixel, 24, srcColorSpace,
				dstColorSpace, srcOffset, dstOffset, width, height, srcSwap,
				false, srcFunc, WriteRGB24);
			break;

		case B_RGB16:
		case B_RGB16_BIG:
			ConvertBits(srcBits, (uint16 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 16, greenShift - 11, blueShift - 5,
				0, 0, 0xf800, 0x07e0, 0x001f, 0x0000, srcBytesPerRow,
				dstBytesPerRow, srcBitsPerPixel, 16, srcColorSpace,
				dstColorSpace, srcOffset, dstOffset, width, height, srcSwap,
				dstColorSpace == B_RGB16_BIG, srcFunc, NULL);
			break;

		case B_RGBA15:
		case B_RGBA15_BIG:
			ConvertBits(srcBits, (uint16 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 15, greenShift - 10, blueShift - 5,
				alphaShift - 16, alphaBits, 0x7c00, 0x03e0, 0x001f, 0x8000,
				srcBytesPerRow, dstBytesPerRow, srcBitsPerPixel, 16,
				srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, srcSwap, dstColorSpace == B_RGBA15_BIG, srcFunc, NULL);
			break;

		case B_RGB15:
		case B_RGB15_BIG:
			ConvertBits(srcBits, (uint16 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 15, greenShift - 10, blueShift - 5,
				0, 0, 0x7c00, 0x03e0, 0x001f, 0x0000, srcBytesPerRow,
				dstBytesPerRow, srcBitsPerPixel, 16, srcColorSpace,
				dstColorSpace, srcOffset, dstOffset, width, height, srcSwap,
				dstColorSpace == B_RGB15_BIG, srcFunc, NULL);
			break;

		case B_GRAY8:
			ConvertBits(srcBits, (uint8 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 24, greenShift - 16, blueShift - 8,
				0, 0, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,
				srcBytesPerRow, dstBytesPerRow, srcBitsPerPixel, 8,
				srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, srcSwap, false, srcFunc, WriteGray8);
			break;

		case B_GRAY1:
			ConvertBits(srcBits, (uint8 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 24, greenShift - 16, blueShift - 8,
				0, 0, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000,
				srcBytesPerRow, dstBytesPerRow, srcBitsPerPixel, 1,
				srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, srcSwap, false, srcFunc, WriteGray1);
			break;

		case B_CMAP8:
			palette_converter();
			ConvertBits(srcBits, (uint8 *)dstBits, srcBitsLength,
				dstBitsLength, redShift - 15, greenShift - 10, blueShift - 5,
				0, 0, 0x7c00, 0x03e0, 0x001f, 0x0000, srcBytesPerRow,
				dstBytesPerRow, srcBitsPerPixel, 8, srcColorSpace,
				dstColorSpace, srcOffset, dstOffset, width, height, srcSwap,
				false, srcFunc, WriteCMAP8);
			break;

		default:
			return B_BAD_VALUE;
			break;
	}

	return B_OK;
}


/*!	\brief Converts a source buffer in one colorspace into a destination
		   buffer of another colorspace.

	\param srcBits The raw source buffer.
	\param dstBits The raw destination buffer.
	\param srcBytesPerRow How many bytes per row the source buffer has got.
	\param dstBytesPerRow How many bytes per row the destination buffer has got.
	\param srcColorSpace The colorspace the source buffer is in.
	\param dstColorSpace The colorspace the buffer shall be converted to.
	\param width The width (in pixels) of each row.
	\param height The height (in pixels) of the buffers.
	\return
	- \c B_OK: Indicates success.
	- \c B_BAD_VALUE: \c NULL buffer or at least one colorspace is unsupported.
*/
status_t
ConvertBits(const void *srcBits, void *dstBits, int32 srcBitsLength,
	int32 dstBitsLength, int32 srcBytesPerRow, int32 dstBytesPerRow,
	color_space srcColorSpace, color_space dstColorSpace, int32 width,
	int32 height)
{
	return ConvertBits(srcBits, dstBits, srcBitsLength, dstBitsLength,
		srcBytesPerRow, dstBytesPerRow, srcColorSpace, dstColorSpace,
		BPoint(0, 0), BPoint(0, 0), width, height);
}


/*!	\brief Converts a source buffer in one colorspace into a destination
		   buffer of another colorspace.

	\param srcBits The raw source buffer.
	\param dstBits The raw destination buffer.
	\param srcBytesPerRow How many bytes per row the source buffer has got.
	\param dstBytesPerRow How many bytes per row the destination buffer has got.
	\param srcColorSpace The colorspace the source buffer is in.
	\param dstColorSpace The colorspace the buffer shall be converted to.
	\param srcOffset The offset at which to start reading in the source.
	\param srcOffset The offset at which to start writing in the destination.
	\param width The width (in pixels) to convert.
	\param height The height (in pixels) to convert.
	\return
	- \c B_OK: Indicates success.
	- \c B_BAD_VALUE: \c NULL buffer or at least one colorspace is unsupported.
*/
status_t
ConvertBits(const void *srcBits, void *dstBits, int32 srcBitsLength,
	int32 dstBitsLength, int32 srcBytesPerRow, int32 dstBytesPerRow,
	color_space srcColorSpace, color_space dstColorSpace, BPoint srcOffset,
	BPoint dstOffset, int32 width, int32 height)
{
	if (!srcBits || !dstBits || srcBitsLength < 0 || dstBitsLength < 0
		|| width < 0 || height < 0 || srcBytesPerRow < 0 || dstBytesPerRow < 0)
		return B_BAD_VALUE;

	switch (srcColorSpace) {
		case B_RGBA32:
			return ConvertBits((const uint32 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 24, 16, 8, 32, 8, srcBytesPerRow,
				dstBytesPerRow, 32, srcColorSpace, dstColorSpace, srcOffset,
				dstOffset, width, height, false, NULL);
			break;

		case B_RGBA32_BIG:
			return ConvertBits((const uint32 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 16, 24, 32, 8, 8, srcBytesPerRow,
				dstBytesPerRow, 32, srcColorSpace, dstColorSpace, srcOffset,
				dstOffset, width, height, false, NULL);
			break;

		case B_RGB32:
			return ConvertBits((const uint32 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 24, 16, 8, 0, 0, srcBytesPerRow, dstBytesPerRow,
				32, srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, false, NULL);
			break;

		case B_RGB32_BIG:
			return ConvertBits((const uint32 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 16, 24, 32, 0, 0, srcBytesPerRow,
				dstBytesPerRow, 32, srcColorSpace, dstColorSpace, srcOffset,
				dstOffset, width, height, false, NULL);
			break;

		case B_RGB24:
			return ConvertBits((const uint8 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 24, 16, 8, 0, 0, srcBytesPerRow, dstBytesPerRow,
				24, srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, false, ReadRGB24);
			break;

		case B_RGB24_BIG:
			return ConvertBits((const uint8 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 8, 16, 24, 0, 0, srcBytesPerRow, dstBytesPerRow,
				24, srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, false, ReadRGB24);
			break;

		case B_RGB16:
		case B_RGB16_BIG:
			return ConvertBits((const uint16 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 16, 11, 5, 0, 0, srcBytesPerRow, dstBytesPerRow,
				16, srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, srcColorSpace == B_RGB16_BIG, NULL);
			break;

		case B_RGBA15:
		case B_RGBA15_BIG:
			return ConvertBits((const uint16 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 15, 10, 5, 16, 1, srcBytesPerRow,
				dstBytesPerRow, 16, srcColorSpace, dstColorSpace, srcOffset,
				dstOffset, width, height, srcColorSpace == B_RGBA15_BIG, NULL);
			break;

		case B_RGB15:
		case B_RGB15_BIG:
			return ConvertBits((const uint16 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 15, 10, 5, 0, 0, srcBytesPerRow, dstBytesPerRow,
				16, srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, srcColorSpace == B_RGB15_BIG, NULL);
			break;

		case B_GRAY8:
			return ConvertBits((const uint8 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 8, 8, 8, 0, 0, srcBytesPerRow, dstBytesPerRow,
				8, srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, false, ReadGray8);
			break;

		case B_GRAY1:
			return ConvertBits((const uint8 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 8, 8, 8, 0, 0, srcBytesPerRow, dstBytesPerRow,
				1, srcColorSpace, dstColorSpace, srcOffset, dstOffset, width,
				height, false, ReadGray1);
			break;

		case B_CMAP8:
			palette_converter();
			return ConvertBits((const uint8 *)srcBits, dstBits, srcBitsLength,
				dstBitsLength, 24, 16, 8, 32, 8, srcBytesPerRow,
				dstBytesPerRow, 8, srcColorSpace, dstColorSpace, srcOffset,
				dstOffset, width, height, false, ReadCMAP8);
			break;

		default:
			return B_BAD_VALUE;
			break;
	}
}

} // namespace BPrivate
