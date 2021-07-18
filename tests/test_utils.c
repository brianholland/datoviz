#include "../include/datoviz/array.h"
#include "../include/datoviz/common.h"
// #include "../include/datoviz/transforms.h"
#include "../src/ticks.h"
// #include "../src/transforms_utils.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestObject TestObject;
typedef struct TestDtype TestDtype;
typedef struct _mvp _mvp;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestObject
{
    DvzObject obj;
    float x;
};



struct TestDtype
{
    uint8_t a;
    float b;
};



struct _mvp
{
    mat4 model;
    mat4 view;
    mat4 proj;
};



/*************************************************************************************************/
/*  Common tests                                                                                 */
/*************************************************************************************************/

int test_utils_container(TestContext* tc)
{
    uint32_t capacity = 2;

    DvzContainer container = dvz_container(capacity, sizeof(TestObject), 0);
    AT(container.items != NULL);
    AT(container.item_size == sizeof(TestObject));
    AT(container.capacity == capacity);
    AT(container.count == 0);

    // Allocate one object.
    TestObject* a = dvz_container_alloc(&container);
    AT(a != NULL);
    a->x = 1;
    dvz_obj_created(&a->obj);
    AT(container.items[0] != NULL);
    AT(container.items[0] == a);
    AT(container.items[1] == NULL);
    AT(container.capacity == capacity);
    AT(container.count == 1);

    // Allocate another one.
    TestObject* b = dvz_container_alloc(&container);
    AT(b != NULL);
    b->x = 2;
    dvz_obj_created(&b->obj);
    AT(container.items[1] != NULL);
    AT(container.items[1] == b);
    AT(container.capacity == capacity);
    AT(container.count == 2);

    // Destroy the first object.
    dvz_obj_destroyed(&a->obj);

    // Allocate another one.
    TestObject* c = dvz_container_alloc(&container);
    AT(c != NULL);
    c->x = 3;
    dvz_obj_created(&c->obj);
    AT(container.items[0] != NULL);
    AT(container.items[0] == c);
    AT(container.capacity == capacity);
    AT(container.count == 2);

    // Allocate another one.
    // Container will be reallocated.
    TestObject* d = dvz_container_alloc(&container);
    AT(d != NULL);
    d->x = 4;
    dvz_obj_created(&d->obj);
    AT(container.capacity == 4);
    AT(container.count == 3);
    AT(container.items[2] != NULL);
    AT(container.items[2] == d);
    AT(container.items[3] == NULL);

    for (uint32_t k = 0; k < 10; k++)
    {
        DvzContainerIterator iter = dvz_container_iterator(&container);
        uint32_t i = 0;
        TestObject* obj = NULL;
        // Iterate through items.
        while (iter.item != NULL)
        {
            obj = iter.item;
            AT(obj != NULL);
            if (i == 0)
                AT(obj->x == 3);
            if (i == 1)
                AT(obj->x == 2);
            if (i == 2)
                AT(obj->x == 4);
            i++;
            dvz_container_iter(&iter);
        }
        ASSERT(i == 3);
    }

    // Destroy all objects.
    dvz_obj_destroyed(&b->obj);
    dvz_obj_destroyed(&c->obj);
    dvz_obj_destroyed(&d->obj);

    // Free all memory. This function will fail if there is at least one object not destroyed.
    dvz_container_destroy(&container);
    return 0;
}



static void* _thread_callback(void* user_data)
{
    ASSERT(user_data != NULL);
    dvz_sleep(10);
    *((int*)user_data) = 42;
    log_debug("from thread");
    return NULL;
}

int test_utils_thread(TestContext* tc)
{
    int data = 0;
    DvzThread thread = dvz_thread(_thread_callback, &data);
    AT(data == 0);
    dvz_thread_join(&thread);
    AT(data == 42);
    return 0;
}



/*************************************************************************************************/
/*  Array tests                                                                                  */
/*************************************************************************************************/

int test_utils_array_1(TestContext* tc)
{
    uint8_t values[] = {1, 2, 3, 4, 5, 6};

    DvzArray arr = dvz_array(6, DVZ_DTYPE_CHAR);

    // 1:6
    dvz_array_data(&arr, 0, 6, 6, values);
    AT(memcmp(arr.data, values, sizeof(values)) == 0);

    // Partial copy of data.
    dvz_array_data(&arr, 2, 3, 3, values); // put [1, 2, 3] in arr[2:5]
    AT(memcmp(arr.data, (uint8_t[]){1, 2, 1, 2, 3, 6}, sizeof(values)) == 0);

    dvz_array_destroy(&arr);
    return 0;
}



