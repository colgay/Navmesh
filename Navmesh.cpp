#include "Navmesh.h"

#include "meta_api.h"

#include <array>
#include <algorithm>

NavNode::~NavNode()
{
}

bool NavNode::AddChild(NavNode *pNode)
{
	auto it = std::find(m_children.cbegin(), m_children.cend(), pNode);
	if (pNode == this || it != m_children.cend())
		return false;

	m_children.push_back(pNode);
	return true;
}

bool NavNode::PopChild(NavNode *pNode)
{
	m_children.erase(std::remove(m_children.begin(), m_children.end(), pNode), m_children.end());
	return true;
}

Navmesh::Navmesh(const Vector &a, const Vector &b, const Vector &c)
{
	m_normal = CrossProduct(b - a, c - a).Normalize();
	m_origin = (a + b + c) / 3;

	Vector angle;
	VEC_TO_ANGLES(m_normal, angle);
	if (angle.x > 180)
		m_normal = m_normal * -1;

	std::array<Vector, 3> arr = { a, b ,c };

	auto mmx = std::minmax_element(arr.begin(), arr.end(),
		[](const Vector& lhs, const Vector& rhs) {
			return lhs.x < rhs.x;
		}
	);

	auto mmy = std::minmax_element(arr.begin(), arr.end(),
		[](const Vector& lhs, const Vector& rhs) {
			return lhs.y < rhs.y;
		}
	);

	Vector min = Vector(mmx.first->x, mmy.first->y, 0);
	Vector max = Vector(mmx.second->x, mmy.second->y, 0);

	m_vertices[0] = min;
	m_vertices[1] = Vector(min.x, max.y, 0);
	m_vertices[2] = max;
	m_vertices[3] = Vector(max.x, min.y, 0);

	for (int i = 0; i < 4; i++)
	{
		m_vertices[i] = Project(m_vertices[i]);
	}

	m_origin = (m_vertices[0] + m_vertices[2]) / 2;
	m_origin = Project(m_origin);

	m_width = max - m_origin;
}

Navmesh::~Navmesh()
{
}

NavChild *Navmesh::GetChild(Navmesh *pMesh)
{
	auto pred = [=](const NavChild *pChild) { return pChild->pmesh == pMesh; };

	auto it = std::find_if(m_children.cbegin(), m_children.cend(), pred);
	if (it == m_children.cend())
		return nullptr;

	return *it;
}

void Navmesh::AddChild(NavChild *pChild)
{
	m_children.push_back(pChild);
}

void Navmesh::PopChild(NavChild *pChild)
{
	m_children.erase(std::remove(m_children.begin(), m_children.end(), pChild), m_children.end());
}

void Navmesh::MakeConnection(NavChild *pChild)
{
	for (auto it = m_children.cbegin(); it != m_children.cend(); it++)
	{
		NavChild *pChild2 = *it;

		pChild2->pnodes[0]->AddChild(pChild->pnodes[0]);
		pChild2->pnodes[0]->AddChild(pChild->pnodes[1]);
		pChild2->pnodes[1]->AddChild(pChild->pnodes[0]);
		pChild2->pnodes[1]->AddChild(pChild->pnodes[1]);
	}
}

void Navmesh::RemoveConnection(NavChild *pChild)
{
	for (auto it = m_children.cbegin(); it != m_children.cend(); it++)
	{
		NavChild *pChild2 = *it;

		pChild2->pnodes[0]->PopChild(pChild->pnodes[0]);
		pChild2->pnodes[0]->PopChild(pChild->pnodes[1]);
		pChild2->pnodes[1]->PopChild(pChild->pnodes[0]);
		pChild2->pnodes[1]->PopChild(pChild->pnodes[1]);
	}
}

int Navmesh::Along(int n, int axis) const
{
	int r = 0;

	if (axis == 0) // x
	{
		switch (n)
		{
		case 0:
			r = 1;
			break;
		case 1:
			r = 0;
			break;
		case 2:
			r = 3;
			break;
		case 3:
			r = 2;
			break;
		}
	}
	else // y
	{
		switch (n)
		{
		case 0:
			r = 3;
			break;
		case 1:
			r = 2;
			break;
		case 2:
			r = 1;
			break;
		case 3:
			r = 0;
			break;
		}
	}

	return r;
}

Vector Navmesh::AlongPos(int n, int axis) const
{
	int x = Along(n, axis);
	return m_vertices[x];
}

Vector Navmesh::Project(const Vector &pos) const
{
	Vector p = pos; p.z = 9999;
	Vector d = Vector(0, 0, -1).Normalize();

	float t = (DotProduct(m_normal, m_origin) - DotProduct(m_normal, p)) / DotProduct(m_normal, d);
	return p + d * t;
}

float Navmesh::ProjectZ(const Vector &pos) const
{
	return Project(pos).z;
}

Vector Navmesh::Clamp(const Vector &pos) const
{
	Vector v = pos;
	v.x = min(m_vertices[2].x, max(v.x, m_vertices[0].x));
	v.y = min(m_vertices[2].y, max(v.y, m_vertices[0].y));
	v = Project(v);

	return v;
}

bool Navmesh::IsInside(const Vector &pos) const
{
	if (pos.x >= m_vertices[0].x && pos.y >= m_vertices[0].y
		&& pos.x <= m_vertices[2].x && pos.y <= m_vertices[2].y)
		return true;

	return false;
}

bool Navmesh::IsOnPlane(const Vector &pos) const
{
	float t = DotProduct(m_normal, pos - m_origin);
	if (t >= 0)
		return true;

	return false;
}

float Navmesh::Distance(const Vector &pos) const
{
	float sb, sn, sd;

	sn = -DotProduct(m_normal, (pos - m_origin));
	sd = DotProduct(m_normal, m_normal);
	sb = sn / sd;

	Vector B = Clamp(pos + sb * m_normal);
	
	return (pos - B).Length();
}

