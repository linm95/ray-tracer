// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/io.hpp>
#include <string.h> // for memset

#include <random>

#include <iostream>
#include <fstream>

#define FIX RAY_EPSILON
using namespace std;
extern TraceUI* traceUI;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through pixel(i,j), i.e. normalized window coordinates (x,y),
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.
double getRandom(){
	return ((double) rand() / RAND_MAX);
}
glm::dvec3 RayTracer::trace(double x, double y)
{
	// Clear out the ray cache in the scene for debugging purposes,

	if (TraceUI::m_debug)
		scene->intersectCache.clear();

	ray r(glm::dvec3(0,0,0), glm::dvec3(0,0,0), glm::dvec3(1,1,1), ray::VISIBILITY);
	scene->getCamera().rayThrough(x,y,r);
	double dummy;

	glm::dvec3 ret = traceRay(r, glm::dvec3(1.0,1.0,1.0), traceUI->getDepth(), dummy);
	ret = glm::clamp(ret, 0.0, 1.0);
	return ret;
}

glm::dvec3 RayTracer::tracePixel(int i, int j, bool aa)
{
	glm::dvec3 col(0,0,0);

	if( ! sceneLoaded() ) return col;

	if(!aa){

		double x = double(i)/double(buffer_width);
		double y = double(j)/double(buffer_height);


		col = trace(x, y);

	}else{
		int s = sqrt(double(samples) + 0.5);
		for(int m = 0; m < s; m++)
			for(int n = 0; n < s; n++){
				double x = s * (i + getRandom()) / (buffer_width * s);
				double y = s * (j + getRandom()) / (buffer_height * s);
				col += trace(x, y);
			}
		col /= double(s * s);
	}
	unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
	return col;

}

#define VERBOSE 0

// Do recursive ray tracing!  You'll want to insert a lot of code here
// (or places called from here) to handle reflection, refraction, etc etc.
glm::dvec3 RayTracer::traceRay(ray& r, const glm::dvec3& thresh, int depth, double& t)
{

	if(depth < 0){
		return glm::dvec3(0.0, 0.0, 0.0);
	}
	isect i;
	glm::dvec3 colorC;
#if VERBOSE
	std::cerr << "== current depth: " << depth << std::endl;
#endif

	if(scene->intersect(r, i)) {
		// YOUR CODE HERE
		//std::cout << i.getT()<< " "<< glm::distance(r.at(i), r.getPosition()) << std::endl;
		// An intersection occurred!  We've got work to do.  For now,
		// this code gets the material for the surface that was intersected,
		// and asks that material to provide a color for the ray.

		// This is a great place to insert code for recursive ray tracing.
		// Instead of just returning the result of shade(), add some
		// more steps: add in the contributions from reflected and refracted
		// rays.
		t = i.getT();
		const Material& m = i.getMaterial();
		colorC = m.shade(scene.get(), r, i);
		glm::dvec3 n = i.getN();
		glm::dvec3 l = - r.getDirection();

		/*if(debugMode){
		std::cout<< "dir " << l << " " << n << " " <<std::endl;
	}*/

		//glm::dvec3 atten = r.getAtten();
		bool internal = glm::dot(l, n) < 0;

		//reflection:
		if(m.Refl() && !internal){
			glm::dvec3 reflect = 2 * glm::dot(l, n) * n - l;
			glm::dvec3 pos = r.at(i) + FIX * n;
			ray reflectRay = ray(pos, reflect, glm::dvec3(1, 1, 1), ray::REFLECTION);
			double tt = 0;
			colorC += m.kr(i) * traceRay(reflectRay, thresh, depth - 1, tt);
		}

		//refraction:
		if(m.Trans()){
			l = -l;
			double index;
			if (internal) {
				index = m.index(i);
				n = -n;
			}
			else{
				index = 1 / m.index(i);
			}
			double cosi = abs(glm::dot(n, l));
			double w = index * cosi;
			double temp = 1 + (w - index) * (w + index);
			if(temp > 0){
				double cost = sqrt(temp);
				glm::dvec3 refract = (w - cost) * n + index * l;
				glm::dvec3 pos = r.at(i) - FIX * n;
				ray refractRay = ray(pos, refract, glm::dvec3(1, 1, 1), ray::REFRACTION);
				double tt = 0;
				glm::dvec3 tempC = traceRay(refractRay, thresh, depth - 1, tt);
				if(debugMode){
					std::cout << "dist: " << tt << std::endl;
					std::cout << "color: " << tempC << std::endl;
				}
				if(!internal)
					for(int k = 0; k < 3; k++)
						tempC[k] *= pow(m.kt(i)[k], tt);
				if(debugMode){
					std::cout << "color: " << tempC << std::endl;
				}
				colorC += tempC;
			}
			else if ( m.Refl()){
				glm::dvec3 reflect = -2 * glm::dot(l, n) * n + l;
				glm::dvec3 pos = r.at(i) + FIX * n;
				ray reflectRay = ray(pos, reflect, glm::dvec3(1, 1, 1), ray::REFLECTION);
				double tt = 0;
				glm::dvec3 tempC = traceRay(reflectRay, thresh, depth - 1, tt);
				if(!internal)
					for(int k = 0; k < 3; k++)
						tempC[k] *= pow(m.kt(i)[k], tt);
				colorC += m.kr(i) * tempC;
			}
		}
		if(debugMode)
		{
			std::cout << "-----------------------" << std::endl;
			std::cout << "depth" << depth << std::endl;
			std::cout << "ray type: " << r.type() << std::endl;
			std::cout << "r: " << r.getDirection() << std::endl;
			std::cout << "color: " << colorC << std::endl;
			std::cout << "-----------------------" << std::endl;
		}
		//}
	} else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.
		//
		// FIXME: Add CubeMap support here.
		// TIPS: CubeMap object can be fetched from traceUI->getCubeMap();
		//       Check traceUI->cubeMap() to see if cubeMap is loaded
		//       and enabled.
		if(traceUI -> cubeMap()){

			colorC = traceUI -> getCubeMap() -> getColor(r);
			if(debugMode){
				std::cout << "-----------------------" << std::endl;
				std::cout << "depth" << depth << std::endl;
				std::cout << "ray type: " << r.type() << std::endl;
				std::cout << "r: " << r.getDirection() << std::endl;
				std::cout << "color from cubemap: " << colorC << std::endl;
				std::cout << "-----------------------" << std::endl;
			}
		}
		else{
			colorC = glm::dvec3(0.0, 0.0, 0.0);
		}

	}
