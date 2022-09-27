#ifndef __rays__
#define __rays__

#include <glm/glm.hpp>
#include <cmath>
#include "structdefs.h"

struct Ray {
    glm::vec3 position;
    glm::vec3 direction;
};

// Important note that these scales are not what is used, as the screen would be
// way off in the distance. We just need the ratios to get the angles and the normalise
ScreenProperties buildScreenProperties(int resX, int resY, int fov); 

// pixelX and pixelY are floats so we can potentially do multiple rays per pixels for free AA
Ray buildRayForScreenPixel(ScreenProperties& screen, float pixelX, float pixelY);

Ray buildRayForPoints(glm::vec3 target, glm::vec3 source);


#endif
