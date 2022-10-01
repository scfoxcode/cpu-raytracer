#include "SDL.h" 
#include <glm/glm.hpp>
#include <chrono>
#include "light.h"
#include "tonemapping.h"
#include "sphere.h"
#include "poly.h"
#include <thread>
#include <math.h>

// One great reference
// https://www.youtube.com/watch?v=HFPlKQGChpE

const int RESX = 1920; //960; //480
const int RESY = 1080; //540; //270
const int pixelCount = RESX * RESY;

struct ShadowMap {
    int resX;
    int resY;
    float fov;
    float ratio;
    float* data;
};

float getData(float x, float y, ShadowMap& map) {
    return map.data[int(round(y) * map.resX + round(x))];
}

float shadowMapInterp(
    int x,
    int y,
    ShadowMap& shadowMap) {

    return 0.0f;
}
//
// This type and maybe intersect are both bad, need better solutions
struct MaybeIntersectObject {
    MaybeIntersect raypath;
    ICollidable* target;
    MaybeIntersectObject(){};
};

// Test a ray against multiple objects and only return the closest
MaybeIntersectObject rayClosestIntersect(
    Ray& ray,
    std::vector<ICollidable*>& objects) {

    float collisionDistance = 100000000;
    ICollidable* closest = NULL;
    MaybeIntersect result;
    for (int i=0; i<objects.size(); i++) {
        MaybeIntersect test = objects[i]->rayIntersection(ray); 
        if (test.intersect && test.distance < collisionDistance) {
            closest = objects[i];
            collisionDistance = test.distance;
            result = test;
        }
    }

    if (!closest) {
        MaybeIntersectObject noIntersect;
        noIntersect.raypath.intersect = false;
        return noIntersect;
    }

    MaybeIntersectObject output;
    output.raypath = result;
    output.target = closest;
    return output;
}

// Build a shadow map for a single scene light
void buildShadowMap(
    Light& light,
    std::vector<ICollidable*>& objects,
    ShadowMap& shadowMap
    ) {
    printf("light %f\n", light.brightness);
    // Ray trace the whole scene but at a low resolution, for a single light
    float xRes = shadowMap.resX;
    float yRes = shadowMap.resY;
    float fov = shadowMap.fov;
    ScreenProperties screenP = buildScreenProperties(xRes, yRes, fov);

    int count = 0;
    for (int y=0; y<yRes; y++) {
        for (int x=0; x<xRes; x++) {
            // Test ray for pixel against every object in the scene
            Ray r = buildRayForScreenPixel(screenP, x, y); 
            MaybeIntersectObject maybe = rayClosestIntersect(r, objects);

            if (!maybe.raypath.intersect) {
                shadowMap.data[count] = 0.0f;
                count++;
                continue;
            }
            ICollidable& hit = *maybe.target;

            // The ray hit an object.
            // Bounce a ray to the light
            // If there is a collision, that light does not contribute to the pixel
            std::vector<glm::vec3> colours;
            glm::vec3 colourForLight(0.0f, 0.0f, 0.0f);
            Ray r2 = buildRayForPoints(light.emitter.position, maybe.raypath.position);
            r2.position += r2.direction * 0.001f; // Tiny hack to move ray slightly away from intial collision

            float dot = glm::dot(r2.direction, hit.getSurfaceNormal(r2.position));
            if (dot < 0) {
                shadowMap.data[count] = 0;
                count++;
                continue; 
            }

            MaybeIntersectObject test2 = rayClosestIntersect(r2, objects);
            bool lightIsVisible = !test2.raypath.intersect;
            shadowMap.data[count] = lightIsVisible ? dot : 0.0f;
            count++;
            // Maybe we'd want to store distance too going forward for inverse square law
        }
    }
}

Camera initCamera() {
    return {
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, -1.0), // Looking into the screen
        75.0f
    };
};


