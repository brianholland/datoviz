/*************************************************************************************************/
/* Data transfer                                                                                 */
/*************************************************************************************************/

#ifndef DVZ_TRANSFERS_UTILS_HEADER
#define DVZ_TRANSFERS_UTILS_HEADER

#include "../include/datoviz/context.h"
#include "../include/datoviz/transfers.h"
#include "../include/datoviz/vklite.h"



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Create tasks                                                                                 */
/*************************************************************************************************/

// Create a mappable buffer transfer task, either UPLOAD or DOWNLOAD.
static DvzDeqItem* _create_buffer_transfer(
    DvzTransferType type, DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data,
    uint32_t deq_idx)
{
    // Upload/download to mappable buffer only (otherwise, will need a GPU-GPU copy task too).

    ASSERT(br.buffer != NULL);
    ASSERT(size > 0);
    ASSERT(data != NULL);

    ASSERT(type == DVZ_TRANSFER_BUFFER_UPLOAD || type == DVZ_TRANSFER_BUFFER_DOWNLOAD);

    DvzTransferBuffer* tr = (DvzTransferBuffer*)calloc(1, sizeof(DvzTransferBuffer));
    tr->br = br;
    tr->offset = offset;
    tr->size = size;
    tr->data = data;

    return dvz_deq_enqueue_custom(deq_idx, (int)type, tr);
}



// Create a buffer copy task.
static DvzDeqItem* _create_buffer_copy(
    DvzBufferRegions src, VkDeviceSize src_offset, DvzBufferRegions dst, VkDeviceSize dst_offset,
    VkDeviceSize size)
{
    ASSERT(src.buffer != NULL);
    ASSERT(dst.buffer != NULL);
    ASSERT(size > 0);

    DvzTransferBufferCopy* tr = (DvzTransferBufferCopy*)calloc(1, sizeof(DvzTransferBufferCopy));
    tr->src = src;
    tr->src_offset = src_offset;
    tr->dst = dst;
    tr->dst_offset = dst_offset;
    tr->size = size;

    return dvz_deq_enqueue_custom(DVZ_TRANSFER_DEQ_COPY, (int)DVZ_TRANSFER_BUFFER_COPY, tr);
}



// Create a buffer <-> image copy task.
static DvzDeqItem* _create_buffer_image_copy(
    DvzTransferType type,                                            //
    DvzBufferRegions br, VkDeviceSize buf_offset, VkDeviceSize size, //
    DvzImages* img, uvec3 img_offset, uvec3 shape                    //
)
{
    ASSERT(type == DVZ_TRANSFER_IMAGE_BUFFER || type == DVZ_TRANSFER_BUFFER_IMAGE);
    ASSERT(br.buffer != NULL);
    ASSERT(size > 0);

    ASSERT(img != NULL);
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    DvzTransferBufferImage* tr = (DvzTransferBufferImage*)calloc(
        1, sizeof(DvzTransferBufferImage)); // will be free-ed by the callbacks

    tr->br = br;
    tr->buf_offset = buf_offset;
    tr->size = size;

    tr->img = img;
    memcpy(tr->img_offset, img_offset, sizeof(uvec3));
    memcpy(tr->shape, shape, sizeof(uvec3));

    return dvz_deq_enqueue_custom(DVZ_TRANSFER_DEQ_COPY, (int)type, tr);
}



// Create a image to image copy task.
static DvzDeqItem* _create_image_copy(
    DvzImages* src, uvec3 src_offset,             //
    DvzImages* dst, uvec3 dst_offset, uvec3 shape //
)
{
    ASSERT(src != NULL);
    ASSERT(dst != NULL);
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    DvzTransferImageCopy* tr = (DvzTransferImageCopy*)calloc(
        1, sizeof(DvzTransferImageCopy)); // will be free-ed by the callbacks
    tr->src = src;
    tr->dst = dst;
    memcpy(tr->src_offset, src_offset, sizeof(uvec3));
    memcpy(tr->dst_offset, dst_offset, sizeof(uvec3));
    memcpy(tr->shape, shape, sizeof(uvec3));

    return dvz_deq_enqueue_custom(DVZ_TRANSFER_DEQ_COPY, (int)DVZ_TRANSFER_IMAGE_COPY, tr);
}



