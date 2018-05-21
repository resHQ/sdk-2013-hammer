//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include <vgui_controls/CircularProgressBar.h>
#include <vgui_controls/Controls.h>

#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>

#include "mathlib/mathlib.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( CircularProgressBar );

BEGIN_PANEL_SETTINGS( CircularProgressBar )
	{ "fg_image", TYPE_STRING },
	{ "bg_image", TYPE_STRING }
END_PANEL_SETTINGS( CircularProgressBar );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CircularProgressBar::CircularProgressBar(Panel *parent, const char *panelName) : ProgressBar(parent, panelName)
{
	m_iProgressDirection = PROGRESS_CCW;

	for ( int i = 0; i < NUM_PROGRESS_TEXTURES; i++ )
	{
		m_nTextureId[i] = -1;
		m_pszImageName[i] = NULL;
	}

	m_iStartSegment = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CircularProgressBar::~CircularProgressBar()
{
	for ( int i = 0; i < NUM_PROGRESS_TEXTURES; i++ )
	{
		if ( vgui::surface() && m_nTextureId[i] )
		{
			vgui::surface()->DeleteTextureByID( m_nTextureId[i] );
			m_nTextureId[i] = -1;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CircularProgressBar::ApplySettings(KeyValues *inResourceData)
{
	const char *imageName = inResourceData->GetString("fg_image", "");
	if (*imageName)
	{
		SetFgImage( imageName );
	}
	imageName = inResourceData->GetString("bg_image", "");
	if (*imageName)
	{
		SetBgImage( imageName );
	}

	BaseClass::ApplySettings( inResourceData );
}

void CircularProgressBar::GetSettings( KeyValues* outResourceData )
{
	BaseClass::GetSettings( outResourceData );

	outResourceData->SetString( "fg_image", m_pszImageName[PROGRESS_TEXTURE_FG] );
	outResourceData->SetString( "bg_image", m_pszImageName[PROGRESS_TEXTURE_BG] );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CircularProgressBar::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(GetSchemeColor("CircularProgressBar.FgColor", pScheme));
	SetBgColor(GetSchemeColor("CircularProgressBar.BgColor", pScheme));
	SetBorder(NULL);

	for ( int i = 0; i < NUM_PROGRESS_TEXTURES; i++ )
	{
		if ( !m_pszImageName[i].IsEmpty() )
		{
			if ( m_nTextureId[i] == -1 )
			{
				m_nTextureId[i] = surface()->CreateNewTextureID();
			}

			surface()->DrawSetTextureFile( m_nTextureId[i], m_pszImageName[i], true, false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets an image by file name
//-----------------------------------------------------------------------------
void CircularProgressBar::SetImage(const char *imageName, progress_textures_t iPos)
{
	m_pszImageName[iPos].Format( "vgui/%s", imageName );
	InvalidateLayout(false, true); // force applyschemesettings to run
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CircularProgressBar::PaintBackground()
{
	// If we don't have a Bg image, use the foreground
	int iTextureID = m_nTextureId[PROGRESS_TEXTURE_BG] != -1 ? m_nTextureId[PROGRESS_TEXTURE_BG] : m_nTextureId[PROGRESS_TEXTURE_FG];
	vgui::surface()->DrawSetTexture( iTextureID );
	vgui::surface()->DrawSetColor( GetBgColor() );

	int wide, tall;
	GetSize(wide, tall);

	vgui::surface()->DrawTexturedRect( 0, 0, wide, tall );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CircularProgressBar::Paint()
{
	float flProgress = GetProgress();
	float flEndAngle;

	if ( m_iProgressDirection == PROGRESS_CW )
	{
		flEndAngle = flProgress;
	}
	else
	{
		flEndAngle = ( 1.0 - flProgress );
	}

	DrawCircleSegment( GetFgColor(), flEndAngle, ( m_iProgressDirection == PROGRESS_CW ) );
}

typedef struct
{
	float minProgressRadians;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} circular_progress_segment_t;

namespace vgui
{
// This defines the properties of the 8 circle segments
// in the circular progress bar.
circular_progress_segment_t Segments[8] =
{
	{ 0.0,			0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ M_PI * 0.25,	1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ M_PI * 0.5,	1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ M_PI * 0.75,	1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ M_PI,			0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ M_PI * 1.25,	0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ M_PI * 1.5,	0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ M_PI * 1.75,	0.0, 0.0, 0.5, 0.0, 1, 0 },
};

};

#define SEGMENT_ANGLE	( M_PI / 4 )

// function to draw from A to B degrees, with a direction
// we draw starting from the top ( 0 progress )
void CircularProgressBar::DrawCircleSegment( Color c, float flEndProgress, bool bClockwise )
{
	if ( m_nTextureId[PROGRESS_TEXTURE_FG] == -1 )
		return;

	int wide, tall;
	GetSize(wide, tall);

	float flWide = (float)wide;
	float flTall = (float)tall;

	float flHalfWide = (float)wide / 2;
	float flHalfTall = (float)tall / 2;

	vgui::surface()->DrawSetTexture( m_nTextureId[PROGRESS_TEXTURE_FG] );
	vgui::surface()->DrawSetColor( c );

	// TODO - if we want to progress CCW, reverse a few things

	float flEndProgressRadians = flEndProgress * M_PI * 2;

	int cur_wedge = m_iStartSegment;
	for ( int i=0;i<8;i++ )
	{
		if ( flEndProgressRadians > Segments[cur_wedge].minProgressRadians)
		{
			vgui::Vertex_t v[3];

			// vert 0 is ( 0.5, 0.5 )
			v[0].m_Position.Init( flHalfWide, flHalfTall );
			v[0].m_TexCoord.Init( 0.5f, 0.5f );

			float flInternalProgress = flEndProgressRadians - Segments[cur_wedge].minProgressRadians;

			if ( flInternalProgress < SEGMENT_ANGLE )
			{
				// Calc how much of this slice we should be drawing

				if ( i % 2 == 1 )
				{
					flInternalProgress = SEGMENT_ANGLE - flInternalProgress;
				}

				float flTan = tan(flInternalProgress);

				float flDeltaX, flDeltaY;

				if ( i % 2 == 1 )
				{
					flDeltaX = ( flHalfWide - flHalfTall * flTan ) * Segments[i].swipe_dir_x;
					flDeltaY = ( flHalfTall - flHalfWide * flTan ) * Segments[i].swipe_dir_y;
				}
				else
				{
					flDeltaX = flHalfTall * flTan * Segments[i].swipe_dir_x;
					flDeltaY = flHalfWide * flTan * Segments[i].swipe_dir_y;
				}

				v[2].m_Position.Init( Segments[i].vert1x * flWide + flDeltaX, Segments[i].vert1y * flTall + flDeltaY );
				v[2].m_TexCoord.Init( Segments[i].vert1x + ( flDeltaX / flHalfWide ) * 0.5, Segments[i].vert1y + ( flDeltaY / flHalfTall ) * 0.5 );
			}
			else
			{
				// full segment, easy calculation
				v[2].m_Position.Init( flHalfWide + flWide * ( Segments[i].vert2x - 0.5 ), flHalfTall + flTall * ( Segments[i].vert2y - 0.5 ) );
				v[2].m_TexCoord.Init( Segments[i].vert2x, Segments[i].vert2y );
			}

			// vert 2 is ( Segments[i].vert1x, Segments[i].vert1y )
			v[1].m_Position.Init( flHalfWide + flWide * ( Segments[i].vert1x - 0.5 ), flHalfTall + flTall * ( Segments[i].vert1y - 0.5 ) );
			v[1].m_TexCoord.Init( Segments[i].vert1x, Segments[i].vert1y );

			vgui::surface()->DrawTexturedPolygon( 3, v );
		}

		cur_wedge++;
		if ( cur_wedge >= 8)
			cur_wedge = 0;
	}
}