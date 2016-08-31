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
	m_MousePressed = false;
	m_MousePressedKey = 0;
	m_SelectedCell = -1;
	m_SelectedQty = 0;
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
			if (m_SelectedCell != -1)
			{
				Client()->SendMoveCell(m_SelectedCell, -1, m_SelectedQty);
				m_SelectedQty = 0;
				m_SelectedCell = -1;
			}

			m_EscapePressed = true;
			m_pClient->m_CellsType = -1;
			m_pClient->m_NumCells = 0;
			mem_free(m_pClient->m_apLatestCells);
			m_pClient->m_apLatestCells = 0x0;
			m_Active = false;
			Client()->SendRelease();
		}
		if (!m_MousePressed && (e.m_Key == KEY_MOUSE_1 || e.m_Key == KEY_MOUSE_2 || e.m_Key == KEY_MOUSE_3))
		{
			m_MousePressed = true;
			m_MousePressedKey = e.m_Key;
		}
	}
	else if (e.m_Flags&IInput::FLAG_RELEASE)
	{
		if (m_MousePressed && e.m_Key == m_MousePressedKey)
		{
			m_MousePressed = false;
			m_MousePressedKey = 0;
		}
	}

	return true;
}

void CModalCell::OnRender()
{
	m_Active = (m_pClient->m_apLatestCells)?true:false;

	if(!m_Active || m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = false;
		return;
	}

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
		RenderChest(MainView);
	else if (m_pClient->m_CellsType == CELLS_INVENTORY)
		RenderInventory(MainView);
	else if (m_pClient->m_CellsType == CELLS_CRAFT_TABLE)
		RenderCraftTable(MainView);

	if (m_SelectedCell == -1)
	{
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1,1,1,1);
		IGraphics::CQuadItem QuadItem(m_SelectorMouse.x,m_SelectorMouse.y,24,24);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}

