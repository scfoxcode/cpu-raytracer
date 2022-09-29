#ifndef __structdefs__
#define __structdefs__

struct Camera {
    glm::vec3 position;
    glm::vec3 direction;
    float fov;
};

struct MaybeIntersect {
    bool intersect;
    float distance;
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

#endif