// Create a download done task.
static DvzDeqItem* _create_download_done(VkDeviceSize size, void* data)
{
    ASSERT(data != NULL);

    DvzTransferDownload* tr = (DvzTransferDownload*)calloc(
        1, sizeof(DvzTransferDownload)); // will be free-ed by the callbacks
    tr->size = size;
    tr->data = data;
    return dvz_deq_enqueue_custom(DVZ_TRANSFER_DEQ_EV, (int)DVZ_TRANSFER_DOWNLOAD_DONE, tr);
}



// Create a mappable buffer dup upload task.
static DvzDeqItem* _create_dup_upload(
    DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size, void* data, uint32_t deq_idx)
{
    ASSERT(br.buffer != NULL);
    ASSERT(size > 0);
    ASSERT(data != NULL);

    DvzTransferDup* tr = (DvzTransferDup*)calloc(1, sizeof(DvzTransferDup));
    tr->type = DVZ_TRANSFER_DUP_UPLOAD;
    tr->br = br;
    tr->offset = offset;
    tr->size = size;
    tr->data = data;
    // TODO: recurrent?

    return dvz_deq_enqueue_custom(deq_idx, (int)DVZ_TRANSFER_DUP_UPLOAD, tr);
}



// Create a dup copy task, copying data from a staging buffer to a non-mappable buffer, on a
// region-per-region basis (multiple copies of the same data to avoid too much GPU sync).
static DvzDeqItem* _create_dup_copy(
    DvzBufferRegions src, VkDeviceSize src_offset, //
    DvzBufferRegions dst, VkDeviceSize dst_offset, //
    VkDeviceSize size, uint32_t deq_idx)
{
    ASSERT(src.buffer != NULL);
    ASSERT(dst.buffer != NULL);
    // NOTE: for now we impose the staging (source) buffer to have a single copy of the data.
    ASSERT(src.count == 1);
    ASSERT(size > 0);

    DvzTransferDup* tr = (DvzTransferDup*)calloc(1, sizeof(DvzTransferDup));
    tr->type = DVZ_TRANSFER_DUP_COPY;
    tr->br = dst;
    tr->offset = dst_offset;
    tr->stg = src;
    tr->stg_offset = src_offset;
    tr->size = size;
    // TODO: recurrent?

    return dvz_deq_enqueue_custom(deq_idx, (int)DVZ_TRANSFER_DUP_COPY, tr);
}



/*************************************************************************************************/
/*  Buffer transfer task enqueuing                                                               */
/*************************************************************************************************/

// WARNING: if there is NO staging buffer, the caller must dequeue the CPY proc manually on the
// main thread to ensure the upload is done. This is to give a chance to the caller to synchronize
// access to the mappable buffer.
static void _enqueue_buffer_upload(
    DvzDeq* deq,                                   //
    DvzBufferRegions br, VkDeviceSize buf_offset,  // destination buffer
    DvzBufferRegions stg, VkDeviceSize stg_offset, // optional staging buffer
    VkDeviceSize size, void* data)
{
    ASSERT(deq != NULL);
    ASSERT(size > 0);
    ASSERT(data != NULL);
    log_trace("enqueue buffer upload");

    DvzDeqItem* deq_item = NULL;
    DvzDeqItem* next_item = NULL;

    // Upload to a mappable buffer, no need for a staging buffer.
    if (stg.buffer == NULL)
    {
        // Upload in one step, directly to the destination buffer that is assumed to be mappable.

        // NOTE: we use the COPY queue, not the UPLOAD one, because we *don't* want this task to be
        // automatically dequeued and processed by the background thread. Since there's no staging
        // buffer, we probably want to synchronize access to this buffer, so we want this to happen
        // in the main thread. It is up to the caller to do the synchronization *and* to dequeue
        // the COPY queue manually. Otherwise the upload won't happen!
        deq_item = _create_buffer_transfer(
            DVZ_TRANSFER_BUFFER_UPLOAD, br, buf_offset, size, data, DVZ_TRANSFER_DEQ_COPY);
    }
    // Upload to a staging buffer first.
    else
    {
        // First, upload to the staging buffer.
        deq_item = _create_buffer_transfer(
            DVZ_TRANSFER_BUFFER_UPLOAD, stg, stg_offset, size, data, DVZ_TRANSFER_DEQ_UL);

        // Then, need to do a copy to the destination buffer.
        next_item = _create_buffer_copy(stg, stg_offset, br, buf_offset, size);

        // Dependency.
        dvz_deq_enqueue_next(deq_item, next_item, false);
    }

    dvz_deq_enqueue_submit(deq, deq_item, false);
}



