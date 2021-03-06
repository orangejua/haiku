/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CharacterStyleData.h"

#include <new>


CharacterStyleData::CharacterStyleData()
	:
	fFont(),

	fAscent(-1.0f),
	fDescent(-1.0f),
	fWidth(-1.0f),

	fGlyphSpacing(0.0f),

	fFgColor((rgb_color){ 0, 0, 0, 255 }),
	fBgColor((rgb_color){ 255, 255, 255, 255 }),
	fStrikeOutColor((rgb_color){ 0, 0, 0, 255 }),
	fUnderlineColor((rgb_color){ 0, 0, 0, 255 }),

	fStrikeOutStyle(STRIKE_OUT_NONE),
	fUnderlineStyle(UNDERLINE_NONE)
{
}


CharacterStyleData::CharacterStyleData(const CharacterStyleData& other)
	:
	fFont(other.fFont),

	fAscent(other.fAscent),
	fDescent(other.fDescent),
	fWidth(other.fWidth),

	fGlyphSpacing(other.fGlyphSpacing),

	fFgColor(other.fFgColor),
	fBgColor(other.fBgColor),
	fStrikeOutColor(other.fStrikeOutColor),
	fUnderlineColor(other.fUnderlineColor),

	fStrikeOutStyle(other.fStrikeOutStyle),
	fUnderlineStyle(other.fUnderlineStyle)
{
}


bool
CharacterStyleData::operator==(const CharacterStyleData& other) const
{
	if (this == &other)
		return true;

	return fFont == other.fFont
		&& fAscent == other.fAscent
		&& fDescent == other.fDescent
		&& fWidth == other.fWidth

		&& fGlyphSpacing == other.fGlyphSpacing

		&& fFgColor == other.fFgColor
		&& fBgColor == other.fBgColor
		&& fStrikeOutColor == other.fStrikeOutColor
		&& fUnderlineColor == other.fUnderlineColor

		&& fStrikeOutStyle == other.fStrikeOutStyle
		&& fUnderlineStyle == other.fUnderlineStyle;
}


bool
CharacterStyleData::operator!=(const CharacterStyleData& other) const
{
	return !(*this == other);
}


CharacterStyleDataRef
CharacterStyleData::SetFont(const BFont& font)
{
	if (fFont == font)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fFont = font;
	return CharacterStyleDataRef(ret, true);
}


CharacterStyleDataRef
CharacterStyleData::SetAscent(float ascent)
{
	if (fAscent == ascent)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fAscent = ascent;
	return CharacterStyleDataRef(ret, true);
}


float
CharacterStyleData::Ascent() const
{
	if (fAscent >= 0.0f)
		return fAscent;

	font_height fontHeight;
	fFont.GetHeight(&fontHeight);
	return fontHeight.ascent;
}


CharacterStyleDataRef
CharacterStyleData::SetDescent(float descent)
{
	if (fDescent == descent)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fDescent = descent;
	return CharacterStyleDataRef(ret, true);
}


float
CharacterStyleData::Descent() const
{
	if (fDescent >= 0.0f)
		return fDescent;

	font_height fontHeight;
	fFont.GetHeight(&fontHeight);
	return fontHeight.descent;
}


CharacterStyleDataRef
CharacterStyleData::SetWidth(float width)
{
	if (fWidth == width)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fWidth = width;
	return CharacterStyleDataRef(ret, true);
}


CharacterStyleDataRef
CharacterStyleData::SetGlyphSpacing(float glyphSpacing)
{
	if (fGlyphSpacing == glyphSpacing)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fGlyphSpacing = glyphSpacing;
	return CharacterStyleDataRef(ret, true);
}


CharacterStyleDataRef
CharacterStyleData::SetForegroundColor(rgb_color color)
{
	if (fFgColor == color)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fFgColor = color;
	return CharacterStyleDataRef(ret, true);
}


CharacterStyleDataRef
CharacterStyleData::SetBackgroundColor(rgb_color color)
{
	if (fBgColor == color)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fBgColor = color;
	return CharacterStyleDataRef(ret, true);
}


CharacterStyleDataRef
CharacterStyleData::SetStrikeOutColor(rgb_color color)
{
	if (fStrikeOutColor == color)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fStrikeOutColor = color;
	return CharacterStyleDataRef(ret, true);
}


CharacterStyleDataRef
CharacterStyleData::SetUnderlineColor(rgb_color color)
{
	if (fUnderlineColor == color)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fUnderlineColor = color;
	return CharacterStyleDataRef(ret, true);
}


CharacterStyleDataRef
CharacterStyleData::SetStrikeOut(uint8 strikeOut)
{
	if (fStrikeOutStyle == strikeOut)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fStrikeOutStyle = strikeOut;
	return CharacterStyleDataRef(ret, true);
}


CharacterStyleDataRef
CharacterStyleData::SetUnderline(uint8 underline)
{
	if (fUnderlineStyle == underline)
		return CharacterStyleDataRef(this);

	CharacterStyleData* ret = new(std::nothrow) CharacterStyleData(*this);
	if (ret == NULL)
		return CharacterStyleDataRef(this);

	ret->fUnderlineStyle = underline;
	return CharacterStyleDataRef(ret, true);
}


// #pragma mark - private


CharacterStyleData&
CharacterStyleData::operator=(const CharacterStyleData& other)
{
	return *this;
}

