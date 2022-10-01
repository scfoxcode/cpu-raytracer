#ifndef __poly__
#define __poly__
#include <glm/glm.hpp>
#include "icollidable.h"

class Poly: public ICollidable {
private:
    glm::vec3 vertices[4];

public:
    Poly();
    Poly(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
    ~Poly();

    void init(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
    
    MaybeIntersect rayIntersection(Ray& ray); 

    glm::vec3 getPosition();

    glm::vec3 getSurfaceNormal(glm::vec3& surfacePoint);
};

#endif
