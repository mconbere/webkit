/*
 * Copyright (C) 2015-2016 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ElementAndTextDescendantIterator.h"
#include "ShadowRoot.h"

#ifndef ComposedTreeIterator_h
#define ComposedTreeIterator_h

namespace WebCore {

class HTMLSlotElement;

class ComposedTreeIterator {
public:
    ComposedTreeIterator();
    ComposedTreeIterator(ContainerNode& root);
    ComposedTreeIterator(ContainerNode& root, Node& current);

    Node& operator*() { return current(); }
    Node* operator->() { return &current(); }

    bool operator==(const ComposedTreeIterator& other) const { return context().iterator == other.context().iterator; }
    bool operator!=(const ComposedTreeIterator& other) const { return context().iterator != other.context().iterator; }

    ComposedTreeIterator& operator++() { return traverseNext(); }

    ComposedTreeIterator& traverseNext();
    ComposedTreeIterator& traverseNextSkippingChildren();
    ComposedTreeIterator& traverseNextSibling();
    ComposedTreeIterator& traversePreviousSibling();

    unsigned depth() const;

private:
    void initializeContextStack(ContainerNode& root, Node& current);
    void traverseNextInShadowTree();
    void traverseNextLeavingContext();
    bool pushContext(ShadowRoot&);
#if ENABLE(SHADOW_DOM) || ENABLE(DETAILS_ELEMENT)
    bool advanceInSlot(int direction);
    void traverseSiblingInSlot(int direction);
#endif

    struct Context {
        Context() { }
        explicit Context(ContainerNode& root)
            : iterator(root)
        { }
        Context(ContainerNode& root, Node& node, size_t slotNodeIndex = notFound)
            : iterator(root, &node)
            , slotNodeIndex(slotNodeIndex)
        { }

        ElementAndTextDescendantIterator iterator;
        size_t slotNodeIndex { notFound };
    };
    Context& context() { return m_contextStack.last(); }
    const Context& context() const { return m_contextStack.last(); }
    Node& current() { return *context().iterator; }

    Vector<Context, 4> m_contextStack;
};

inline ComposedTreeIterator::ComposedTreeIterator()
{
    m_contextStack.uncheckedAppend({ });
}

inline ComposedTreeIterator& ComposedTreeIterator::traverseNext()
{
    if (auto* shadowRoot = context().iterator->shadowRoot()) {
        if (pushContext(*shadowRoot))
            return *this;
    }

    if (m_contextStack.size() > 1) {
        traverseNextInShadowTree();
        return *this;
    }

    context().iterator.traverseNext();
    return *this;
}

inline ComposedTreeIterator& ComposedTreeIterator::traverseNextSkippingChildren()
{
    context().iterator.traverseNextSkippingChildren();

    if (!context().iterator && m_contextStack.size() > 1)
        traverseNextLeavingContext();
    
    return *this;
}

inline ComposedTreeIterator& ComposedTreeIterator::traverseNextSibling()
{
#if ENABLE(SHADOW_DOM) || ENABLE(DETAILS_ELEMENT)
    if (current().parentNode()->shadowRoot()) {
        traverseSiblingInSlot(1);
        return *this;
    }
#endif
    context().iterator.traverseNextSibling();
    return *this;
}

inline ComposedTreeIterator& ComposedTreeIterator::traversePreviousSibling()
{
#if ENABLE(SHADOW_DOM) || ENABLE(DETAILS_ELEMENT)
    if (current().parentNode()->shadowRoot()) {
        traverseSiblingInSlot(-1);
        return *this;
    }
#endif
    context().iterator.traversePreviousSibling();
    return *this;
}

inline unsigned ComposedTreeIterator::depth() const
{
    unsigned depth = 0;
    for (auto& context : m_contextStack)
        depth += context.iterator.depth();
    return depth;
}

class ComposedTreeDescendantAdapter {
public:
    ComposedTreeDescendantAdapter(ContainerNode& parent)
        : m_parent(parent)
    { }

    ComposedTreeIterator begin() { return ComposedTreeIterator(m_parent); }
    ComposedTreeIterator end() { return { }; }
    ComposedTreeIterator at(const Node& child) { return ComposedTreeIterator(m_parent, const_cast<Node&>(child)); }
    
private:
    ContainerNode& m_parent;
};

class ComposedTreeChildAdapter {
public:
    class Iterator : public ComposedTreeIterator {
    public:
        Iterator() = default;
        explicit Iterator(ContainerNode& root)
            : ComposedTreeIterator(root)
        { }
        Iterator(ContainerNode& root, Node& current)
            : ComposedTreeIterator(root, current)
        { }

        Iterator& operator++() { return static_cast<Iterator&>(traverseNextSibling()); }
        Iterator& operator--() { return static_cast<Iterator&>(traversePreviousSibling()); }
    };

    ComposedTreeChildAdapter(ContainerNode& parent)
        : m_parent(parent)
    { }

    Iterator begin() { return Iterator(m_parent); }
    Iterator end() { return { }; }
    Iterator at(const Node& child) { return Iterator(m_parent, const_cast<Node&>(child)); }

private:
    ContainerNode& m_parent;
};

// FIXME: We should have const versions too.
inline ComposedTreeDescendantAdapter composedTreeDescendants(ContainerNode& parent)
{
    return ComposedTreeDescendantAdapter(parent);
}

inline ComposedTreeChildAdapter composedTreeChildren(ContainerNode& parent)
{
    return ComposedTreeChildAdapter(parent);
}

}

#endif
