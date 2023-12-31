/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* File modified by Alexandre Díaz */
#include <base/math.h>
#include <engine/graphics.h>
#include <engine/demo.h>

#include <game/generated/client_data.h>
#include <game/client/render.h>
#include <game/client/components/mapimages.h> // MineTee
#include <game/gamecore.h>
#include "particles.h"

CParticles::CParticles()
{
	OnReset();
	m_RenderTrail.m_pParts = this;
	m_RenderExplosions.m_pParts = this;
	m_RenderGeneral.m_pParts = this;
	m_RenderMineTeeTumbstone.m_pParts = this; // MineTee
	m_RenderBlocks.m_pParts = this; // MineTee
}


void CParticles::OnReset()
{
	// reset particles
	for(int i = 0; i < MAX_PARTICLES; i++)
	{
		m_aParticles[i].m_PrevPart = i-1;
		m_aParticles[i].m_NextPart = i+1;
	}

	m_aParticles[0].m_PrevPart = 0;
	m_aParticles[MAX_PARTICLES-1].m_NextPart = -1;
	m_FirstFree = 0;

	for(int i = 0; i < NUM_GROUPS; i++)
		m_aFirstPart[i] = -1;
}

void CParticles::Add(int Group, CParticle *pPart)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
		if(pInfo->m_Paused)
			return;
	}
	else
	{
		if(m_pClient->m_Snap.m_pGameInfoObj && (m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
			return;
	}

	if (m_FirstFree == -1)
		return;

	// remove from the free list
	int Id = m_FirstFree;
	m_FirstFree = m_aParticles[Id].m_NextPart;
	if(m_FirstFree != -1)
		m_aParticles[m_FirstFree].m_PrevPart = -1;

	// copy data
	m_aParticles[Id] = *pPart;

	// insert to the group list
	m_aParticles[Id].m_PrevPart = -1;
	m_aParticles[Id].m_NextPart = m_aFirstPart[Group];
	if(m_aFirstPart[Group] != -1)
		m_aParticles[m_aFirstPart[Group]].m_PrevPart = Id;
	m_aFirstPart[Group] = Id;

	// set some parameters
	m_aParticles[Id].m_Life = 0;
}

void CParticles::Update(float TimePassed)
{
	static float FrictionFraction = 0;
	FrictionFraction += TimePassed;

	if(FrictionFraction > 2.0f) // safty messure
		FrictionFraction = 0;

	int FrictionCount = 0;
	while(FrictionFraction > 0.05f)
	{
		FrictionCount++;
		FrictionFraction -= 0.05f;
	}

	for(int g = 0; g < NUM_GROUPS; g++)
	{
		int i = m_aFirstPart[g];
		while(i != -1)
		{
			int Next = m_aParticles[i].m_NextPart;
			//m_aParticles[i].vel += flow_get(m_aParticles[i].pos)*time_passed * m_aParticles[i].flow_affected;
			m_aParticles[i].m_Vel.y += m_aParticles[i].m_Gravity*TimePassed;

			for(int f = 0; f < FrictionCount; f++) // apply friction
				m_aParticles[i].m_Vel *= m_aParticles[i].m_Friction;

			// move the point
			vec2 Vel = m_aParticles[i].m_Vel*TimePassed;
			if (m_aParticles[i].m_Collide) // MineTee
				Collision()->MovePoint(&m_aParticles[i].m_Pos, &Vel, 0.1f+0.9f*frandom(), NULL);
			else
				m_aParticles[i].m_Pos += Vel;
			m_aParticles[i].m_Vel = Vel* (1.0f/TimePassed);

			m_aParticles[i].m_Life += TimePassed;
			m_aParticles[i].m_Rot += TimePassed * m_aParticles[i].m_Rotspeed;

			// check particle death
			if(m_aParticles[i].m_Life > m_aParticles[i].m_LifeSpan)
			{
				// remove it from the group list
				if(m_aParticles[i].m_PrevPart != -1)
					m_aParticles[m_aParticles[i].m_PrevPart].m_NextPart = m_aParticles[i].m_NextPart;
				else
					m_aFirstPart[g] = m_aParticles[i].m_NextPart;

				if(m_aParticles[i].m_NextPart != -1)
					m_aParticles[m_aParticles[i].m_NextPart].m_PrevPart = m_aParticles[i].m_PrevPart;

				// insert to the free list
				if(m_FirstFree != -1)
					m_aParticles[m_FirstFree].m_PrevPart = i;
				m_aParticles[i].m_PrevPart = -1;
				m_aParticles[i].m_NextPart = m_FirstFree;
				m_FirstFree = i;
			}

			i = Next;
		}
	}
}

void CParticles::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	static int64 LastTime = 0;
	int64 t = time_get();

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
		if(!pInfo->m_Paused)
			Update((float)((t-LastTime)/(double)time_freq())*pInfo->m_Speed);
	}
	else
	{
		if(m_pClient->m_Snap.m_pGameInfoObj && !(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
			Update((float)((t-LastTime)/(double)time_freq()));
	}

	LastTime = t;
}

void CParticles::RenderGroup(int Group)
{
	Graphics()->BlendNormal();
	//gfx_blend_additive();
    if (Group == GROUP_HCLIENT_TOMBSTONE) // MineTee
        Graphics()->TextureSet(g_pData->m_aImages[IMAGE_MINETEE_FX_TOMBSTONE].m_Id);
    else if (Group == GROUP_BLOCKS) // MineTee
    {
    	if (!m_pClient || !m_pClient->m_pMapimages || !Layers() || !Layers()->MineTeeLayer())
    		return;
    	Graphics()->TextureSet(m_pClient->m_pMapimages->Get(Layers()->MineTeeLayer()->m_Image));
    }
    else
    	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
	Graphics()->QuadsBegin();


	// MineTee
	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
	//


	int i = m_aFirstPart[Group];
	while(i != -1)
	{
		RenderTools()->SelectSprite(m_aParticles[i].m_Spr);
		float a = m_aParticles[i].m_Life / m_aParticles[i].m_LifeSpan;
		vec2 p = m_aParticles[i].m_Pos;
		float Size = mix(m_aParticles[i].m_StartSize, m_aParticles[i].m_EndSize, a);

		Graphics()->QuadsSetRotation(m_aParticles[i].m_Rot);

		Graphics()->SetColor(
			m_aParticles[i].m_Color.r,
			m_aParticles[i].m_Color.g,
			m_aParticles[i].m_Color.b,
			m_aParticles[i].m_Color.a); // pow(a, 0.75f) *

        if (Group == GROUP_HCLIENT_TOMBSTONE)
        {
        	IGraphics::CQuadItem QuadItem(p.x, p.y-Size/2, Size, Size);
			Graphics()->QuadsDraw(&QuadItem, 1);
        }
        else if (Group == GROUP_BLOCKS)
        {
        	// calculate the final pixelsize for the tiles
        	const float TilePixelSize = 1024/32.0f;
        	const float FinalTileSize = Size/(ScreenX1-ScreenX0) * Graphics()->ScreenWidth();
        	const float FinalTilesetScale = FinalTileSize/TilePixelSize;

        	// adjust the texture shift according to mipmap level
        	const float TexSize = 1024.0f;
        	const float Frac = (1.25f/TexSize) * (1/FinalTilesetScale);
        	const float Nudge = (0.5f/TexSize) * (1/FinalTilesetScale);

            const int tx = m_aParticles[i].m_Spr%16;
            const int ty = m_aParticles[i].m_Spr/16;
            const int Px0 = tx*(1024/16);
            const int Py0 = ty*(1024/16);
            const int Px1 = Px0+(1024/16)-1;
            const int Py1 = Py0+(1024/16)-1;

            const float x0 = Nudge + Px0/TexSize+Frac;
            const float y0 = Nudge + Py0/TexSize+Frac;
            const float x1 = Nudge + Px1/TexSize-Frac;
            const float y1 = Nudge + Py0/TexSize+Frac;
            const float x2 = Nudge + Px1/TexSize-Frac;
            const float y2 = Nudge + Py1/TexSize-Frac;
            const float x3 = Nudge + Px0/TexSize+Frac;
            const float y3 = Nudge + Py1/TexSize-Frac;

            Graphics()->QuadsSetSubsetFree(x0, y0, x1, y1, x2, y2, x3, y3);
            IGraphics::CQuadItem QuadItem(p.x, p.y, Size, Size);
            Graphics()->QuadsDraw(&QuadItem, 1);
        }
        else
        {
        	IGraphics::CQuadItem QuadItem(p.x, p.y, Size, Size);
			Graphics()->QuadsDraw(&QuadItem, 1);
        }

		i = m_aParticles[i].m_NextPart;
	}
	Graphics()->QuadsEnd();
	Graphics()->BlendNormal();
}