int test_utils_array_2(TestContext* tc)
{
    float values[] = {1, 2, 3, 4, 5, 6};

    DvzArray arr = dvz_array(6, DVZ_DTYPE_FLOAT);

    // 1:6
    dvz_array_data(&arr, 0, 6, 6, values);
    AT(memcmp(arr.data, values, sizeof(values)) == 0);

    // Partial copy of data.
    dvz_array_data(&arr, 2, 3, 3, values); // put [1, 2, 3] in arr[2:5]
    AT(memcmp(arr.data, (float[]){1, 2, 1, 2, 3, 6}, sizeof(values)) == 0);

    // Resize with less elements. Should not lose the original data.
    dvz_array_resize(&arr, 3);
    AT(memcmp(arr.data, (float[]){1, 2, 1}, 3 * sizeof(float)) == 0);

    // Resize with more elements.
    dvz_array_resize(&arr, 9);
    // Resizing should repeat the last element.
    AT(memcmp(arr.data, (float[]){1, 2, 1, 2, 3, 6, 6, 6, 6}, 9 * sizeof(float)) == 0);

    // Fill the array with a constant value.
    float value = 12.0f;
    ASSERT(arr.item_count == 9);
    dvz_array_data(&arr, 0, 9, 1, &value);
    for (uint32_t i = 0; i < 9; i++)
    {
        AT(((float*)arr.data)[i] == value);
    }

    dvz_array_destroy(&arr);
    return 0;
}



int test_utils_array_3(TestContext* tc)
{
    // uint8, float32
    DvzArray arr = dvz_array_struct(3, sizeof(TestDtype));

    TestDtype value = {1, 2};
    dvz_array_data(&arr, 0, 3, 1, &value);
    for (uint32_t i = 0; i < 3; i++)
    {
        AT(((TestDtype*)(dvz_array_item(&arr, i)))->a == 1);
        AT(((TestDtype*)(dvz_array_item(&arr, i)))->b == 2);
    }

    // Copy data to the second column.
    float b = 20.0f;
    dvz_array_column(
        &arr, offsetof(TestDtype, b), sizeof(float), 1, 2, 1, &b, 0, 0, DVZ_ARRAY_COPY_SINGLE, 1);

    // Row #0.
    AT(((TestDtype*)(dvz_array_item(&arr, 0)))->a == 1);
    AT(((TestDtype*)(dvz_array_item(&arr, 0)))->b == 2);

    // Row #1.
    AT(((TestDtype*)(dvz_array_item(&arr, 1)))->a == 1);
    AT(((TestDtype*)(dvz_array_item(&arr, 1)))->b == 20);

    // Row #2.
    AT(((TestDtype*)(dvz_array_item(&arr, 2)))->a == 1);
    AT(((TestDtype*)(dvz_array_item(&arr, 2)))->b == 20);

    // Resize.
    dvz_array_resize(&arr, 4);
    // Row #3
    AT(((TestDtype*)(dvz_array_item(&arr, 3)))->a == 1);
    AT(((TestDtype*)(dvz_array_item(&arr, 3)))->b == 20);

    dvz_array_destroy(&arr);
    return 0;
}



int test_utils_array_4(TestContext* tc)
{
    // uint8, float32
    DvzArray arr = dvz_array_struct(4, sizeof(TestDtype));
    TestDtype* item = NULL;
    float b[] = {0.5f, 2.5f};

    // Test single copy
    {
        dvz_array_column(
            &arr, offsetof(TestDtype, b), sizeof(float), 0, 4, 2, &b, 0, 0, DVZ_ARRAY_COPY_SINGLE,
            2);

        for (uint32_t i = 0; i < 4; i++)
        {
            item = dvz_array_item(&arr, i);
            AT(item->b == (i % 2 == 0 ? i + .5f : 0));
        }
    }

    dvz_array_clear(&arr);

    // Test repeat copy
    {
        dvz_array_column(
            &arr, offsetof(TestDtype, b), sizeof(float), 0, 4, 2, &b, 0, 0, DVZ_ARRAY_COPY_REPEAT,
            2);

        for (uint32_t i = 0; i < 4; i++)
        {
            item = dvz_array_item(&arr, i);
            AT(item->b == (i % 2 == 0 ? (i + .5f) : (i - .5f)));
        }
    }

    dvz_array_destroy(&arr);
    return 0;
}



