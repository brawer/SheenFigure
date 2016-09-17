/*
 * Copyright (C) 2016 Muhammad Tayyab Akram
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstddef>
#include <cstring>

extern "C" {
#include <Source/SFAssert.h>
#include <Source/SFFont.h>
}

#include "FontTester.h"

using namespace SheenFigure::Tester;

static void *OBJECT_FONT = &OBJECT_FONT;
static int FINALIZE_COUNT = 0;

static const char *TABLE_GDEF = "GDEF";
static const char *TABLE_GSUB = "GSUB";
static const char *TABLE_GPOS = "GPOS";

static void finalize(void *object)
{
    SFAssert(object == OBJECT_FONT);
    FINALIZE_COUNT++;
}

static void loadTable(void *object, SFTag tag, SFUInt8 *buffer, SFUInteger *length)
{
    SFAssert(object == OBJECT_FONT);

    switch (tag) {
    case SFTagMake('G', 'D', 'E', 'F'):
        if (buffer) {
            memcpy(buffer, TABLE_GDEF, 4);
        }
        if (length) {
            *length = 4;
        }
        break;

    case SFTagMake('G', 'S', 'U', 'B'):
        if (buffer) {
            memcpy(buffer, TABLE_GSUB, 4);
        }
        if (length) {
            *length = 4;
        }
        break;

    case SFTagMake('G', 'P', 'O', 'S'):
        if (buffer) {
            memcpy(buffer, TABLE_GPOS, 4);
        }
        if (length) {
            *length = 4;
        }
        break;

    default:
        if (length) {
            *length = 0;
        }
        break;
    }
}

static SFGlyphID getGlyphIDForCodepoint(void *object, SFCodepoint codepoint)
{
    SFAssert(object == OBJECT_FONT);

    return (SFGlyphID)((codepoint >> 16) ^ (codepoint & 0xFFFF));
}

static SFAdvance getAdvanceForGlyph(void *object, SFFontLayout fontLayout, SFGlyphID glyphID)
{
    SFAssert(object == OBJECT_FONT);

    switch (fontLayout) {
    case SFFontLayoutHorizontal:
        return glyphID * 11;

    case SFFontLayoutVertical:
        return glyphID * 7;

    default:
        return -1;
    }
}

static SFFontRef SFFontCreateWithCompleteFunctionality(void)
{
    const SFFontProtocol protocol = {
        .finalize = &finalize,
        .loadTable = &loadTable,
        .getGlyphIDForCodepoint = &getGlyphIDForCodepoint,
        .getAdvanceForGlyph = &getAdvanceForGlyph,
    };
    return SFFontCreateWithProtocol(&protocol, (void *)OBJECT_FONT);
}

static SFFontRef SFFontCreateWithRequiredFunctionality(void)
{
    const SFFontProtocol protocol = {
        .loadTable = &loadTable,
        .getGlyphIDForCodepoint = &getGlyphIDForCodepoint,
    };
    return SFFontCreateWithProtocol(&protocol, (void *)OBJECT_FONT);
}

FontTester::FontTester()
{
}

void FontTester::testBadProtocol()
{
    /* Test with null protocol. */
    {
        SFFontRef font = SFFontCreateWithProtocol(NULL, NULL);
        SFAssert(font == NULL);
    }

    /* Test with missing required functions. */
    {
        const SFFontProtocol protocol = {
            .finalize = &finalize,
            .getAdvanceForGlyph = &getAdvanceForGlyph,
        };
        SFFontRef font = SFFontCreateWithProtocol(&protocol, NULL);

        SFAssert(font == NULL);
    }
}

void FontTester::testFinalizeCallback()
{
    FINALIZE_COUNT = 0;

    SFFontRef font = SFFontCreateWithCompleteFunctionality();
    SFAssert(FINALIZE_COUNT == 0);

    SFFontRelease(font);
    SFAssert(FINALIZE_COUNT == 1);
}

void FontTester::testLoadedTables()
{
    SFFontRef font = SFFontCreateWithCompleteFunctionality();

    SFAssert(memcmp(font->tables.gdef, TABLE_GDEF, 4) == 0);
    SFAssert(memcmp(font->tables.gsub, TABLE_GSUB, 4) == 0);
    SFAssert(memcmp(font->tables.gpos, TABLE_GPOS, 4) == 0);

    SFFontRelease(font);
}

