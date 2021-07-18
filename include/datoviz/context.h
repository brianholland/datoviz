/*************************************************************************************************/
/*  GPU context holding buffers and textures in video memory                                     */
/*************************************************************************************************/

#ifndef DVZ_CONTEXT_HEADER
#define DVZ_CONTEXT_HEADER

#include "colormaps.h"
#include "common.h"
#include "fifo.h"
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

#define DVZ_BUFFER_TYPE_STAGING_SIZE (16 * 1024 * 1024)
#define DVZ_BUFFER_TYPE_VERTEX_SIZE  (16 * 1024 * 1024)
#define DVZ_BUFFER_TYPE_INDEX_SIZE   (16 * 1024 * 1024)
#define DVZ_BUFFER_TYPE_STORAGE_SIZE (16 * 1024 * 1024)
#define DVZ_BUFFER_TYPE_UNIFORM_SIZE (4 * 1024 * 1024)

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



// Filter type.
typedef enum
{
    DVZ_FILTER_MIN,
    DVZ_FILTER_MAG,
} DvzFilterType;



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzFontAtlas DvzFontAtlas;
typedef struct DvzColorTexture DvzColorTexture;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzFontAtlas
{
    const char* name;
    uint32_t width, height;
    uint32_t cols, rows;
    uint8_t* font_texture;
    float glyph_width, glyph_height;
    const char* font_str;
    DvzTexture* texture;
};



struct DvzColorTexture
{
    unsigned char* arr;
    DvzTexture* texture;
};



struct DvzContext
{
    DvzObject obj;
    DvzGpu* gpu;

    DvzContainer buffers;
    DvzContainer images;
    DvzContainer samplers;
    DvzContainer textures;
    DvzContainer computes;

    // Data transfers.
    DvzDeq deq;
    DvzThread thread; // transfer thread

    // Font atlas.
    DvzFontAtlas font_atlas;
    DvzColorTexture color_texture;
    DvzTexture* transfer_texture; // Default linear 1D texture
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

/**
 * Update the colormap texture on the GPU after it has changed on the CPU.
 *
 * @param context the context
 */
DVZ_EXPORT void dvz_context_colormap(DvzContext* context);



/*************************************************************************************************/
/*  Buffer allocation                                                                            */
/*************************************************************************************************/

/**
 * Allocate one of several buffer regions on the GPU.
 *
 * @param context the context
 * @param buffer_type the type of buffer to allocate the regions on
 * @param buffer_count the number of buffer regions to allocate
 * @param size the size of each region to allocate, in bytes
 */
DVZ_EXPORT DvzBufferRegions dvz_ctx_buffers(
    DvzContext* context, DvzBufferType buffer_type, uint32_t buffer_count, VkDeviceSize size);

/**
 * Resize a set of buffer regions.
 *
 * @param context the context
 * @param br the buffer regions to resize
 * @param new_size the new size of each buffer region, in bytes
 */
DVZ_EXPORT void
dvz_ctx_buffers_resize(DvzContext* context, DvzBufferRegions* br, VkDeviceSize new_size);



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

/**
 * Create a new compute pipeline.
 *
 * @param context the context
 * @param shader_path path to the `.spirv` file containing the compute shader
 */
DVZ_EXPORT DvzCompute* dvz_ctx_compute(DvzContext* context, const char* shader_path);



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

/**
 * Create a new GPU texture.
 *
 * @param context the context
 * @param dims the number of dimensions of the texture (1, 2, or 3)
 * @param size the width, height, and depth
 * @param format the format of each pixel
 */
DVZ_EXPORT DvzTexture*
dvz_ctx_texture(DvzContext* context, uint32_t dims, uvec3 size, VkFormat format);

/**
 * Resize a texture.
 *
 * !!! warning
 *     This function will delete the texture data.
 *
 * @param texture the texture
 * @param size the new size (width, height, depth)
 */
DVZ_EXPORT void dvz_texture_resize(DvzTexture* texture, uvec3 size);

/**
 * Set the texture filter.
 *
 * @param texture the texture
 * @param type the filter type
 * @param filter the filter
 */
DVZ_EXPORT void dvz_texture_filter(DvzTexture* texture, DvzFilterType type, VkFilter filter);

/**
 * Set the texture address mode.
 *
 * @param texture the texture
 * @param axis the axis
 * @param address_mode the address mode
 */
DVZ_EXPORT void dvz_texture_address_mode(
    DvzTexture* texture, DvzTextureAxis axis, VkSamplerAddressMode address_mode);

/**
 * Copy part of a texture to another texture.
 *
 * This function does not involve CPU-GPU data transfers.
 *
 * @param src the source texture
 * @param src_offset offset within the source texture
 * @param dst the target texture
 * @param dst_offset offset within the target texture
 * @param shape shape of the part of the texture to copy
 */
DVZ_EXPORT void dvz_texture_copy(
    DvzTexture* src, uvec3 src_offset, DvzTexture* dst, uvec3 dst_offset, uvec3 shape);

DVZ_EXPORT void dvz_texture_copy_from_buffer(
    DvzTexture* tex, uvec3 tex_offset, uvec3 shape, //
    DvzBufferRegions br, VkDeviceSize buf_offset, VkDeviceSize size);

DVZ_EXPORT void dvz_texture_copy_to_buffer(
    DvzTexture* tex, uvec3 tex_offset, uvec3 shape, //
    DvzBufferRegions br, VkDeviceSize buf_offset, VkDeviceSize size);

/**
 * Transition a texture to its layout.
 *
 * @param texture the texture to transition
 */
DVZ_EXPORT void dvz_texture_transition(DvzTexture* tex);

/**
 * Destroy a texture.
 *
 * @param texture the texture
 */
DVZ_EXPORT void dvz_texture_destroy(DvzTexture* texture);



#ifdef __cplusplus
}
#endif

#endif
