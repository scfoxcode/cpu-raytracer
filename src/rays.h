#ifndef __rays__
#define __rays__

#include <glm/glm.hpp>
#include <cmath>

struct Camera {
    glm::vec3 position;
    glm::vec3 direction;
    float fov;
};

struct Ray {
    glm::vec3 position;
    glm::vec3 direction;
};

struct MaybeIntersect {
    bool intersect;
    glm::vec3 position;
};

struct ScreenProperties {
    int resX;
    int resY;
    int fov; // The horizontal fov
    float pixelDX; // X world units between pixels 
    float pixelDY; // Y world units between pixels 
    float virtualDist; // Virtual dist to screen for this fov and res
};

// All these structs and initialisers should be in their own files
struct Sphere {
    glm::vec3 position;
    float radius;
};

Sphere initSphere(glm::vec3 position, float radius);

// Important note that these scales are not what is used, as the screen would be
// way off in the distance. We just need the ratios to get the angles and the normalise
ScreenProperties buildScreenProperties(int resX, int resY, int fov); 

// pixelX and pixelY are floats so we can potentially do multiple rays per pixels for free AA
Ray buildRayForScreenPixel(ScreenProperties& screen, float pixelX, float pixelY);

Ray buildRayForPoints(glm::vec3 target, glm::vec3 source);

// Returns a negative number if no intersection
// Otherwise returns the distance to intersection point
MaybeIntersect raySphereIntersection(Ray ray, Sphere sphere); 

#endif
