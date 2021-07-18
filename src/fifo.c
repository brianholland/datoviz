#include "../include/datoviz/fifo.h"



/*************************************************************************************************/
/*  Thread-safe FIFO queue                                                                       */
/*************************************************************************************************/

DvzFifo dvz_fifo(int32_t capacity)
{
    log_trace("creating generic FIFO queue with a capacity of %d items", capacity);
    ASSERT(capacity >= 2);
    DvzFifo fifo = {0};
    ASSERT(capacity <= DVZ_MAX_FIFO_CAPACITY);
    fifo.capacity = capacity;
    fifo.is_empty = true;
    fifo.items = calloc((uint32_t)capacity, sizeof(void*));

    if (pthread_mutex_init(&fifo.lock, NULL) != 0)
        log_error("mutex creation failed");
    if (pthread_cond_init(&fifo.cond, NULL) != 0)
        log_error("cond creation failed");

    return fifo;
}



static void _fifo_resize(DvzFifo* fifo)
{
    // Old size
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;

    // Old capacity
    int old_cap = fifo->capacity;

    // Resize if queue is full.
    if ((fifo->tail + 1) % fifo->capacity == fifo->head)
    {
        ASSERT(fifo->items != NULL);
        ASSERT(size == fifo->capacity - 1);
        ASSERT(fifo->capacity <= DVZ_MAX_FIFO_CAPACITY);

        fifo->capacity *= 2;
        log_debug("FIFO queue is full, enlarging it to %d", fifo->capacity);
        REALLOC(fifo->items, (uint32_t)fifo->capacity * sizeof(void*));
    }

    if ((fifo->tail + 1) % fifo->capacity == fifo->head)
    {
        // Here, the queue buffer has been resized, but the new space should be used instead of the
        // part of the buffer before the head.

        ASSERT(fifo->tail > 0);
        ASSERT(old_cap < fifo->capacity);
        memcpy(&fifo->items[old_cap], &fifo->items[0], (uint32_t)fifo->tail * sizeof(void*));

        // Move the tail to the new position.
        fifo->tail += old_cap;

        // Check new size.
        ASSERT(fifo->tail - fifo->head > 0);
        ASSERT(fifo->tail - fifo->head == size);
    }
}



void dvz_fifo_enqueue(DvzFifo* fifo, void* item)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    // Resize the FIFO queue if needed.
    _fifo_resize(fifo);

    ASSERT((fifo->tail + 1) % fifo->capacity != fifo->head);
    fifo->items[fifo->tail] = item;
    fifo->tail++;
    if (fifo->tail >= fifo->capacity)
        fifo->tail -= fifo->capacity;
    fifo->is_empty = false;

    ASSERT(0 <= fifo->tail && fifo->tail < fifo->capacity);
    pthread_cond_signal(&fifo->cond);
    pthread_mutex_unlock(&fifo->lock);
}



void dvz_fifo_enqueue_first(DvzFifo* fifo, void* item)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    // Resize the FIFO queue if needed.
    _fifo_resize(fifo);

    ASSERT((fifo->tail + 1) % fifo->capacity != fifo->head);
    fifo->head--;
    if (fifo->head < 0)
        fifo->head += fifo->capacity;
    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);

    fifo->items[fifo->head] = item;
    fifo->is_empty = false;

    ASSERT(0 <= fifo->tail && fifo->tail < fifo->capacity);
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;
    ASSERT(0 <= size && size < fifo->capacity);

    pthread_cond_signal(&fifo->cond);
    pthread_mutex_unlock(&fifo->lock);
}



void* dvz_fifo_dequeue(DvzFifo* fifo, bool wait)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    // Wait until the queue is not empty.
    if (wait)
    {
        log_trace("waiting for the queue to be non-empty");
        while (fifo->tail == fifo->head)
            // NOTE: this call automatically releases the mutex while waiting, and reacquires it
            // afterwards
            pthread_cond_wait(&fifo->cond, &fifo->lock);
    }

    // Empty queue.
    if (fifo->tail == fifo->head)
    {
        // log_trace("FIFO queue was empty");
        // Don't forget to unlock the mutex before exiting this function.
        pthread_mutex_unlock(&fifo->lock);
        fifo->is_empty = true;
        return NULL;
    }

    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);

    // log_trace("dequeue item, tail %d, head %d", fifo->tail, fifo->head);
    void* item = fifo->items[fifo->head];

    fifo->head++;
    if (fifo->head >= fifo->capacity)
        fifo->head -= fifo->capacity;

    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);

    if (fifo->tail == fifo->head)
        fifo->is_empty = true;

    pthread_mutex_unlock(&fifo->lock);
    return item;
}