int test_utils_array_5(TestContext* tc)
{
    uint8_t values[] = {1, 2, 3, 4, 5, 6};

    DvzArray arr = dvz_array(3, DVZ_DTYPE_CHAR);

    // Check automatic resize when setting larger data array.
    AT(arr.item_count == 3);
    dvz_array_data(&arr, 0, 6, 6, values);
    AT(arr.item_count == 6);
    AT(memcmp(arr.data, values, sizeof(values)) == 0);

    dvz_array_destroy(&arr);
    return 0;
}



int test_utils_array_6(TestContext* tc)
{
    int32_t values[] = {1, 2, 3, 4, 5, 6};

    DvzArray arr = dvz_array(3, DVZ_DTYPE_INT);
    dvz_array_data(&arr, 0, 6, 6, values);

    // Insert 3 values in the array.
    // {1, 2, 10, 11, 12, 3, 4, 5, 6}

    // Insertion index is the index of the first inserted element in the new, increased array.
    dvz_array_insert(&arr, 2, 3, (int32_t[]){10, 11, 12});

    // Check.
    int32_t* arr_values = (int32_t*)arr.data;
    AT(arr_values[0] == 1);
    AT(arr_values[1] == 2);
    AT(arr_values[2] == 10);
    AT(arr_values[3] == 11);
    AT(arr_values[4] == 12);
    AT(arr_values[5] == 3);
    AT(arr_values[6] == 4);
    AT(arr_values[7] == 5);
    AT(arr_values[8] == 6);

    dvz_array_destroy(&arr);
    return 0;
}



int test_utils_array_7(TestContext* tc)
{
    int32_t values[] = {0, 1, 2, 3, 4, 5};

    DvzArray arr = dvz_array(6, DVZ_DTYPE_INT);
    dvz_array_data(&arr, 0, 6, 6, values);

    DvzArray arr2 = dvz_array(10, DVZ_DTYPE_INT);

    dvz_array_copy_region(&arr, &arr2, 4, 8, 2);
    int32_t* a = NULL;
    for (uint32_t i = 0; i < 8; i++)
    {
        a = dvz_array_item(&arr2, i);
        AT(*a == 0);
    }
    a = dvz_array_item(&arr2, 8);
    AT(*a == 4);
    a = dvz_array_item(&arr2, 9);
    AT(*a == 5);

    dvz_array_destroy(&arr);
    return 0;
}



int test_utils_array_cast(TestContext* tc)
{
    // uint8, float32
    DvzArray arr = dvz_array_struct(4, sizeof(TestDtype));
    TestDtype* item = NULL;
    double b[] = {0.5, 2.5};

    dvz_array_column(
        &arr, offsetof(TestDtype, b), sizeof(double), 0, 4, 2, &b, DVZ_DTYPE_DOUBLE,
        DVZ_DTYPE_FLOAT, DVZ_ARRAY_COPY_SINGLE, 2);

    for (uint32_t i = 0; i < 4; i++)
    {
        item = dvz_array_item(&arr, i);
        AT(item->b == (i % 2 == 0 ? i + .5 : 0));
    }

    dvz_array_destroy(&arr);
    return 0;
}



int test_utils_array_mvp(TestContext* tc)
{
    DvzArray arr = dvz_array_struct(1, sizeof(_mvp));

    _mvp id = {0};
    glm_mat4_identity(id.model);
    glm_mat4_identity(id.view);
    glm_mat4_identity(id.proj);

    dvz_array_column(
        &arr, offsetof(_mvp, model), sizeof(mat4), 0, 1, 1, id.model, 0, 0, DVZ_ARRAY_COPY_SINGLE,
        1);

    dvz_array_column(
        &arr, offsetof(_mvp, view), sizeof(mat4), 0, 1, 1, id.view, 0, 0, DVZ_ARRAY_COPY_SINGLE,
        1);

    dvz_array_column(
        &arr, offsetof(_mvp, proj), sizeof(mat4), 0, 1, 1, id.proj, 0, 0, DVZ_ARRAY_COPY_SINGLE,
        1);

    _mvp* mvp = dvz_array_item(&arr, 0);
    for (uint32_t i = 0; i < 4; i++)
    {
        for (uint32_t j = 0; j < 4; j++)
        {
            AT(mvp->model[i][j] == (i == j ? 1 : 0));
            AT(mvp->view[i][j] == (i == j ? 1 : 0));
            AT(mvp->proj[i][j] == (i == j ? 1 : 0));
        }
    }

    dvz_array_destroy(&arr);
    return 0;
}