static void _enqueue_buffer_download(
    DvzDeq* deq,                                   //
    DvzBufferRegions br, VkDeviceSize buf_offset,  //
    DvzBufferRegions stg, VkDeviceSize stg_offset, //
    VkDeviceSize size, void* data)
{
    ASSERT(deq != NULL);
    ASSERT(size > 0);
    ASSERT(data != NULL);
    log_trace("enqueue buffer download");

    DvzDeqItem* deq_item = NULL;
    DvzDeqItem* next_item = NULL;
    DvzDeqItem* done_item = NULL;

    // Upload to a mappable buffer, no need for a staging buffer.
    if (stg.buffer == NULL)
    {
        // Upload in one step, directly to the destination buffer that is assumed to be mappable.
        deq_item = _create_buffer_transfer(
            DVZ_TRANSFER_BUFFER_DOWNLOAD, br, buf_offset, size, data, DVZ_TRANSFER_DEQ_DL);

        // Enqueue the DOWNLOAD_DONE item after the download has finished.
        done_item = _create_download_done(size, data);
        dvz_deq_enqueue_next(deq_item, done_item, false);
    } // Upload to a staging buffer first.
    else
    {
        // First, need to do a copy to the staging buffer.
        deq_item = _create_buffer_copy(br, buf_offset, stg, stg_offset, size);

        // Then, download from the staging buffer.
        next_item = _create_buffer_transfer(
            DVZ_TRANSFER_BUFFER_DOWNLOAD, stg, stg_offset, size, data, DVZ_TRANSFER_DEQ_DL);

        // Dependency.
        dvz_deq_enqueue_next(deq_item, next_item, false);

        // Enqueue the DOWNLOAD_DONE item after the download has finished.
        done_item = _create_download_done(size, data);
        dvz_deq_enqueue_next(next_item, done_item, false);
    }

    dvz_deq_enqueue_submit(deq, deq_item, false);
}



static void _enqueue_buffer_copy(
    DvzDeq* deq,                                   //
    DvzBufferRegions src, VkDeviceSize src_offset, //
    DvzBufferRegions dst, VkDeviceSize dst_offset, //
    VkDeviceSize size)
{
    ASSERT(deq != NULL);
    ASSERT(src.buffer != NULL);
    ASSERT(dst.buffer != NULL);
    ASSERT(size > 0);
    log_trace("enqueue buffer copy");

    DvzDeqItem* deq_item = _create_buffer_copy(src, src_offset, dst, dst_offset, size);
    dvz_deq_enqueue_submit(deq, deq_item, false);
}



// static void _enqueue_buffer_download_done(DvzDeq* deq, VkDeviceSize size, void* data)
// {
//     ASSERT(deq != NULL);
//     ASSERT(size > 0);
//     log_info("enqueue buffer download done");
//     dvz_deq_enqueue_submit(deq, _create_download_done(size, data), false);
// }



/*************************************************************************************************/
/*  Dup transfer task enqueuing                                                                  */
/*************************************************************************************************/

