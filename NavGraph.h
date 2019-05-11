#pragma once

#include "extdll.h"

#include <vector>

class Navmesh;
class NavNode;

class NavNodeList
{
public:
	NavNodeList() {};
	~NavNodeList();

	NavNode *Create(const Vector &pos);
	void Clear(NavNode *pNode) { m_nodes.clear(); }
	void Remove(NavNode *pNode);

	NavNode *GetClosest(const Vector &origin, float distance);

	std::vector<NavNode *> &GetNodes() { return m_nodes; };

private:
	std::vector<NavNode *> m_nodes;
};

class NavGraph
{
public:
	NavGraph() {};
	~NavGraph();

	Navmesh *Create(const Vector &a, const Vector &b, const Vector &c);
	void Remove(Navmesh *pMesh);

	bool Connect(Navmesh *pa, Navmesh *pb);
	bool Disconnect(Navmesh *pa, Navmesh *pb);

	Navmesh *GetByAim(edict_t *pEntity, float distance) const;
	Navmesh *GetClosest(const Vector &origin, float distance, bool onPlane = true, bool inside = false) const;

	int GetIndex(Navmesh *pMesh) const;
	size_t GetSize() const { return m_navmeshes.size(); };
	void Clear();

	std::vector<Navmesh *> &GetNavmeshes() { return m_navmeshes; };
	void Save(const char *pszPath) const;
	void Load(const char *pszPath);

	NavGraph *Clone() { return new NavGraph(*this); }
	
	Navmesh *At(int x) {
		return m_navmeshes.at(x);
	}

	NavNodeList m_nodeList;

private:
	std::vector<Navmesh *> m_navmeshes;
};