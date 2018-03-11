#pragma once

// Note: you can put kd-tree here
#include "bbox.h"
#include "scene.h"
#include "ray.h"
//#include "../SceneObjects/trimesh.h"
#include <iostream>

#define STACK_SIZE 512
#define BIG_DOUBLE 99999999.0


struct stackNode{
  int prt;
  int start, end;
};

struct node{
  BoundingBox box;
  int start, num, offset;
};

struct travNode{
  int i;
  double mint;
  travNode() {}
  travNode(int _i, double _mint) : i(_i), mint(_mint) {}
};

class Geometry;
class Scene;
class MaterialSceneObject;
class Trimesh;
class TrimeshFace;

class BVH{
public:
  BVH(const Scene* scene){
    this->scene = scene;
  }
  ~BVH(){
    delete[] tree;
  }
  void build();

  bool getIntersection(ray& r, isect& i) const;

private:
  int nodeNum, leafs;
  const Scene* scene;
  std::vector<Geometry*> objects;
  std::vector<BoundingBox*> boxes;
  node* tree;
};