int test_utils_array_3D(TestContext* tc)
{
    DvzArray arr = dvz_array_3D(2, 2, 3, 1, DVZ_DTYPE_CHAR);

    uint8_t value = 12;
    dvz_array_data(&arr, 0, 6, 1, &value);

    void* data = arr.data;
    VkDeviceSize size = arr.buffer_size;
    uint8_t* item = dvz_array_item(&arr, 5);
    AT(*item == value);

    // Reshaping deletes the data.
    dvz_array_reshape(&arr, 3, 2, 1);
    item = dvz_array_item(&arr, 5);
    AT(*item == 0);
    // If the total size is the same, the data pointer doesn't change.
    AT(arr.data == data);
    AT(arr.buffer_size == size);

    // Reshaping deletes the data.
    dvz_array_reshape(&arr, 4, 3, 1);
    item = dvz_array_item(&arr, 5);
    AT(*item == 0);
    // If the total size is different, the data buffer is reallocated.
    AT(arr.buffer_size != size);

    dvz_array_destroy(&arr);
    return 0;
}



/*************************************************************************************************/
/* Transform tests                                                                               */
/*************************************************************************************************/

// int test_utils_transforms_1(TestContext* tc)
// {
//     const uint32_t n = 100000;
//     const double eps = 1e-3;

//     // Compute the data bounds of an array of dvec3.
//     DvzArray pos_in = dvz_array(n, DVZ_DTYPE_DOUBLE);
//     double* positions = (double*)pos_in.data;
//     for (uint32_t i = 0; i < n; i++)
//         positions[i] = -5 + 10 * dvz_rand_float();

//     DvzBox box = _box_bounding(&pos_in);
//     AT(fabs(box.p0[0] + 5) < eps);
//     AT(fabs(box.p1[0] - 5) < eps);

//     box = _box_cube(box);
//     // AT(fabs(box.p0[0] + 2.5) < eps);
//     // AT(fabs(box.p1[0] - 7.5) < eps);
//     // AT(fabs(box.p0[1] - 3.5) < eps);
//     // AT(fabs(box.p1[1] - 13.5) < eps);
//     AT(fabs(box.p0[0] + 5) < eps);
//     AT(fabs(box.p1[0] - 5) < eps);


//     // // Normalize the data.
//     // DvzArray pos_out = dvz_array(n, DVZ_DTYPE_DOUBLE);
//     // _transform_linear(box, &pos_in, DVZ_BOX_NDC, &pos_out);
//     // positions = (double*)pos_out.data;
//     // double* pos = NULL;
//     // double v = 0;
//     // for (uint32_t i = 0; i < n; i++)
//     // {
//     //     pos = dvz_array_item(&pos_out, i);
//     //     //     v = (*pos)[0];
//     //     //     AT(-1 <= v && v <= +1);
//     //     //     v = (*pos)[1];
//     //     //     AT(-1 <= v && v <= +1);
//     //     v = (*pos);
//     //     AT(-1 - eps <= v && v <= +1 + eps);
//     // }

//     dvz_array_destroy(&pos_in);
//     // dvz_array_destroy(&pos_out);

//     return 0;
// }



// int test_utils_transforms_2(TestContext* tc)
// {
//     DvzBox box0 = {{0, 0, -1}, {10, 10, 1}};
//     DvzTransform tr = _transform_interp(box0, DVZ_BOX_NDC);

//     {
//         dvec3 in = {0, 0, -1};
//         dvec3 out = {0};
//         _transform_apply(&tr, in, out);
//         for (uint32_t i = 0; i < 3; i++)
//             AT(out[i] == -1);
//     }

//     {
//         dvec3 in = {10, 10, 1};
//         dvec3 out = {0};
//         _transform_apply(&tr, in, out);
//         for (uint32_t i = 0; i < 3; i++)
//             AT(out[i] == +1);
//     }

//     tr = _transform_inv(&tr);

//     {
//         dvec3 in = {-1, -1, -1};
//         dvec3 out = {0};
//         _transform_apply(&tr, in, out);
//         AT(out[0] == 0);
//         AT(out[1] == 0);
//         AT(out[2] == -1);
//     }