int dvz_fifo_size(DvzFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);
    // log_debug("tail %d head %d", fifo->tail, fifo->head);
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;
    ASSERT(0 <= size && size <= fifo->capacity);
    pthread_mutex_unlock(&fifo->lock);
    return size;
}



void dvz_fifo_wait(DvzFifo* fifo)
{
    ASSERT(fifo != NULL);
    while (dvz_fifo_size(fifo) > 0)
        dvz_sleep(1);
}



void dvz_fifo_discard(DvzFifo* fifo, int max_size)
{
    ASSERT(fifo != NULL);
    if (max_size == 0)
        return;
    pthread_mutex_lock(&fifo->lock);
    int size = fifo->tail - fifo->head;
    if (size < 0)
        size += fifo->capacity;
    ASSERT(0 <= size && size <= fifo->capacity);
    if (size > max_size)
    {
        log_trace(
            "discarding %d items in the FIFO queue which is getting overloaded", size - max_size);
        fifo->head = fifo->tail - max_size;
        if (fifo->head < 0)
            fifo->head += fifo->capacity;
    }
    pthread_mutex_unlock(&fifo->lock);
}



void dvz_fifo_reset(DvzFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);
    fifo->tail = 0;
    fifo->head = 0;
    pthread_cond_signal(&fifo->cond);
    pthread_mutex_unlock(&fifo->lock);
}



void dvz_fifo_destroy(DvzFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_destroy(&fifo->lock);
    pthread_cond_destroy(&fifo->cond);

    ASSERT(fifo->items != NULL);
    FREE(fifo->items);
}



/*************************************************************************************************/
/*  Dequeue utils                                                                                */
/*************************************************************************************************/

static DvzFifo* _deq_fifo(DvzDeq* deq, uint32_t deq_idx)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);

    DvzFifo* fifo = &deq->queues[deq_idx];
    ASSERT(fifo != NULL);
    ASSERT(fifo->capacity > 0);
    return fifo;
}



// Call all callback functions registered with a deq_idx and type on a deq item.
static void _deq_callbacks(DvzDeq* deq, DvzDeqItem* item)
{
    ASSERT(deq != NULL);
    ASSERT(item->item != NULL);
    DvzDeqCallbackRegister* reg = NULL;
    for (uint32_t i = 0; i < deq->callback_count; i++)
    {
        reg = &deq->callbacks[i];
        ASSERT(reg != NULL);
        if (reg->deq_idx == item->deq_idx && reg->type == item->type)
        {
            reg->callback(deq, item->item, reg->user_data);
        }
    }
}



// Return the total size of the Deq.
static int _deq_size(DvzDeq* deq, uint32_t queue_count, uint32_t* queue_ids)
{
    ASSERT(deq != NULL);
    ASSERT(queue_count > 0);
    ASSERT(queue_ids != NULL);
    int size = 0;
    uint32_t deq_idx = 0;
    for (uint32_t i = 0; i < queue_count; i++)
    {
        deq_idx = queue_ids[i];
        ASSERT(deq_idx < deq->queue_count);
        size += dvz_fifo_size(&deq->queues[deq_idx]);
    }
    return size;
}



static void
_proc_callbacks(DvzDeq* deq, uint32_t proc_idx, DvzDeqProcCallbackPosition pos, DvzDeqItem* item)
{
    ASSERT(deq != NULL);
    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];

    for (uint32_t i = 0; i < proc->callback_count; i++)
    {
        if (proc->callbacks[i].pos == pos)
        {
            ASSERT(proc->callbacks[i].callback != NULL);
            proc->callbacks[i].callback(
                deq, item->deq_idx, item->type, item->item, proc->callbacks[i].user_data);
        }
    }
}



