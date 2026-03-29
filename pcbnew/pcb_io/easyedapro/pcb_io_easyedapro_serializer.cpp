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

#include "pcb_io_easyedapro_serializer.h"

#include <board.h>
#include <board_design_settings.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <footprint.h>
#include <pad.h>
#include <zone.h>
#include <connectivity/connectivity_data.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/eda_angle.h>

#include <wx/string.h>
#include <wx/filename.h>
#include <wx/datetime.h>

#include <random>
#include <sstream>
#include <iomanip>


PCB_IO_EASYEDAPRO_SERIALIZER::PCB_IO_EASYEDAPRO_SERIALIZER( BOARD* aBoard )
        : m_board( aBoard ),
          m_settings( nullptr ),
          m_uuidCounter( 0 )
{
    if( m_board )
        m_settings = m_board->GetDesignSettings();
}


PCB_IO_EASYEDAPRO_SERIALIZER::~PCB_IO_EASYEDAPRO_SERIALIZER()
{
}


double PCB_IO_EASYEDAPRO_SERIALIZER::IuToMm( int aIu )
{
    return aIu / 1000000.0;
}


int PCB_IO_EASYEDAPRO_SERIALIZER::MmToIu( double aMm )
{
    return KiROUND( aMm * 1000000.0 );
}


VECTOR2D PCB_IO_EASYEDAPRO_SERIALIZER::ScalePos( const VECTOR2I& aPos ) const
{
    // KiCad uses Y-up, EasyEDA uses Y-down
    // Also convert from internal units (nm) to mm
    double x = IuToMm( aPos.x );
    double y = IuToMm( aPos.y );
    return VECTOR2D( x, -y );
}


double PCB_IO_EASYEDAPRO_SERIALIZER::ScaleSize( int aIu ) const
{
    return IuToMm( aIu );
}


int PCB_IO_EASYEDAPRO_SERIALIZER::LayerToEasyEDA( PCB_LAYER_ID aLayer ) const
{
    // Reverse mapping from PCB_IO_EASYEDAPRO_PARSER::LayerToKi()
    switch( aLayer )
    {
    case F_Cu:       return 1;
    case B_Cu:       return 2;
    case F_SilkS:    return 3;
    case B_SilkS:    return 4;
    case F_Mask:     return 5;
    case B_Mask:     return 6;
    case F_Paste:    return 7;
    case B_Paste:    return 8;
    case F_Fab:      return 9;
    case B_Fab:      return 10;
    case Edge_Cuts:  return 11;
    case Dwgs_User:  return 13;
    case Eco2_User:  return 14;
    case In1_Cu:     return 15;
    case In2_Cu:     return 16;
    case In3_Cu:     return 17;
    case In4_Cu:     return 18;
    case In5_Cu:     return 19;
    case In6_Cu:     return 20;
    case In7_Cu:     return 21;
    case In8_Cu:     return 22;
    case In9_Cu:     return 23;
    case In10_Cu:    return 24;
    case In11_Cu:    return 25;
    case In12_Cu:    return 26;
    case In13_Cu:    return 27;
    case In14_Cu:    return 28;
    case In15_Cu:    return 29;
    case In16_Cu:    return 30;
    case In17_Cu:    return 31;
    case In18_Cu:    return 32;
    case In19_Cu:    return 33;
    case In20_Cu:    return 34;
    case In21_Cu:    return 35;
    case In22_Cu:    return 36;
    case In23_Cu:    return 37;
    case In24_Cu:    return 38;
    case In25_Cu:    return 39;
    case In26_Cu:    return 40;
    case In27_Cu:    return 41;
    case In28_Cu:    return 42;
    case In29_Cu:    return 43;
    case In30_Cu:    return 44;
    case User_4:     return 53; // 3D shell outline
    case User_5:     return 54; // 3D shell top
    case User_6:     return 55; // 3D shell bottom
    case User_7:     return 56; // Drill drawing
    default:         return 0;
    }
}