#if VERBOSE
	std::cerr << "== depth: " << depth+1 << " done, returning: " << colorC << std::endl;
#endif
	return colorC;
}

RayTracer::RayTracer()
	: scene(nullptr), buffer(0), thresh(0), buffer_width(0), buffer_height(0), m_bBufferReady(false)
{
}

RayTracer::~RayTracer()
{
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer.data();
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene(const char* fn)
{
	ifstream ifs(fn);
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}

	// Strip off filename, leaving only the path:
	string path( fn );
	if (path.find_last_of( "\\/" ) == string::npos)
		path = ".";
	else
		path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
	Parser parser( tokenizer, path );
	try {
		scene.reset(parser.parseScene());
	}
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	} catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	} catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}

	if (!sceneLoaded())
		return false;

	return true;
}

void RayTracer::traceSetup(int w, int h)
{
	if (buffer_width != w || buffer_height != h)
	{
		buffer_width = w;
		buffer_height = h;
		bufferSize = buffer_width * buffer_height * 3;
		buffer.resize(bufferSize);
	}
	std::fill(buffer.begin(), buffer.end(), 0);
	m_bBufferReady = true;

	//scene->getCamera().setFOV(135);
	/*
	 * Sync with TraceUI
	 */

	threads = traceUI->getThreads();
	block_size = traceUI->getBlockSize();
	thresh = traceUI->getThreshold();
	samples = traceUI->getSuperSamples();
	aaThresh = traceUI->getAaThreshold();

	// YOUR CODE HERE
	// FIXME: Additional initializations
	scene->buildBVH();
	threadStatus.resize(threads);
}

void RayTracer::threadAux(int tid, int w, int h, bool aa){

	for(int k = tid; k < w * h; k += threads){
		int i = k / h;
		int j = k % h;
		glm::dvec3 color = tracePixel(i, j, aa);
		setPixel(i, j, color);
	}
	threadStatus[tid] = true;
}

/*
 * RayTracer::traceImage
 *
 *	Trace the image and store the pixel data in RayTracer::buffer.
 *
 *	Arguments:
 *		w:	width of the image buffer
 *		h:	height of the image buffer
 *
 */
void RayTracer::traceImage(int w, int h)
{
	// Always call traceSetup before rendering anything. more thread
	traceSetup(w,h);

	// YOUR CODE HERE
	// FIXME: Start one or more threads for ray tracing
	//
	// TIPS: Ideally, the traceImage should be executed asynchronously,
	//       i.e. returns IMMEDIATELY after working threads are launched.
	//
	//       An asynchronous traceImage lets the GUI update your results
	//       while rendering.
	th.clear();
	for(int i = 0; i < threads; i++){
		threadStatus[i] = false;
		th.push_back(std::thread(&RayTracer::threadAux, this, i, w, h, false));
	}
}



int RayTracer::aaImage()
{

	// YOUR CODE HERE
	// FIXME: Implement Anti-aliasing here
	//
	// TIP: samples and aaThresh have been synchronized with TraceUI by
	//      RayTracer::traceSetup() function

	//th.clear();
	for(int i = 0; i < threads; i++){
		threadStatus[i] = false;
		th.push_back(std::thread(&RayTracer::threadAux, this, i, buffer_width, buffer_height, true));
	}
	return 0;
}

bool RayTracer::checkRender()
{
	// YOUR CODE HERE
	// FIXME: Return true if tracing is done.
	//        This is a helper routine for GUI.
	//
	// TIPS: Introduce an array to track the status of each worker thread.
	//       This array is maintained by the worker threads.
	for(auto const& status: threadStatus){
		if(! status)
			return false;
	}
	return true;
}

void RayTracer::waitRender()
{
	// YOUR CODE HERE
	// FIXME: Wait until the rendering process is done.
	//        This function is essential if you are using an asynchronous
	//        traceImage implementation.
	//
	// TIPS: Join all worker threads here.
	for(auto &t : th){
		t.join();
	}
}


glm::dvec3 RayTracer::getPixel(int i, int j)
{
	unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;
	return glm::dvec3((double)pixel[0]/255.0, (double)pixel[1]/255.0, (double)pixel[2]/255.0);
}

void RayTracer::setPixel(int i, int j, glm::dvec3 color)
{
	unsigned char *pixel = buffer.data() + ( i + j * buffer_width ) * 3;

	pixel[0] = (int)( 255.0 * color[0]);
	pixel[1] = (int)( 255.0 * color[1]);
	pixel[2] = (int)( 255.0 * color[2]);
}