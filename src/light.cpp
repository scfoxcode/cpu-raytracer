#include "light.h"

Light::Light() {
    emitter = initSphere(glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
    colour = glm::vec3(1.0f, 1.0f, 1.0f);
    brightness = 1.0f;

}

Light::Light(Sphere emitter, glm::vec3 colour, float brightness) {
    this->emitter = emitter;
    this->colour = colour;
    this->brightness = brightness;
}
