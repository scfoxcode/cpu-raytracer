#include "sphere.h"

Sphere::Sphere() {
    this->position = glm::vec3(0.0f, 0.0f, 0.0f);
    this->radius = 1.0f;
}

Sphere::Sphere(glm::vec3 position, float radius) {
    init(position, radius);
}

Sphere::~Sphere() {
}

void Sphere::init(glm::vec3 position, float radius) {
    this->position = position;
    this->radius = radius;
};

// Should build a real maybe type, not this hackery, or pass a reference in and return bool
MaybeIntersect Sphere::rayIntersection(Ray& ray) {

    float numLengthsToClosest = glm::dot(position - ray.position, ray.direction);
    glm::vec3 closestPos = ray.position + ray.direction * numLengthsToClosest;
    // ^^ adding ray.position here just broke everything, but I think it's needed

    float distToSphere = glm::length(position - closestPos);
    //
    // What will this do if a ray starts inside the sphere...
    if (distToSphere > radius) {
        return { .intersect = false };
    }

    // Otherwise calculate points of intersection
    // Get the distance between the closest point and points of intersection
    float dist = sqrt(radius * radius - distToSphere * distToSphere);
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