static void _proc_wait_callbacks(DvzDeq* deq, uint32_t proc_idx)
{
    ASSERT(deq != NULL);
    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];

    for (uint32_t i = 0; i < proc->wait_callback_count; i++)
    {
        ASSERT(proc->wait_callbacks[i].callback != NULL);
        proc->wait_callbacks[i].callback(deq, proc->wait_callbacks[i].user_data);
    }
}



static void _proc_batch_callbacks(
    DvzDeq* deq, uint32_t proc_idx, DvzDeqProcBatchPosition pos, uint32_t item_count,
    DvzDeqItem* items)
{
    ASSERT(deq != NULL);
    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];

    for (uint32_t i = 0; i < proc->batch_callback_count; i++)
    {
        ASSERT(proc->batch_callbacks[i].callback != NULL);
        proc->batch_callbacks[i].callback(
            deq, pos, item_count, items, proc->batch_callbacks[i].user_data);
    }
}



// Return 0 if an item was dequeued, or a non-zero integer if the timeout-ed wait was unsuccessful
// (in which case we need to continue waiting).
static int _proc_wait(DvzDeqProc* proc)
{
    ASSERT(proc != NULL);

    if (proc->max_wait == 0)
    {
        // NOTE: this call automatically releases the mutex while waiting, and reacquires it
        // afterwards
        return pthread_cond_wait(&proc->cond, &proc->lock);
    }
    else
    {
        struct timeval now;
        uint32_t wait_s = proc->max_wait / 1000; // in seconds
        //                  ^^ in ms  ^^   ^^^^ convert to s
        uint32_t wait_us = (proc->max_wait - 1000 * wait_s) * 1000; // in us
        //                  ^^ in ms  ^^^    ^^^ in ms ^^^    ^^^ to us
        // Determine until when to wait for the cond.

        gettimeofday(&now, NULL);

        // How many seconds after now?
        proc->wait.tv_sec = now.tv_sec + wait_s;
        // How many nanoseconds after the X seconds?
        proc->wait.tv_nsec = (now.tv_usec + wait_us) * 1000; // from us to ns

        // NOTE: this call automatically releases the mutex while waiting, and reacquires it
        // afterwards
        return pthread_cond_timedwait(&proc->cond, &proc->lock, &proc->wait);
    }
}



/*************************************************************************************************/
/*  Dequeues                                                                                     */
/*************************************************************************************************/

DvzDeq dvz_deq(uint32_t nq)
{
    DvzDeq deq = {0};
    ASSERT(nq <= DVZ_DEQ_MAX_QUEUES);
    deq.queue_count = nq;
    for (uint32_t i = 0; i < nq; i++)
        deq.queues[i] = dvz_fifo(DVZ_MAX_FIFO_CAPACITY);
    return deq;
}



void dvz_deq_callback(
    DvzDeq* deq, uint32_t deq_idx, int type, DvzDeqCallback callback, void* user_data)
{
    ASSERT(deq != NULL);
    ASSERT(callback != NULL);

    DvzDeqCallbackRegister* reg = &deq->callbacks[deq->callback_count++];
    ASSERT(reg != NULL);

    reg->deq_idx = deq_idx;
    reg->type = type;
    reg->callback = callback;
    reg->user_data = user_data;
}



void dvz_deq_proc(DvzDeq* deq, uint32_t proc_idx, uint32_t queue_count, uint32_t* queue_ids)
{
    ASSERT(deq != NULL);
    ASSERT(queue_ids != NULL);

    // HACK: calls to dvz_deq_proc(deq, proc_idx, ...) must be with proc_idx strictly increasing:
    // 0, 1, 2...
    ASSERT(proc_idx == deq->proc_count);

    DvzDeqProc* proc = &deq->procs[deq->proc_count++];
    ASSERT(proc != NULL);

    ASSERT(queue_count <= DVZ_DEQ_MAX_PROC_SIZE);
    proc->queue_count = queue_count;
    // Copy the queue ids to the DvzDeqProc struct.
    for (uint32_t i = 0; i < queue_count; i++)
    {
        ASSERT(queue_ids[i] < deq->queue_count);
        proc->queue_indices[i] = queue_ids[i];

        // Register, for each of the indicated queue, which proc idx is handling it.
        ASSERT(queue_ids[i] < DVZ_DEQ_MAX_QUEUES);
        deq->q_to_proc[queue_ids[i]] = proc_idx;
    }

    // Initialize the thread objects.
    if (pthread_mutex_init(&proc->lock, NULL) != 0)
        log_error("mutex creation failed");
    if (pthread_cond_init(&proc->cond, NULL) != 0)
        log_error("cond creation failed");
    atomic_init(&proc->is_processing, false);
}



