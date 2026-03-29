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

#ifndef PCB_IO_EASYEDAPRO_SERIALIZER_H_
#define PCB_IO_EASYEDAPRO_SERIALIZER_H_

#include <nlohmann/json_fwd.hpp>
#include <vector>
#include <map>

#include <wx/string.h>

#include <math/vector2d.h>
#include <geometry/eda_angle.h>
#include <geometry/shape_poly_set.h>
#include <layer_ids.h>

class BOARD;
class BOARD_ITEM;
class PCB_TRACK;
class PCB_VIA;
class PAD;
class FOOTPRINT;
class ZONE;
class PCB_TEXT;
class PCB_SHAPE;
class BOARD_DESIGN_SETTINGS;


class PCB_IO_EASYEDAPRO_SERIALIZER
{
public:
    explicit PCB_IO_EASYEDAPRO_SERIALIZER( BOARD* aBoard );
    ~PCB_IO_EASYEDAPRO_SERIALIZER();

    /**
     * Serialize the entire board to EasyEDA Pro JSON format.
     * Returns a vector of JSON arrays representing the PCB file.
     */
    std::vector<nlohmann::json> SerializeBoard();

    /**
     * Serialize a footprint to EasyEDA Pro format (.efoo).
     */
    std::vector<nlohmann::json> SerializeFootprint( const FOOTPRINT* aFootprint );

    /**
     * Convert a KiCad layer ID to EasyEDA layer ID.
     */
    int LayerToEasyEDA( PCB_LAYER_ID aLayer ) const;

    /**
     * Convert KiCad internal units (nm) to EasyEDA mm.
     */
    static double IuToMm( int aIu );

    /**
     * Convert EasyEDA mm to KiCad internal units.
     */
    static int MmToIu( double aMm );

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
     * Serialize a track/arc to EasyEDA format.
     */
    nlohmann::json SerializeTrack( const PCB_TRACK* aTrack );

    /**
     * Serialize a via to EasyEDA format.
     */
    nlohmann::json SerializeVia( const PCB_VIA* aVia );

    /**
     * Serialize a pad to EasyEDA format.
     */
    nlohmann::json SerializePad( const PAD* aPad, const wxString& aParentUuid );

    /**
     * Serialize a zone/polygon to EasyEDA format.
     */
    nlohmann::json SerializeZone( const ZONE* aZone );

    /**
     * Serialize text to EasyEDA format.
     */
    nlohmann::json SerializeText( const PCB_TEXT* aText );

    /**
     * Serialize a shape (line, circle, arc, polygon) to EasyEDA format.
     */
    nlohmann::json SerializeShape( const PCB_SHAPE* aShape );

    /**
     * Serialize a footprint instance placement.
     */
    nlohmann::json SerializeFootprintInstance( const FOOTPRINT* aFp );

    /**
     * Generate a unique ID for EasyEDA objects.
     */
    wxString GenerateUuid() const;

    /**
     * Get the board bounding box for canvas sizing.
     */
    BOX2I GetBoardBoundingBox() const;

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
     * Create layer definitions.
     */
    std::vector<nlohmann::json> CreateLayers() const;

    BOARD*                 m_board;
    BOARD_DESIGN_SETTINGS* m_settings;
    mutable int            m_uuidCounter;
    BOX2D                  m_canvasBounds;
};


#endif // PCB_IO_EASYEDAPRO_SERIALIZER_H_
