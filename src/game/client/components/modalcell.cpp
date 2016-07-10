/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/keys.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/ui.h>
#include <game/client/render.h>
#include "modalcell.h"

CModalCell::CModalCell()
{
	m_LastInput = 0;
	m_EscapePressed = false;
	OnReset();
}

void CModalCell::OnConsoleInit()
{
}

void CModalCell::OnReset()
{
	m_CellsType = -1;
	m_Active = false;
}

void CModalCell::OnRelease()
{
	m_Active = false;
}

void CModalCell::OnMessage(int MsgType, void *pRawMsg)
{
}

bool CModalCell::OnMouseMove(float x, float y)
{
	if(!m_Active)
		return false;

	UI()->ConvertMouseMove(&x, &y);
	m_SelectorMouse += vec2(x,y);
	return true;
}

bool CModalCell::OnInput(IInput::CEvent e)
{
	if (!m_Active)
		return false;

	m_LastInput = time_get();

	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		if(e.m_Key == KEY_ESCAPE)
		{
			m_EscapePressed = true;
			m_pClient->m_CellsType = -1;
			mem_free(m_pClient->m_apLatestCells);
			m_pClient->m_apLatestCells = 0x0;
			m_Active = false;
			return true;
		}
	}

	return false;
}


void CModalCell::OnRender()
{
	if(m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = false;
		return;
	}

	if (m_pClient->m_apLatestCells)
	{
		m_Active = true;
		CUIRect Screen = *UI()->Screen();
		CUIRect MainView;
		Screen.Margin(80.0f, &MainView);
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

		if (!Screen.Contains(m_SelectorMouse))
		{
			m_SelectorMouse.x = clamp(m_SelectorMouse.x, 0.0f, Screen.w);
			m_SelectorMouse.y = clamp(m_SelectorMouse.y, 0.0f, Screen.h);
		}

		Graphics()->BlendNormal();
		Graphics()->TextureSet(-1);

		if (m_pClient->m_CellsType == CELLS_CHEST)
		{

			CUIRect Chest = MainView;
			Chest.h = 200.0f;
			Chest.y = Screen.h/2.0f - Chest.h/2.0f;
			// render background
			RenderTools()->DrawUIRect(&Chest, vec4(0,0,0,0.5f), CUI::CORNER_T, 10.0f);

			Chest.Margin(5.0f, &Chest);
			const float CellSize = Chest.w/9.0f;
			CUIRect Button, ButtonLine;
			for (int y=1; y<3; y++)
			{
				Chest.HSplitTop(30.0f, &ButtonLine, &Chest);
				for (int x=0; x<9; x++)
				{
					ButtonLine.VSplitLeft(CellSize, &Button, &ButtonLine);
					Button.Margin(3.0f, &Button);
					RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0f,1.0f,0.5f), 0, 10.0f);
				}
			}

			Chest.HSplitTop(60.0f, &ButtonLine, &Chest);
			for (int x=0; x<9; x++)
			{
				ButtonLine.VSplitLeft(CellSize, &Button, &ButtonLine);
				Button.Margin(3.0f, &Button);
				RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0f,1.0f,0.5f), 0, 10.0f);
			}
		}

		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1,1,1,1);
		IGraphics::CQuadItem QuadItem(m_SelectorMouse.x+Screen.w/2,m_SelectorMouse.y+Screen.h/2,24,24);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}
