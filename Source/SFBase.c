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

#include "SFShapingKnowledge.h"
#include "SFUnifiedEngine.h"
#include "SFBase.h"

const SFRange SFRangeEmpty = { 0, 0 };

SF_INTERNAL SFBoolean SFRangeFallsInLength(SFRange range, SFUInteger maxLength)
{
    SFUInteger possibleLimit = range.start + range.count;

    return range.start < maxLength && range.start <= possibleLimit && possibleLimit <= maxLength;
}

SF_INTERNAL void SFRangeReduceToLength(SFRange *refRange, SFUInteger maxLength)
{
    if (refRange->start < maxLength) {
        SFUInteger possibleLimit = refRange->start + refRange->count;

        if (refRange->start <= possibleLimit && possibleLimit <= maxLength) {
            /* The range is valid. Nothing to do here. */
        } else {
            refRange->count = maxLength - refRange->start;
        }
    } else {
        *refRange = SFRangeEmpty;
    }
}

SFTextDirection SFScriptGetDefaultDirection(SFTag scriptTag)
{
    SFScriptKnowledgeRef knowledge = SFShapingKnowledgeSeekScript(&SFUnifiedKnowledgeInstance, scriptTag);
    if (knowledge != NULL) {
        return knowledge->defaultDirection;
    }

    return SFTextDirectionLeftToRight;
}