void FontTester::testGetGlyphIDForCodepoint()
{
    SFFontRef font = SFFontCreateWithCompleteFunctionality();

    SFGlyphID glyph1 = SFFontGetGlyphIDForCodepoint(font, 0);
    SFGlyphID glyph2 = SFFontGetGlyphIDForCodepoint(font, 0xFFFF);
    SFGlyphID glyph3 = SFFontGetGlyphIDForCodepoint(font, 0x10FFFF);

    SFAssert(glyph1 == getGlyphIDForCodepoint(OBJECT_FONT, 0));
    SFAssert(glyph2 == getGlyphIDForCodepoint(OBJECT_FONT, 0xFFFF));
    SFAssert(glyph3 == getGlyphIDForCodepoint(OBJECT_FONT, 0x10FFFF));

    SFFontRelease(font);
}

void FontTester::testGetAdvanceForGlyph()
{
    /* Test as implemented function. */
    {
        SFFontRef font = SFFontCreateWithCompleteFunctionality();

        SFAdvance advance1 = SFFontGetAdvanceForGlyph(font, SFFontLayoutHorizontal, 0);
        SFAdvance advance2 = SFFontGetAdvanceForGlyph(font, SFFontLayoutHorizontal, 0x7FFF);
        SFAdvance advance3 = SFFontGetAdvanceForGlyph(font, SFFontLayoutHorizontal, 0xFFFF);
        SFAdvance advance4 = SFFontGetAdvanceForGlyph(font, SFFontLayoutVertical, 0);
        SFAdvance advance5 = SFFontGetAdvanceForGlyph(font, SFFontLayoutVertical, 0x7FFF);
        SFAdvance advance6 = SFFontGetAdvanceForGlyph(font, SFFontLayoutVertical, 0xFFFF);

        SFAssert(advance1 == getAdvanceForGlyph(OBJECT_FONT, SFFontLayoutHorizontal, 0));
        SFAssert(advance2 == getAdvanceForGlyph(OBJECT_FONT, SFFontLayoutHorizontal, 0x7FFF));
        SFAssert(advance3 == getAdvanceForGlyph(OBJECT_FONT, SFFontLayoutHorizontal, 0xFFFF));
        SFAssert(advance4 == getAdvanceForGlyph(OBJECT_FONT, SFFontLayoutVertical, 0));
        SFAssert(advance5 == getAdvanceForGlyph(OBJECT_FONT, SFFontLayoutVertical, 0x7FFF));
        SFAssert(advance6 == getAdvanceForGlyph(OBJECT_FONT, SFFontLayoutVertical, 0xFFFF));

        SFFontRelease(font);
    }

    /* Test as un-implemented function. */
    {
        SFFontRef font = SFFontCreateWithRequiredFunctionality();

        SFAdvance advance1 = SFFontGetAdvanceForGlyph(font, SFFontLayoutHorizontal, 0);
        SFAdvance advance2 = SFFontGetAdvanceForGlyph(font, SFFontLayoutHorizontal, 0x7FFF);
        SFAdvance advance3 = SFFontGetAdvanceForGlyph(font, SFFontLayoutHorizontal, 0xFFFF);
        SFAdvance advance4 = SFFontGetAdvanceForGlyph(font, SFFontLayoutVertical, 0);
        SFAdvance advance5 = SFFontGetAdvanceForGlyph(font, SFFontLayoutVertical, 0x7FFF);
        SFAdvance advance6 = SFFontGetAdvanceForGlyph(font, SFFontLayoutVertical, 0xFFFF);

        SFAssert(advance1 == 0);
        SFAssert(advance2 == 0);
        SFAssert(advance3 == 0);
        SFAssert(advance4 == 0);
        SFAssert(advance5 == 0);
        SFAssert(advance6 == 0);

        SFFontRelease(font);
    }
}

void FontTester::test()
{
    testBadProtocol();
    testFinalizeCallback();
    testLoadedTables();
    testGetGlyphIDForCodepoint();
    testGetAdvanceForGlyph();
}
