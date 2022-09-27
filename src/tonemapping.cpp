#include "tonemapping.h"

float reinhardExtendedFloat(float lume) {
    float white = 16.0f; // We should get this from the brightest value on screen before tone mapping
    float extended = 1 + (lume / white);

    return lume * extended / (1.0f + lume);
}

glm::vec3 reinhardExtended(glm::vec3 pixel) {
    glm::vec3 out;
    out.r = reinhardExtendedFloat(pixel.r);
    out.g = reinhardExtendedFloat(pixel.g);
    out.b = reinhardExtendedFloat(pixel.b);
    return out;
}