//     {
//         dvec3 in = {1, 1, 1};
//         dvec3 out = {0};
//         _transform_apply(&tr, in, out);
//         AT(out[0] == 10);
//         AT(out[1] == 10);
//         AT(out[2] == 1);
//     }

//     return 0;
// }



// int test_utils_transforms_3(TestContext* tc)
// {
//     DvzMVP mvp = {0};

//     glm_mat4_identity(mvp.model);
//     glm_mat4_identity(mvp.view);
//     glm_mat4_identity(mvp.proj);

//     glm_rotate(mvp.model, M_PI, (vec3){0, 1, 0});

//     DvzTransform tr = _transform_mvp(&mvp);

//     dvec3 in = {0.5, 0.5, 0};
//     dvec3 out = {0};
//     _transform_apply(&tr, in, out);
//     AC(out[0], -.5, EPS);
//     AC(out[1], -.5, EPS);
//     AC(out[2], +.5, EPS);

//     return 0;
// }



// int test_utils_transforms_4(TestContext* tc)
// {
//     DvzBox box0 = {{0, 0, -1}, {10, 10, 1}};
//     DvzBox box1 = {{0, 0, 0}, {1, 2, 3}};
//     DvzTransform tr = _transform_interp(box0, DVZ_BOX_NDC);

//     DvzTransformChain tch = _transforms();
//     _transforms_append(&tch, tr);
//     tr = _transform_interp(DVZ_BOX_NDC, box1);
//     _transforms_append(&tch, tr);

//     {
//         dvec3 in = {10, 10, 1}, out = {0};
//         _transforms_apply(&tch, in, out);
//         AT(out[0] == 1);
//         AT(out[1] == 2);
//         AT(out[2] == 3);
//     }

//     {
//         dvec3 in = {1, 2, 3}, out = {0};
//         tch = _transforms_inv(&tch);
//         _transforms_apply(&tch, in, out);
//         AT(out[0] == 10);
//         AT(out[1] == 10);
//         AT(out[2] == 1);
//     }

//     return 0;
// }



// DO NOT UNCOMMENT:

// int test_utils_transforms_5(TestContext* tc)
// {
//     DvzApp* app = dvz_app(DVZ_BACKEND_GLFW);
//     DvzGpu* gpu = dvz_gpu_best(app);
//     DvzCanvas* canvas = dvz_canvas(gpu, TEST_WIDTH, TEST_HEIGHT, 0);
//     DvzGrid grid = dvz_grid(canvas, 2, 4);
//     DvzPanel* panel = dvz_panel(&grid, 1, 2);

//     dvz_app_run(app, 3);

//     panel->data_coords.box.p0[0] = 0;
//     panel->data_coords.box.p0[1] = 0;
//     panel->data_coords.box.p0[2] = -1;

//     panel->data_coords.box.p1[0] = 10;
//     panel->data_coords.box.p1[1] = 20;
//     panel->data_coords.box.p1[2] = 1;

//     dvec3 in, out;
//     in[0] = 5;
//     in[1] = 10;
//     in[2] = 0;

//     dvz_transform(panel, DVZ_CDS_DATA, in, DVZ_CDS_SCENE, out);
//     AC(out[0], 0, EPS);
//     AC(out[1], 0, EPS);
//     AC(out[2], 0, EPS);

//     dvz_transform(panel, DVZ_CDS_DATA, in, DVZ_CDS_VULKAN, out);
//     AC(out[0], 0, EPS);
//     AC(out[1], 0, EPS);
//     AC(out[2], 0.5, EPS);

//     in[0] = 0;
//     in[1] = 20;
//     in[2] = 0;
//     uvec2 size = {0};

//     dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_FRAMEBUFFER, size);
//     dvz_transform(panel, DVZ_CDS_DATA, in, DVZ_CDS_FRAMEBUFFER, out);
//     AC(out[0], size[0] / 2., EPS);
//     AC(out[1], size[1] / 2., EPS);
//     AC(out[2], 0.5, EPS);

//     dvz_canvas_size(canvas, DVZ_CANVAS_SIZE_SCREEN, size);
//     dvz_transform(panel, DVZ_CDS_DATA, in, DVZ_CDS_WINDOW, out);
//     AC(out[0], size[0] / 2., EPS);
//     AC(out[1], size[1] / 2., EPS);
//     AC(out[2], 0.5, EPS);

