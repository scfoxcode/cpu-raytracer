#ifndef __collidable__
#define __collidable__
#include "rays.h"

class ICollidable {
public:
    ICollidable(){};
    virtual ~ICollidable(){}
    virtual MaybeIntersect rayIntersection(Ray& ray) = 0;
};

#endif
