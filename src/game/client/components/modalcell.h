/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MODALCELL_H
#define GAME_CLIENT_COMPONENTS_MODALCELL_H
#include <base/vmath.h>
#include <game/client/component.h>

class CModalCell : public CComponent
{
	void DrawCircle(float x, float y, float r, int Segments);

	int m_CellsType;
	bool m_Active;

	vec2 m_SelectorMouse;
	int m_SelectedCell;

	bool m_EscapePressed;
	int64 m_LastInput;

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