static void _enqueue_dup_transfer(
    DvzDeq* deq,                                   //
    DvzBufferRegions br, VkDeviceSize buf_offset,  // destination buffer
    DvzBufferRegions stg, VkDeviceSize stg_offset, // optional staging buffer
    VkDeviceSize size, void* data)
{
    ASSERT(deq != NULL);
    ASSERT(size > 0);
    ASSERT(data != NULL);

    DvzDeqItem* deq_item = NULL;
    DvzDeqItem* next_item = NULL;

    // Upload to a mappable buffer, no need for a staging buffer.
    if (stg.buffer == NULL)
    {
        log_trace("enqueue dup direct upload");
        // Upload in one step, directly to the destination buffer that is assumed to be mappable.

        // NOTE: we use the COPY queue, not the UPLOAD one, because we *don't* want this task to be
        // automatically dequeued and processed by the background thread. Since there's no staging
        // buffer, we probably want to synchronize access to this buffer, so we want this to happen
        // in the main thread. It is up to the caller to do the synchronization *and* to dequeue
        // the COPY queue manually. Otherwise the upload won't happen!
        deq_item = _create_dup_upload(br, buf_offset, size, data, DVZ_TRANSFER_DEQ_DUP);
    }
    // Upload to a staging buffer first.
    else
    {
        log_trace("enqueue upload to staging and dup copy");

        // First, upload to the staging buffer.
        deq_item = _create_buffer_transfer(
            DVZ_TRANSFER_BUFFER_UPLOAD, stg, stg_offset, size, data, DVZ_TRANSFER_DEQ_UL);

        // Then, need to enqueue a dup transfer from staging.
        next_item = _create_dup_copy(stg, stg_offset, br, buf_offset, size, DVZ_TRANSFER_DEQ_DUP);

        // Dependency.
        dvz_deq_enqueue_next(deq_item, next_item, false);
    }

    dvz_deq_enqueue_submit(deq, deq_item, false);
}



/*************************************************************************************************/
/*  Image transfer task enqueuing                                                              */
/*************************************************************************************************/

static void _enqueue_image_upload(
    DvzDeq* deq, DvzImages* img, uvec3 offset, uvec3 shape, //
    DvzBufferRegions stg, VkDeviceSize stg_offset, VkDeviceSize size, void* data)
{
    ASSERT(deq != NULL);

    ASSERT(img != NULL);
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    ASSERT(stg.buffer != NULL);

    ASSERT(size > 0);
    ASSERT(data != NULL);

    log_trace("enqueue image upload");

    DvzDeqItem* deq_item = NULL;
    DvzDeqItem* next_item = NULL;

    // First, upload to the staging buffer.
    deq_item = _create_buffer_transfer(
        DVZ_TRANSFER_BUFFER_UPLOAD, stg, stg_offset, size, data, DVZ_TRANSFER_DEQ_UL);

    // Then, need to do a copy to the image.
    next_item = _create_buffer_image_copy(
        DVZ_TRANSFER_BUFFER_IMAGE, stg, stg_offset, size, img, offset, shape);

    // Dependency.
    dvz_deq_enqueue_next(deq_item, next_item, false);

    dvz_deq_enqueue_submit(deq, deq_item, false);
}



static void _enqueue_image_download(
    DvzDeq* deq, DvzImages* img, uvec3 offset, uvec3 shape, //
    DvzBufferRegions stg, VkDeviceSize stg_offset, VkDeviceSize size, void* data)
{
    ASSERT(deq != NULL);

    ASSERT(img != NULL);
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    ASSERT(stg.buffer != NULL);

    ASSERT(size > 0);
    ASSERT(data != NULL);

    log_trace("enqueue image download");

    DvzDeqItem* deq_item = NULL;
    DvzDeqItem* next_item = NULL;

    // First, need to do a copy to the image.
    deq_item = _create_buffer_image_copy(
        DVZ_TRANSFER_IMAGE_BUFFER, stg, stg_offset, size, img, offset, shape);

    // Then, download from the staging buffer.
    next_item = _create_buffer_transfer(
        DVZ_TRANSFER_BUFFER_DOWNLOAD, stg, stg_offset, size, data, DVZ_TRANSFER_DEQ_DL);

    // Dependency.
    dvz_deq_enqueue_next(deq_item, next_item, false);

    // Enqueue the DOWNLOAD_DONE item after the download has finished.
    DvzDeqItem* done_item = _create_download_done(size, data);
    dvz_deq_enqueue_next(next_item, done_item, false);

    dvz_deq_enqueue_submit(deq, deq_item, false);
}



