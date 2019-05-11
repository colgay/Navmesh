#include "NavGraph.h"
#include "Navmesh.h"
#include "meta_api.h"

#include <algorithm>
#include <string>

NavGraph::~NavGraph()
{
	while (m_navmeshes.size())
	{
		// release memory
		Remove(m_navmeshes.front());
	}
}

Navmesh *NavGraph::Create(const Vector &a, const Vector &b, const Vector &c)
{
	Navmesh *pMesh = new Navmesh(a, b, c);

	m_navmeshes.push_back(pMesh);
	return pMesh;
}

void NavGraph::Remove(Navmesh *pMesh)
{
	// Disconnect the mesh that was connected to this mesh
	for (auto it = m_navmeshes.begin(); it != m_navmeshes.end(); ++it)
	{
		Navmesh *pmesh = *it;
		Disconnect(pmesh, pMesh);
	}

	m_navmeshes.erase(std::remove(m_navmeshes.begin(), m_navmeshes.end(), pMesh), m_navmeshes.end());
	delete pMesh; // release memory
}

bool NavGraph::Connect(Navmesh * pa, Navmesh * pb)
{
	struct
	{
		int ax, c0, c1;
	} closest;

	float gap = FLT_MAX;

	for (int axis = 0; axis < 2; axis++)
	{
		int c0 = 0, c1 = 0;

		if (pb->m_vertices[2][axis] < pa->m_vertices[0][axis])
		{
			c0 = 0; c1 = 2;
		}
		else if (pb->m_vertices[0][axis] > pa->m_vertices[2][axis])
		{
			c0 = 2; c1 = 0;
		}
		else if (pb->m_vertices[0][axis] <= pa->m_vertices[2][axis])
		{
			c0 = 2; c1 = 0;
		}
		else if (pb->m_vertices[2][axis] >= pa->m_vertices[0][axis])
		{
			c0 = 0; c1 = 2;
		}
		else
		{
			continue;
		}

		float d = fabs(pa->m_vertices[c0][axis] - pb->m_vertices[c1][axis]);
		if (d < gap)
		{
			gap = d;

			closest.c0 = c0;
			closest.c1 = c1;
			closest.ax = axis;
		}
	}

	if (gap == FLT_MAX)
		return nullptr;

	std::vector<Vector> list = {
		pa->m_vertices[closest.c0],
		pa->AlongPos(closest.c0, closest.ax),
		pb->m_vertices[closest.c1],
		pb->AlongPos(closest.c1, closest.ax)
	};

	int ax = closest.ax ? 0 : 1;

	auto pred = [ax](const Vector &a, const Vector &b) -> bool
	{
		return a[ax] > b[ax];
	};

	std::sort(list.begin(), list.end(), pred);

	NavChild *pc0 = new NavChild;
	pc0->pmesh = pb;

	NavChild *pc1 = pb->GetChild(pa);
	if (pc1 != nullptr)
	{
		pc0->pnodes[0] = pc1->pnodes[0];
		pc0->pnodes[1] = pc1->pnodes[1];

		auto children = pa->GetChildren();
		for (auto it = children.cbegin(); it != children.cend(); it++)
		{
			NavChild *pch = *it;

			for (int i = 0; i < 2; i++)
			{
				for (int j = 0; j < 2; j++)
				{
					pc0->pnodes[i]->AddChild(pch->pnodes[j]);
					pch->pnodes[i]->AddChild(pc0->pnodes[j]);
				}
			}
		}
	}
	else
	{
		Vector v[2];
		v[0] = (pa->Clamp(list[1]) + pb->Clamp(list[1])) / 2;
		v[1] = (pa->Clamp(list[2]) + pb->Clamp(list[2])) / 2;

		NavNode *pnodes[2] = { nullptr, nullptr };

		auto nodes = m_nodeList.GetNodes();

		for (int i = 0; i < 2; i++)
		{
			for (auto it = nodes.begin(); it != nodes.end(); it++)
			{
				Vector p = (*it)->GetPos();

				if ((p - v[i]).Length() <= 20)
				{
					SERVER_PRINT("PASS dist\n");
					TraceResult ptr;
					TRACE_HULL(Vector(p.x, p.y, p.z + 36), Vector(v[i].x, v[i].y, v[i].z + 36),
						ignore_monsters, head_hull, NULL, &ptr);

					// not blocked
					if (!ptr.fStartSolid && !ptr.fAllSolid && ptr.fInOpen)
					{
						SERVER_PRINT("PASS solid\n");
						(*it)->SetPos((p + v[i]) / 2);
						pnodes[i] = *it;
						break;
					}
				}
			}
		}

		for (int i = 0; i < 2; i++)
		{
			if (pnodes[i] == nullptr)
				pc0->pnodes[i] = m_nodeList.Create(v[i]);
			else
				pc0->pnodes[i] = pnodes[i];
		}

		pc0->pnodes[0]->AddChild(pc0->pnodes[1]);
		pc0->pnodes[1]->AddChild(pc0->pnodes[0]);

		auto children = pb->GetChildren();
		for (auto it = children.cbegin(); it != children.cend(); it++)
		{
			NavChild *pch = *it;

			for (int i = 0; i < 2; i++)
			{
				for (int j = 0; j < 2; j++)
				{
					pc0->pnodes[i]->AddChild(pch->pnodes[j]);
					pch->pnodes[i]->AddChild(pc0->pnodes[j]);
				}
			}
		}

		children = pa->GetChildren();
		for (auto it = children.cbegin(); it != children.cend(); it++)
		{
			NavChild *pch = *it;

			for (int i = 0; i < 2; i++)
			{
				for (int j = 0; j < 2; j++)
				{
					pc0->pnodes[i]->AddChild(pch->pnodes[j]);
					pch->pnodes[i]->AddChild(pc0->pnodes[j]);
				}
			}
		}
	}

	Vector a, b, c;
	c = (pc0->pnodes[0]->GetPos() + pc0->pnodes[1]->GetPos()) / 2;
	a = pa->Clamp(c);
	b = pb->Clamp(c);
	pc0->dir = (b - a).Normalize();

	pa->AddChild(pc0);
	return true;
}

