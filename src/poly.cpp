#include "poly.h"
#include <stdio.h>
//
// This should be moved elsewhere, just a helper
// Moller Trumbore
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
// TODO: Spend some time understanding this better
MaybeIntersect triIntersection(Ray& ray, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    MaybeIntersect result = { .intersect = false };
    const float epsilon = 0.000001;
    glm::vec3 edge1 = b - a;
    glm::vec3 edge2 = c - a;
    
    glm::vec3 h = glm::cross(ray.direction, edge2);
    float dot = glm::dot(edge1, h);
    if (dot > -epsilon && dot < epsilon) {
        return result; // Ray is parallel to triangle
    }

    float f = 1.0/dot;
    glm::vec3 s = ray.position - a;
    float u = f * glm::dot(s, h);
    if (u < 0.0 || u > 1.0) {
        return result;
    }

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray.direction, q);
    if (v < 0.0 || u + v > 1.0) {
        return result;
    }

    // Compute 't' to find out where intersection point is on the line
    float t = f * glm::dot(edge2, q);
    if (t > epsilon) {
        glm::vec3 intersectionP = ray.position + ray.direction * t;
        result.intersect = true;
        result.position = intersectionP;
        result.distance = t;

        return result; // Return intersection
    } else {
        return result; // Line intersection but not ray
    }
}

Poly::Poly() {
}

Poly::Poly(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
    init(a, b, c, d);
}

Poly::~Poly() {
}

void Poly::init(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
    vertices[0] = a;
    vertices[1] = b;
    vertices[2] = c;
    vertices[3] = d;
}

MaybeIntersect Poly::rayIntersection(Ray& ray) {
    MaybeIntersect tri1 = triIntersection(ray, vertices[0], vertices[1], vertices[2]);
    if (tri1.intersect) {
        return tri1;
    }
    return triIntersection(ray, vertices[0], vertices[2], vertices[3]);
}

// Well this won't work, using it for normal in sphere, should be getNormal
glm::vec3 Poly::getPosition() {
    return glm::vec3(0.0, 0.0, 0.0);
}

glm::vec3 Poly::getSurfaceNormal(glm::vec3& surfacePoint) {
    // We don't need the surface point for the plane
    glm::vec3 edge1 = vertices[1] - vertices[0];
    glm::vec3 edge2 = vertices[3] - vertices[0];
    glm::vec3 normal = glm::cross(glm::normalize(edge2), glm::normalize(edge1));
    return normal;
}

