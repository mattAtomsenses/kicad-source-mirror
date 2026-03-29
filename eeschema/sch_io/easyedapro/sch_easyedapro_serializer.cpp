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

#include "sch_easyedapro_serializer.h"

#include <schematic.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_text.h>
#include <lib_symbol.h>
#include <sch_pin.h>
#include <sch_item.h>

#include <wx/string.h>
#include <wx/filename.h>
#include <wx/datetime.h>

#include <random>


SCH_EASYEDAPRO_SERIALIZER::SCH_EASYEDAPRO_SERIALIZER( SCHEMATIC* aSchematic )
        : m_schematic( aSchematic ),
          m_uuidCounter( 0 )
{
}


SCH_EASYEDAPRO_SERIALIZER::~SCH_EASYEDAPRO_SERIALIZER()
{
}


double SCH_EASYEDAPRO_SERIALIZER::IuToEasyeda( int aIu )
{
    // EasyEDA schematic uses 10-mil units (same as KiCad internal units)
    return aIu / 1.0;
}


int SCH_EASYEDAPRO_SERIALIZER::EasyedaToIu( double aValue )
{
    return KiROUND( aValue );
}


VECTOR2D SCH_EASYEDAPRO_SERIALIZER::ScalePos( const VECTOR2I& aPos ) const
{
    // KiCad schematic uses Y-up, EasyEDA uses Y-down
    return VECTOR2D( IuToEasyeda( aPos.x ), -IuToEasyeda( aPos.y ) );
}


double SCH_EASYEDAPRO_SERIALIZER::ScaleSize( int aIu ) const
{
    return IuToEasyeda( aIu );
}


wxString SCH_EASYEDAPRO_SERIALIZER::GenerateUuid() const
{
    // Generate a simple UUID-like string (32 hex chars)
    int      counter = ++m_uuidCounter;
    wxString result;

    for( int i = 0; i < 32; i++ )
    {
        int digit = ( counter + i * 7 ) % 16;
        if( digit < 10 )
            result += wxString::Format( wxS( "%d" ), digit );
        else
            result += wxString::Format( wxS( "%c" ), 'a' + digit - 10 );
    }

    return result;
}


nlohmann::json SCH_EASYEDAPRO_SERIALIZER::CreateDocType() const
{
    return nlohmann::json::array( { "DOCTYPE", "SCHEMATIC", "1.7" } );
}


nlohmann::json SCH_EASYEDAPRO_SERIALIZER::CreateHead() const
{
    nlohmann::json head;
    head[ "editorVersion" ] = "2.2.43.4";
    head[ "importFlag" ] = 0;
    head[ "uuid" ] = GenerateUuid();
    return nlohmann::json::array( { "HEAD", head } );
}


nlohmann::json SCH_EASYEDAPRO_SERIALIZER::CreateCanvas() const
{
    // Default canvas size (can be adjusted based on sheet content)
    return nlohmann::json::array( {
            "CANVAS", 1200, 1200, "#FFFFFF", true, "#CCCCCC", 10, 1200, 1200, "line", 10, "pixel", 5, 400,
            300
    } );
}


nlohmann::json SCH_EASYEDAPRO_SERIALIZER::SerializeSymbolInstance( const SCH_SYMBOL* aSymbol )
{
    if( !aSymbol )
        return nlohmann::json::array();

    VECTOR2D pos = ScalePos( aSymbol->GetPosition() );
    double rotation = aSymbol->GetOrientation().AsDegrees();

    wxString libId = aSymbol->GetLibId().GetLibItemName().ToUTF8();
    wxString ref = aSymbol->GetRefDes();

    // SYMBOL instance format: ["SYMBOL_INSTANCE", id, x, y, rotation, mirror, ...]
    return nlohmann::json::array( {
            "SYMBOL_INSTANCE",
            GenerateUuid(),  // instance id
            pos.x, pos.y,
            rotation,
            aSymbol->IsMirroredX() || aSymbol->IsMirroredY() ? 1 : 0,
            libId.ToUTF8(),
            ref.ToUTF8()
    } );
}


nlohmann::json SCH_EASYEDAPRO_SERIALIZER::SerializeWire( const SCH_LINE* aLine )
{
    if( !aLine )
        return nlohmann::json::array();

    VECTOR2D start = ScalePos( aLine->GetStartPoint() );
    VECTOR2D end = ScalePos( aLine->GetEndPoint() );
    int width = ScaleSize( aLine->GetStroke().GetWidth() );

    wxString type;
    if( aLine->GetLayer() == LAYER_BUS )
        type = wxS( "BUS" );
    else if( aLine->GetStroke().GetType() == STROKE_DOT_DASH )
        type = wxS( "WIRE_DASH" );
    else
        type = wxS( "WIRE" );

    // WIRE format: ["WIRE", x1, y1, x2, y2, ...]
    return nlohmann::json::array( { type, start.x, start.y, end.x, end.y } );
}


