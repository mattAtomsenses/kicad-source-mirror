/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SCH_EASYEDAPRO_SERIALIZER_H_
#define SCH_EASYEDAPRO_SERIALIZER_H_

#include <nlohmann/json_fwd.hpp>
#include <vector>

#include <wx/string.h>

#include <math/vector2d.h>

class SCHEMATIC;
class SCH_SHEET;
class SCH_SYMBOL;
class SCH_LINE;
class SCH_JUNCTION;
class SCH_LABEL;
class SCH_FIELD;
class LIB_SYMBOL;


class SCH_EASYEDAPRO_SERIALIZER
{
public:
    explicit SCH_EASYEDAPRO_SERIALIZER( SCHEMATIC* aSchematic );
    ~SCH_EASYEDAPRO_SERIALIZER();

    /**
     * Serialize a schematic sheet to EasyEDA Pro JSON format.
     */
    std::vector<nlohmann::json> SerializeSchematic( SCH_SHEET* aSheet );

    /**
     * Serialize a symbol to EasyEDA Pro format (.esym).
     */
    std::vector<nlohmann::json> SerializeSymbol( const LIB_SYMBOL* aSymbol );

    /**
     * Convert KiCad internal units to EasyEDA units (mils * 10).
     */
    static double IuToEasyeda( int aIu );

    /**
     * Convert EasyEDA units to KiCad internal units.
     */
    static int EasyedaToIu( double aValue );

    /**
     * Scale position from KiCad to EasyEDA coordinate system.
     * EasyEDA uses Y-down, KiCad uses Y-up.
     */
    VECTOR2D ScalePos( const VECTOR2I& aPos ) const;

    /**
     * Scale size from KiCad to EasyEDA.
     */
    double ScaleSize( int aIu ) const;

private:
    /**
     * Serialize a symbol instance on the schematic.
     */
    nlohmann::json SerializeSymbolInstance( const SCH_SYMBOL* aSymbol );

    /**
     * Serialize a wire/bus.
     */
    nlohmann::json SerializeWire( const SCH_LINE* aLine );

    /**
     * Serialize a junction.
     */
    nlohmann::json SerializeJunction( const SCH_JUNCTION* aJunction );

    /**
     * Serialize a label (hierarchical, global, or net label).
     */
    nlohmann::json SerializeLabel( const SCH_LABEL* aLabel );

    /**
     * Serialize a field (reference, value, user fields).
     */
    nlohmann::json SerializeField( const SCH_FIELD* aField, const wxString& aParentId );

    /**
     * Serialize a schematic sheet (hierarchical sheet).
     */
    nlohmann::json SerializeSheet( const SCH_SHEET* aSheet );

    /**
     * Create the DOCTYPE entry.
     */
    nlohmann::json CreateDocType() const;

    /**
     * Create the HEAD entry.
     */
    nlohmann::json CreateHead() const;

    /**
     * Create the CANVAS entry.
     */
    nlohmann::json CreateCanvas() const;

    /**
     * Generate a unique ID for EasyEDA objects.
     */
    wxString GenerateUuid() const;

    SCHEMATIC* m_schematic;
    mutable int m_uuidCounter;
};


#endif // SCH_EASYEDAPRO_SERIALIZER_H_