bool NavGraph::Disconnect(Navmesh * pa, Navmesh * pb)
{
	auto pred = [=](const NavChild *pChild)
	{
		return pChild->pmesh == pb;
	};

	auto children = pa->GetChildren();

	auto iter = std::find_if(children.begin(), children.end(), pred);
	if (iter == children.end())
		return false;

	NavChild *pc0 = *iter;

	for (auto it = children.cbegin(); it != children.cend(); it++)
	{
		NavChild *pch = *it;

		pch->pnodes[0]->PopChild(pc0->pnodes[0]);
		pch->pnodes[0]->PopChild(pc0->pnodes[1]);
		pch->pnodes[1]->PopChild(pc0->pnodes[0]);
		pch->pnodes[1]->PopChild(pc0->pnodes[1]);
	}

	children = pb->GetChildren();
	for (auto it = children.cbegin(); it != children.cend(); it++)
	{
		NavChild *pch = *it;

		pch->pnodes[0]->PopChild(pc0->pnodes[0]);
		pch->pnodes[0]->PopChild(pc0->pnodes[1]);
		pch->pnodes[1]->PopChild(pc0->pnodes[0]);
		pch->pnodes[1]->PopChild(pc0->pnodes[1]);
	}

	if (pb->GetChild(pa) == nullptr)
	{
		for (int i = 0; i < 2; i++)
		{
			m_nodeList.Remove(pc0->pnodes[i]);
		}
	}

	pa->PopChild(pc0);
}

void NavGraph::Clear()
{
	for (auto it = m_navmeshes.begin(); it != m_navmeshes.end(); ++it)
	{
		delete *it;
	}

	m_navmeshes.clear();
}

Navmesh *NavGraph::GetByAim(edict_t *pEntity, float distance) const
{
	Vector start, end;
	start = pEntity->v.origin + pEntity->v.view_ofs;
	MAKE_VECTORS(pEntity->v.v_angle);
	end = start + gpGlobals->v_forward * 9999;

	TraceResult ptr;
	TRACE_LINE(start, end, ignore_monsters, pEntity, &ptr);

	Vector pos = ptr.vecEndPos;

	return GetClosest(pos, distance, false, false);
}

Navmesh *NavGraph::GetClosest(const Vector &origin, float distance, bool onPlane, bool inside) const
{
	float minDist = distance;
	Navmesh *pResult = nullptr;

	for (auto it = m_navmeshes.cbegin(); it != m_navmeshes.cend(); ++it)
	{
		Navmesh *pMesh = *it;
		
		if (onPlane && !pMesh->IsOnPlane(origin))
			continue;

		if (inside && !pMesh->IsInside(origin))
			continue;

		float dist = pMesh->Distance(origin);
		if (dist < minDist)
		{
			minDist = dist;
			pResult = pMesh;
		}
	}

	return pResult;
}

int NavGraph::GetIndex(Navmesh *pMesh) const
{
	auto it = std::find(m_navmeshes.cbegin(), m_navmeshes.cend(), pMesh);

	return std::distance(m_navmeshes.cbegin(), it);
}

void NavGraph::Save(const char *pszPath) const
{
	FILE *pFile = fopen(pszPath, "w");
	if (pFile != NULL)
	{
		int i = 0;
		for (auto it = m_navmeshes.cbegin(); it != m_navmeshes.cend(); ++it)
		{
			Navmesh *pMesh = *it;
			Vector orig = pMesh->GetOrigin();
			Vector norm = pMesh->GetNormal();
			Vector width = pMesh->GetWidth();

			fprintf(pFile, "#%d  %f %f %f  %f %f %f  %f %f %f ", GetIndex(pMesh),
				orig.x, orig.y, orig.z,
				norm.x, norm.y, norm.z,
				width.x, width.y, width.z);

			for (int j = 0; j < 4; j++)
			{
				orig = pMesh->m_vertices[j];

				fprintf(pFile, "  %f %f %f",
					orig.x, orig.y, orig.z);
			}

			fputs("   [", pFile);

			auto children = pMesh->GetChildren();

			for (auto iter = children.cbegin(); iter != children.cend(); iter++)
			{
				NavChild *pChild = *iter;
				fprintf(pFile, "%d,", GetIndex(pChild->pmesh));
			}

			fputs("]\n", pFile);
		}

		fclose(pFile);
	}
}

