/*************************************************************************************************/
/*  Singleton application, managing all GPU objects and windows                                  */
/*************************************************************************************************/

#ifndef DVZ_APP_HEADER
#define DVZ_APP_HEADER

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// #define GLFW_INCLUDE_VULKAN
#include <vulkan/vulkan.h>

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Type definitions */
/*************************************************************************************************/

typedef struct DvzApp DvzApp;
typedef struct DvzClock DvzClock;



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Backend.
typedef enum
{
    DVZ_BACKEND_NONE,
    DVZ_BACKEND_GLFW,
    DVZ_BACKEND_QT5,
    DVZ_BACKEND_OFFSCREEN,
} DvzBackend;



/*************************************************************************************************/
/*  Clock                                                                                        */
/*************************************************************************************************/

struct DvzClock
{
    double elapsed;  // time in seconds elapsed since calling _start_time(clock)
    double interval; // interval since the last clock update

    struct timeval start, current;
    // double checkpoint_time;
    // uint64_t checkpoint_value;
};



static inline void _clock_init(DvzClock* clock) { gettimeofday(&clock->start, NULL); }



static inline double _clock_get(DvzClock* clock)
{
    gettimeofday(&clock->current, NULL);
    double elapsed = (clock->current.tv_sec - clock->start.tv_sec) +
                     (clock->current.tv_usec - clock->start.tv_usec) / 1000000.0;
    return elapsed;
}



static inline void _clock_set(DvzClock* clock)
{
    // Typically called at every frame.
    double elapsed = _clock_get(clock);
    clock->interval = elapsed - clock->elapsed;
    clock->elapsed = elapsed;
}



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzApp
{
    DvzObject obj;
    uint32_t n_errors;

    // Backend
    DvzBackend backend;

    // Global clock
    DvzClock clock;
    bool is_running;

    // Vulkan objects.
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;

    // Containers.
    DvzContainer gpus;
    DvzContainer windows;
    DvzContainer canvases;

    // Threads.
    DvzThread timer_thread;
};



#ifdef __cplusplus
}
#endif

#endif
