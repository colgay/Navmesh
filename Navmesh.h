#pragma once

#include "extdll.h"

#include <vector>

class NavNode
{
public:
	NavNode(const Vector &pos)
	{
		m_pos = pos;
	}

	~NavNode();

	const Vector &GetPos() const
	{
		return m_pos;
	}

	void SetPos(const Vector &pos)
	{
		m_pos = pos;
	}

	bool AddChild(NavNode *pNode);
	bool PopChild(NavNode *pNode);

	const std::vector<NavNode *> &GetChildren() const { return m_children; }

private:
	Vector m_pos;
	std::vector<NavNode *> m_children;
};

class Navmesh;

struct NavChild
{
	Navmesh *pmesh;
	NavNode *pnodes[2];
	Vector dir;
};

class Navmesh
{
public:
	Navmesh() {};
	Navmesh(const Vector &a, const Vector &b, const Vector &c);

	~Navmesh();

	void SetOrigin(const Vector &origin)
	{
		m_origin = origin;
	}

	void SetWidth(const Vector &width)
	{
		m_width = width;
	}

	void SetNormal(const Vector &normal)
	{
		m_normal = normal;
	}

	const Vector &GetOrigin() const
	{
		return m_origin;
	}

	const Vector &GetWidth() const
	{
		return m_width;
	}

	const Vector &GetNormal() const
	{
		return m_normal;
	}

	const std::vector<NavChild *> &GetChildren() const
	{
		return m_children;
	}

	void MakeConnection(NavChild *pChild);
	void RemoveConnection(NavChild *pChild);

	void AddChild(NavChild *pChild);
	void PopChild(NavChild *pChild);
	NavChild *GetChild(Navmesh *pMesh);

	int Along(int n, int axis) const;
	Vector AlongPos(int n, int axis) const;

	Vector Project(const Vector &pos) const;
	float ProjectZ(const Vector &pos) const;
	Vector Clamp(const Vector &pos) const;
	float Distance(const Vector &pos) const;

	bool IsInside(const Vector &pos) const;
	bool IsOnPlane(const Vector &pos) const;

	Vector m_vertices[4];

private:
	Vector m_origin;
	Vector m_width;
	Vector m_normal;

	std::vector<NavChild *> m_children;
};