#include "SDL.h" 
#include <glm/glm.hpp>
#include <chrono>
#include "rays.h"

// One great reference
// https://www.youtube.com/watch?v=HFPlKQGChpE

const int RESX = 1280;
const int RESY = 720;
const int bpp = 4; // RGBA
const int pixelCount = RESX * RESY;
const int pixelIntCount = pixelCount * bpp;

Camera initCamera() {
    return {
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.0, 0.0, 1.0), // Looking into the screen
        40.0f
    };
};

Sphere initSphere(glm::vec3 position, float radius) {
    return {
        position,
        radius
    };
};

float unitFloat(float a, float b, float v) {
    return (v - a) / (b - a);
}

uint32_t buildPixelFromRays(int x, int y, bool useMultiRays, ScreenProperties &screenP, Sphere &sphere) {
    float avgDist = 0;

    if (useMultiRays) {
        float dists[5];

        Ray r0 = buildRayForScreenPixel(screenP, x, y); 
        dists[0] = raySphereIntersection(r0, sphere); 
        Ray r1 = buildRayForScreenPixel(screenP, x-1.5, y-1.5); 
        dists[1] = raySphereIntersection(r1, sphere); 
        Ray r2 = buildRayForScreenPixel(screenP, x+1.5, y-1.5); 
        dists[2] = raySphereIntersection(r2, sphere); 
        Ray r3 = buildRayForScreenPixel(screenP, x-1.5, y+1.5); 
        dists[3] = raySphereIntersection(r3, sphere); 
        Ray r4 = buildRayForScreenPixel(screenP, x+1.5, y+1.5); 
        dists[4] = raySphereIntersection(r4, sphere); 

        // We have a bug where we return negative or 0 for no intersect
        // but this results in massive brightness, we want to increase the average dist to lower our brightness....
        float sum = 0;
        int numIncluded = 0;
        for (int i=0; i<5; i++) {
            if (dists[i] > 0) {
                sum += dists[i];
                numIncluded++;
            }
        }
        // We want a larger dist for lower brightness if some have not intersected
        if (numIncluded == 0) {
            avgDist = 1000000; // Inf
        } else {
            avgDist = sum * (5 / numIncluded) / numIncluded;
        }

    } else {
        Ray r = buildRayForScreenPixel(screenP, x, y); 
        avgDist = raySphereIntersection(r, sphere); 
    }

    if (avgDist <= 0) {
        return 0;
    }

    // We need to map the values in a reasonable range. Lets say 10 meters is our max vision and linearly interp
    float brightness = 1.0f / (avgDist + 0.6); // +0.5 is a hack, need a better way to clamp
    // Massive hack, we want to actually check normals and lightsources going forwards
    // float brightness = unitFloat(sphere.position.z, sphere.position.z - sphere.radius, avgDist);
    uint32_t val256 = brightness * 256;
    if (val256 > 255) {
        val256 = 255;
    }
    return (val256 | (val256 << 8) | (val256 << 16) | (255 << 24));
    
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    bool running = true;

    // Create a camera and a sphere in the scene
    Camera camera = initCamera();
    Sphere sphere1 = initSphere({0.0, 0.0, 6.0}, 5.8f);


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

    // okay. Lets try and build a ray traced sphere
    auto startTime = std::chrono::high_resolution_clock::now();
    ScreenProperties screenP = buildScreenProperties(RESX, RESY, camera.fov);

    int count = 0;
    for (int y=0; y<RESY; y++) {
        for (int x=0; x<RESX; x++) {
            pixels[count] = buildPixelFromRays(x, y, true, screenP, sphere1);
            count++;
        }
    }

    // Copy ray traced scene into texture
    SDL_UpdateTexture(sceneTexture, NULL, pixels, RESX * sizeof(uint32_t));
    auto endTime = std::chrono::high_resolution_clock::now();

    double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    printf("Time taken to ray trace and copy texture buffer: %fms\n", elapsedMs);

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
        SDL_Delay(10);
    }

    delete [] pixels;

    SDL_DestroyTexture(sceneTexture);
    SDL_DestroyWindow(window);
    SDL_Quit();


    return 0;
}
