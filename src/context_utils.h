#ifndef DVZ_CONTEXT_UTILS_HEADER
#define DVZ_CONTEXT_UTILS_HEADER

#include "../include/datoviz/context.h"

#ifdef __cplusplus
extern "C" {
#endif



/*************************************************************************************************/
/*  Staging buffer                                                                               */
/*************************************************************************************************/

// Get the staging buffer, and make sure it can contain `size` bytes.
static DvzBuffer* staging_buffer(DvzContext* context, VkDeviceSize size)
{
    log_trace("requesting staging buffer of size %s", pretty_size(size));
    DvzBuffer* staging = (DvzBuffer*)dvz_container_get(&context->buffers, DVZ_BUFFER_TYPE_STAGING);
    ASSERT(staging != NULL);
    ASSERT(staging->buffer != VK_NULL_HANDLE);

    // Resize the staging buffer is needed.
    // TODO: keep staging buffer fixed and copy parts of the data to staging buffer in several
    // steps?
    if (staging->size < size)
    {
        VkDeviceSize new_size = dvz_next_pow2(size);
        log_debug("reallocating staging buffer to %s", pretty_size(new_size));
        dvz_buffer_resize(staging, new_size);
    }
    ASSERT(staging->size >= size);
    return staging;
}



static void _copy_buffer_from_staging(
    DvzContext* context, DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(context != NULL);

    DvzGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    DvzCommands* cmds = &cmds_;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    VkBufferCopy region = {0};
    region.size = size;
    region.srcOffset = 0;
    region.dstOffset = br.offsets[0] + offset;
    vkCmdCopyBuffer(cmds->cmds[0], staging->buffer, br.buffer->buffer, br.count, &region);
    dvz_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we stop all
    // rendering so that we're sure that the buffer we're going to write to is not
    // being used by the GPU.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    log_debug("copy %s from staging buffer", pretty_size(size));
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



static void _copy_buffer_to_staging(
    DvzContext* context, DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(context != NULL);

    DvzGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    DvzCommands* cmds = &cmds_;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    // Determine the offset in the source buffer.
    // Should be consecutive offsets.
    VkDeviceSize vk_offset = br.offsets[0];
    uint32_t n_regions = br.count;
    for (uint32_t i = 1; i < n_regions; i++)
    {
        ASSERT(br.offsets[i] == vk_offset + i * size);
    }
    // Take into account the transfer offset.
    vk_offset += offset;

    // Copy to staging buffer
    ASSERT(br.buffer != 0);
    dvz_cmd_copy_buffer(cmds, 0, br.buffer, vk_offset, staging, 0, size * n_regions);
    dvz_cmd_end(cmds, 0);

    // Wait for the compute queue to be idle, as we assume the buffer to be copied from may
    // be modified by compute shaders.
    // TODO: more efficient synchronization (semaphores?)
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_COMPUTE);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    log_debug("copy %s to staging buffer", pretty_size(size));
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



static void _copy_texture_from_staging(
    DvzContext* context, DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size)
{
    ASSERT(context != NULL);

    DvzGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    DvzCommands* cmds = &cmds_;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    // Image transition.
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);
    dvz_barrier_images(&barrier, texture->image);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    dvz_cmd_copy_buffer_to_image(cmds, 0, staging, texture->image);

    // Image transition.
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture->image->layout);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    dvz_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



static void _copy_texture_to_staging(
    DvzContext* context, DvzTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size)
{
    ASSERT(context != NULL);

    DvzGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    DvzBuffer* staging = staging_buffer(context, size);
    ASSERT(staging != NULL);

    // Take transfer cmd buf.
    DvzCommands cmds_ = dvz_commands(gpu, 0, 1);
    DvzCommands* cmds = &cmds_;
    dvz_cmd_reset(cmds, 0);
    dvz_cmd_begin(cmds, 0);

    // Image transition.
    DvzBarrier barrier = dvz_barrier(gpu);
    dvz_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);
    dvz_barrier_images(&barrier, texture->image);
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    dvz_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_READ_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    dvz_cmd_copy_image_to_buffer(cmds, 0, texture->image, staging);

    // Image transition.
    dvz_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->image->layout);
    dvz_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT);
    dvz_cmd_barrier(cmds, 0, &barrier);

    dvz_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    DvzSubmit submit = dvz_submit(gpu);
    dvz_submit_commands(&submit, cmds);
    dvz_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    // TODO: less brutal synchronization with semaphores. Here we wait for the
    // transfer to be complete before we send new rendering commands.
    // dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



/*************************************************************************************************/
/*  Default resources                                                                            */
/*************************************************************************************************/

static DvzTexture* _default_transfer_texture(DvzContext* context)
{
    ASSERT(context != NULL);
    DvzGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    uvec3 shape = {256, 1, 1};
    DvzTexture* texture = dvz_ctx_texture(context, 1, shape, VK_FORMAT_R32_SFLOAT);
    float* tex_data = (float*)calloc(256, sizeof(float));
    for (uint32_t i = 0; i < 256; i++)
        tex_data[i] = i / 255.0;
    dvz_texture_address_mode(texture, DVZ_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    uvec3 offset = {0, 0, 0};

    dvz_texture_upload(texture, offset, offset, 256 * sizeof(float), tex_data);
    dvz_queue_wait(gpu, DVZ_DEFAULT_QUEUE_TRANSFER);

    FREE(tex_data);
    return texture;
}



#ifdef __cplusplus
}
#endif

#endif
