#include "SDL.h" 
#include <glm/glm.hpp>
#include <chrono>
#include "light.h"
#include "tonemapping.h"
#include "sphere.h"
#include <thread>

// One great reference
// https://www.youtube.com/watch?v=HFPlKQGChpE


const int RESX = 1920;
const int RESY = 1080;
const int bpp = 4; // RGBA
const int pixelCount = RESX * RESY;
const int pixelIntCount = pixelCount * bpp;

Camera initCamera() {
    return {
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, -1.0), // Looking into the screen
        75.0f
    };
};

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

uint32_t buildPixel(
    int x,
    int y,
    ScreenProperties& screenP,
    Lights& lights,
    std::vector<ICollidable*>& objects) {

    // Test ray for pixel against every object in the scene
    Ray r = buildRayForScreenPixel(screenP, x, y); 
    MaybeIntersectObject maybe = rayClosestIntersect(r, objects);

    if (!maybe.raypath.intersect) {
        return 0; // Ray didn't hit anything
    }
    ICollidable& hit = *maybe.target;

    // The ray hit an object.
    // Bounce a ray to each light in the scene, checking for collisions with any object
    // If there is a collision, that light does not contribute to the pixel
    std::vector<glm::vec3> colours;
    for (int i=0; i<lights.size(); i++) {
        Ray r2 = buildRayForPoints(lights[i]->emitter.position, maybe.raypath.position);
        r2.position += r2.direction * 0.01f; // Tiny hack to move ray slightly away from intial collision

        // Dangerous assumption here that target is sphere by getting center, but best we can do for now
        float dot = glm::dot(r2.direction, glm::normalize(r2.position - hit.getPosition()));
        if (dot < 0) {
            continue; // Surface is facing away from the light
        }

        MaybeIntersectObject test2 = rayClosestIntersect(r2, objects);
        bool isLightInView = !test2.raypath.intersect;

        if (!isLightInView) {
            continue;
        }

        // Light is in view, add colour
        colours.push_back(lights[i]->colour * lights[i]->brightness * dot);
    }
    
    // Add all contributing hdr colours together
    glm::vec3 colour;
    for (int i=0; i<colours.size(); i++) {
        colour += colours[i];
    }

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
        Sphere(glm::vec3(200.0, 0.0, -75.0), 0.1f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        2.4f
    ));
    lights.push_back(new Light(
        Sphere(glm::vec3(-40.0, 60.0, 8.0f), 0.1f),
        glm::vec3(0.1f, 0.0f, 0.9f),
        2.0f
    ));
    lights.push_back(new Light(
        Sphere(glm::vec3(-2.0, -10.0, -17.0f), 0.1f),
        glm::vec3(0.2f, 0.8f, 0.1f),
        1.5f
    ));
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    bool running = true;

    // Count cores, maybe useful one day
    const int cpuCount = std::thread::hardware_concurrency();
    printf("%i Cores available, but only single treaded is currently implemented\n", cpuCount);

    // Create a camera and a sphere in the scene
    Camera camera = initCamera();

    std::vector<ICollidable*> sceneObjects; 
    Sphere sphere1(glm::vec3(0.0, 0.0, -20.5), 5.5f);
    Sphere sphere2(glm::vec3(8.0, 0.0, -17.0), 2.0f);
    Sphere sphere3(glm::vec3(0.0, -6.0, -16.0), 1.0f);
    Sphere sphere4(glm::vec3(-15.0, -5.0, -28.0), 3.5f);
    sceneObjects.push_back(&sphere1);
    sceneObjects.push_back(&sphere2);
    sceneObjects.push_back(&sphere3);
    sceneObjects.push_back(&sphere4);

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
            // pixels[count] = buildPixelFromRays(x, y, screenP, sphere1, lights);
            pixels[count] = buildPixel(x, y, screenP, lights, sceneObjects);
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

    SDL_DestroyTexture(sceneTexture);
    SDL_DestroyWindow(window);
    SDL_Quit();


    return 0;
}
