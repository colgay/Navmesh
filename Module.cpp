#include "amxxmodule.h"
#include "Navmesh.h"
#include "NavGraph.h"
#include "Utils.h"

#include <algorithm>

edict_t *g_pEditor;
Vector g_marked[3];
NavGraph g_graph;
short g_sprite;
float g_time;
char g_pszFilePath[256];

void OnPluginsLoaded()
{
	g_sprite = PRECACHE_MODEL("sprites/zbeam4.spr");

	MF_BuildPathnameR(g_pszFilePath, sizeof(g_pszFilePath), "%s/navmesh/%s.txt", MF_GetLocalInfo("amxx_configsdir", "addons/amxmodx/configs"), STRING(gpGlobals->mapname));

	g_graph.Load(g_pszFilePath);
}

void ServerDeactivate()
{
	g_pEditor = nullptr;
	g_marked[0] = g_marked[1] = g_marked[2] = Vector(0, 0, 0);
	g_graph.Clear();
	g_time = 0;
}

void ClientCommand(edict_t *pEntity)
{
	const char *pszCmd = CMD_ARGV(0);

	if (strcmp(pszCmd, "nav") != 0)
		RETURN_META(MRES_IGNORED);

	pszCmd = CMD_ARGV(1);

	if (strcmp(pszCmd, "editor") == 0)
	{
		const char *pszArg = CMD_ARGV(2);

		if (atoi(pszArg) == 0)
		{
			UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("[Nav] %s has DISABLED navmesh edit mode. (test=%d)\n", STRING(pEntity->v.netname), -1 % 4));
			g_pEditor = nullptr;
			RETURN_META(MRES_SUPERCEDE);
		}
		else if (atoi(pszArg) == 1)
		{
			g_pEditor = pEntity;
			UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("[Nav] %s has ENABLED navmesh edit mode.\n", STRING(pEntity->v.netname)));
			RETURN_META(MRES_SUPERCEDE);
		}

		RETURN_META(MRES_SUPERCEDE);
	}

	if (g_pEditor != pEntity)
	{
		UTIL_ClientPrintAll(HUD_PRINTTALK, "[Nav] Only the navmesh editor can use this command.\n");
		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "mark") == 0)
	{
		int i = -1;

		for (int j = 0; j < 3; j++)
		{
			if (g_marked[j] == Vector(0, 0, 0))
			{
				i = j;
				break;
			}
		}

		if (i == -1)
		{
			UTIL_ClientPrintAll(HUD_PRINTTALK, "[Nav] The operation failed. All 3 points were marked. (You must clear the marked points before you can mark new point.)\n");
			RETURN_META(MRES_SUPERCEDE);
		}

		Vector start, end;
		start = pEntity->v.origin + pEntity->v.view_ofs;
		MAKE_VECTORS(pEntity->v.v_angle);
		end = start + gpGlobals->v_forward * 9999;

		TraceResult ptr;
		TRACE_LINE(start, end, ignore_monsters, pEntity, &ptr);
		
		g_marked[i] = ptr.vecEndPos;

		UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("[Nav] Point %d has been marked {%.2f, %.2f, %.2f}\n", i, g_marked[i].x, g_marked[i].y, g_marked[i].z));
		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "clear") == 0)
	{
		for (int i = 0; i < 3; i++)
		{
			g_marked[i] = Vector(0, 0, 0);
		}

		UTIL_ClientPrintAll(HUD_PRINTTALK, "[Nav] All 3 points have been cleared.\n");
		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "conn") == 0)
	{
		Navmesh *pCurr = g_graph.GetClosest(pEntity->v.origin, 1000, true, true);
		Navmesh *pAim = g_graph.GetByAim(pEntity, 32);

		if (pCurr == nullptr || pAim == nullptr || pCurr == pAim)
		{
			UTIL_ClientPrintAll(HUD_PRINTTALK, "[Nav] Error.");
			RETURN_META(MRES_SUPERCEDE);
		}

		g_graph.Connect(pCurr, pAim);
		g_graph.Connect(pAim, pCurr);

		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "disconn") == 0)
	{
		Navmesh *pCurr = g_graph.GetClosest(pEntity->v.origin, 1000, true, true);
		Navmesh *pAim = g_graph.GetByAim(pEntity, 32);

		if (pCurr == nullptr || pAim == nullptr || pCurr == pAim)
		{
			UTIL_ClientPrintAll(HUD_PRINTTALK, "[Nav] Error.");
			RETURN_META(MRES_SUPERCEDE);
		}

		g_graph.Disconnect(pCurr, pAim);
		g_graph.Disconnect(pAim, pCurr);

		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "create") == 0)
	{
		for (int i = 0; i < 3; i++)
		{
			if (g_marked[i] == Vector(0, 0, 0))
			{
				UTIL_ClientPrintAll(HUD_PRINTTALK, "[Nav] You have to mark down 3 points before you can create navmesh.\n");
				RETURN_META(MRES_SUPERCEDE);
			}
		}

		// Create a navmesh base on 3 points
		Navmesh *pMesh = g_graph.Create(g_marked[0], g_marked[1], g_marked[2]);

		// Clear after
		for (int i = 0; i < 3; i++)
		{
			g_marked[i] = Vector(0, 0, 0);
		}

		UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("[Nav] Navmesh #%d created.\n", g_graph.GetSize()));
		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "remove") == 0)
	{
		Navmesh *pAim = g_graph.GetByAim(pEntity, 32);

		if (pAim == nullptr)
		{
			UTIL_ClientPrintAll(HUD_PRINTTALK, "[Nav] Error.");
			RETURN_META(MRES_SUPERCEDE);
		}

		UTIL_ClientPrintAll(HUD_PRINTTALK, UTIL_VarArgs("[Nav] Navmesh #%d removed.\n", g_graph.GetIndex(pAim)));
		g_graph.Remove(pAim);
		RETURN_META(MRES_SUPERCEDE);
	}

	if (strcmp(pszCmd, "remove") == 0)
	{
		g_graph.Save(g_pszFilePath);
		RETURN_META(MRES_SUPERCEDE);
	}

	RETURN_META(MRES_IGNORED);
}