void dvz_deq_proc_callback(
    DvzDeq* deq, uint32_t proc_idx, DvzDeqProcCallbackPosition pos, DvzDeqProcCallback callback,
    void* user_data)
{
    ASSERT(deq != NULL);

    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];
    ASSERT(proc != NULL);

    ASSERT(callback != NULL);

    DvzDeqProcCallbackRegister* reg = &proc->callbacks[proc->callback_count++];
    ASSERT(reg != NULL);

    reg->callback = callback;
    reg->pos = pos;
    reg->user_data = user_data;
}



void dvz_deq_proc_wait_delay(DvzDeq* deq, uint32_t proc_idx, uint32_t delay_ms)
{
    ASSERT(deq != NULL);
    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];
    ASSERT(proc != NULL);

    proc->max_wait = delay_ms;
}



void dvz_deq_proc_wait_callback(
    DvzDeq* deq, uint32_t proc_idx, DvzDeqProcWaitCallback callback, void* user_data)
{
    ASSERT(deq != NULL);

    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];
    ASSERT(proc != NULL);

    ASSERT(callback != NULL);

    DvzDeqProcWaitCallbackRegister* reg = &proc->wait_callbacks[proc->wait_callback_count++];
    ASSERT(reg != NULL);

    reg->callback = callback;
    reg->user_data = user_data;
}



void dvz_deq_proc_batch_callback(
    DvzDeq* deq, uint32_t proc_idx, DvzDeqProcBatchPosition pos, DvzDeqProcBatchCallback callback,
    void* user_data)
{
    ASSERT(deq != NULL);

    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];
    ASSERT(proc != NULL);

    ASSERT(callback != NULL);

    DvzDeqProcBatchCallbackRegister* reg = &proc->batch_callbacks[proc->batch_callback_count++];
    ASSERT(reg != NULL);

    reg->callback = callback;
    reg->pos = pos;
    reg->user_data = user_data;
}



static void _deq_enqueue(DvzDeq* deq, uint32_t deq_idx, int type, void* item, bool enqueue_first)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);
    ASSERT(deq_idx < DVZ_DEQ_MAX_QUEUES);

    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    DvzDeqItem* deq_item = calloc(1, sizeof(DvzDeqItem));
    ASSERT(deq_item != NULL);
    deq_item->deq_idx = deq_idx;
    deq_item->type = type;
    deq_item->item = item;

    // Find the proc that processes the specified queue.
    uint32_t proc_idx = deq->q_to_proc[deq_idx];
    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];

    // We signal that proc that an item has been enqueued to one of its queues.
    log_trace("enqueue to queue #%d item type %d", deq_idx, type);
    pthread_mutex_lock(&proc->lock);
    if (!enqueue_first)
        dvz_fifo_enqueue(fifo, deq_item);
    else
        dvz_fifo_enqueue_first(fifo, deq_item);
    log_trace("signal cond of proc #%d", proc_idx);
    pthread_cond_signal(&proc->cond);
    pthread_mutex_unlock(&proc->lock);
}

void dvz_deq_enqueue(DvzDeq* deq, uint32_t deq_idx, int type, void* item)
{
    _deq_enqueue(deq, deq_idx, type, item, false);
}

void dvz_deq_enqueue_first(DvzDeq* deq, uint32_t deq_idx, int type, void* item)
{
    _deq_enqueue(deq, deq_idx, type, item, true);
}



void dvz_deq_discard(DvzDeq* deq, uint32_t deq_idx, int max_size)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    dvz_fifo_discard(fifo, max_size);
}



