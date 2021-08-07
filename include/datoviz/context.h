/*************************************************************************************************/
/*  GPU context holding buffers and textures in video memory                                     */
/*************************************************************************************************/

#ifndef DVZ_CONTEXT_HEADER
#define DVZ_CONTEXT_HEADER

#include "allocs.h"
#include "colormaps.h"
#include "common.h"
#include "fifo.h"
#include "resources.h"
#include "transfers.h"
#include "vklite.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/

#define DVZ_DEFAULT_WIDTH  800
#define DVZ_DEFAULT_HEIGHT 600

#define DVZ_ZERO_OFFSET                                                                           \
    (uvec3) { 0, 0, 0 }



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

// Default queue.
typedef enum
{
    // NOTE: by convention in vklite, the first queue MUST support transfers
    DVZ_DEFAULT_QUEUE_TRANSFER,
    DVZ_DEFAULT_QUEUE_COMPUTE,
    DVZ_DEFAULT_QUEUE_RENDER,
    DVZ_DEFAULT_QUEUE_PRESENT,
    DVZ_DEFAULT_QUEUE_COUNT,
} DvzDefaultQueue;



struct DvzContext
{
    DvzObject obj;
    DvzGpu* gpu;

    // Companion objects, all of them should be testable independently of the others and the
    // context. However, the DvzAllocs objects *depends* on the DvzResources.
    DvzTransfers transfers;
    DvzResources res;
    DvzAllocs allocs;
};



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

/**
 * Create a GPU with default queues and features.
 *
 * @param gpu the GPU
 * @param window the associated window (optional)
 */
DVZ_EXPORT void dvz_gpu_default(DvzGpu* gpu, DvzWindow* window);

/**
 * Create a context associated to a GPU.
 *
 * !!! note
 *     The GPU must have been created beforehand.
 *
 * @param gpu the GPU
 */
DVZ_EXPORT DvzContext* dvz_context(DvzGpu* gpu);

/**
 * Destroy all GPU resources in a GPU context.
 *
 * @param context the context
 */
DVZ_EXPORT void dvz_context_reset(DvzContext* context);

/**
 * Reset all GPUs.
 *
 * @param app the application instance
 */
DVZ_EXPORT void dvz_app_reset(DvzApp* app);



/*************************************************************************************************/
/*  Dats                                                                                         */
/*************************************************************************************************/

// TODO: docstrings

DVZ_EXPORT DvzDat*
dvz_dat(DvzContext* ctx, DvzBufferType type, VkDeviceSize size, uint32_t count, int flags);

DVZ_EXPORT void
dvz_dat_upload(DvzDat* dat, VkDeviceSize offset, VkDeviceSize size, void* data, int flags);

DVZ_EXPORT void dvz_dat_download(DvzDat* dat, VkDeviceSize size, void* data, int flags);

DVZ_EXPORT void dvz_dat_destroy(DvzDat* dat);



/*************************************************************************************************/
/*  Texs                                                                                         */
/*************************************************************************************************/

DVZ_EXPORT DvzTex* dvz_tex(DvzContext* ctx, DvzTexDims dims, uvec3 shape, int flags);

DVZ_EXPORT void
dvz_tex_upload(DvzTex* tex, uvec3 offset, uvec3 shape, VkDeviceSize size, void* data, int flags);

DVZ_EXPORT void
dvz_tex_download(DvzTex* tex, uvec3 offset, uvec3 shape, VkDeviceSize size, void* data, int flags);

DVZ_EXPORT void dvz_tex_destroy(DvzTex* tex);



#ifdef __cplusplus
}
#endif

#endif
