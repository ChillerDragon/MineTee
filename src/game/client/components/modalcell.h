/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MODALCELL_H
#define GAME_CLIENT_COMPONENTS_MODALCELL_H
#include <base/vmath.h>
#include <game/client/component.h>

class CModalCell : public CComponent
{
	enum
	{
		NUM_CELLS_LINE=9
	};

	bool m_Active;

	vec2 m_SelectorMouse;
	bool m_MousePressed;
	int m_MousePressedKey;
	int m_SelectedCell;
	int m_SelectedQty;

	bool m_EscapePressed;
	int64 m_LastInput;

	void RenderChest(CUIRect MainView);
	void RenderInventory(CUIRect MainView);
	void RenderCraftTable(CUIRect MainView);

	void DrawToolTipItemName(int ItemID);

	void OnClose();
	void OnOpen();
	void DoMoveCell(int From, int To, int Qty);

public:
	CModalCell();

	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnMouseMove(float x, float y);
	virtual bool OnInput(IInput::CEvent Event);
};

#endif
