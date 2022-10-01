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
    int xRes,
    int yRes, 
    float fov,
    Light& light,
    std::vector<ICollidable*>& objects,
    float* data // this must be initailised to the right sense when provided
    ) {
    printf("light %f\n", light.brightness);
    // Ray trace the whole scene but at a low resolution, for a single light
    ScreenProperties screenP = buildScreenProperties(xRes, yRes, fov);

    int count = 0;
    for (int y=0; y<yRes; y++) {
        for (int x=0; x<xRes; x++) {
            // Test ray for pixel against every object in the scene
            Ray r = buildRayForScreenPixel(screenP, x, y); 
            MaybeIntersectObject maybe = rayClosestIntersect(r, objects);

            if (!maybe.raypath.intersect) {
                data[count] = 0.0f;
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
                data[count] = 0;
                count++;
                continue; 
            }

            MaybeIntersectObject test2 = rayClosestIntersect(r2, objects);
            bool lightIsVisible = !test2.raypath.intersect;
            data[count] = lightIsVisible ? dot : 0.0f;
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
    float* shadowMap
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
        // Magic numbers, hardcoded shadow map ratio 4 here
        float sx = float(x) / 4.0f;
        float sy = float(y) / 4.0f;
        float cx = int(sx);
        float cy = int(sy);
        float left = 1.0f - (sx - cx);
        float right = (sx - cx);
        float up = 1.0f - (sy - cy);
        float down = (sy - cy);

        // Remove all these hardcoded 480
        float sample = shadowMap[int(cy * 480 + cx)];
        float sampleLeft = shadowMap[int(cy * 480 + (cx > 0 ? cx - 1 : 0))] * right;
        float sampleRight = shadowMap[int(cy * 480 + (cx < 479 ? cx + 1 : 479))] * left;
        float sampleUp = shadowMap[int((cy > 0 ? cy - 1 : cy) * 480 + cx)] * up;
        float sampleDown = shadowMap[int((cy < 269  ? cy + 1 : cy) * 480 + cx)] * down;
        float occlusionValue = (sample + sampleLeft + sampleRight + sampleUp + sampleDown) * 0.2;
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
    float* shadowMap = new float[480 * 270];
    buildShadowMap(480, 270, camera.fov, *lights[0], sceneObjects, shadowMap);
    

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
