#include "rays.h"
#include <stdio.h>

ScreenProperties buildScreenProperties(int resX, int resY, int fov) {
    ScreenProperties screen = {resX, resY, fov, 1, 0, 0};

    float halfHorizontalFov = (fov / 2) * M_PI / 180.0f;

    // This is the distance that at the given field of view makes each ray 1 "pixel" apart
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
        -halfResX + pixelX,
        // Must flip y here as screen y and world y are backwards
        halfResY - pixelY,
        -screen.virtualDist);

    ray.direction = glm::normalize(ray.direction);

    return ray;
}

Ray buildRayForPoints(glm::vec3 target, glm::vec3 source) {
    Ray ray;
    ray.position = source;
    ray.direction = glm::normalize(target - source);
    return ray;
}