// TODO: rework mouse actions... more duplicated code in all 'renders'...
void CModalCell::RenderChest(CUIRect MainView)
{
	static bool m_LastMousePressed = m_MousePressed;
	const int NumRows = (m_pClient->m_NumCells-NUM_CELLS_LINE)/NUM_CELLS_LINE; // One line for fast inventory (the first 9 cells)

	CUIRect Modal = MainView;
	Modal.h = (NumRows+3.0f)*30.0f+5.0f;
	Modal.y = MainView.h/2.0f;
	CUIRect OrgModal = Modal;
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
			if (CurItemID != 0 && (m_SelectedCell != InvIndex || m_pClient->m_apLatestCells[InvIndex].m_Amount-m_SelectedQty > 0))
			{
				vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
				if (CurItemID >= NUM_WEAPONS)
					DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

				RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
				char aBuf[8];
				if (m_SelectedCell == InvIndex)
					str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[InvIndex].m_Amount-m_SelectedQty);
				else
					str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[InvIndex].m_Amount);
				TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

				if (m_SelectedCell == -1 && Button.Contains(m_SelectorMouse) && m_MousePressed)
				{
					if (m_MousePressedKey == KEY_MOUSE_1)
						m_SelectedQty = m_pClient->m_apLatestCells[InvIndex].m_Amount;
					else if (m_MousePressedKey == KEY_MOUSE_3)
						m_SelectedQty = m_pClient->m_apLatestCells[InvIndex].m_Amount/2;
					else
						m_SelectedQty = 1;

					if (m_SelectedQty > 0)
						m_SelectedCell = InvIndex;
				}
			}

			if (m_SelectedCell != -1 && Button.Contains(m_SelectorMouse) && !m_MousePressed)
			{
				Client()->SendMoveCell(m_SelectedCell, InvIndex, m_SelectedQty);
				m_SelectedCell = -1;
			}

			++InvIndex;
		}
	}

	Modal.HSplitTop(15.0f, &ButtonLine, &Modal);
	Modal.HSplitTop(30.0f, &ButtonLine, &Modal);
	for (int q=0; q<2; q++)
	{
		CUIRect OrgButtonLine = ButtonLine;
		for (int x=0; x<NUM_CELLS_LINE; x++)
		{
			CurItemID = m_pClient->m_apLatestCells[x].m_ItemId;
			OrgButtonLine.VSplitLeft(CellSize, &Button, &OrgButtonLine);
			Button.Margin(3.0f, &Button);

			if (q == 0)
			{
				RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0f,1.0f,0.5f), 0, 10.0f);

				if (m_SelectedCell != -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
				{
					Client()->SendMoveCell(m_SelectedCell, x, m_SelectedQty);
					m_SelectedCell = -1;
					m_SelectedQty = 0;
					m_LastMousePressed = true;
				}

				if (CurItemID != 0 && (m_SelectedCell != x || m_pClient->m_apLatestCells[x].m_Amount-m_SelectedQty > 0))
				{
					vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
					if (CurItemID >= NUM_WEAPONS)
						DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

					RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
					char aBuf[8];
					if (m_SelectedCell == x)
						str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[x].m_Amount-m_SelectedQty);
					else
						str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[x].m_Amount);
					TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

					if (m_SelectedCell == -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
					{
						if (m_MousePressedKey == KEY_MOUSE_1)
							m_SelectedQty = m_pClient->m_apLatestCells[x].m_Amount;
						else if (m_MousePressedKey == KEY_MOUSE_3)
							m_SelectedQty = m_pClient->m_apLatestCells[x].m_Amount/2;
						else
							m_SelectedQty = 1;

						if (m_SelectedQty > 0)
							m_SelectedCell = x;
						m_LastMousePressed = true;
					}
				}
			}
			else if (Button.Contains(m_SelectorMouse))
				DrawToolTipItemName(CurItemID);
		}
	}

	if (m_SelectedCell != -1)
	{
		if (!m_LastMousePressed && m_MousePressed && !OrgModal.Contains(m_SelectorMouse))
		{
			Client()->SendMoveCell(m_SelectedCell, -1, m_SelectedQty);
			m_SelectedCell = -1;
			m_LastMousePressed = true;
		}
		else
		{
			RenderTools()->RenderItem(m_pClient->m_apLatestCells[m_SelectedCell].m_ItemId, vec2(m_SelectorMouse.x-12.0f, m_SelectorMouse.y-12.0f), m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
			char aBuf[8];
			str_format(aBuf, sizeof(aBuf), "x%d", m_SelectedQty);
			TextRender()->Text(0x0, m_SelectorMouse.x-12.0f, m_SelectorMouse.y-12.0f, 8.0f, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	if (!m_MousePressed)
		m_LastMousePressed = false;
}

void CModalCell::RenderInventory(CUIRect MainView)
{
	static bool m_LastMousePressed = m_MousePressed;
	int CurItemID = -1;
	const int NumRows = (m_pClient->m_NumCells-(NUM_CELLS_LINE*2+1))/NUM_CELLS_LINE; // One line for fast inventory (the first 9 cells) + One line for craft (the last 9 cells) + One craft result (latest cell)

	CUIRect Button, ButtonLine;
	CUIRect OrgModal = MainView, Modal;
	OrgModal.h = (NumRows+3.0f)*30.0f+5.0f+80.0f;
	OrgModal.y = MainView.h/2.0f;
	OrgModal.VMargin(120.0f, &OrgModal);
	// render background
	RenderTools()->DrawUIRect(&OrgModal, vec4(0,0,0,0.5f), CUI::CORNER_T, 10.0f);

	OrgModal.Margin(5.0f, &Modal);

	CUIRect Title;
	Modal.HSplitTop(30.0f, &Title, &Modal);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
	TextRender()->Text(0x0, Title.x, Title.y, 28.0f, "INVENTORY", -1);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	Modal.HSplitTop(10.0f, 0x0, &Modal);

	// Craft Zone
	CUIRect CraftZone, CraftTable, CraftResult;
	Modal.HSplitTop(80.0f, &CraftZone, &Modal);
	CraftZone.VSplitRight(150.0f, 0x0, &CraftZone);
	CraftZone.HMargin(10.0f, &CraftZone);
	CraftZone.VSplitMid(&CraftTable, &CraftResult);

	const float CraftCellSize = CraftTable.w/2;

	for (int q=0; q<2; q++)
	{
		int StartInvIndex = NUM_CELLS_LINE*4;
		for (int y=0; y<2; y++)
		{
			CraftTable.HSplitTop(30.0f, &ButtonLine, &CraftTable);
			for (int x=0; x<2; x++)
			{
				CurItemID = m_pClient->m_apLatestCells[StartInvIndex].m_ItemId;
				ButtonLine.VSplitLeft(CraftCellSize, &Button, &ButtonLine);
				Button.Margin(1.0f, &Button);

				if (q == 0)
				{
					RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0,1.0,0.5f), 0, 0.0f);

					if (m_SelectedCell != -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
					{
						Client()->SendMoveCell(m_SelectedCell, StartInvIndex, m_SelectedQty);
						m_SelectedCell = -1;
						m_SelectedQty = 0;
						m_LastMousePressed = true;
					}

					if (CurItemID != 0 && (m_SelectedCell != StartInvIndex || m_pClient->m_apLatestCells[StartInvIndex].m_Amount-m_SelectedQty > 0))
					{
						vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
						if (CurItemID >= NUM_WEAPONS)
							DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-8.0f));

						RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
						char aBuf[8];
						if (m_SelectedCell == StartInvIndex)
							str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[StartInvIndex].m_Amount-m_SelectedQty);
						else
							str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[StartInvIndex].m_Amount);
						TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

						if (m_SelectedCell == -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
						{
							if (m_MousePressedKey == KEY_MOUSE_1)
								m_SelectedQty = m_pClient->m_apLatestCells[StartInvIndex].m_Amount;
							else if (m_MousePressedKey == KEY_MOUSE_3)
								m_SelectedQty = m_pClient->m_apLatestCells[StartInvIndex].m_Amount/2;
							else
								m_SelectedQty = 1;

							if (m_SelectedQty > 0)
								m_SelectedCell = StartInvIndex;

							m_LastMousePressed = true;
						}
					}
				}
				else if (Button.Contains(m_SelectorMouse))
					DrawToolTipItemName(CurItemID);

				++StartInvIndex;
			}
			++StartInvIndex; // Do this because craft system works with a 3x3 table and inventory shows 2x2
		}
	}

	// Craft Result
	const int CraftResultIndex = NUM_CELLS_LINE*5;
	CurItemID = m_pClient->m_apLatestCells[CraftResultIndex].m_ItemId;
	CraftResult.Margin(15.0f, &Button);
	RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0,1.0,0.5f), 0, 0.0f);

	if (CurItemID != 0 && (m_SelectedCell != CraftResultIndex || m_pClient->m_apLatestCells[CraftResultIndex].m_Amount-m_SelectedQty > 0))
	{
		vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
		if (CurItemID >= NUM_WEAPONS)
			DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

		RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
		char aBuf[8];
		if (m_SelectedCell == NUM_CELLS_LINE*5)
			str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[CraftResultIndex].m_Amount-m_SelectedQty);
		else
			str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[CraftResultIndex].m_Amount);
		TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}

	if (Button.Contains(m_SelectorMouse))
	{
		if (m_SelectedCell == -1 && !m_LastMousePressed && m_MousePressed)
		{
			m_SelectedQty = m_pClient->m_apLatestCells[CraftResultIndex].m_Amount;
			if (m_SelectedQty > 0)
				m_SelectedCell = CraftResultIndex;

			m_LastMousePressed = true;
		}

		DrawToolTipItemName(CurItemID);
	}

	RenderTools()->DrawUIRect(&Modal, vec4(0,0,0,0.5f), 0, 0.0f); // Cool? background :/

	// Invetory Zone
	CurItemID = -1;
	const float CellSize = Modal.w/NUM_CELLS_LINE;

	CUIRect ModalTemp;
	for (int q=0; q<2; q++) // Two pass (1. Render | 2. Show Info)
	{
		ModalTemp = Modal;
		int InvIndex = NUM_CELLS_LINE;
		for (int y=0; y<NumRows; y++)
		{
			ModalTemp.HSplitTop(30.0f, &ButtonLine, &ModalTemp);
			for (int x=0; x<NUM_CELLS_LINE; x++)
			{
				CurItemID = m_pClient->m_apLatestCells[InvIndex].m_ItemId;
				ButtonLine.VSplitLeft(CellSize, &Button, &ButtonLine);
				Button.Margin(3.0f, &Button);

				if (q == 0)
				{
					RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0f,1.0f,0.5f), 0, 0.0f);

					if (m_SelectedCell != -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
					{
						Client()->SendMoveCell(m_SelectedCell, InvIndex, m_SelectedQty);
						m_SelectedCell = -1;
						m_SelectedQty = 0;
						m_LastMousePressed = true;
					}

					if (CurItemID != 0 && (m_SelectedCell != InvIndex || m_pClient->m_apLatestCells[InvIndex].m_Amount-m_SelectedQty > 0))
					{
						vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
						if (CurItemID >= NUM_WEAPONS)
							DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

						RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
						char aBuf[8];
						if (m_SelectedCell == InvIndex)
							str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[InvIndex].m_Amount-m_SelectedQty);
						else
							str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[InvIndex].m_Amount);
						TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

						if (m_SelectedCell == -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
						{
							if (m_MousePressedKey == KEY_MOUSE_1)
								m_SelectedQty = m_pClient->m_apLatestCells[InvIndex].m_Amount;
							else if (m_MousePressedKey == KEY_MOUSE_3)
								m_SelectedQty = m_pClient->m_apLatestCells[InvIndex].m_Amount/2;
							else
								m_SelectedQty = 1;

							if (m_SelectedQty > 0)
								m_SelectedCell = InvIndex;

							m_LastMousePressed = true;
						}
					}
				}
				else if (Button.Contains(m_SelectorMouse))
					DrawToolTipItemName(CurItemID);
				++InvIndex;
			}
		}
	}
	Modal = ModalTemp;

	// Fast inventory zone
	Modal.HSplitTop(15.0f, &ButtonLine, &Modal);
	Modal.HSplitTop(30.0f, &ButtonLine, &Modal);
	for (int q=0; q<2; q++)
	{
		CUIRect OrgButtonLine = ButtonLine;
		for (int x=0; x<NUM_CELLS_LINE; x++)
		{
			CurItemID = m_pClient->m_apLatestCells[x].m_ItemId;
			OrgButtonLine.VSplitLeft(CellSize, &Button, &OrgButtonLine);
			Button.Margin(3.0f, &Button);

			if (q == 0)
			{
				RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0f,1.0f,0.5f), 0, 10.0f);

				if (m_SelectedCell != -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
				{
					Client()->SendMoveCell(m_SelectedCell, x, m_SelectedQty);
					m_SelectedCell = -1;
					m_SelectedQty = 0;
					m_LastMousePressed = true;
				}

				if (CurItemID != 0 && (m_SelectedCell != x || m_pClient->m_apLatestCells[x].m_Amount-m_SelectedQty > 0))
				{
					vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
					if (CurItemID >= NUM_WEAPONS)
						DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

					RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
					char aBuf[8];
					if (m_SelectedCell == x)
						str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[x].m_Amount-m_SelectedQty);
					else
						str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[x].m_Amount);
					TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

					if (m_SelectedCell == -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
					{
						if (m_MousePressedKey == KEY_MOUSE_1)
							m_SelectedQty = m_pClient->m_apLatestCells[x].m_Amount;
						else if (m_MousePressedKey == KEY_MOUSE_3)
							m_SelectedQty = m_pClient->m_apLatestCells[x].m_Amount/2;
						else
							m_SelectedQty = 1;

						if (m_SelectedQty > 0)
							m_SelectedCell = x;

						m_LastMousePressed = true;
					}
				}
			}
			else if (Button.Contains(m_SelectorMouse))
				DrawToolTipItemName(CurItemID);
		}
	}

	// Out of zone & Mouse
	if (m_SelectedCell != -1)
	{
		if (!m_LastMousePressed && m_MousePressed && !OrgModal.Contains(m_SelectorMouse))
		{
			Client()->SendMoveCell(m_SelectedCell, -1, m_SelectedQty);
			m_SelectedCell = -1;
			m_SelectedQty = 0;
			m_LastMousePressed = true;
		}
		else
		{
			RenderTools()->RenderItem(m_pClient->m_apLatestCells[m_SelectedCell].m_ItemId, vec2(m_SelectorMouse.x-12.0f, m_SelectorMouse.y-12.0f), m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
			char aBuf[8];
			str_format(aBuf, sizeof(aBuf), "x%d", m_SelectedQty);
			TextRender()->Text(0x0, m_SelectorMouse.x-12.0f, m_SelectorMouse.y-12.0f, 8.0f, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	if (!m_MousePressed)
		m_LastMousePressed = false;
}

void CModalCell::RenderCraftTable(CUIRect MainView)
{
	static bool m_LastMousePressed = m_MousePressed;
	int CurItemID = -1;
	const int NumRows = (m_pClient->m_NumCells-(NUM_CELLS_LINE*2+1))/NUM_CELLS_LINE; // One line for fast inventory (the first 9 cells) + One line for craft (the last 9 cells) + One craft result (latest cell)

	CUIRect Button, ButtonLine;
	CUIRect Modal = MainView;
	Modal.h = (NumRows+3.0f)*30.0f+5.0f+120.0f;
	Modal.y = MainView.h/2.0f;
	CUIRect OrgModal = Modal;
	// render background
	RenderTools()->DrawUIRect(&Modal, vec4(0,0,0,0.5f), CUI::CORNER_T, 10.0f);

	Modal.Margin(5.0f, &Modal);

	CUIRect Title;
	Modal.HSplitTop(30.0f, &Title, &Modal);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
	TextRender()->Text(0x0, Title.x, Title.y, 28.0f, "CRAFTING TABLE", -1);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	Modal.HSplitTop(10.0f, 0x0, &Modal);

	// Craft Zone
	CUIRect CraftZone, CraftTable, CraftResult;
	Modal.HSplitTop(90.0f, &CraftZone, &Modal);
	CraftZone.HMargin(10.0f, &CraftZone);
	CraftZone.VSplitLeft(90.0f, &CraftTable, &CraftResult);

	const float CraftCellSize = CraftTable.w/3;

	for (int q=0; q<2; q++)
	{
		int StartInvIndex = NUM_CELLS_LINE*4;
		CUIRect OrgCraftTable = CraftTable;
		for (int y=0; y<3; y++)
		{
			OrgCraftTable.HSplitTop(30.0f, &ButtonLine, &OrgCraftTable);
			for (int x=0; x<3; x++)
			{
				CurItemID = m_pClient->m_apLatestCells[StartInvIndex].m_ItemId;
				ButtonLine.VSplitLeft(CraftCellSize, &Button, &ButtonLine);
				Button.Margin(1.0f, &Button);

				if (q == 0)
				{
					RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0,1.0,0.5f), 0, 0.0f);

					if (m_SelectedCell != -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
					{
						Client()->SendMoveCell(m_SelectedCell, StartInvIndex, m_SelectedQty);
						m_SelectedCell = -1;
						m_SelectedQty = 0;
						m_LastMousePressed = true;
					}

					if (CurItemID != 0 && (m_SelectedCell != StartInvIndex || m_pClient->m_apLatestCells[StartInvIndex].m_Amount-m_SelectedQty > 0))
					{
						vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
						if (CurItemID >= NUM_WEAPONS)
							DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-8.0f));

						RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
						char aBuf[8];
						if (m_SelectedCell == StartInvIndex)
							str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[StartInvIndex].m_Amount-m_SelectedQty);
						else
							str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[StartInvIndex].m_Amount);
						TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

						if (m_SelectedCell == -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
						{
							if (m_MousePressedKey == KEY_MOUSE_1)
								m_SelectedQty = m_pClient->m_apLatestCells[StartInvIndex].m_Amount;
							else if (m_MousePressedKey == KEY_MOUSE_3)
								m_SelectedQty = m_pClient->m_apLatestCells[StartInvIndex].m_Amount/2;
							else
								m_SelectedQty = 1;

							if (m_SelectedQty > 0)
								m_SelectedCell = StartInvIndex;
							m_LastMousePressed = true;
						}
					}
				}
				else if (Button.Contains(m_SelectorMouse))
					DrawToolTipItemName(CurItemID);

				++StartInvIndex;
			}
		}
	}

	// Craft Result
	const int CraftResultIndex = NUM_CELLS_LINE*5;
	CurItemID = m_pClient->m_apLatestCells[CraftResultIndex].m_ItemId;
	CraftResult.Margin(15.0f, &Button);
	Button.VSplitLeft(40.0f, &Button, 0x0);
	RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0,1.0,0.5f), 0, 0.0f);

	if (CurItemID != 0 && (m_SelectedCell != CraftResultIndex || m_pClient->m_apLatestCells[CraftResultIndex].m_Amount-m_SelectedQty > 0))
	{
		vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
		if (CurItemID >= NUM_WEAPONS)
			DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

		RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
		char aBuf[8];
		if (m_SelectedCell == NUM_CELLS_LINE*5)
			str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[CraftResultIndex].m_Amount-m_SelectedQty);
		else
			str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[CraftResultIndex].m_Amount);
		TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}

	if (Button.Contains(m_SelectorMouse))
	{
		if (m_SelectedCell == -1 && !m_LastMousePressed && m_MousePressed)
		{
			m_SelectedQty = m_pClient->m_apLatestCells[CraftResultIndex].m_Amount;
			if (m_SelectedQty > 0)
				m_SelectedCell = CraftResultIndex;
			m_LastMousePressed = true;
		}
		DrawToolTipItemName(CurItemID);
	}

	Modal.HSplitTop(30.0f, 0x0, &Modal);

	RenderTools()->DrawUIRect(&Modal, vec4(0,0,0,0.5f), 0, 0.0f); // Cool? background :/

	// Invetory Zone
	CurItemID = -1;
	const float CellSize = Modal.w/NUM_CELLS_LINE;

	for (int q=0; q<2; q++)
	{
		int InvIndex = NUM_CELLS_LINE;
		CUIRect OrgButtonLine = ButtonLine;
		for (int y=0; y<NumRows; y++)
		{
			Modal.HSplitTop(30.0f, &ButtonLine, &Modal);
			for (int x=0; x<NUM_CELLS_LINE; x++)
			{
				CurItemID = m_pClient->m_apLatestCells[InvIndex].m_ItemId;
				OrgButtonLine.VSplitLeft(CellSize, &Button, &OrgButtonLine);
				Button.Margin(3.0f, &Button);

				if (q == 0)
				{
					RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0f,1.0f,0.5f), 0, 0.0f);

					if (m_SelectedCell != -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
					{
						Client()->SendMoveCell(m_SelectedCell, InvIndex, m_SelectedQty);
						m_SelectedCell = -1;
						m_SelectedQty = 0;
						m_LastMousePressed = true;
					}

					if (CurItemID != 0 && (m_SelectedCell != InvIndex || m_pClient->m_apLatestCells[InvIndex].m_Amount-m_SelectedQty > 0))
					{
						vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
						if (CurItemID >= NUM_WEAPONS)
							DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

						RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
						char aBuf[8];
						if (m_SelectedCell == InvIndex)
							str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[InvIndex].m_Amount-m_SelectedQty);
						else
							str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[InvIndex].m_Amount);
						TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

						if (m_SelectedCell == -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
						{
							if (m_MousePressedKey == KEY_MOUSE_1)
								m_SelectedQty = m_pClient->m_apLatestCells[InvIndex].m_Amount;
							else if (m_MousePressedKey == KEY_MOUSE_3)
								m_SelectedQty = m_pClient->m_apLatestCells[InvIndex].m_Amount/2;
							else
								m_SelectedQty = 1;

							if (m_SelectedQty > 0)
								m_SelectedCell = InvIndex;

							m_LastMousePressed = true;
						}
					}
				}
				else if (Button.Contains(m_SelectorMouse))
					DrawToolTipItemName(CurItemID);

				++InvIndex;
			}
		}
	}

	// Fast inventory zone
	Modal.HSplitTop(15.0f, &ButtonLine, &Modal);
	Modal.HSplitTop(30.0f, &ButtonLine, &Modal);

	for (int q=0; q<2; q++)
	{
		for (int x=0; x<NUM_CELLS_LINE; x++)
		{
			CurItemID = m_pClient->m_apLatestCells[x].m_ItemId;
			ButtonLine.VSplitLeft(CellSize, &Button, &ButtonLine);
			Button.Margin(3.0f, &Button);

			if (q == 0)
			{
				RenderTools()->DrawUIRect(&Button, vec4(1.0f,1.0f,1.0f,0.5f), 0, 10.0f);
				if (m_SelectedCell != -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
				{
					Client()->SendMoveCell(m_SelectedCell, x, m_SelectedQty);
					m_SelectedCell = -1;
					m_SelectedQty = 0;
					m_LastMousePressed = true;
				}
				if (CurItemID != 0 && (m_SelectedCell != x || m_pClient->m_apLatestCells[x].m_Amount-m_SelectedQty > 0))
				{
					vec2 DPos = vec2(Button.x+(Button.w/2-8.0f), Button.y+(Button.h/2-8.0f));
					if (CurItemID >= NUM_WEAPONS)
						DPos = vec2(Button.x+(Button.w/2-16.0f), Button.y+(Button.h/2-12.0f));

					RenderTools()->RenderItem(CurItemID, DPos, m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
					char aBuf[8];
					if (m_SelectedCell == x)
						str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[x].m_Amount-m_SelectedQty);
					else
						str_format(aBuf, sizeof(aBuf), "x%d", m_pClient->m_apLatestCells[x].m_Amount);
					TextRender()->Text(0x0, Button.x+2.0f, Button.y, 8.0f, aBuf, -1);
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

					if (m_SelectedCell == -1 && Button.Contains(m_SelectorMouse) && !m_LastMousePressed && m_MousePressed)
					{
						if (m_MousePressedKey == KEY_MOUSE_1)
							m_SelectedQty = m_pClient->m_apLatestCells[x].m_Amount;
						else if (m_MousePressedKey == KEY_MOUSE_3)
							m_SelectedQty = m_pClient->m_apLatestCells[x].m_Amount/2;
						else
							m_SelectedQty = 1;

						if (m_SelectedQty > 0)
							m_SelectedCell = x;
						m_LastMousePressed = true;
					}
				}
			}
			else if (Button.Contains(m_SelectorMouse))
				DrawToolTipItemName(CurItemID);
		}
	}

	// Out of zone & Mouse
	// Out of zone & Mouse
	if (m_SelectedCell != -1)
	{
		if (!m_LastMousePressed && m_MousePressed && !OrgModal.Contains(m_SelectorMouse))
		{
			Client()->SendMoveCell(m_SelectedCell, -1, m_SelectedQty);
			m_SelectedCell = -1;
			m_SelectedQty = 0;
			m_LastMousePressed = true;
		}
		else
		{
			RenderTools()->RenderItem(m_pClient->m_apLatestCells[m_SelectedCell].m_ItemId, vec2(m_SelectorMouse.x-12.0f, m_SelectorMouse.y-12.0f), m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image), 16.0f, vec2(24.0f, 16.0f));

			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.85f);
			char aBuf[8];
			str_format(aBuf, sizeof(aBuf), "x%d", m_SelectedQty);
			TextRender()->Text(0x0, m_SelectorMouse.x-12.0f, m_SelectorMouse.y-12.0f, 8.0f, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	if (!m_MousePressed)
		m_LastMousePressed = false;
}

void CModalCell::DrawToolTipItemName(int ItemID)
{
	if (ItemID == 0 || m_SelectedCell != -1)
		return;

	char aItemName[64];

	if (ItemID < NUM_WEAPONS)
		str_copy(aItemName, gs_aWeaponNames[ItemID], sizeof(aItemName));
	else
	{
		CBlockManager::CBlockInfo *pBlockInfo = m_pClient->Collision()->BlockManager()->GetBlockInfo(ItemID-NUM_WEAPONS);
		str_copy(aItemName, pBlockInfo->m_aName, sizeof(aItemName));
	}

	int TextWidth = TextRender()->TextWidth(0, 8.0f, aItemName, -1);
	CUIRect Tooltip;
	Tooltip.x = m_SelectorMouse.x-TextWidth/2.0f-2.0f;
	Tooltip.y = m_SelectorMouse.y+24.0f;
	Tooltip.w = TextWidth+7.0f;
	Tooltip.h = 8.0f+4.0f;
	RenderTools()->DrawUIRect(&Tooltip, vec4(0.0f,0.0,0.0,0.5f), CUI::CORNER_ALL, 3.0f);

	TextRender()->Text(0x0, m_SelectorMouse.x-TextWidth/2, m_SelectorMouse.y+24.0f, 8.0f, aItemName, -1);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}
