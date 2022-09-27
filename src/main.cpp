#include "SDL.h" 
#include <glm/glm.hpp>
#include <chrono>
#include "light.h"
#include "tonemapping.h"
#include "sphere.h"

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

uint32_t buildPixel(
    int x,
    int y,
    ScreenProperties& screenP,
    Lights& lights,
    std::vector<ICollidable>& objects) {

    return 0;
}

// We want to keep buildPixelFromRays parallel friendly so we can split across cores later if needed
uint32_t buildPixelFromRays(
    int x,
    int y,
    ScreenProperties &screenP,
    Sphere &sphere,
    Lights& lights) {

    Ray r = buildRayForScreenPixel(screenP, x, y); 
    MaybeIntersect test = sphere.rayIntersection(r); 

    std::vector<glm::vec3> colours;
    if (test.intersect) {
        for (int i=0; i<lights.size(); i++) {
            Ray r2 = buildRayForPoints(lights[i]->emitter.position, test.position);
            // We know it intersect with light because we've build our ray to point straight at it
            // What we need to know is does the sphere or other geometry block the ray
            
            r2.position += r2.direction * 0.01f; // Tiny hack to move ray slightly away from intial collision
            float dot = glm::dot(r2.direction, glm::normalize(r2.position - sphere.position));

            // Lighting impossible (FOR POINT LIGHTS ONLY!!!)
            if (dot < 0) {
                // Optimisation, only works because we are dealing with point lights
                continue; // Skip intersection test that will never pass 
            }

            MaybeIntersect test2 = sphere.rayIntersection(r2);
            if (!test2.intersect) { // If we don't intersect with ourself, add light colour
                // Use dot product between surface normal and ray to determine how
                // strong the colour should be
                colours.push_back(lights[i]->colour * lights[i]->brightness * dot);
            }
        }
    } else {
        return 0;
    }
    if (colours.size() < 1) {
        return 0; // We couldn't see any lights jack
    }

    glm::vec3 colour;
    // Add all contributing hdr colours together
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

    // Create a camera and a sphere in the scene
    Camera camera = initCamera();
    Sphere sphere1(glm::vec3(0.0, 0.0, -20.0), 5.8f);

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
            pixels[count] = buildPixelFromRays(x, y, screenP, sphere1, lights);
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
