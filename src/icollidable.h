#ifndef __collidable__
#define __collidable__
#include "rays.h"
#include <glm/glm.hpp>

class ICollidable {
public:
    ICollidable(){};
    virtual ~ICollidable(){}
    virtual MaybeIntersect rayIntersection(Ray& ray) = 0;
    virtual glm::vec3 getPosition() = 0;
    virtual glm::vec3 getSurfaceNormal(glm::vec3& surfacePoint) = 0;
};

#endif
