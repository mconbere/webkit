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

#ifndef AirInstInlines_h
#define AirInstInlines_h

#if ENABLE(B3_JIT)

#include "AirInst.h"
#include "AirOpcodeUtils.h"
#include "AirSpecial.h"
#include "AirStackSlot.h"
#include "B3Value.h"

namespace JSC { namespace B3 { namespace Air {

template<typename T> struct ForEach;
template<> struct ForEach<Tmp> {
    template<typename Functor>
    static void forEach(Inst& inst, const Functor& functor)
    {
        inst.forEachTmp(functor);
    }
};

template<> struct ForEach<Arg> {
    template<typename Functor>
    static void forEach(Inst& inst, const Functor& functor)
    {
        inst.forEachArg(functor);
    }
};

template<> struct ForEach<StackSlot*> {
    template<typename Functor>
    static void forEach(Inst& inst, const Functor& functor)
    {
        inst.forEachArg(
            [&] (Arg& arg, Arg::Role role, Arg::Type type, Arg::Width width) {
                if (!arg.isStack())
                    return;
                StackSlot* stackSlot = arg.stackSlot();

                // FIXME: This is way too optimistic about the meaning of "Def". It gets lucky for
                // now because our only use of "Anonymous" stack slots happens to want the optimistic
                // semantics. We could fix this by just changing the comments that describe the
                // semantics of "Anonymous".
                // https://bugs.webkit.org/show_bug.cgi?id=151128
                
                functor(stackSlot, role, type, width);
                arg = Arg::stack(stackSlot, arg.offset());
            });
    }
};

template<> struct ForEach<Reg> {
    template<typename Functor>
    static void forEach(Inst& inst, const Functor& functor)
    {
        inst.forEachTmp(
            [&] (Tmp& tmp, Arg::Role role, Arg::Type type, Arg::Width width) {
                if (!tmp.isReg())
                    return;

                Reg reg = tmp.reg();
                functor(reg, role, type, width);
                tmp = Tmp(reg);
            });
    }
};

template<typename Thing, typename Functor>
void Inst::forEach(const Functor& functor)
{
    ForEach<Thing>::forEach(*this, functor);
}

inline const RegisterSet& Inst::extraClobberedRegs()
{
    ASSERT(opcode == Patch);
    return args[0].special()->extraClobberedRegs(*this);
}

inline const RegisterSet& Inst::extraEarlyClobberedRegs()
{
    ASSERT(opcode == Patch);
    return args[0].special()->extraEarlyClobberedRegs(*this);
}

template<typename Thing, typename Functor>
inline void Inst::forEachDef(Inst* prevInst, Inst* nextInst, const Functor& functor)
{
    if (prevInst) {
        prevInst->forEach<Thing>(
            [&] (Thing& thing, Arg::Role role, Arg::Type argType, Arg::Width argWidth) {
                if (Arg::isLateDef(role))
                    functor(thing, role, argType, argWidth);
            });
    }

    if (nextInst) {
        nextInst->forEach<Thing>(
            [&] (Thing& thing, Arg::Role role, Arg::Type argType, Arg::Width argWidth) {
                if (Arg::isEarlyDef(role))
                    functor(thing, role, argType, argWidth);
            });
    }
}

template<typename Thing, typename Functor>
inline void Inst::forEachDefWithExtraClobberedRegs(
    Inst* prevInst, Inst* nextInst, const Functor& functor)
{
    forEachDef<Thing>(prevInst, nextInst, functor);

    Arg::Role regDefRole;
    
    auto reportReg = [&] (Reg reg) {
        Arg::Type type = reg.isGPR() ? Arg::GP : Arg::FP;
        functor(Thing(reg), regDefRole, type, Arg::conservativeWidth(type));
    };

    if (prevInst && prevInst->opcode == Patch) {
        regDefRole = Arg::Def;
        prevInst->extraClobberedRegs().forEach(reportReg);
    }

    if (nextInst && nextInst->opcode == Patch) {
        regDefRole = Arg::EarlyDef;
        nextInst->extraEarlyClobberedRegs().forEach(reportReg);
    }
}

inline void Inst::reportUsedRegisters(const RegisterSet& usedRegisters)
{
    ASSERT(opcode == Patch);
    args[0].special()->reportUsedRegisters(*this, usedRegisters);
}

inline bool Inst::admitsStack(Arg& arg)
{
    return admitsStack(&arg - &args[0]);
}

inline Optional<unsigned> Inst::shouldTryAliasingDef()
{
    if (!isX86())
        return Nullopt;

    switch (opcode) {
    case Add32:
    case Add64:
    case And32:
    case And64:
    case Mul32:
    case Mul64:
    case Or32:
    case Or64:
    case Xor32:
    case Xor64:
    case AddDouble:
    case AddFloat:
    case AndFloat:
    case AndDouble:
    case MulDouble:
    case MulFloat:
    case XorDouble:
    case XorFloat:
        if (args.size() == 3)
            return 2;
        break;
    case BranchAdd32:
    case BranchAdd64:
        if (args.size() == 4)
            return 3;
        break;
    case MoveConditionally32:
    case MoveConditionally64:
    case MoveConditionallyTest32:
    case MoveConditionallyTest64:
    case MoveConditionallyDouble:
    case MoveConditionallyFloat:
        if (args.size() == 6)
            return 5;
        break;
        break;
    case Patch:
        return PatchCustom::shouldTryAliasingDef(*this);
    default:
        break;
    }
    return Nullopt;
}

inline bool isShiftValid(const Inst& inst)
{
#if CPU(X86) || CPU(X86_64)
    return inst.args[0] == Tmp(X86Registers::ecx);
#else
    UNUSED_PARAM(inst);
    return true;
#endif
}

inline bool isLshift32Valid(const Inst& inst)
{
    return isShiftValid(inst);
}

inline bool isLshift64Valid(const Inst& inst)
{
    return isShiftValid(inst);
}

inline bool isRshift32Valid(const Inst& inst)
{
    return isShiftValid(inst);
}

inline bool isRshift64Valid(const Inst& inst)
{
    return isShiftValid(inst);
}

inline bool isUrshift32Valid(const Inst& inst)
{
    return isShiftValid(inst);
}

inline bool isUrshift64Valid(const Inst& inst)
{
    return isShiftValid(inst);
}

inline bool isX86DivHelperValid(const Inst& inst)
{
#if CPU(X86) || CPU(X86_64)
    return inst.args[0] == Tmp(X86Registers::eax)
        && inst.args[1] == Tmp(X86Registers::edx);
#else
    UNUSED_PARAM(inst);
    return false;
#endif
}

inline bool isX86ConvertToDoubleWord32Valid(const Inst& inst)
{
    return isX86DivHelperValid(inst);
}

inline bool isX86ConvertToQuadWord64Valid(const Inst& inst)
{
    return isX86DivHelperValid(inst);
}

inline bool isX86Div32Valid(const Inst& inst)
{
    return isX86DivHelperValid(inst);
}

inline bool isX86Div64Valid(const Inst& inst)
{
    return isX86DivHelperValid(inst);
}

} } } // namespace JSC::B3::Air

#endif // ENABLE(B3_JIT)

#endif // AirInstInlines_h

