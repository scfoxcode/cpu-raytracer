#include "SDL.h" 
#include <glm/glm.hpp>
#include <chrono>
#include "light.h"

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

float unitFloat(float a, float b, float v) {
    return (v - a) / (b - a);
}

uint32_t buildPixelFromRays(
    int x,
    int y,
    bool useMultiRays,
    ScreenProperties &screenP,
    Sphere &sphere,
    Lights& lights) {

    Ray r = buildRayForScreenPixel(screenP, x, y); 
    MaybeIntersect test = raySphereIntersection(r, sphere); 
    // This bug could happen if the test position returned was always the one
    // closest to the target... which it will be in the next case???
    std::vector<glm::vec3> colours;
    if (test.intersect) {
        for (int i=0; i<lights.size(); i++) {
            Ray r2 = buildRayForPoints(lights[i]->emitter.position, test.position);
            // We know it intersect with light because we've build our ray to point straight at it
            // What we need to know is does the sphere or other geometry block the ray
            // r2.position += (r2.position - sphere.position) * 0.001f; 
            
            r2.position += r2.direction * 0.01f; // Tiny hack to move ray slightly away from intial collision
            MaybeIntersect test2 = raySphereIntersection(r2, sphere);
            if (x == 1280/2 + 100 && y == 720/2 + 100) {
                printf("What we got here\n");
                printf(test2.intersect ? "true\n" : "false\n");
                printf("xyz %f %f %f\n", test2.position[0], test2.position[1], test2.position[2]);
                printf("xyz pos %f %f %f\n", r2.position[0], r2.position[1], r2.position[2]);
                printf("xyz dir %f %f %f\n", r2.direction[0], r2.direction[1], r2.direction[2]);
                printf("xyz sphere%f %f %f\n", sphere.position[0], sphere.position[1], sphere.position[2]);
                glm::vec3 ep = lights[i]->emitter.position;
                printf("xyz em%f %f %f\n", ep[0], ep[1], ep[2]);
            }
            if (!test2.intersect) { // If we don't intersect with ourself, add light colour
                colours.push_back(lights[i]->colour);
            }
            // For a sphere there is a hack we could do here, just see if the ray origin is behind
            // the imaginary plane bisecting the sphere perpendicular to the light
            // But I need to understand the error
            // Could also just use the surface normal of the sphere at that point
        }
    } else {
        return 0;
    }
    if (colours.size() < 1) {
        // printf("crap\n");
        return 0; // We couldn't see any lights jack
    }
    // Strange... I'd expect only the side facing the sun to be hit

    glm::vec3 colour;
    // Average colours and normalise, ignore brightness for now
    for (int i=0; i<colours.size(); i++) {
        colour += colours[i];
    }
    colour = glm::normalize(colour);
    uint32_t red = colour[0] * 255;
    uint32_t green = colour[1] * 255;
    uint32_t blue = colour[2] * 255;

    uint32_t output = (blue | ( green << 8) | (red << 16) | (255 << 24));
    return output;
    
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    bool running = true;

    // Create a camera and a sphere in the scene
    Camera camera = initCamera();
    Sphere sphere1 = initSphere(glm::vec3(0.0, 0.0, -6.0), 5.8f);

    // Create lightsources
    Lights lights;
    lights.push_back(new Light(
        initSphere(glm::vec3(20.0, 0.0, -0.15), 0.1f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        1.0f
    ));
    lights.push_back(new Light(
        initSphere(glm::vec3(-40.0, 20.0, 5.0f), 0.1f),
        glm::vec3(0.1f, 0.5f, 0.9f),
        1.0f
    ));

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
            pixels[count] = buildPixelFromRays(x, y, true, screenP, sphere1, lights);
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