//     TEST_END
// }



/*************************************************************************************************/
/* Colormap tests                                                                                */
/*************************************************************************************************/

int test_utils_colormap_idx(TestContext* tc)
{
    DvzColormap cmap = DVZ_CMAP_BLUES;
    uint8_t value = 128;
    cvec2 ij = {0};

    dvz_colormap_idx(cmap, value, ij);
    AT(ij[0] == (int)cmap);
    AT(ij[1] == value);

    return 0;
}



int test_utils_colormap_uv(TestContext* tc)
{
    DvzColormap cmap = DVZ_CMAP_BLUES;
    DvzColormap cpal32 = DVZ_CPAL032_PAIRED;
    DvzColormap cpal = DVZ_CPAL256_GLASBEY;
    uint8_t value = 128;
    vec2 uv = {0};
    float eps = .01;

    dvz_colormap_uv(cmap, value, uv);
    AC(uv[0], .5, .05);
    AC(uv[1], (int)cmap / 256.0, .05);

    dvz_colormap_uv(cpal, value, uv);
    AC(uv[0], .5, .05);
    AC(uv[1], (int)cpal / 256.0, .05);

    dvz_colormap_uv(cpal32, value, uv);
    AC(uv[0], .7520, eps);
    AC(uv[1], .9395, eps);

    return 0;
}



int test_utils_colormap_extent(TestContext* tc)
{
    DvzColormap cmap = DVZ_CMAP_BLUES;
    DvzColormap cpal32 = DVZ_CPAL032_PAIRED;
    DvzColormap cpal = DVZ_CPAL256_GLASBEY;
    vec4 uvuv = {0};
    float eps = .01;

    dvz_colormap_extent(cmap, uvuv);
    AC(uvuv[0], 0, eps);
    AC(uvuv[2], 1, eps);
    AC(uvuv[1], .029, eps);
    AC(uvuv[3], .029, eps);

    dvz_colormap_extent(cpal, uvuv);
    AC(uvuv[0], 0, eps);
    AC(uvuv[2], 1, eps);
    AC(uvuv[1], .69, eps);
    AC(uvuv[3], .69, eps);

    dvz_colormap_extent(cpal32, uvuv);
    AC(uvuv[0], .25, eps);
    AC(uvuv[2], .37, eps);
    AC(uvuv[1], .94, eps);
    AC(uvuv[3], .94, eps);

    return 0;
}



int test_utils_colormap_default(TestContext* tc)
{
    DvzColormap cmap = DVZ_CMAP_HSV;
    cvec4 color = {0};
    cvec4 expected = {0, 0, 0, 255};

    dvz_colormap(cmap, 0, color);
    expected[0] = 255;
    AEn(4, color, expected);

    dvz_colormap(cmap, 128, color);
    expected[0] = 0;
    expected[1] = 255;
    expected[2] = 245;
    AEn(4, color, expected);

    dvz_colormap(cmap, 255, color);
    expected[0] = 255;
    expected[1] = 0;
    expected[2] = 23;
    AEn(4, color, expected);

    return 0;
}



int test_utils_colormap_scale(TestContext* tc)
{
    DvzColormap cmap = DVZ_CMAP_HSV;
    cvec4 color = {0};
    cvec4 expected = {0, 0, 0, 255};
    float vmin = -1;
    float vmax = +1;

    dvz_colormap_scale(cmap, -1, vmin, vmax, color);
    expected[0] = 255;
    expected[1] = 0;
    expected[2] = 0;
    AEn(4, color, expected);

    dvz_colormap_scale(cmap, 0, vmin, vmax, color);
    expected[0] = 0;
    expected[1] = 255;
    expected[2] = 245;
    AEn(4, color, expected);

    dvz_colormap_scale(cmap, 1, vmin, vmax, color);
    expected[0] = 255;
    expected[1] = 0;
    expected[2] = 23;
    AEn(4, color, expected);

    return 0;
}



int test_utils_colormap_packuv(TestContext* tc)
{
    vec2 uv = {0};

    dvz_colormap_packuv((cvec3){10, 20, 30}, uv);
    AT(uv[1] == -1);
    AT(uv[0] == 10 + 256 * 20 + 256 * 256 * 30);

    return 0;
}



