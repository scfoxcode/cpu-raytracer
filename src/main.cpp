#include "SDL.h" 
#include <glm/glm.hpp>
#include "rays.h"

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

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    bool running = true;

    // Create a camera and a sphere in the scene
    Camera camera = initCamera();
    Sphere sphere1 = initSphere({0.0, 0.0, 6.0}, 5.8f);

    // okay. Lets try and build a ray traced sphere
    uint32_t* pixels = new uint32_t[pixelCount];
    ScreenProperties screenP = buildScreenProperties(RESX, RESY, camera.fov);

    int count = 0;
    for (int y=0; y<RESY; y++) {
        for (int x=0; x<RESX; x++) {
            Ray r = buildRayForScreenPixel(screenP, x, y); 
            float dist = raySphereIntersection(camera, r, sphere1); 
            if (dist > 0) {
                 pixels[count] = 0xffffffff;
            } else {
                 pixels[count] = 0;
            }
            count++;
        }
    }

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

    // Build texture from my ray tracing data
    
    SDL_Texture* sceneTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STATIC,
        RESX,
        RESY
    );

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
        SDL_Delay(10);
    }

    delete [] pixels;

    SDL_DestroyTexture(sceneTexture);
    SDL_DestroyWindow(window);
    SDL_Quit();


    return 0;
}