glm::vec3 buildPixel(
    int x,
    int y,
    ScreenProperties& screenP,
    Lights& lights,
    std::vector<ICollidable*>& objects,
    ShadowMap& shadowMap
    ) {

    glm::vec3 colour(0.0f, 0.0f, 0.0f);

    // Test ray for pixel against every object in the scene
    Ray r = buildRayForScreenPixel(screenP, x, y); 
    MaybeIntersectObject maybe = rayClosestIntersect(r, objects);

    if (!maybe.raypath.intersect) {
        return colour; // Ray didn't hit anything
    }
    ICollidable& hit = *maybe.target;

    // The ray hit an object.
    // Check shadow map to determine colour
    std::vector<glm::vec3> colours;
    for (int i=0; i<lights.size(); i++) {
        // Interpolate from shadow map
        float sx = float(x) / shadowMap.ratio;
        float sy = float(y) / shadowMap.ratio;
        float cx = int(sx);
        float cy = int(sy);
        float left = 1.0f - (sx - cx);
        float right = (sx - cx);
        float up = 1.0f - (sy - cy);
        float down = (sy - cy);
        float width = shadowMap.resX;
        float height = shadowMap.resY;
        
        // I want to do a weighted box blur, using the floats I have
        float samples[3][3];
        for (int i=0; i<3; i++) {
            for (int j=0; j<3; j++) {
                samples[i][j] = -1;
            }
        }

        if (cx > 0 && cy > 0)
            samples[0][0] = getData(cx - 1, cy - 1, shadowMap); 
        if (cy > 0)
            samples[1][0] = getData(cx, cy - 1, shadowMap); 
        if (cx < width - 1 && cy > 0)
            samples[2][0] = getData(cx + 1, cy - 1, shadowMap); 
        
        samples[1][1] = getData(cx, cy, shadowMap); 
        if (cx > 0)
            samples[0][1] = getData(cx - 1, cy, shadowMap); 
        if (cx < width - 1)
            samples[2][1] = getData(cx + 1, cy, shadowMap); 

        if (cx > 0 && cy < height - 1)
            samples[0][2] = getData(cx - 1, cy + 1, shadowMap); 
        if (cy < height - 1)
            samples[1][2] = getData(cx, cy + 1, shadowMap); 
        if (cx < width - 1 && cy < height - 1)
            samples[2][2] = getData(cx + 1, cy + 1, shadowMap); 

        float count = 0.0f;
        float occlusionValue = 0.0f;
        for (int i=0; i<3; i++) {
            for (int j=0; j<3; j++) {
                if (samples[i][j] > -0.1) {
                    count++;
                    occlusionValue += samples[i][j];
                }
            }
        }
        occlusionValue = occlusionValue / count;
        colour = (lights[i]->colour * lights[i]->brightness * occlusionValue);
    }
    

    return colour;

}

uint32_t rgbToInt32(glm::vec3& colour) {
    // Tone map hdr into 0-1 range
    colour = reinhardExtended(colour);

    uint32_t red = colour.r * 255;
    uint32_t green = colour.g * 255;
    uint32_t blue = colour.b * 255;

    uint32_t output = (blue | ( green << 8) | (red << 16) | (255 << 24));
    return output;
}

void setupSceneLights(Lights& lights) {
    lights.push_back(new Light(
        Sphere(glm::vec3(-5.0, 20.0, -35.0), 2.0f),
        glm::vec3(0.9f, 0.9f, 0.7f),
        2.5f
    ));
    /*
    lights.push_back(new Light(
        Sphere(glm::vec3(-18.0, 20.0, -10.0), 1.0f),
        glm::vec3(0.8f, 0.0f, 0.0f),
        0.5f
    ));
    */
}

void setupSpheres(std::vector<ICollidable*>& sceneObjects) {
    sceneObjects.push_back(new Sphere(glm::vec3(0.0, 0.0, -30.0), 3.0f));
    sceneObjects.push_back(new Sphere(glm::vec3(8.0, -8.0, -27.0), 2.0f));
} 