nlohmann::json SCH_EASYEDAPRO_SERIALIZER::SerializeJunction( const SCH_JUNCTION* aJunction )
{
    if( !aJunction )
        return nlohmann::json::array();

    VECTOR2D pos = ScalePos( aJunction->GetPosition() );

    // JUNCTION format: ["JUNCTION", x, y]
    return nlohmann::json::array( { "JUNCTION", pos.x, pos.y } );
}


nlohmann::json SCH_EASYEDAPRO_SERIALIZER::SerializeLabel( const SCH_LABEL* aLabel )
{
    if( !aLabel )
        return nlohmann::json::array();

    VECTOR2D pos = ScalePos( aLabel->GetPosition() );
    double rotation = aLabel->GetTextAngle().AsDegrees();
    wxString text = aLabel->GetText();

    int labelType = 0;  // 0=net, 1=hierarchical, 2=global

    if( dynamic_cast<const SCH_HIERLABEL*>( aLabel ) )
        labelType = 1;
    else if( dynamic_cast<const SCH_GLOBALLABEL*>( aLabel ) )
        labelType = 2;

    // LABEL format: ["LABEL", type, x, y, rotation, text, ...]
    return nlohmann::json::array( { "LABEL", labelType, pos.x, pos.y, rotation, text.ToUTF8() } );
}


nlohmann::json SCH_EASYEDAPRO_SERIALIZER::SerializeField( const SCH_FIELD* aField,
                                                          const wxString& aParentId )
{
    if( !aField )
        return nlohmann::json::array();

    VECTOR2D pos = ScalePos( aField->GetPosition() );
    wxString text = aField->GetText();
    double rotation = aField->GetTextAngle().AsDegrees();
    int height = ScaleSize( aField->GetTextHeight() );

    // TEXT format: ["TEXT", layer, text, x, y, rotation, height, ...]
    return nlohmann::json::array( {
            "TEXT", 0, text.ToUTF8(), pos.x, pos.y, rotation, height, aField->IsVisible() ? 1 : 0,
            aParentId.ToUTF8()
    } );
}


nlohmann::json SCH_EASYEDAPRO_SERIALIZER::SerializeSheet( const SCH_SHEET* aSheet )
{
    if( !aSheet )
        return nlohmann::json::array();

    VECTOR2D pos = ScalePos( aSheet->GetPosition() );
    VECTOR2I size = aSheet->GetSize();

    // SHEET format: ["SHEET", id, x, y, width, height, name, filename]
    return nlohmann::json::array( {
            "SHEET",
            GenerateUuid(),
            pos.x, pos.y,
            ScaleSize( size.x ), ScaleSize( size.y ),
            aSheet->GetName().ToUTF8(),
            aSheet->GetFileName().ToUTF8()
    } );
}


std::vector<nlohmann::json> SCH_EASYEDAPRO_SERIALIZER::SerializeSchematic( SCH_SHEET* aSheet )
{
    std::vector<nlohmann::json> lines;

    if( !aSheet || !m_schematic )
        return lines;

    // Add header entries
    lines.push_back( CreateDocType() );
    lines.push_back( CreateHead() );
    lines.push_back( CreateCanvas() );

    // Get the screen for this sheet
    SCH_SCREEN* screen = aSheet->GetScreen();
    if( !screen )
        return lines;

    // Serialize all items in the schematic

    // 1. Symbols
    for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        lines.push_back( SerializeSymbolInstance( symbol ) );

        // Add symbol fields
        for( SCH_FIELD* field : symbol->GetFields() )
        {
            if( field->IsVisible() )
                lines.push_back( SerializeField( field, GenerateUuid() ) );
        }
    }

    // 2. Wires and buses
    for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );
        lines.push_back( SerializeWire( line ) );
    }

    // 3. Junctions
    for( SCH_ITEM* item : screen->Items().OfType( SCH_JUNCTION_T ) )
    {
        SCH_JUNCTION* junction = static_cast<SCH_JUNCTION*>( item );
        lines.push_back( SerializeJunction( junction ) );
    }

    // 4. Labels
    for( SCH_ITEM* item : screen->Items() )
    {
        if( SCH_LABEL* label = dyn_cast<SCH_LABEL*>( item ) )
        {
            lines.push_back( SerializeLabel( label ) );
        }
    }

    // 5. Hierarchical sheets
    for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
        lines.push_back( SerializeSheet( sheet ) );
    }

    // 6. Text items
    for( SCH_ITEM* item : screen->Items().OfType( SCH_TEXT_T ) )
    {
        SCH_TEXT* text = static_cast<SCH_TEXT*>( item );
        VECTOR2D pos = ScalePos( text->GetPosition() );
        double rotation = text->GetTextAngle().AsDegrees();
        int height = ScaleSize( text->GetTextHeight() );

        lines.push_back( nlohmann::json::array( {
                "TEXT", 0, text->GetText().ToUTF8(), pos.x, pos.y, rotation, height,
                text->IsVisible() ? 1 : 0, GenerateUuid()
        } ) );
    }

    return lines;
}


