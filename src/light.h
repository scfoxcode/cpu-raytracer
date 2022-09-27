#ifndef __light__
#define __light__
#include <vector>
#include <glm/glm.hpp>
#include "sphere.h"

// For now assume spherical light
// We really do want rectangular lights, and beam lighting (spots) though going forward

class Light {
public:
    Sphere emitter;
    glm::vec3 colour;
    float brightness;

    Light();
    Light(Sphere emitter, glm::vec3 colour, float brightness);

};

typedef std::vector<Light*> Lights;

#endif
