/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <engine/keys.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/components/mapimages.h>
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
			m_pClient->m_NumCells = 0;
			mem_free(m_pClient->m_apLatestCells);
			m_pClient->m_apLatestCells = 0x0;
			m_Active = false;
		}
		else if (e.m_Key == KEY_MOUSE_1)
		{
			m_MousePressed = true;
		}
	}
	else if (e.m_Flags&IInput::FLAG_RELEASE)
	{
		if (e.m_Key == KEY_MOUSE_1)
		{
			m_MousePressed = false;
		}
	}

	return true;
}


void CModalCell::OnRender()
{
	static int SelectedIndex = -1;

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
		Screen.Margin(180.0f, &MainView);
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

		// Mouse Clamp
		m_SelectorMouse.x = clamp(m_SelectorMouse.x, 0.0f, Screen.w);
		m_SelectorMouse.y = clamp(m_SelectorMouse.y, 0.0f, Screen.h);

		Graphics()->BlendNormal();
		Graphics()->TextureSet(-1);

		if (m_pClient->m_CellsType == CELLS_CHEST)
		{
			const int NumRows = (m_pClient->m_NumCells-NUM_CELLS_LINE)/NUM_CELLS_LINE; // One line for fast inventory (the first 9 cells)

			CUIRect Modal = MainView;
			Modal.h = (NumRows+3.0f)*30.0f+5.0f;
			Modal.y = Screen.h/2.0f - Modal.h/2.0f;
			// render background
			RenderTools()->DrawUIRect(&Modal, vec4(0,0,0,0.5f), CUI::CORNER_T, 10.0f);

			Modal.Margin(5.0f, &Modal);

			CUIRect Title;
			Modal.HSplitTop(30.0f, &Title, &Modal);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
			TextRender()->Text(0x0, Title.x, Title.y, 28.0f, "CHEST", -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

			Modal.HSplitTop(10.0f, 0x0, &Modal);

			int CurItemID = -1;
			const float CellSize = Modal.w/NUM_CELLS_LINE;
			CUIRect Button, ButtonLine;
			int InvIndex = NUM_CELLS_LINE;
			for (int y=0; y<NumRows; y++)
			{
				Modal.HSplitTop(30.0f, &ButtonLine, &Modal);
				for (int x=0; x<NUM_CELLS_LINE; x++)
				{
					CurItemID = m_pClient->m_apLatestCells[InvIndex].m_ItemId;
					ButtonLine.VSplitLeft(CellSize, &Button, &ButtonLine);
					Button.Margin(3.0f, &Button);
					RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0f,1.0f,0.5f), 0, 10.0f);
					if (CurItemID != 0 && SelectedIndex != InvIndex)
					{
						vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
						if (CurItemID >= NUM_WEAPONS)
							DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

						RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
						char aBuf[8];
						str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[InvIndex].m_Amount);
						TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

						if (SelectedIndex == -1 && Button.Contains(m_SelectorMouse) && m_MousePressed)
							SelectedIndex = InvIndex;
					}

					if (SelectedIndex != -1 && Button.Contains(m_SelectorMouse) && !m_MousePressed)
					{
						MoveItem(SelectedIndex, InvIndex);
						SelectedIndex = -1;
					}

					++InvIndex;
				}
			}

			Modal.HSplitTop(15.0f, &ButtonLine, &Modal);
			Modal.HSplitTop(30.0f, &ButtonLine, &Modal);
			for (int x=0; x<NUM_CELLS_LINE; x++)
			{
				CurItemID = m_pClient->m_apLatestCells[x].m_ItemId;
				ButtonLine.VSplitLeft(CellSize, &Button, &ButtonLine);
				Button.Margin(3.0f, &Button);
				RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0f,1.0f,0.5f), 0, 10.0f);
				if (CurItemID != 0 && SelectedIndex != x)
				{
					vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
					if (CurItemID >= NUM_WEAPONS)
						DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

					RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
					char aBuf[8];
					str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[x].m_Amount);
					TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

					if (SelectedIndex == -1 && Button.Contains(m_SelectorMouse) && m_MousePressed)
						SelectedIndex = x;
				}
				if (SelectedIndex != -1 && Button.Contains(m_SelectorMouse) && !m_MousePressed)
				{
					MoveItem(SelectedIndex, x);
					SelectedIndex = -1;
				}
			}

			if (SelectedIndex != -1)
			{
				if (!m_MousePressed)
				{
					MoveItem(SelectedIndex, -1);
					SelectedIndex = -1;
				}
				else
				{
					RenderTools()->RenderItem(m_pClient->m_apLatestCells[SelectedIndex].m_ItemId, vec2(m_SelectorMouse.x-12.0f, m_SelectorMouse.y-12.0f), m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
					char aBuf[8];
					str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[SelectedIndex].m_Amount);
					TextRender()->Text(0x0, m_SelectorMouse.x-12.0f, m_SelectorMouse.y-12.0f, 8.0f, aBuf, -1);
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
				}
			}
		}

		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1,1,1,1);
		IGraphics::CQuadItem QuadItem(m_SelectorMouse.x,m_SelectorMouse.y,24,24);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}

void CModalCell::MoveItem(int From, int To)
{
	if (To == -1)
	{
		m_pClient->m_apLatestCells[From].m_ItemId = 0;
		m_pClient->m_apLatestCells[From].m_Amount = 0;
	}
	else
	{
		CCellData TempCellData = m_pClient->m_apLatestCells[To];
		m_pClient->m_apLatestCells[To] = m_pClient->m_apLatestCells[From];
		m_pClient->m_apLatestCells[From] = TempCellData;
	}

	Client()->SendMoveCell(From, To);
}