std::vector<nlohmann::json> SCH_EASYEDAPRO_SERIALIZER::SerializeSymbol( const LIB_SYMBOL* aSymbol )
{
    std::vector<nlohmann::json> lines;

    if( !aSymbol )
        return lines;

    // Add DOCTYPE for symbol
    lines.push_back( nlohmann::json::array( { "DOCTYPE", "SYMBOL", "1.7" } ) );

    // Add HEAD entry
    nlohmann::json head;
    head[ "editorVersion" ] = "2.2.43.4";
    head[ "importFlag" ] = 0;
    head[ "uuid" ] = GenerateUuid();
    head[ "source" ] = aSymbol->GetName().ToUTF8();
    head[ "title" ] = aSymbol->GetName().ToUTF8();
    head[ "type" ] = 2;  // NORMAL symbol type
    lines.push_back( nlohmann::json::array( { "HEAD", head } ) );

    // Add canvas
    lines.push_back( nlohmann::json::array( {
            "CANVAS", 1200, 1200, "#FFFFFF", true, "#CCCCCC", 10, 1200, 1200, "line", 10, "pixel",
            5, 400, 300
    } ) );

    // Add symbol pins
    for( LIB_PIN* pin : aSymbol->GetPins() )
    {
        VECTOR2D pos = ScalePos( pin->GetPosition() );
        double length = ScaleSize( pin->GetLength() );
        double rotation = pin->GetOrientation().AsDegrees();

        // PIN format: ["PIN", id, x, y, rotation, length, name, number, ...]
        lines.push_back( nlohmann::json::array( {
                "PIN",
                GenerateUuid(),
                pos.x, pos.y,
                rotation,
                length,
                pin->GetName().ToUTF8(),
                pin->GetNumber().ToUTF8()
        } ) );
    }

    // Add symbol drawings (lines, rectangles, circles, polygons)
    for( LIB_ITEM* item : aSymbol->GetDrawItems() )
    {
        switch( item->Type() )
        {
        case LIB_SHAPE_T:
        {
            LIB_SHAPE* shape = static_cast<LIB_SHAPE*>( item );
            VECTOR2D start = ScalePos( shape->GetPosition() );
            VECTOR2D end = ScalePos( shape->GetEnd() );
            int width = ScaleSize( shape->GetWidth() );

            if( shape->GetShape() == SHAPE_T::SEGMENT )
            {
                lines.push_back( nlohmann::json::array( { "LINE", start.x, start.y, end.x, end.y, width } ) );
            }
            else if( shape->GetShape() == SHAPE_T::CIRCLE )
            {
                double radius = ScaleSize( shape->GetRadius() );
                lines.push_back( nlohmann::json::array( { "CIRCLE", start.x, start.y, radius, width } ) );
            }
            else if( shape->GetShape() == SHAPE_T::RECTANGLE )
            {
                lines.push_back( nlohmann::json::array(
                        { "RECT", start.x, start.y, end.x, end.y, width } ) );
            }
            else if( shape->GetShape() == SHAPE_T::ARC )
            {
                VECTOR2D mid = ScalePos( shape->GetArcMid() );
                lines.push_back( nlohmann::json::array(
                        { "ARC", start.x, start.y, mid.x, mid.y, end.x, end.y, width } ) );
            }
            break;
        }

        case LIB_TEXT_T:
        {
            LIB_TEXT* text = static_cast<LIB_TEXT*>( item );
            VECTOR2D pos = ScalePos( text->GetPosition() );
            double rotation = text->GetTextAngle().AsDegrees();
            int height = ScaleSize( text->GetTextHeight() );

            lines.push_back( nlohmann::json::array( {
                    "TEXT", 0, text->GetText().ToUTF8(), pos.x, pos.y, rotation, height, 1,
                    GenerateUuid()
            } ) );
            break;
        }

        default:
            break;
        }
    }

    return lines;
}