DvzDeqItem dvz_deq_peek_first(DvzDeq* deq, uint32_t deq_idx)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    return *((DvzDeqItem*)(fifo->items[fifo->head]));
}



DvzDeqItem dvz_deq_peek_last(DvzDeq* deq, uint32_t deq_idx)
{
    ASSERT(deq != NULL);
    ASSERT(deq_idx < deq->queue_count);
    DvzFifo* fifo = _deq_fifo(deq, deq_idx);
    int32_t last = fifo->tail - 1;
    if (last < 0)
        last += fifo->capacity;
    ASSERT(0 <= last && last < fifo->capacity);
    return *((DvzDeqItem*)(fifo->items[last]));
}



void dvz_deq_strategy(DvzDeq* deq, uint32_t proc_idx, DvzDeqStrategy strategy)
{
    ASSERT(deq != NULL);
    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];
    ASSERT(proc != NULL);
    proc->strategy = strategy;
}



DvzDeqItem dvz_deq_dequeue(DvzDeq* deq, uint32_t proc_idx, bool wait)
{
    ASSERT(deq != NULL);

    DvzFifo* fifo = NULL;
    DvzDeqItem* deq_item = NULL;
    DvzDeqItem item_s = {0};

    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];

    pthread_mutex_lock(&proc->lock);

    // Wait until the queue is not empty.
    if (wait)
    {
        log_trace("waiting for one of the queues in proc #%d to be non-empty", proc_idx);
        while (_deq_size(deq, proc->queue_count, proc->queue_indices) == 0)
        {
            log_trace("waiting for proc #%d cond", proc_idx);
            if (_proc_wait(proc) != 0)
            {
                // If the timeout-ed wait was unsuccessful, we will continue waiting at the next
                // iteration. But before that, we call the proc wait callbacks.
                _proc_wait_callbacks(deq, proc_idx);
            }
            else
            {
                log_trace("proc #%d cond signaled!", proc_idx);
                break; // NOTE: this is not necessary, because if the cond is signaled, it means
                       // the queue is non empty, and the while() condition at the very next
                       // iteration will fail, thereby ending the while loop.
            }
        }
        log_trace("proc #%d has an item", proc_idx);
    }

    // Here, we know there is at least one item to dequeue because one of the queues is non-empty.

    // Go through the passed queue indices.
    uint32_t deq_idx = 0;
    for (uint32_t i = 0; i < proc->queue_count; i++)
    {
        // This is the ID of the queue.
        // NOTE: process the queues circularly so that all queues successively get a chance to be
        // dequeued, even if another queue is getting filled more quickly.
        deq_idx = proc->queue_indices[(i + proc->queue_offset) % proc->queue_count];
        ASSERT(deq_idx < deq->queue_count);

        // Get that FIFO queue.
        fifo = _deq_fifo(deq, deq_idx);

        // Dequeue it immediately, return NULL if the queue was empty.
        deq_item = dvz_fifo_dequeue(fifo, false);
        if (deq_item != NULL)
        {
            // Make a copy of the struct.
            item_s = *deq_item;
            // Consistency check.
            ASSERT(deq_idx == item_s.deq_idx);
            log_trace("dequeue item from FIFO queue #%d with type %d", deq_idx, item_s.type);
            FREE(deq_item);
            break;
        }
        log_trace("queue #%d was empty", deq_idx);
    }
    // IMPORTANT: we must unlock BEFORE calling the callbacks if we want to permit callbacks to
    // enqueue new tasks.
    pthread_mutex_unlock(&proc->lock);

    // First, call the generic Proc pre callbacks.
    _proc_callbacks(deq, proc_idx, DVZ_DEQ_PROC_CALLBACK_PRE, &item_s);

    // Then, call the typed callbacks.
    if (item_s.item != NULL)
    {
        atomic_store(&proc->is_processing, true);
        _deq_callbacks(deq, &item_s);
    }

    // Finally, call the generic Proc post callbacks.
    _proc_callbacks(deq, proc_idx, DVZ_DEQ_PROC_CALLBACK_POST, &item_s);

    atomic_store(&proc->is_processing, false);

    // Implement the dequeue strategy here. If queue_offset remains at 0, the dequeue will first
    // empty the first queue, then move to the second queue, etc. Otherwise, all queues will be
    // handled one after the other.
    if (proc->strategy == DVZ_DEQ_STRATEGY_BREADTH_FIRST)
        proc->queue_offset = (proc->queue_offset + 1) % proc->queue_count;

    return item_s;
}



