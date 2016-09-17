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

#include <SFConfig.h>

#include "SFAlbum.h"
#include "SFArtist.h"
#include "SFAssert.h"
#include "SFBase.h"
#include "SFCommon.h"
#include "SFData.h"
#include "SFFont.h"
#include "SFGDEF.h"
#include "SFPattern.h"

#include "SFGlyphDiscovery.h"
#include "SFGlyphManipulation.h"
#include "SFGlyphPositioning.h"
#include "SFGlyphSubstitution.h"
#include "SFTextProcessor.h"

static SFData _SFGetLookupFromHeader(SFData header, SFUInt16 lookupIndex);

static void _SFApplyFeatureRange(SFTextProcessorRef processor, SFFeatureKind featureKind, SFUInteger index, SFUInteger count);
static void _SFApplyFeatureUnit(SFTextProcessorRef processor, SFFeatureKind featureKind, SFFeatureUnitRef featureUnit);

SF_INTERNAL void SFTextProcessorInitialize(SFTextProcessorRef textProcessor, SFPatternRef pattern,
    SFAlbumRef album, SFTextDirection textDirection, SFTextMode textMode)
{
    SFData gdef;

    /* Pattern must NOT be null. */
    SFAssert(pattern != NULL);
    /* Album must NOT be null. */
    SFAssert(album != NULL);

    textProcessor->_pattern = pattern;
    textProcessor->_album = album;
    textProcessor->_glyphClassDef = NULL;
    textProcessor->_textDirection = textDirection;
    textProcessor->_textMode = textMode;

    gdef = pattern->font->tables.gdef;
    if (gdef) {
        SFOffset offset = SFGDEF_GlyphClassDefOffset(gdef);
        textProcessor->_glyphClassDef = SFData_Subdata(gdef, offset);
    }

    SFLocatorInitialize(&textProcessor->_locator, album, gdef);
}

SF_INTERNAL void SFTextProcessorDiscoverGlyphs(SFTextProcessorRef textProcessor)
{
    SFAlbumBeginFilling(textProcessor->_album);
    _SFDiscoverGlyphs(textProcessor);
}

SF_INTERNAL void SFTextProcessorSubstituteGlyphs(SFTextProcessorRef textProcessor)
{
    SFPatternRef pattern = textProcessor->_pattern;

    _SFApplyFeatureRange(textProcessor, SFFeatureKindSubstitution, 0, pattern->featureUnits.gsub);
    SFAlbumEndFilling(textProcessor->_album);
}

SF_INTERNAL void SFTextProcessorPositionGlyphs(SFTextProcessorRef textProcessor)
{
    SFPatternRef pattern = textProcessor->_pattern;
    SFFontRef font = pattern->font;
    SFAlbumRef album = textProcessor->_album;
    SFUInteger glyphCount = album->glyphCount;
    SFUInteger index;

    SFAlbumBeginArranging(album);

    /* Set positions and advances of all glyphs. */
    for (index = 0; index < glyphCount; index++) {
        SFGlyphID glyphID = SFAlbumGetGlyph(album, index);
        SFAdvance advance = SFFontGetAdvanceForGlyph(font, SFFontLayoutHorizontal, glyphID);

        SFAlbumSetX(album, index, 0);
        SFAlbumSetY(album, index, 0);
        SFAlbumSetAdvance(album, index, advance);
    }

    _SFApplyFeatureRange(textProcessor, SFFeatureKindPositioning, pattern->featureUnits.gsub, pattern->featureUnits.gpos);
    _SFResolveAttachments(textProcessor);

    SFAlbumEndArranging(album);
}

SF_INTERNAL void SFTextProcessorWrapUp(SFTextProcessorRef textProcessor)
{
    SFAlbumWrapUp(textProcessor->_album);
}

static SFData _SFGetLookupFromHeader(SFData header, SFUInt16 lookupIndex)
{
    SFOffset listOffset = SFHeader_LookupListOffset(header);
    SFData lookupList = SFData_Subdata(header, listOffset);
    SFOffset lookupOffset = SFLookupList_LookupOffset(lookupList, lookupIndex);
    SFData lookup = SFData_Subdata(lookupList, lookupOffset);

    return lookup;
}

static void _SFApplyFeatureRange(SFTextProcessorRef processor, SFFeatureKind featureKind, SFUInteger index, SFUInteger count)
{
    SFPatternRef pattern = processor->_pattern;
    SFUInteger limit = index + count;

    for (; index < limit; index++) {
        /* Apply the feature unit. */
        _SFApplyFeatureUnit(processor, featureKind, &pattern->featureUnits.items[index]);
    }
}

static void _SFApplyFeatureUnit(SFTextProcessorRef processor, SFFeatureKind featureKind, SFFeatureUnitRef featureUnit)
{
    SFUInt16 *lookupArray = featureUnit->lookupIndexes.items;
    SFUInteger lookupCount = featureUnit->lookupIndexes.count;
    SFUInteger lookupIndex;

    /* Apply all lookups of the feature unit. */
    for (lookupIndex = 0; lookupIndex < lookupCount; lookupIndex++) {
        SFLocatorRef locator = &processor->_locator;
        SFLocatorReset(locator, 0, processor->_album->glyphCount);
        SFLocatorSetFeatureMask(locator, featureUnit->featureMask);

        while (SFLocatorMoveNext(locator)) {
            _SFApplyLookup(processor, featureKind, lookupArray[lookupIndex]);
        }
    }
}

SF_PRIVATE void _SFApplyLookup(SFTextProcessorRef processor, SFFeatureKind featureKind, SFUInt16 lookupIndex)
{
    SFFontRef font = processor->_pattern->font;
    SFData lookup;

    switch (featureKind) {
        case SFFeatureKindSubstitution:
            lookup = _SFGetLookupFromHeader(font->tables.gsub, lookupIndex);
            _SFApplySubstitutionLookup(processor, lookup);
            break;

        case SFFeatureKindPositioning:
            lookup = _SFGetLookupFromHeader(font->tables.gpos, lookupIndex);
            _SFApplyPositioningLookup(processor, lookup);
            break;
    }
}