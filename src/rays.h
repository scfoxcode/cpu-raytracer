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

struct ScreenProperties {
    int resX;
    int resY;
    int fov; // The horizontal fov
    float pixelDX; // X world units between pixels 
    float pixelDY; // Y world units between pixels 
    float virtualDist; // Virtual dist to screen for this fov and res
};

struct Sphere {
    glm::vec3 position;
    float radius;
};

// Important note that these scales are not what is used, as the screen would be
// way off in the distance. We just need the ratios to get the angles and the normalise
ScreenProperties buildScreenProperties(int resX, int resY, int fov) {
    ScreenProperties screen = {resX, resY, fov, 1, 0, 0};
    screen.pixelDX = resX / fov;
    screen.pixelDY = resY / fov;

    float halfHorizontalFov = (fov / 2) * M_PI / 180.0f; // Half fov and convert to rads

    screen.virtualDist = (resX / 2) / tan(halfHorizontalFov);

    return screen;
}

// pixelX and pixelY are floats so we can potentially do multiple rays per pixels for free AA
Ray buildRayForScreenPixel(ScreenProperties& screen, float pixelX, float pixelY) {
    Ray ray;
    ray.position = glm::vec3(0.0, 0.0, 0.0);

    // Calculate ray direction using screen properties
    // Because we are always starting at 0,0,0 for now, this is just the unit vector
    // of the virtual screen pixel
    int halfResX = screen.resX / 2;
    int halfResY = screen.resY / 2;
    
    ray.direction = glm::vec3(
        (-halfResX * screen.pixelDX) + pixelX * screen.pixelDX,
        (-halfResY * screen.pixelDX) + pixelY * screen.pixelDX, // was using pixelDY here but we got ovals
        screen.virtualDist);

    ray.direction = glm::normalize(ray.direction);

    return ray;
}

// Returns a negative number if no intersection
// Otherwise returns the distance to intersection point
float raySphereIntersection(Camera camera, Ray ray, Sphere sphere) {
    glm::vec3 sphereV = sphere.position - camera.position;    

    float numLengthsToClosest = glm::dot(sphereV, ray.direction);
    glm::vec3 closestPos = ray.direction * numLengthsToClosest;

    float distToSphere = glm::length(closestPos - sphere.position);
    if (distToSphere > sphere.radius) {
        return -1;
    }
    // Dist to sphere is not what we want. Dist to camera at intersection point is correct
    return numLengthsToClosest;
    // We still need to calculate the intercept points if we do intersect
}