static void _enqueue_image_copy(
    DvzDeq* deq, DvzImages* src, uvec3 src_offset, DvzImages* dst, uvec3 dst_offset, uvec3 shape)
{
    ASSERT(deq != NULL);
    ASSERT(src != NULL);
    ASSERT(dst != NULL);

    log_trace("enqueue image copy");

    DvzDeqItem* deq_item = _create_image_copy(src, src_offset, dst, dst_offset, shape);
    dvz_deq_enqueue_submit(deq, deq_item, false);
}



/*************************************************************************************************/
/*  Buffer/Image copy transfer task enqueuing                                                  */
/*************************************************************************************************/

static void _enqueue_image_buffer(
    DvzDeq* deq,                                   //
    DvzImages* img, uvec3 img_offset, uvec3 shape, //
    DvzBufferRegions br, VkDeviceSize buf_offset, VkDeviceSize size)
{
    ASSERT(deq != NULL);

    ASSERT(img != NULL);
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    ASSERT(br.buffer != NULL);
    ASSERT(size > 0);

    log_trace("enqueue image buffer copy");

    // First, need to do a copy to the image.
    DvzDeqItem* deq_item = _create_buffer_image_copy(
        DVZ_TRANSFER_IMAGE_BUFFER, br, buf_offset, size, img, img_offset, shape);
    dvz_deq_enqueue_submit(deq, deq_item, false);
}



static void _enqueue_buffer_image(
    DvzDeq* deq,                                                     //
    DvzBufferRegions br, VkDeviceSize buf_offset, VkDeviceSize size, //
    DvzImages* img, uvec3 img_offset, uvec3 shape                    //
)
{
    ASSERT(deq != NULL);

    ASSERT(img != NULL);
    ASSERT(shape[0] > 0);
    ASSERT(shape[1] > 0);
    ASSERT(shape[2] > 0);

    ASSERT(br.buffer != NULL);
    ASSERT(size > 0);

    log_trace("enqueue image buffer copy");

    // First, need to do a copy to the image.
    DvzDeqItem* deq_item = _create_buffer_image_copy(
        DVZ_TRANSFER_IMAGE_BUFFER, br, buf_offset, size, img, img_offset, shape);
    dvz_deq_enqueue_submit(deq, deq_item, false);
}



/*************************************************************************************************/
/*  Buffer transfer task processing                                                              */
/*************************************************************************************************/

// NOTE: only upload to mappable buffer
static void _process_buffer_upload(DvzDeq* deq, void* item, void* user_data)
{
    DvzTransferBuffer* tr = (DvzTransferBuffer*)item;
    ASSERT(tr != NULL);
    log_trace("process mappable buffer upload");

    // Copy the data to the staging buffer.
    ASSERT(tr->br.buffer != NULL);
    ASSERT(tr->br.size > 0);
    ASSERT(tr->size > 0);
    ASSERT(tr->offset + tr->size <= tr->br.size);

    // Take offset and size into account in the staging buffer.
    // NOTE: this call blocks while the data is being copied from CPU to GPU (mapped memcpy).
    dvz_buffer_regions_upload(&tr->br, 0, tr->offset, tr->size, tr->data);
}



static void _process_buffer_download(DvzDeq* deq, void* item, void* user_data)
{
    DvzTransferBuffer* tr = (DvzTransferBuffer*)item;
    ASSERT(tr != NULL);
    log_trace("process mappable buffer download");

    // Copy the data to the staging buffer.
    ASSERT(tr->br.buffer != NULL);
    ASSERT(tr->br.size > 0);
    ASSERT(tr->size > 0);
    ASSERT(tr->offset + tr->size <= tr->br.size);

    // Take offset and size into account in the staging buffer.
    // NOTE: this call blocks while the data is being copied from GPU to CPU (mapped
    // memcpy).
    dvz_buffer_regions_download(&tr->br, 0, tr->offset, tr->size, tr->data);
}



