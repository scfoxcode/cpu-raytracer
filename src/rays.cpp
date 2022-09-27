#include "rays.h"
#include <stdio.h>

Sphere initSphere(glm::vec3 position, float radius) {
    return {
        position,
        radius
    };
};

// Important note that these scales are not what is used, as the screen would be
// way off in the distance. We just need the ratios to get the angles and the normalise
// Note: This thing is very sus
ScreenProperties buildScreenProperties(int resX, int resY, int fov) {
    ScreenProperties screen = {resX, resY, fov, 1, 0, 0};
    printf("test %i %i %i\n", resX, resY);
    // These dx values are wrong. When we hack it in the next function it works
    screen.pixelDX = resX / fov; // this feels wrong?
    screen.pixelDY = resY / fov; // but is it
    printf("dx %f %f\n", screen.pixelDX, screen.pixelDY);

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

// Returns a negative number if no intersection
// Otherwise returns the distance to intersection point
// Should build a real maybe type, not this hackery, or pass a reference in and return bool
MaybeIntersect raySphereIntersection(Ray ray, Sphere sphere) {

    float numLengthsToClosest = glm::dot(sphere.position - ray.position, ray.direction);
    glm::vec3 closestPos = ray.position + ray.direction * numLengthsToClosest;
    // ^^ adding ray.position here just broke everything, but I think it's needed

    float distToSphere = glm::length(sphere.position - closestPos);
    //
    // What will this do if a ray starts inside the sphere...
    if (distToSphere > sphere.radius) {
        return { .intersect = false };
    }

    // Otherwise calculate points of intersection
    // Get the distance between the closest point and points of intersection
    float dist = sqrt(sphere.radius * sphere.radius - distToSphere * distToSphere);
    glm::vec3 intersect1 = closestPos + dist * ray.direction;
    glm::vec3 intersect2 = closestPos - dist * ray.direction;

    bool i1 = true;
    bool i2 = true;
    if (glm::dot(intersect1 - ray.position, ray.direction) < 0) {
        i1 = false; // Intersect point is behind ray origin
    }
    if (glm::dot(intersect2 - ray.position, ray.direction) < 0) {
        i2 = false; // Intersect point is behind ray origin
    }
    if (i1 && i2) {
        // Determine which point the ray hit first, smallest distance
        float intersect1Dist = glm::length(intersect1 - ray.position);
        float intersect2Dist = glm::length(intersect2 - ray.position);
        glm::vec3& intersectP = intersect1Dist < intersect2Dist ? intersect1 : intersect2;
        return { .intersect = true, .position = intersectP };
    } else if (!i1 && !i2) {
        return { .intersect = false };
    } else {
        return { .intersect = true, .position = i1 ? intersect1 : intersect2 };
    }

}