void dvz_deq_dequeue_batch(DvzDeq* deq, uint32_t proc_idx)
{
    ASSERT(deq != NULL);

    DvzFifo* fifo = NULL;
    DvzDeqItem* deq_item = NULL;
    DvzDeqItem item_s = {0};

    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];

    pthread_mutex_lock(&proc->lock);

    // Find the number of items that should be dequeued now.
    int size = _deq_size(deq, proc->queue_count, proc->queue_indices);
    ASSERT(size >= 0);
    uint32_t item_count = (uint32_t)size;
    DvzDeqItem* items = NULL;
    // Allocate the memory for the items to be dequeued.
    if (item_count > 0)
        items = calloc(item_count, sizeof(DvzDeqItem));
    uint32_t k = 0;

    // Call the BEGIN batch callbacks.
    atomic_store(&proc->is_processing, true);
    // NOTE: we cannot pass the items array at BEGIN because we haven't dequeued the items yet.
    _proc_batch_callbacks(deq, proc_idx, DVZ_DEQ_PROC_BATCH_BEGIN, item_count, NULL);
    atomic_store(&proc->is_processing, false);

    // Go through the queue indices.
    uint32_t deq_idx = 0;
    for (uint32_t i = 0; i < proc->queue_count; i++)
    {
        // This is the ID of the queue.
        deq_idx = proc->queue_indices[i];
        ASSERT(deq_idx < deq->queue_count);

        // Get that FIFO queue.
        fifo = _deq_fifo(deq, deq_idx);

        // Dequeue it immediately, return NULL if the queue was empty.
        deq_item = dvz_fifo_dequeue(fifo, false);
        if (deq_item != NULL)
        {
            // Make a copy of the struct.
            item_s = *deq_item;
            // Consistency check.
            ASSERT(deq_idx == item_s.deq_idx);
            log_trace("dequeue item from FIFO queue #%d with type %d", deq_idx, item_s.type);
            FREE(deq_item);
            // Copy the item into the array allocated above.
            items[k++] = item_s;
        }
        log_trace("queue #%d was empty", deq_idx);
    }
    ASSERT(k == item_count);

    // IMPORTANT: we must unlock BEFORE calling the callbacks if we want to permit callbacks to
    // enqueue new tasks.
    pthread_mutex_unlock(&proc->lock);

    // Call the typed callbacks.
    atomic_store(&proc->is_processing, true);
    for (uint32_t i = 0; i < item_count; i++)
    {
        if (items[i].item != NULL)
        {
            _deq_callbacks(deq, &items[i]);
        }
    }

    // Call the END batch callbacks.
    _proc_batch_callbacks(deq, proc_idx, DVZ_DEQ_PROC_BATCH_END, item_count, items);

    atomic_store(&proc->is_processing, false);
}



void dvz_deq_wait(DvzDeq* deq, uint32_t proc_idx)
{
    ASSERT(deq != NULL);

    ASSERT(proc_idx < deq->proc_count);
    DvzDeqProc* proc = &deq->procs[proc_idx];
    log_trace("start waiting for proc #%d", proc_idx);

    while (_deq_size(deq, proc->queue_count, proc->queue_indices) > 0 ||
           atomic_load(&proc->is_processing))
    {
        dvz_sleep(1);
    }
    log_trace("finished waiting for empty queues");
}



void dvz_deq_destroy(DvzDeq* deq)
{
    ASSERT(deq != NULL);

    for (uint32_t i = 0; i < deq->queue_count; i++)
        dvz_fifo_destroy(&deq->queues[i]);

    for (uint32_t i = 0; i < deq->proc_count; i++)
    {
        pthread_mutex_destroy(&deq->procs[i].lock);
        pthread_cond_destroy(&deq->procs[i].cond);
    }
}