wxString PCB_IO_EASYEDAPRO_SERIALIZER::GenerateUuid() const
{
    // Generate a simple UUID-like string (32 hex chars)
    int counter = ++m_uuidCounter;

    // Use timestamp and counter for uniqueness
    wxDateTime now = wxDateTime::Now();
    long long timestamp = now.GetValue().GetValue();

    wxString result;
    result.reserve( 32 );

    // Generate a simple hash-like string from timestamp and counter
    for( int i = 0; i < 32; i++ )
    {
        int value = ( ( timestamp >> ( i * 2 ) ) & 0xF ) + ( counter & 0xF ) + i;
        int digit = value % 16;

        if( digit < 10 )
            result += wxString::Format( wxS( "%d" ), digit );
        else
            result += wxString::Format( wxS( "%c" ), 'a' + digit - 10 );
    }

    return result;
}


BOX2I PCB_IO_EASYEDAPRO_SERIALIZER::GetBoardBoundingBox() const
{
    if( !m_board )
        return BOX2I();

    BOX2I bbox = m_board->GetBoardEdgesBoundingBox();

    if( bbox.GetWidth() == 0 || bbox.GetHeight() == 0 )
    {
        // Fallback to all items bounding box
        bbox = m_board->ComputeBoundingBox( false );
    }

    return bbox;
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::CreateDocType() const
{
    return nlohmann::json::array( { "DOCTYPE", "PCB", "1.7" } );
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::CreateHead() const
{
    nlohmann::json head;
    head[ "editorVersion" ] = "2.2.43.4";  // Compatible version
    head[ "importFlag" ] = 0;
    head[ "uuid" ] = GenerateUuid();
    return nlohmann::json::array( { "HEAD", head } );
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::CreateCanvas() const
{
    BOX2I bbox = GetBoardBoundingBox();

    // Convert to mm, add some margin
    double width = IuToMm( bbox.GetWidth() ) + 10;
    double height = IuToMm( bbox.GetHeight() ) + 10;

    // Center the canvas
    double originX = IuToMm( bbox.GetX() ) - 5;
    double originY = IuToMm( bbox.GetY() ) + IuToMm( bbox.GetHeight() ) + 5;

    return nlohmann::json::array( {
            "CANVAS",
            originX,  // origin X
            originY,  // origin Y (Y-down for EasyEDA)
            "mm",     // unit
            2,        // grid interval
            2,        // grid sub division
            2,        // grid interval X
            2,        // grid interval Y
            0.5,      // cursor snap size
            0.5,      // min cursor snap size
            0,        // X
            0,        // Y
            5         // grid type
    } );
}


std::vector<nlohmann::json> PCB_IO_EASYEDAPRO_SERIALIZER::CreateLayers() const
{
    // EasyEDA layer definitions
    // Format: ["LAYER", id, "name", "display_name", type, "#color", visible, active, width]
    std::vector<nlohmann::json> layers = {
            { "LAYER", 1, "TOP", "Top Layer", 3, "#ff0000", 1, "#7f0000", 0.5 },
            { "LAYER", 2, "BOTTOM", "Bottom Layer", 3, "#0000ff", 1, "#00007f", 0.5 },
            { "LAYER", 3, "TOP_SILK", "Top Silkscreen Layer", 3, "#ffcc00", 1, "#7f6600", 0.5 },
            { "LAYER", 4, "BOT_SILK", "Bottom Silkscreen Layer", 3, "#66cc33", 1, "#336619", 0.5 },
            { "LAYER", 5, "TOP_SOLDER_MASK", "Top Solder Mask Layer", 3, "#800080", 1, "#400040", 0.5 },
            { "LAYER", 6, "BOT_SOLDER_MASK", "Bottom Solder Mask Layer", 3, "#aa00ff", 1, "#55007f", 0.5 },
            { "LAYER", 7, "TOP_PASTE_MASK", "Top Paste Mask Layer", 3, "#808080", 1, "#404040", 0.5 },
            { "LAYER", 8, "BOT_PASTE_MASK", "Bottom Paste Mask Layer", 3, "#800000", 1, "#400000", 0.5 },
            { "LAYER", 9, "TOP_ASSEMBLY", "Top Assembly Layer", 3, "#33cc99", 1, "#19664c", 0.5 },
            { "LAYER", 10, "BOT_ASSEMBLY", "Bottom Assembly Layer", 3, "#5555ff", 1, "#2a2a7f", 0.5 },
            { "LAYER", 11, "OUTLINE", "Board Outline Layer", 3, "#ff00ff", 1, "#7f007f", 0.5 },
            { "LAYER", 12, "MULTI", "Multi-Layer", 3, "#c0c0c0", 1, "#606060", 0.5 },
            { "LAYER", 13, "DOCUMENT", "Document Layer", 3, "#ffffff", 1, "#7f7f7f", 0.5 },
            { "LAYER", 14, "MECHANICAL", "Mechanical Layer", 3, "#f022f0", 1, "#781178", 0.5 },
            { "LAYER", 47, "HOLE", "Hole Layer", 3, "#222222", 1, "#111111", 0.5 },
            { "LAYER", 57, "OTHER", "Ratline Layer", 7, "#6464ff", 1, "#32327f", 0.5 }
    };

    // Add internal copper layers if needed
    for( int i = 15; i <= 44; i++ )
    {
        int innerLayer = i - 14;
        if( innerLayer <= m_board->GetCopperLayerCount() - 2 )
        {
            layers.push_back( { "LAYER", i, "SIGNAL", wxString::Format( "Inner%d", innerLayer ).ToUTF8(),
                               0, "#999966", 1, "#4c4c33", 0.5 } );
        }
    }

    return layers;
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::SerializeTrack( const PCB_TRACK* aTrack )
{
    std::vector<nlohmann::json> result;

    if( !aTrack )
        return nlohmann::json::array();

    int layerId = LayerToEasyEDA( aTrack->GetLayer() );
    if( layerId == 0 )
        return nlohmann::json::array();

    VECTOR2D start = ScalePos( aTrack->GetStart() );
    VECTOR2D end = ScalePos( aTrack->GetEnd() );
    double width = ScaleSize( aTrack->GetWidth() );

    wxString netName = aTrack->GetNetname();
    wxString netId = netName.IsEmpty() ? "" : netName;

    if( aTrack->Type() == PCB_ARC_T )
    {
        const PCB_ARC* arc = static_cast<const PCB_ARC*>( aTrack );
        VECTOR2D mid = ScalePos( arc->GetMid() );
        // Calculate the arc angle from start, mid, and end points
        EDA_ANGLE angleStart = arc->GetArcAngleStart();
        EDA_ANGLE angleEnd = arc->GetArcAngleEnd();
        double angle = ( angleEnd - angleStart ).AsDegrees();

        result = { "TRACK", layerId, netId, width, start.x, start.y, mid.x, mid.y, end.x, end.y, angle };
    }
    else
    {
        result = { "TRACK", layerId, netId, width, start.x, start.y, end.x, end.y };
    }

    return nlohmann::json( result );
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::SerializeVia( const PCB_VIA* aVia )
{
    if( !aVia )
        return nlohmann::json::array();

    VECTOR2D pos = ScalePos( aVia->GetStart() );
    double diameter = ScaleSize( aVia->GetWidth() );
    double drill = ScaleSize( aVia->GetDrillValue() );

    // Via format: ["VIA", x, y, diameter, drill, net_id]
    wxString netName = aVia->GetNetname();

    return nlohmann::json::array( { "VIA", pos.x, pos.y, diameter, drill, netName.ToUTF8() } );
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::SerializePad( const PAD* aPad, const wxString& aParentUuid )
{
    if( !aPad )
        return nlohmann::json::array();

    VECTOR2D pos = ScalePos( aPad->GetPosition() );
    double rotation = aPad->GetOrientation().AsDegrees();
    int layerId = LayerToEasyEDA( aPad->GetLayer() );

    wxString padNumber = aPad->GetNumber();
    wxString netName = aPad->GetNetname();

    // Determine pad shape
    int shapeType = 0;  // 0=elliptical, 1=rect, 2=oval, 3=polygon
    switch( aPad->GetShape() )
    {
    case PAD_SHAPE::CIRCLE: shapeType = 0; break;
    case PAD_SHAPE::RECTANGLE: shapeType = 1; break;
    case PAD_SHAPE::OVAL: shapeType = 2; break;
    case PAD_SHAPE::CHAMFERED_RECT:
    case PAD_SHAPE::ROUNDRECT:
    case PAD_SHAPE::TRAPEZOID: shapeType = 1; break;
    case PAD_SHAPE::CUSTOM: shapeType = 3; break;
    default: shapeType = 0; break;
    }

    VECTOR2I size = aPad->GetSize();
    double sizeX = ScaleSize( size.x );
    double sizeY = ScaleSize( size.y );
    double drillSize = aPad->GetDrillSize().x > 0 ? ScaleSize( aPad->GetDrillSize().x ) : 0;

    // PAD format: ["PAD", id, hole_count, net_id, layer, number, x, y, rotation, shape, size_x, size_y,
    //               drill_size, paste_expansion_ratio, ...]
    return nlohmann::json::array( {
            "PAD",
            GenerateUuid(),  // pad id
            aPad->GetDrillSize().x > 0 ? 1 : 0,  // hole count
            netName.ToUTF8(),  // net id
            layerId,  // layer
            padNumber.ToUTF8(),  // number
            pos.x, pos.y,
            rotation,
            shapeType,
            sizeX, sizeY,
            drillSize,
            0.0,  // paste expansion ratio
            0.0,  // mask expansion ratio
            0.0,  // courtyard expansion ratio
            aParentUuid.ToUTF8()  // parent footprint id
    } );
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::SerializeZone( const ZONE* aZone )
{
    if( !aZone )
        return nlohmann::json::array();

    int layerId = LayerToEasyEDA( aZone->GetLayer() );
    if( layerId == 0 )
        return nlohmann::json::array();

    wxString netName = aZone->GetNetname();

    // Get the outline polygon
    const SHAPE_POLY_SET& outline = aZone->GetOutline();

    if( outline.OutlineCount() == 0 )
        return nlohmann::json::array();

    const SHAPE_LINE_CHAIN& poly = outline.Outline( 0 );
    std::vector<double> coords;

    for( int i = 0; i < poly.PointCount(); i++ )
    {
        VECTOR2I pt = poly.CPoint( i );
        VECTOR2D scaled = ScalePos( pt );
        coords.push_back( scaled.x );
        coords.push_back( scaled.y );
    }

    double width = ScaleSize( aZone->GetMinThickness() );

    // REGION format: ["REGION", layer_id, net_id, width, is_closed, coords..., "ggeXXX"]
    nlohmann::json result = { "REGION", layerId, netName.ToUTF8(), width, 1 };
    for( double coord : coords )
        result.push_back( coord );
    result.push_back( GenerateUuid() );

    return result;
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::SerializeText( const PCB_TEXT* aText )
{
    if( !aText )
        return nlohmann::json::array();

    int layerId = LayerToEasyEDA( aText->GetLayer() );
    if( layerId == 0 )
        return nlohmann::json::array();

    VECTOR2D pos = ScalePos( aText->GetPosition() );
    double rotation = aText->GetTextAngle().AsDegrees();
    double height = ScaleSize( aText->GetTextHeight() );
    double stroke = ScaleSize( aText->GetStroke().GetWidth() );

    wxString text = aText->GetText();

    // TEXT format: ["TEXT", layer_id, text, x, y, rotation, height, stroke, mirror, "ggeXXX"]
    return nlohmann::json::array( {
            "TEXT",
            layerId,
            text.ToUTF8(),
            pos.x, pos.y,
            rotation,
            height,
            stroke,
            aText->IsMirrored() ? 1 : 0,
            GenerateUuid()
    } );
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::SerializeShape( const PCB_SHAPE* aShape )
{
    if( !aShape )
        return nlohmann::json::array();

    int layerId = LayerToEasyEDA( aShape->GetLayer() );
    if( layerId == 0 )
        return nlohmann::json::array();

    double width = ScaleSize( aShape->GetWidth() );

    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    {
        VECTOR2D start = ScalePos( aShape->GetStart() );
        VECTOR2D end = ScalePos( aShape->GetEnd() );
        return nlohmann::json::array( { "TRACK", layerId, "", width, start.x, start.y, end.x, end.y } );
    }

    case SHAPE_T::CIRCLE:
    {
        VECTOR2D center = ScalePos( aShape->GetStart() );
        double radius = ScaleSize( aShape->GetRadius() );
        return nlohmann::json::array( { "CIRCLE", layerId, "", width, center.x, center.y, radius } );
    }

    case SHAPE_T::ARC:
    {
        VECTOR2D start = ScalePos( aShape->GetStart() );
        VECTOR2D end = ScalePos( aShape->GetEnd() );
        VECTOR2D mid = ScalePos( aShape->GetArcMid() );
        return nlohmann::json::array(
                { "ARC", layerId, "", width, start.x, start.y, mid.x, mid.y, end.x, end.y } );
    }

    case SHAPE_T::RECTANGLE:
    {
        VECTOR2D start = ScalePos( aShape->GetStart() );
        VECTOR2D end = ScalePos( aShape->GetEnd() );
        return nlohmann::json::array(
                { "RECT", layerId, "", width, start.x, start.y, end.x, end.y, GenerateUuid() } );
    }

    case SHAPE_T::POLY:
    {
        const SHAPE_POLY_SET& poly = aShape->GetPolyShape();
        if( poly.OutlineCount() == 0 )
            return nlohmann::json::array();

        std::vector<double> coords;
        const SHAPE_LINE_CHAIN& outline = poly.Outline( 0 );

        for( int i = 0; i < outline.PointCount(); i++ )
        {
            VECTOR2I pt = outline.CPoint( i );
            VECTOR2D scaled = ScalePos( pt );
            coords.push_back( scaled.x );
            coords.push_back( scaled.y );
        }

        nlohmann::json result = { "POLY", layerId, "", width, 1 };
        for( double coord : coords )
            result.push_back( coord );
        result.push_back( GenerateUuid() );

        return result;
    }

    default:
        break;
    }

    return nlohmann::json::array();
}


nlohmann::json PCB_IO_EASYEDAPRO_SERIALIZER::SerializeFootprintInstance( const FOOTPRINT* aFp )
{
    if( !aFp )
        return nlohmann::json::array();

    VECTOR2D pos = ScalePos( aFp->GetPosition() );
    double rotation = aFp->GetOrientation().AsDegrees();
    wxString ref = aFp->GetReference();

    // Footprint instance format: ["COMPONENT", id, x, y, rotation, side, locked, uuid]
    int side = (aFp->GetLayer() == F_Cu) ? 0 : 1;  // 0=top, 1=bottom

    return nlohmann::json::array( {
            "COMPONENT",
            GenerateUuid(),  // component id
            pos.x, pos.y,
            rotation,
            side,
            aFp->IsLocked() ? 1 : 0,
            GenerateUuid()  // footprint reference id
    } );
}


std::vector<nlohmann::json> PCB_IO_EASYEDAPRO_SERIALIZER::SerializeBoard()
{
    std::vector<nlohmann::json> lines;

    if( !m_board )
        return lines;

    // Add header entries
    lines.push_back( CreateDocType() );
    lines.push_back( CreateHead() );
    lines.push_back( CreateCanvas() );

    // Add layers
    for( const auto& layer : CreateLayers() )
        lines.push_back( layer );

    // Serialize all board items

    // 1. Tracks and arcs
    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            lines.push_back( SerializeVia( static_cast<PCB_VIA*>( track ) ) );
        }
        else
        {
            lines.push_back( SerializeTrack( track ) );
        }
    }

    // 2. Footprints and their pads
    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        // Add footprint instance
        lines.push_back( SerializeFootprintInstance( fp ) );

        // Add all pads
        wxString fpUuid = GenerateUuid();
        for( PAD* pad : fp->Pads() )
        {
            lines.push_back( SerializePad( pad, fpUuid ) );
        }

        // Add footprint graphics (text, shapes)
        for( BOARD_ITEM* item : fp->GraphicalItems() )
        {
            if( PCB_TEXT* text = dyn_cast<PCB_TEXT*>( item ) )
            {
                lines.push_back( SerializeText( text ) );
            }
            else if( PCB_SHAPE* shape = dyn_cast<PCB_SHAPE*>( item ) )
            {
                lines.push_back( SerializeShape( shape ) );
            }
        }

        // Reference and value text
        lines.push_back( SerializeText( &fp->Reference() ) );
        lines.push_back( SerializeText( &fp->Value() ) );
    }

    // 3. Zones
    for( ZONE* zone : m_board->Zones() )
    {
        lines.push_back( SerializeZone( zone ) );
    }

    // 4. Drawings (shapes and text on board)
    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( PCB_TEXT* text = dyn_cast<PCB_TEXT*>( item ) )
        {
            lines.push_back( SerializeText( text ) );
        }
        else if( PCB_SHAPE* shape = dyn_cast<PCB_SHAPE*>( item ) )
        {
            lines.push_back( SerializeShape( shape ) );
        }
    }

    // 5. Dimensions and other items
    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        // TODO: Add support for dimensions, targets, etc.
    }

    return lines;
}


std::vector<nlohmann::json> PCB_IO_EASYEDAPRO_SERIALIZER::SerializeFootprint(
        const FOOTPRINT* aFootprint )
{
    std::vector<nlohmann::json> lines;

    if( !aFootprint )
        return lines;

    // Add DOCTYPE for footprint
    lines.push_back( nlohmann::json::array( { "DOCTYPE", "FOOTPRINT", "1.7" } ) );

    // Add HEAD entry
    nlohmann::json head;
    head[ "editorVersion" ] = "2.2.43.4";
    head[ "importFlag" ] = 0;
    head[ "uuid" ] = GenerateUuid();
    head[ "source" ] = aFootprint->GetFPID().GetLibItemName().ToUTF8();
    head[ "title" ] = aFootprint->GetFPID().GetLibItemName().ToUTF8();
    lines.push_back( nlohmann::json::array( { "HEAD", head } ) );

    // Add canvas
    BOX2I bbox = aFootprint->GetBoundingBox( false, false );
    double width = IuToMm( bbox.GetWidth() ) + 2;
    double height = IuToMm( bbox.GetHeight() ) + 2;

    lines.push_back( nlohmann::json::array( {
            "CANVAS", 0, 0, "mm", 2, 2, 2, 2, 0.5, 0.5, 0, 0, 5
    } ) );

    // Add layers (simplified for footprint)
    std::vector<nlohmann::json> fpLayers = {
            { "LAYER", 1, "TOP", "Top Layer", 3, "#ff0000", 1, "#7f0000", 0.5 },
            { "LAYER", 11, "OUTLINE", "Board Outline Layer", 3, "#ff00ff", 1, "#7f007f", 0.5 },
            { "LAYER", 48, "COMPONENT_SHAPE", "Component Shape Layer", 3, "#00cccc", 1, "#006666", 0.5 },
            { "LAYER", 49, "COMPONENT_MARKING", "Component Marking Layer", 3, "#66ffcc", 1, "#337f66", 0.5 }
    };

    for( const auto& layer : fpLayers )
        lines.push_back( layer );

    // Add all pads
    wxString fpUuid = GenerateUuid();
    for( PAD* pad : aFootprint->Pads() )
    {
        lines.push_back( SerializePad( pad, fpUuid ) );
    }

    // Add graphical items
    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( PCB_TEXT* text = dyn_cast<PCB_TEXT*>( item ) )
        {
            lines.push_back( SerializeText( text ) );
        }
        else if( PCB_SHAPE* shape = dyn_cast<PCB_SHAPE*>( item ) )
        {
            lines.push_back( SerializeShape( shape ) );
        }
    }

    // Add reference/value placeholders
    lines.push_back( SerializeText( &aFootprint->Reference() ) );
    lines.push_back( SerializeText( &aFootprint->Value() ) );

    return lines;
}
