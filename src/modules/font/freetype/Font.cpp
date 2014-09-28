/**
 * Copyright (c) 2006-2014 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#include "Font.h"

#include "TrueTypeRasterizer.h"
#include "font/ImageRasterizer.h"
#include "font/BMFontRasterizer.h"

#include "libraries/utf8/utf8.h"

namespace love
{
namespace font
{
namespace freetype
{

Font::Font()
{
	if (FT_Init_FreeType(&library))
		throw love::Exception("TrueTypeFont Loading error: FT_Init_FreeType failed");
}

Font::~Font()
{
	FT_Done_FreeType(library);
}

Rasterizer *Font::newRasterizer(love::filesystem::FileData *data)
{
	if (TrueTypeRasterizer::accepts(library, data))
		return newTrueTypeRasterizer(data, 12);
	else if (BMFontRasterizer::accepts(data))
		return newBMFontRasterizer(data, std::vector<image::ImageData *>());

	throw love::Exception("Invalid font file: %s", data->getFilename().c_str());
}

Rasterizer *Font::newTrueTypeRasterizer(love::filesystem::FileData *data, int size)
{
	return new TrueTypeRasterizer(library, data, size);
}

Rasterizer *Font::newBMFontRasterizer(love::filesystem::FileData *fontdef, const std::vector<image::ImageData *> &images)
{
	return new BMFontRasterizer(fontdef, images);
}

Rasterizer *Font::newImageRasterizer(love::image::ImageData *data, const std::string &text)
{
	std::vector<uint32> glyphs;
	glyphs.reserve(text.size());

	try
	{
		utf8::iterator<std::string::const_iterator> i(text.begin(), text.begin(), text.end());
		utf8::iterator<std::string::const_iterator> end(text.end(), text.begin(), text.end());

		while (i != end)
			glyphs.push_back(*i++);
	}
	catch (utf8::exception &e)
	{
		throw love::Exception("Decoding error: %s", e.what());
	}

	return newImageRasterizer(data, &glyphs[0], glyphs.size());
}

Rasterizer *Font::newImageRasterizer(love::image::ImageData *data, uint32 *glyphs, int numglyphs)
{
	return new ImageRasterizer(data, glyphs, numglyphs);
}

GlyphData *Font::newGlyphData(Rasterizer *r, const std::string &text)
{
	uint32 codepoint = 0;

	try
	{
		codepoint = utf8::peek_next(text.begin(), text.end());
	}
	catch (utf8::exception &e)
	{
		throw love::Exception("Decoding error: %s", e.what());
	}

	return r->getGlyphData(codepoint);
}

GlyphData *Font::newGlyphData(Rasterizer *r, uint32 glyph)
{
	return r->getGlyphData(glyph);
}

const char *Font::getName() const
{
	return "love.font.freetype";
}

} // freetype
} // font
} // love