void PlayerPreThink(edict_t *pEntity)
{
	if (pEntity != g_pEditor)
		RETURN_META(MRES_IGNORED);

	if (gpGlobals->time < g_time + 0.5)
		RETURN_META(MRES_IGNORED);

	g_time = gpGlobals->time;

	for (int i = 0; i < 3; i++)
	{
		if (g_marked[i] == Vector(0, 0, 0))
			continue;

		UTIL_BeamPoints(pEntity,
			Vector(g_marked[i].x, g_marked[i].y, g_marked[i].z),
			Vector(g_marked[i].x, g_marked[i].y, g_marked[i].z + 16),
			g_sprite, 0, 0, 5, 20, 0, 0, 0, 255, 255, 0);
	}

	if (g_graph.GetSize() > 0)
	{
		std::vector<Navmesh *> navmeshes = g_graph.GetNavmeshes();

		Navmesh *pAim = g_graph.GetByAim(pEntity, 32);

		if (pAim != nullptr)
		{
			Vector origin = pAim->GetOrigin();
			Vector norm = pAim->GetNormal();

			UTIL_BeamPoints(pEntity, origin, origin + norm * 16, g_sprite, 0, 0, 5, 10, 0, 255, 255, 0, 255, 0);

			for (int i = 0, j = 4 - 1; i < 4; j = i++)
			{
				UTIL_BeamPoints(pEntity,
					Vector(pAim->m_vertices[i].x, pAim->m_vertices[i].y, pAim->m_vertices[i].z + 1),
					Vector(pAim->m_vertices[j].x, pAim->m_vertices[j].y, pAim->m_vertices[j].z + 1),
					g_sprite, 0, 0, 5, 10, 0, 255, 255, 0, 255, 0);
			}

			auto children = pAim->GetChildren();

			for (auto it = children.cbegin(); it != children.cend(); ++it)
			{
				NavChild *pChild = *it;
				Vector c = (pChild->pnodes[0]->GetPos() + pChild->pnodes[1]->GetPos()) / 2;
				Vector a = c + pChild->dir * 10;
				Vector b = c - pChild->dir * 10;

				UTIL_BeamPoints(pEntity,
					Vector(a.x, a.y, a.z + 3),
					Vector(b.x, b.y, b.z + 3),
					g_sprite, 0, 0, 5, 10, 4, 250, 100, 0, 255, 5);
			}
		}

		for (int i = 0; i < 8; i++)
		{
			Navmesh *pMin = nullptr;
			float minDist = FLT_MAX;

			for (auto it = navmeshes.cbegin(); it != navmeshes.cend(); ++it)
			{
				Navmesh *pMesh = *it;

				if (!pMesh->IsOnPlane(pEntity->v.origin))
					continue;

				float dist = pMesh->Distance(pEntity->v.origin);
				if (dist < minDist)
				{
					minDist = dist;
					pMin = pMesh;
				}
			}

			if (pMin == nullptr)
				continue;

			navmeshes.erase(std::remove(navmeshes.begin(), navmeshes.end(), pMin), navmeshes.end());

			if (pAim == pMin)
				continue;

			for (int i = 0, j = 4 - 1; i < 4; j = i++)
			{
				UTIL_BeamPoints(pEntity,
					Vector(pMin->m_vertices[i].x, pMin->m_vertices[i].y, pMin->m_vertices[i].z + 1), 
					Vector(pMin->m_vertices[j].x, pMin->m_vertices[j].y, pMin->m_vertices[j].z + 1),
					g_sprite, 0, 0, 5, 10, 0, 0, 255, 0, 255, 0);
			}
		}

		std::vector<NavNode *> nodes = g_graph.m_nodeList.GetNodes();
		
		NavNode *pClosest = g_graph.m_nodeList.GetClosest(pEntity->v.origin, 50);

		if (pClosest != nullptr)
		{
			Vector p = pClosest->GetPos();

			UTIL_BeamPoints(pEntity,
				Vector(p.x, p.y, p.z),
				Vector(p.x, p.y, p.z + 20),
				g_sprite, 0, 0, 5, 10, 0, 255, 255, 0, 255, 0);

			auto children = pClosest->GetChildren();

			for (auto it = children.cbegin(); it != children.cend(); it++)
			{
				NavNode *pNode = *it;

				Vector a = pClosest->GetPos();
				Vector b = pNode->GetPos();

				UTIL_BeamPoints(pEntity,
					Vector(a.x, a.y, a.z + 20),
					Vector(b.x, b.y, b.z + 20),
					g_sprite, 0, 0, 5, 10, 5, 0, 50, 255, 255, 0);
			}
		}

		for (int i = 0; i < 10; i++)
		{
			NavNode *pMin = nullptr;
			float minDist = FLT_MAX;

			for (auto it = nodes.cbegin(); it != nodes.cend(); ++it)
			{
				NavNode *pNode = *it;

				float dist = (pNode->GetPos() - pEntity->v.origin).Length();
				if (dist < minDist)
				{
					minDist = dist;
					pMin = pNode;
				}
			}

			if (pMin == nullptr)
				continue;

			nodes.erase(std::remove(nodes.begin(), nodes.end(), pMin), nodes.end());

			if (pMin == pClosest)
				continue;

			Vector p = pMin->GetPos();

			UTIL_BeamPoints(pEntity,
				Vector(p.x, p.y, p.z),
				Vector(p.x, p.y, p.z + 20),
				g_sprite, 0, 0, 5, 10, 0, 0, 200, 100, 255, 0);
		}
	}

	RETURN_META(MRES_IGNORED);
}