int test_utils_colormap_array(TestContext* tc)
{
    DvzColormap cmap = DVZ_CMAP_BLUES;
    double vmin = -1;
    double vmax = +1;
    cvec4 color = {0};

    uint32_t count = 100;
    double* values = calloc(count, sizeof(double));
    for (uint32_t i = 0; i < count; i++)
        values[i] = -1.0 + 2.0 * i / (double)(count - 1);

    cvec4* colors = calloc(count, sizeof(cvec4));
    dvz_colormap_array(cmap, count, values, vmin, vmax, colors);
    for (uint32_t i = 0; i < count; i++)
    {
        dvz_colormap_scale(cmap, values[i], vmin, vmax, color);
        AEn(4, color, colors[i])
    }

    FREE(values);
    FREE(colors);

    return 0;
}



/*************************************************************************************************/
/* Tick tests                                                                                    */
/*************************************************************************************************/

int test_utils_ticks_1(TestContext* context)
{
    DvzAxesContext ctx = {0};
    ctx.coord = DVZ_AXES_COORD_X;
    ctx.size_viewport = 1000;
    ctx.size_glyph = 10;

    DvzAxesTicks ticks = create_ticks(0, 1, 11, ctx);
    ticks.lmin_in = 0;
    ticks.lmax_in = 1;
    ticks.lstep = .1;
    const uint32_t N = tick_count(ticks.lmin_in, ticks.lmax_in, ticks.lstep);
    ticks.value_count = N;

    ticks.labels = calloc(N * MAX_GLYPHS_PER_TICK, sizeof(char));
    ticks.precision = 3;

    for (bool scientific = false; scientific; scientific = true)
    {
        ticks.format = scientific ? DVZ_TICK_FORMAT_SCIENTIFIC : DVZ_TICK_FORMAT_DECIMAL;
        make_labels(&ticks, &ctx, false);
        char* s = NULL;
        for (uint32_t i = 0; i < N; i++)
        {
            s = &ticks.labels[i * MAX_GLYPHS_PER_TICK];
            if (strlen(s) == 0)
                break;
            log_debug("%s ", s);
            if (i > 0)
            {
                if (scientific)
                {
                    AT(strchr(s, 'e') != NULL);
                }
                else
                {
                    AT(strchr(s, 'e') == NULL);
                }
            }
        }
    }

    FREE(ticks.labels);
    return 0;
}



int test_utils_ticks_2(TestContext* context)
{
    DvzAxesContext ctx = {0};
    ctx.coord = DVZ_AXES_COORD_X;
    ctx.size_viewport = 5000;
    ctx.size_glyph = 5;
    ctx.extensions = 0;

    double x = 1.23456;
    DvzAxesTicks ticks = dvz_ticks(x, x + 1e-8, ctx);
    AT(ticks.format == DVZ_TICK_FORMAT_DECIMAL);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);
    return 0;
}



int test_utils_ticks_duplicate(TestContext* context)
{
    DvzAxesContext ctx = {0};
    ctx.coord = DVZ_AXES_COORD_X;
    ctx.size_viewport = 2000;
    ctx.size_glyph = 5;

    DvzAxesTicks ticks = dvz_ticks(-10.12, 20.34, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);
    AT(!duplicate_labels(&ticks, &ctx));

    ticks = dvz_ticks(.001, .002, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);
    AT(!duplicate_labels(&ticks, &ctx));

    ticks = dvz_ticks(-0.131456, -0.124789, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);
    AT(!duplicate_labels(&ticks, &ctx));

    dvz_ticks_destroy(&ticks);
    return 0;
}



int test_utils_ticks_extend(TestContext* context)
{
    DvzAxesContext ctx = {0};
    ctx.coord = DVZ_AXES_COORD_X;
    ctx.size_viewport = 1000;
    ctx.size_glyph = 10;

    DvzAxesTicks ticks = {0};

    // No extensions.
    double x0 = -2.123, x1 = +2.456;
    ctx.extensions = 0;
    ticks = dvz_ticks(x0, x1, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);

    // 1 extension on each side.
    ctx.extensions = 1;
    ticks = dvz_ticks(x0, x1, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug("tick #%02d: %s", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK]);

    // 2 extension on each side.
    ctx.extensions = 2;
    ticks = dvz_ticks(x0, x1, ctx);
    for (uint32_t i = 0; i < ticks.value_count; i++)
        log_debug(
            "tick #%02d: %s (%f)", i, &ticks.labels[i * MAX_GLYPHS_PER_TICK], ticks.values[i]);

    dvz_ticks_destroy(&ticks);

    return 0;
}
