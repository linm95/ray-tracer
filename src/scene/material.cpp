#include "material.h"
#include "../ui/TraceUI.h"
#include "light.h"
#include "ray.h"
extern TraceUI* traceUI;

#include <glm/gtx/io.hpp>
#include <iostream>
#include "../fileio/images.h"

using namespace std;
extern bool debugMode;

#define FIX RAY_EPSILON

Material::~Material()
{
}

// Apply the phong model to this point on the surface of the object, returning
// the color of that point.
glm::dvec3 Material::shade(Scene* scene, const ray& r, const isect& i) const
{
	// YOUR CODE HERE

	// For now, this method just returns the diffuse color of the object.
	// This gives a single matte color for every distinct surface in the
	// scene, and that's it.  Simple, but enough to get you started.
	// (It's also inconsistent with the phong model...)

	// Your mission is to fill in this method with the rest of the phong
	// shading model, including the contributions of all the light sources.
	// You will need to call both distanceAttenuation() and
	// shadowAttenuation()
	// somewhere in your code in order to compute shadows and light falloff.
	//	if( debugMode )
	//		std::cout << "Debugging Phong code..." << std::endl;


	// When you're iterating through the lights,
	// you'll want to use code that looks something
	// like this:
	//
	// for ( const auto& pLight : scene->getAllLights() )
	// {
	//              // pLight has type unique_ptr<Light>
	// 		.
	// 		.
	// 		.
	// }

	glm::dvec3 color = glm::dvec3(0,0,0);

	glm::dvec3 n = i.getN();
	glm::dvec3 v = -r.getDirection();
	bool internal = glm::dot(n, v) < 0;
	n = internal ? -n : n;
	glm::dvec3 pos = r.at(i) + FIX * n;
	glm::dvec3 kdd = kd(i);
	glm::dvec3 kss = ks(i);
	bool ignoreKD = true, ignoreKS = true;
	for(int k = 0; k < 3; k++){
		if(ignoreKD && kdd[k] > RAY_EPSILON) ignoreKD = false;
		if(ignoreKS && kss[k] > RAY_EPSILON) ignoreKS = false;
	}
	double shine = shininess(i);
	for (const auto& pLight : scene->getAllLights()){
		glm::dvec3 l = pLight->getDirection(pos);
		glm::dvec3 rvec = 2 * glm::dot(l, n) * n - l;
		double ln = i.getMaterial().Trans() ? abs(glm::dot(l, n)) : max(glm::dot(l, n), 0.0);
		double vr = pow(max(glm::dot(v, rvec), 0.0), shine);
		if((ignoreKD || ln < RAY_EPSILON) && (ignoreKS || vr < RAY_EPSILON)) continue;

		glm::dvec3 Iin = pLight->shadowAttenuation(r, pos);
		if(debugMode){
			std::cout << "Iin: " << Iin << std::endl;
		}
		Iin *= pLight->distanceAttenuation(r.at(i));
		glm::dvec3 lightColor = pLight->getColor();

		color += (kdd * ln + kss * vr) * lightColor * Iin;
	}

	color += ke(i) + ka(i) * scene->ambient();
	// for (int k = 0; k < 3; k++){
	// 	color[k] *= r.getAtten()[k];
	// }
	// if(debugMode){
	// 	std::cout << "color: " << color << std::endl<<std::endl;
	// }
	return color;
}

TextureMap::TextureMap(string filename)
{
	data = readImage(filename.c_str(), width, height);
	if (data.empty()) {
		width = 0;
		height = 0;
		string error("Unable to load texture map '");
		error.append(filename);
		error.append("'.");
		throw TextureMapException(error);
	}
}

glm::dvec3 TextureMap::getMappedValue(const glm::dvec2& coord) const
{
	// YOUR CODE HERE
	//
	// In order to add texture mapping support to the
	// raytracer, you need to implement this function.
	// What this function should do is convert from
	// parametric space which is the unit square
	// [0, 1] x [0, 1] in 2-space to bitmap coordinates,
	// and use these to perform bilinear interpolation
	// of the values.

	double x = coord[0] * double(width - 1);
	double y = coord[1] * double(height - 1);

	int x1 = int(x);
	int x2 = x1 + 1;
	int y1 = int(y);
	int y2 = y1 + 1;
	glm::dvec3 color11 = getPixelAt(x1, y1);
	glm::dvec3 color12 = getPixelAt(x1, y2);
	glm::dvec3 color21 = getPixelAt(x2, y1);
	glm::dvec3 color22 = getPixelAt(x2, y2);
	glm::dvec3 color = color11 * (double(x2) - x) * (double(y2) - y) +
										 color21 * (x - double(x1)) * (double(y2) - y) +
										 color12 * (double(x2) - x) * (y - double(y1)) +
										 color22 * (x - double(x1)) * (y - double(y1));

	return color;
}

glm::dvec3 TextureMap::getPixelAt(int x, int y) const
{
	// YOUR CODE HERE
	//
	// In order to add texture mapping support to the
	// raytracer, you need to implement this function.
	glm::dvec3 color(0, 0, 0);
	if(x < 0 || x >= width || y < 0 || y >= height )
		return color;
	if(data.size() < 3 * width * height)
		return color;
	int offset = x + y * width;
	for(int k = 0; k < 3; k++)
		color[k] = double(data[k + offset * 3]);
	return color / 255.0;
}

glm::dvec3 MaterialParameter::value(const isect& is) const
{
	if (0 != _textureMap)
		return _textureMap->getMappedValue(is.getUVCoordinates());
	else
		return _value;
}

double MaterialParameter::intensityValue(const isect& is) const
{
	if (0 != _textureMap) {
		glm::dvec3 value(
		        _textureMap->getMappedValue(is.getUVCoordinates()));
		return (0.299 * value[0]) + (0.587 * value[1]) +
		       (0.114 * value[2]);
	} else
		return (0.299 * _value[0]) + (0.587 * _value[1]) +
		       (0.114 * _value[2]);
}
