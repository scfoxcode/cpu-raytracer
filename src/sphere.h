#ifndef __sphere__
#define __sphere__
#include <glm/glm.hpp>
#include "icollidable.h"

class Sphere: public ICollidable {
public:
    glm::vec3 position;
    float radius;

    Sphere();
    Sphere(glm::vec3 position, float radius);
    ~Sphere();
    void init(glm::vec3 position, float radius);
    
    MaybeIntersect rayIntersection(Ray& ray); 

    glm::vec3 getPosition();

    glm::vec3 getSurfaceNormal(glm::vec3& surfacePoint);
};

#endif