void setupBox(std::vector<ICollidable*>& sceneObjects) {
    float wh = 20; // Half width
    float hh = 20; // Half height
    float y = 10; // Y offset
    float d = 50;

    // Ground plane
    sceneObjects.push_back(new Poly(
        glm::vec3(-wh, -y, 0.0),
        glm::vec3(-wh, -y, -d),
        glm::vec3(wh, -y, -d),
        glm::vec3(wh, -y, 0.0)
    ));

    // Left plane
    sceneObjects.push_back(new Poly(
        glm::vec3(-wh, -y, 0.0),
        glm::vec3(-wh, -y + hh, 0.0),
        glm::vec3(-wh, -y + hh, -d),
        glm::vec3(-wh, -y, -d)
    ));

    // Right plane
    sceneObjects.push_back(new Poly(
        glm::vec3(wh, -y, -d),
        glm::vec3(wh, -y + hh, -d),
        glm::vec3(wh, -y + hh, 0.0),
        glm::vec3(wh, -y, 0.0)
    ));
    //
    // Back plane
    sceneObjects.push_back(new Poly(
        glm::vec3(-wh, -y, -d),
        glm::vec3(-wh, -y + hh, -d),
        glm::vec3(wh, -y + hh, -d),
        glm::vec3(wh, -y, -d)
    ));

}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    bool running = true;

    // Count cores, maybe useful one day
    const int cpuCount = std::thread::hardware_concurrency();
    printf("%i Cores available, but only single threaded is currently implemented\n", cpuCount);

    // Create a camera and a sphere in the scene
    Camera camera = initCamera();

    // Create scene geometry
    std::vector<ICollidable*> sceneObjects; 
    setupBox(sceneObjects);
    setupSpheres(sceneObjects);

    // Create the scenes light sources
    Lights lights;
    setupSceneLights(lights);

    SDL_Window *window = SDL_CreateWindow(
        "cpu-raytracer",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        RESX,
        RESY,
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);

    // Build the shadow map
    ShadowMap shadowMap;
    shadowMap.resX = 240; // 480;
    shadowMap.resY = 135; // 270
    shadowMap.fov = camera.fov;
    shadowMap.ratio = RESX / shadowMap.resX;
    shadowMap.data = new float[shadowMap.resX * shadowMap.resY];
    buildShadowMap(*lights[0], sceneObjects, shadowMap);
    

    // Don't include this in time as we only need to allocate the memory once
    uint32_t* pixels = new uint32_t[pixelCount];

    // Build texture for my ray tracing data
    SDL_Texture* sceneTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STATIC,
        RESX,
        RESY
    );

    // Perform ray tracing
    auto startTime = std::chrono::high_resolution_clock::now();
    ScreenProperties screenP = buildScreenProperties(RESX, RESY, camera.fov);

    int count = 0;
    for (int y=0; y<RESY; y++) {
        for (int x=0; x<RESX; x++) {
            glm::vec3 sum = buildPixel(x, y, screenP, lights, sceneObjects, shadowMap);
            pixels[count] = rgbToInt32(sum);
            count++;
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    printf("Time taken to ray trace: %fms\n", elapsedMs);

    // Copy ray traced scene into texture
    SDL_UpdateTexture(sceneTexture, NULL, pixels, RESX * sizeof(uint32_t));

    if (!sceneTexture) {
        printf("Failed to build scene texture\n");
    }
    
    SDL_Event event;
    while (running) {
        SDL_RenderClear(renderer);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        SDL_RenderCopy(renderer, sceneTexture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    delete [] pixels;

    for (int i=0; i<sceneObjects.size(); i++) {
        delete sceneObjects[i];
    }

    SDL_DestroyTexture(sceneTexture);
    SDL_DestroyWindow(window);
    SDL_Quit();


    return 0;
}
