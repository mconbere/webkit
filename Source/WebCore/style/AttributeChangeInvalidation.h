/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AttributeChangeInvalidation_h
#define AttributeChangeInvalidation_h

#include "Element.h"

namespace WebCore {

class RuleSet;

namespace Style {

class AttributeChangeInvalidation {
public:
    AttributeChangeInvalidation(Element&, const QualifiedName&, const AtomicString& oldValue, const AtomicString& newValue);
    ~AttributeChangeInvalidation();

private:
    void invalidateStyle(const QualifiedName&, const AtomicString& oldValue, const AtomicString& newValue);
    void invalidateDescendants();

    const bool m_isEnabled;
    Element& m_element;

    RuleSet* m_descendantInvalidationRuleSet { nullptr };
};

inline AttributeChangeInvalidation::AttributeChangeInvalidation(Element& element, const QualifiedName& attributeName, const AtomicString& oldValue, const AtomicString& newValue)
    : m_isEnabled(element.needsStyleInvalidation())
    , m_element(element)
{
    if (!m_isEnabled)
        return;
    invalidateStyle(attributeName, oldValue, newValue);
    invalidateDescendants();
}

inline AttributeChangeInvalidation::~AttributeChangeInvalidation()
{
    if (!m_isEnabled)
        return;
    invalidateDescendants();
}
    
}
}

#endif