void NavGraph::Load(const char *pszPath)
{
	Clear();

	FILE *pFile = fopen(pszPath, "r");
	if (pFile != NULL)
	{
		std::vector<std::string> neightbors;
		char szLine[512];
		int index = -1;

		while (fgets(szLine, sizeof(szLine), pFile))
		{
			if (!szLine[0])
				continue;

			if (szLine[0] == '#')
			{
				char *pszChar = strtok(szLine, " ") + 1;
				index = atoi(pszChar);

				Navmesh *pMesh = new Navmesh();
				m_navmeshes.push_back(pMesh);

				// origin
				pszChar = strtok(NULL, " ");

				Vector v;
				for (int i = 0; i < 3; i++)
				{
					v[i] = atof(pszChar);
					pszChar = strtok(NULL, " ");
				}

				SERVER_PRINT(UTIL_VarArgs("orig = %.f %.f %.f\n", v.x, v.y, v.z));
				pMesh->SetOrigin(v);

				// normal
				for (int i = 0; i < 3; i++)
				{
					v[i] = atof(pszChar);
					pszChar = strtok(NULL, " ");
				}

				SERVER_PRINT(UTIL_VarArgs("norm = %.f %.f %.f\n", v.x, v.y, v.z));
				pMesh->SetNormal(v);

				// width
				for (int i = 0; i < 3; i++)
				{
					v[i] = atof(pszChar);
					pszChar = strtok(NULL, " ");
				}

				SERVER_PRINT(UTIL_VarArgs("width = %.f %.f %.f\n", v.x, v.y, v.z));
				pMesh->SetWidth(v);

				// corners
				for (int i = 0; i < 4; i++)
				{
					for (int j = 0; j < 3; j++)
					{
						v[j] = atof(pszChar);
						pszChar = strtok(NULL, " ");
					}

					SERVER_PRINT(UTIL_VarArgs("m_vertices[%d] = %.f %.f %.f\n", i, v.x, v.y, v.z));
					pMesh->m_vertices[i] = v;
				}

				// neighbors
				if (strlen(pszChar) > 2)
				{
					pszChar[strlen(pszChar) - 2] = 0;
					pszChar++;

					neightbors.push_back(std::to_string(index) + ":" + pszChar);
					SERVER_PRINT(UTIL_VarArgs("lala = %s\n", (std::to_string(index) + ":" + pszChar).c_str()));
				}
			}
		}

		for (auto it = neightbors.cbegin(); it != neightbors.cend(); ++it)
		{
			std::string str = *it;
			std::string::size_type start = str.find_first_of(':');
			std::string token = str.substr(0, start);
			int index = std::stoi(token);

			Navmesh *pMesh = At(index);
			if (pMesh == nullptr)
				continue;

			std::string delim = ",";
			start++;
			std::string::size_type end = str.find(delim);

			while (end != std::string::npos)
			{
				token = str.substr(start, end - start);
				SERVER_PRINT(UTIL_VarArgs("token=%s", token.c_str()));
				index = std::stoi(token);

				Navmesh *pMesh2 = At(index);
				if (pMesh2 == nullptr)
					continue;

				this->Connect(pMesh, pMesh2);
				SERVER_PRINT(UTIL_VarArgs("#%d connected to #%d\n", GetIndex(pMesh), GetIndex(pMesh2)));

				start = end + delim.length();
				end = str.find(delim, start);
			}
		}

		fclose(pFile);
	}
}

NavNodeList::~NavNodeList()
{
	while (m_nodes.size() > 0)
	{
		Remove(m_nodes.front());
	}
}

NavNode *NavNodeList::Create(const Vector &pos)
{
	NavNode *pNode = new NavNode(pos);
	m_nodes.push_back(pNode);
	return pNode;
}

void NavNodeList::Remove(NavNode * pNode)
{
	for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it)
	{
		NavNode *pnode = *it;
		pnode->PopChild(pNode);
	}

	m_nodes.erase(std::remove(m_nodes.begin(), m_nodes.end(), pNode), m_nodes.end());
	delete pNode;
}

NavNode * NavNodeList::GetClosest(const Vector & origin, float distance)
{
	float minDist = distance;
	NavNode *pResult = nullptr;

	for (auto it = m_nodes.cbegin(); it != m_nodes.cend(); ++it)
	{
		NavNode *pNode = *it;

		float dist = (pNode->GetPos() - origin).Length();
		if (dist < minDist)
		{
			minDist = dist;
			pResult = pNode;
		}
	}

	return pResult;
}