#include "cubeMap.h"
#include "ray.h"
#include "../ui/TraceUI.h"
#include "../scene/material.h"
extern TraceUI* traceUI;

#include <iostream>

glm::dvec3 CubeMap::getColor(ray r) const
{
	// YOUR CODE HERE
	// FIXME: Implement Cube Map here
	glm::dvec3 d = r.getDirection();
	//std::cout << d << std::endl;
	double x = d[0], y = d[1], z = - d[2];

	double abx = fabs(x), aby = fabs(y), abz = fabs(z);
	double max, u, v;
	int idx;
	if(abx >= aby && abx >= abz){
		max = abx;
		u = x > 0 ? -z : z;
		v = y;
		idx = x > 0 ? 0 : 1;
	}
	else if (aby >= abx && aby >= abz) {
    max = aby;
    u = x;
    v = y > 0 ? -z : z;
    idx = y > 0 ? 2 : 3;
  }
  else if (abz >= abx && abz >= aby) {
    max = abz;
    u = z > 0 ? x : -x;
    v = y;
    idx = z > 0 ? 4 : 5;
  }
	u = 0.5 * (u / max + 1.0);
	v = 0.5 * (v / max + 1.0);
	glm::dvec2 coord = glm::dvec2(u, v);
	return tMap[idx] -> getMappedValue(coord);
}

CubeMap::CubeMap()
{
}

CubeMap::~CubeMap()
{
}

void CubeMap::setNthMap(int n, TextureMap* m)
{
	if (m != tMap[n].get())
		tMap[n].reset(m);
}
