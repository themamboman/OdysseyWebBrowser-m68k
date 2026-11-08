/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include "FilterEffect.h"

#include "Filter.h"
#include <wtf/Vector.h>

namespace WebCore {

enum ComponentTransferType {
    FECOMPONENTTRANSFER_TYPE_UNKNOWN  = 0,
    FECOMPONENTTRANSFER_TYPE_IDENTITY = 1,
    FECOMPONENTTRANSFER_TYPE_TABLE    = 2,
    FECOMPONENTTRANSFER_TYPE_DISCRETE = 3,
    FECOMPONENTTRANSFER_TYPE_LINEAR   = 4,
    FECOMPONENTTRANSFER_TYPE_GAMMA    = 5
};

struct ComponentTransferFunction {
    ComponentTransferFunction() = default;
    /* A class with any user-declared constructor (even the defaulted
     * default constructor above) is not an aggregate, so brace-init at
     * SVGComponentTransferFunctionElement.cpp's call site can't use
     * aggregate-initialization syntax -- some older GCC versions
     * tolerated this anyway as a non-conforming extension; this one
     * doesn't (same issue as CanvasStyle::CMYKAColor -- see
     * WebCore_status.md). Explicit matching constructor instead. */
    ComponentTransferFunction(ComponentTransferType type, float slope, float intercept, float amplitude, float exponent, float offset, Vector<float> tableValues)
        : type(type), slope(slope), intercept(intercept), amplitude(amplitude), exponent(exponent), offset(offset), tableValues(WTFMove(tableValues))
    {
    }

    ComponentTransferType type { FECOMPONENTTRANSFER_TYPE_UNKNOWN };

    float slope { 0 };
    float intercept { 0 };
    float amplitude { 0 };
    float exponent { 0 };
    float offset { 0 };

    Vector<float> tableValues;
};

class FEComponentTransfer : public FilterEffect {
public:
    static Ref<FEComponentTransfer> create(Filter&, const ComponentTransferFunction& redFunc, const ComponentTransferFunction& greenFunc,
                                           const ComponentTransferFunction& blueFunc, const ComponentTransferFunction& alphaFunc);

    ComponentTransferFunction redFunction() const { return m_redFunction; }
    ComponentTransferFunction greenFunction() const { return m_greenFunction; }
    ComponentTransferFunction blueFunction() const { return m_blueFunction; }
    ComponentTransferFunction alphaFunction() const { return m_alphaFunction; }

private:
    FEComponentTransfer(Filter&, const ComponentTransferFunction& redFunc, const ComponentTransferFunction& greenFunc,
                        const ComponentTransferFunction& blueFunc, const ComponentTransferFunction& alphaFunc);

     const char* filterName() const final { return "FEComponentTransfer"; }

    using LookupTable = std::array<uint8_t, 256>;

    static void computeIdentityTable(LookupTable&, const ComponentTransferFunction&);
    static void computeTabularTable(LookupTable&, const ComponentTransferFunction&);
    static void computeDiscreteTable(LookupTable&, const ComponentTransferFunction&);
    static void computeLinearTable(LookupTable&, const ComponentTransferFunction&);
    static void computeGammaTable(LookupTable&, const ComponentTransferFunction&);

    void computeLookupTables(LookupTable& redTable, LookupTable& greenTable, LookupTable& blueTable, LookupTable& alphaTable);

    void platformApplySoftware() override;

    WTF::TextStream& externalRepresentation(WTF::TextStream&, RepresentationType) const override;

    ComponentTransferFunction m_redFunction;
    ComponentTransferFunction m_greenFunction;
    ComponentTransferFunction m_blueFunction;
    ComponentTransferFunction m_alphaFunction;
};

} // namespace WebCore