static void _process_buffer_copy(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(user_data != NULL);
    DvzTransfers* transfers = (DvzTransfers*)user_data;
    log_trace("process buffer copy (sync)");

    DvzTransferBufferCopy* tr = (DvzTransferBufferCopy*)item;
    ASSERT(tr != NULL);

    // Make the GPU-GPU buffer copy (block the GPU and wait for the copy to finish).

    // TODO OPTIM: use a batch callback to wait only if there is >= 1 dequeued tasks, instead of
    // waiting at every dequeued item in a given batch.

    dvz_queue_wait(transfers->gpu, DVZ_DEFAULT_QUEUE_RENDER);
    dvz_buffer_regions_copy(
        &tr->src, UINT32_MAX, tr->src_offset, &tr->dst, UINT32_MAX, tr->dst_offset, tr->size);
    dvz_queue_wait(transfers->gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



/*************************************************************************************************/
/*  Buffer/Image copy transfer task processing                                                 */
/*************************************************************************************************/

static void _process_image_buffer(DvzDeq* deq, void* item, void* user_data)
{
    DvzTransferBufferImage* tr = (DvzTransferBufferImage*)item;
    ASSERT(tr != NULL);
    log_trace("process copy image to buffer (sync)");

    // Copy the data to the staging buffer.
    ASSERT(tr->img != NULL);
    ASSERT(tr->br.buffer != NULL);

    DvzTransfers* transfers = (DvzTransfers*)user_data;
    ASSERT(transfers != NULL);

    ASSERT(tr->shape[0] > 0);
    ASSERT(tr->shape[1] > 0);
    ASSERT(tr->shape[2] > 0);

    // TODO
    // dvz_image_copy_to_buffer(tr->img, tr->img_offset, tr->shape, tr->br, tr->buf_offset,
    // tr->size); Wait for the copy to be finished.
    dvz_queue_wait(transfers->gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



static void _process_buffer_image(DvzDeq* deq, void* item, void* user_data)
{
    DvzTransferBufferImage* tr = (DvzTransferBufferImage*)item;
    ASSERT(tr != NULL);
    log_trace("process copy buffer to image (sync)");

    // Copy the data to the staging buffer.
    ASSERT(tr->img != NULL);
    ASSERT(tr->br.buffer != NULL);

    DvzTransfers* transfers = (DvzTransfers*)user_data;
    ASSERT(transfers != NULL);

    ASSERT(tr->shape[0] > 0);
    ASSERT(tr->shape[1] > 0);
    ASSERT(tr->shape[2] > 0);

    // TODO
    // dvz_image_copy_from_buffer(
    //     tr->img, tr->img_offset, tr->shape, tr->br, tr->buf_offset, tr->size);
    // Wait for the copy to be finished.
    dvz_queue_wait(transfers->gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



/*************************************************************************************************/
/*  Image transfer task processing                                                             */
/*************************************************************************************************/

static void _process_image_copy(DvzDeq* deq, void* item, void* user_data)
{
    ASSERT(user_data != NULL);
    DvzTransfers* transfers = (DvzTransfers*)user_data;
    log_trace("process image copy");

    DvzTransferImageCopy* tr = (DvzTransferImageCopy*)item;
    ASSERT(tr != NULL);

    // Make the GPU-GPU buffer copy (block the GPU and wait for the copy to finish).
    dvz_queue_wait(transfers->gpu, DVZ_DEFAULT_QUEUE_RENDER);
    // TODO
    // dvz_image_copy(tr->src, tr->src_offset, tr->dst, tr->dst_offset, tr->shape);
    dvz_queue_wait(transfers->gpu, DVZ_DEFAULT_QUEUE_TRANSFER);
}



/*************************************************************************************************/
/*  Dup transfers                                                                                */
/*************************************************************************************************/

static DvzTransferDups _dups()
{
    DvzTransferDups dups = {0};
    dups.count = 0;
    return dups;
}

static uint32_t _dups_get_idx(
    DvzTransferDups* dups, DvzTransferType type, DvzBufferRegions br, VkDeviceSize offset,
    VkDeviceSize size)
{
    DvzTransferDup* tr = NULL;
    for (uint32_t i = 0; i < DVZ_DUPS_MAX; i++)
    {
        tr = &dups->dups[i].tr;
        if (tr->type == type &&                   //
            tr->br.buffer == br.buffer &&         //
            tr->br.offsets[0] == br.offsets[0] && //
            tr->offset == offset &&               //
            tr->size == size                      //
        )
            return i;
    }
    // log_error("could not find buffer region in DvzTransferDups structure");
    return UINT32_MAX;
}

static DvzTransferDupItem* _dups_get(
    DvzTransferDups* dups, DvzTransferType type, DvzBufferRegions br, VkDeviceSize offset,
    VkDeviceSize size)
{
    ASSERT(dups != NULL);
    uint32_t idx = _dups_get_idx(dups, type, br, offset, size);
    if (idx >= DVZ_DUPS_MAX)
        return NULL;
    ASSERT(idx < DVZ_DUPS_MAX);
    DvzTransferDupItem* dup = &dups->dups[idx];
    return dup;
}

static bool _dups_empty(DvzTransferDups* dups)
{
    ASSERT(dups != NULL);
    return dups->count == 0;
}

static bool _dups_has(
    DvzTransferDups* dups, DvzTransferType type, //
    DvzBufferRegions br, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(dups != NULL);
    return _dups_get(dups, type, br, offset, size) != NULL;
}

static void _dups_append(DvzTransferDups* dups, DvzTransferDup* tr)
{
    ASSERT(dups != NULL);
    ASSERT(tr != NULL);
    DvzBufferRegions* br = &tr->br;
    VkDeviceSize offset = tr->offset;
    VkDeviceSize size = tr->size;

    // Avoid duplicates.
    if (_dups_has(dups, tr->type, *br, offset, size))
    {
        log_debug("skip dup append as the item already exists");
        return;
    }
    for (uint32_t i = 0; i < DVZ_DUPS_MAX; i++)
    {
        if (!dups->dups[i].is_set)
        {
            dups->dups[i].is_set = 1;
            dups->dups[i].tr = *tr;
            dups->count++;
            return;
        }
    }
    log_error("dups list is full!");
}



static void _dups_remove(DvzTransferDups* dups, DvzTransferDupItem* item)
{
    ASSERT(dups != NULL);
    ASSERT(dups->count > 0);
    memset(item, 0, sizeof(DvzTransferDupItem));
    dups->count--;
}

static void _dups_mark_done(DvzTransferDups* dups, DvzTransferDupItem* item, uint32_t buf_idx)
{
    ASSERT(dups != NULL);
    ASSERT(item != NULL);
    item->done[buf_idx] = true;
}

static bool _dups_is_done(DvzTransferDups* dups, DvzTransferDupItem* item, uint32_t idx)
{
    ASSERT(dups != NULL);
    ASSERT(item != NULL);
    return item->done[idx];
}

static bool _dups_all_done(DvzTransferDups* dups, DvzTransferDupItem* item)
{
    ASSERT(dups != NULL);
    bool all_done = true;
    ASSERT(item != NULL);
    for (uint32_t i = 0; i < item->tr.br.count; i++)
    {
        all_done &= item->done[i];
    }
    return all_done;
}



static void _append_dup_item(DvzDeq* deq, void* item, void* user_data)
{
    DvzTransferDup* tr = (DvzTransferDup*)item;
    ASSERT(tr != NULL);
    log_trace("process dup task with type %d", tr->type);

    DvzTransfers* transfers = (DvzTransfers*)user_data;
    ASSERT(transfers != NULL);

    DvzTransferDups* dups = &transfers->dups;
    ASSERT(dups != NULL);

    // Append the dequeue TransferDup struct in the DvzTransfers specialized structure that keeps
    // track of all ongoing transfer dups.
    _dups_append(dups, tr);
}



#endif
