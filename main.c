/**
 * TODO: only draw on draw calls!! CPU usage is through the roof rn
 */

#include "cc8.h"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>

#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define WINDOW_NAME "CHIP-8"

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

#define return_defer(value) do { result = (value); goto defer; } while(0)

// TODO: support different configurations
//       e.g. different keymaps, input modes
//            runtime configuration
int main(int argc, char **argv)
{

    // TODO: allow user to pass filepath arg to run chip-8 binary
    (void) argc;
    (void) argv;

    int result = 0;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Surface *surface = NULL;
    SDL_AudioStream *audiostream = NULL;

    if(!SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "pulseaudio"))
    {
        fprintf(stderr, "failed to set SDL hint: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    if(!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        fprintf(stderr, "failed to initialize SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    window = SDL_CreateWindow(WINDOW_NAME, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if(!window)
    {
        fprintf(stderr, "failed to create window: %s\n", SDL_GetError());
        return_defer(-1);
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if(!renderer)
    {
        fprintf(stderr, "failed to create renderer: %s\n", SDL_GetError());
        return_defer(-1);
    }

    const SDL_Color color_on = { .r = 255, .g = 255, .b = 255, .a = 255 };
    const SDL_Color color_off = { .r = 0, .g = 0, .b = 0, .a = 255 };

    const SDL_PixelFormatDetails *pixel_format_details = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);

    SDL_Rect grid[64 * 32] = {0};

    for(size_t y = 0; y < 32; ++y)
    {
        for(size_t x = 0; x < 64; ++x)
        {
            grid[(64 * y) + x] = (SDL_Rect) { x * (WINDOW_WIDTH / 64), y * (WINDOW_HEIGHT / 32), WINDOW_WIDTH / 64, WINDOW_HEIGHT / 32 };
        }
    }

    surface = SDL_CreateSurface(WINDOW_WIDTH, WINDOW_HEIGHT, SDL_PIXELFORMAT_RGBA8888);

    audiostream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL, NULL, NULL);

    Cc8_Context ctx = {0};
    cc8_init(&ctx);

    // TODO: create cc8_write_instruction_to_memory(...) function
    //       must swap bytes before writing to memory

    if(argc > 1) cc8_read_file(&ctx, argv[1]);
    /*cc8_read_file(&ctx, "chip-8/heartmonitor/heart_monitor.ch8");*/

    /*long time = 0;*/
    /*long last_time = 0;*/
    /*long delta_time = 0;*/
    /*long accumulator = 0;*/

    bool running = true;
    while(running)
    {
        struct timespec tp;
        if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp) != 0)
        {
            fprintf(stderr, "failed to get process time: %s\n", strerror(errno));
        }

        /*time = tp.tv_nsec;*/
        /*delta_time = time - last_time;*/
        /*last_time = time;*/

        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_EVENT_QUIT: running = false; break;

                case SDL_EVENT_KEY_DOWN: {
                    if(event.key.key == SDLK_ESCAPE) running = false;

                    else switch(event.key.scancode)
                    {
                        case SDL_SCANCODE_1: cc8_set_key(&ctx, CC8_KEY_1); break;
                        case SDL_SCANCODE_2: cc8_set_key(&ctx, CC8_KEY_2); break;
                        case SDL_SCANCODE_3: cc8_set_key(&ctx, CC8_KEY_3); break;
                        case SDL_SCANCODE_4: cc8_set_key(&ctx, CC8_KEY_C); break;

                        case SDL_SCANCODE_Q: cc8_set_key(&ctx, CC8_KEY_4); break;
                        case SDL_SCANCODE_W: cc8_set_key(&ctx, CC8_KEY_5); break;
                        case SDL_SCANCODE_E: cc8_set_key(&ctx, CC8_KEY_6); break;
                        case SDL_SCANCODE_R: cc8_set_key(&ctx, CC8_KEY_D); break;

                        case SDL_SCANCODE_A: cc8_set_key(&ctx, CC8_KEY_7); break;
                        case SDL_SCANCODE_S: cc8_set_key(&ctx, CC8_KEY_8); break;
                        case SDL_SCANCODE_D: cc8_set_key(&ctx, CC8_KEY_9); break;
                        case SDL_SCANCODE_F: cc8_set_key(&ctx, CC8_KEY_E); break;

                        case SDL_SCANCODE_Z: cc8_set_key(&ctx, CC8_KEY_A); break;
                        case SDL_SCANCODE_X: cc8_set_key(&ctx, CC8_KEY_0); break;
                        case SDL_SCANCODE_C: cc8_set_key(&ctx, CC8_KEY_B); break;
                        case SDL_SCANCODE_V: cc8_set_key(&ctx, CC8_KEY_F); break;

                        /*case SDL_SCANCODE_K: cc8_execute(&ctx, cc8_fetch_debug(&ctx)); break;*/
                        default: break;
                    }
                } break;

                case SDL_EVENT_KEY_UP: {
                    switch(event.key.scancode)
                    {
                        case SDL_SCANCODE_1: cc8_unset_key(&ctx, CC8_KEY_1); break;
                        case SDL_SCANCODE_2: cc8_unset_key(&ctx, CC8_KEY_2); break;
                        case SDL_SCANCODE_3: cc8_unset_key(&ctx, CC8_KEY_3); break;
                        case SDL_SCANCODE_4: cc8_unset_key(&ctx, CC8_KEY_C); break;

                        case SDL_SCANCODE_Q: cc8_unset_key(&ctx, CC8_KEY_4); break;
                        case SDL_SCANCODE_W: cc8_unset_key(&ctx, CC8_KEY_5); break;
                        case SDL_SCANCODE_E: cc8_unset_key(&ctx, CC8_KEY_6); break;
                        case SDL_SCANCODE_R: cc8_unset_key(&ctx, CC8_KEY_D); break;

                        case SDL_SCANCODE_A: cc8_unset_key(&ctx, CC8_KEY_7); break;
                        case SDL_SCANCODE_S: cc8_unset_key(&ctx, CC8_KEY_8); break;
                        case SDL_SCANCODE_D: cc8_unset_key(&ctx, CC8_KEY_9); break;
                        case SDL_SCANCODE_F: cc8_unset_key(&ctx, CC8_KEY_E); break;

                        case SDL_SCANCODE_Z: cc8_unset_key(&ctx, CC8_KEY_A); break;
                        case SDL_SCANCODE_X: cc8_unset_key(&ctx, CC8_KEY_0); break;
                        case SDL_SCANCODE_C: cc8_unset_key(&ctx, CC8_KEY_B); break;
                        case SDL_SCANCODE_V: cc8_unset_key(&ctx, CC8_KEY_F); break;
                        default: break;
                    }
                } break;
            }
        }

        // TODO: sync timer tick to 60/s
        // TODO: play sound on sound timer
        /*if(delta_time / 1000000 > 1.0f / 60.0f)*/
        /*{*/
            cc8_tick_timers(&ctx);
        /*}*/

        // TODO: sync execution to 500/s
        /*if(delta_time / 1000000 > 1.0f / 500.0f)*/
        /*{*/
#ifdef DEBUG
            cc8_execute(&ctx, cc8_fetch_debug(&ctx));
#else
            cc8_execute(&ctx, cc8_fetch(&ctx));
#endif // DEBUG
        /*}*/

        /*printf("KEY: %X\n", ctx.key);*/

        SDL_RenderClear(renderer);

        for(size_t y = 0; y < 32; ++y)
        {
            for(size_t x = 0; x < 64; ++x)
            {
                SDL_Color color = (cc8_cell_is_on_xy(&ctx, x, y) ? color_on : color_off);
                SDL_FillSurfaceRect(surface, &grid[(y * 64) + x], SDL_MapRGBA(pixel_format_details, NULL, color.r, color.g, color.b, color.a));
            }
        }

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_DestroyTexture(texture);

        SDL_RenderPresent(renderer);
    }

defer:

    if(audiostream) SDL_DestroyAudioStream(audiostream);
    if(surface) SDL_DestroySurface(surface);
    if(renderer) SDL_DestroyRenderer(renderer);
    if(window) SDL_DestroyWindow(window);
    SDL_Quit();

    return result;
}
