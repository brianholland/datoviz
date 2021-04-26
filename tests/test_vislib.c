#include "../include/datoviz/vislib.h"
#include "../include/datoviz/visuals.h"
#include "proto.h"
#include "tests.h"



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct TestScene TestScene;



/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct TestScene
{
    DvzCanvas* canvas;
    DvzVisual* visual;
};



/*************************************************************************************************/
/*  Utils                                                                                        */
/*************************************************************************************************/

static void _visual_canvas_fill(DvzCanvas* canvas, DvzEvent ev)
{
    ASSERT(canvas != NULL);
    ASSERT(ev.user_data != NULL);
    DvzVisual* visual = (DvzVisual*)ev.user_data;

    // TODO: choose which of all canvas command buffers need to be filled with the visual
    // For now, update all of them.
    for (uint32_t i = 0; i < ev.u.rf.cmd_count; i++)
    {
        dvz_visual_fill_begin(canvas, ev.u.rf.cmds[i], ev.u.rf.img_idx);
        dvz_cmd_viewport(ev.u.rf.cmds[i], ev.u.rf.img_idx, canvas->viewport.viewport);
        dvz_visual_fill_event(
            visual, ev.u.rf.clear_color, ev.u.rf.cmds[i], ev.u.rf.img_idx, canvas->viewport, NULL);
        dvz_visual_fill_end(canvas, ev.u.rf.cmds[i], ev.u.rf.img_idx);
    }
}

static void _visual_common(DvzVisual* visual)
{
    ASSERT(visual != NULL);

    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    DvzContext* ctx = canvas->gpu->context;
    ASSERT(ctx != NULL);

    dvz_visual_data(visual, DVZ_PROP_MODEL, 0, 1, GLM_MAT4_IDENTITY);
    dvz_visual_data(visual, DVZ_PROP_VIEW, 0, 1, GLM_MAT4_IDENTITY);
    dvz_visual_data(visual, DVZ_PROP_PROJ, 0, 1, GLM_MAT4_IDENTITY);
    dvz_visual_data(visual, DVZ_PROP_TIME, 0, 1, (float[]){0});
    dvz_visual_data(visual, DVZ_PROP_VIEWPORT, 0, 1, &canvas->viewport);

    dvz_event_callback(
        canvas, DVZ_EVENT_REFILL, 0, DVZ_EVENT_MODE_SYNC, _visual_canvas_fill, visual);
}

static int _visual_screenshot(DvzVisual* visual, const char* name)
{
    ASSERT(visual != NULL);
    DvzCanvas* canvas = visual->canvas;

    char path[1024];
    snprintf(path, sizeof(path), "test_visuals_%s", name);
    int res = check_canvas(canvas, path);
    snprintf(path, sizeof(path), "%s/docs/images/visuals/%s.png", ROOT_DIR, name);
    dvz_screenshot_file(canvas, path);
    return res;
}

static int _visual_run(DvzVisual* visual, const char* name)
{
    ASSERT(visual != NULL);

    DvzCanvas* canvas = visual->canvas;
    ASSERT(canvas != NULL);

    // Update the visual's data.
    dvz_visual_update(visual, canvas->viewport, (DvzDataCoords){0}, NULL);

    // Run app.
    dvz_app_run(canvas->app, N_FRAMES);

    // Screenshot.
    int res = _visual_screenshot(visual, name);

    dvz_visual_destroy(visual);
    return res;
}



/*************************************************************************************************/
/*  Visuals tests                                                                                */
/*************************************************************************************************/

int test_vislib_point(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_POINT, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 50;
    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));
    double t = 0;
    double aspect = dvz_canvas_aspect(canvas);
    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n);
        pos[i][0] = .5 * cos(M_2PI * t);
        pos[i][1] = aspect * .5 * sin(M_2PI * t);
        dvz_colormap(DVZ_CMAP_HSV, TO_BYTE(t), color[i]);
        color[i][3] = 128;
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    // Params.
    dvz_visual_data(&visual, DVZ_PROP_MARKER_SIZE, 0, 1, (float[]){50});

    return _visual_run(&visual, "point");
}



int test_vislib_line_list(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_LINE, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 4 * 16;

    double t = 0, r = .75;
    double aspect = dvz_canvas_aspect(canvas);

    dvec3* p0 = calloc(n, sizeof(dvec3));
    dvec3* p1 = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = .5 * i / (double)n;

        p0[i][0] = r * cos(M_2PI * t);
        p0[i][1] = aspect * r * sin(M_2PI * t);

        p1[i][0] = -p0[i][0];
        p1[i][1] = -p0[i][1];

        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, n, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, p0);
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, n, p1);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(p0);
    FREE(p1);
    FREE(color);

    return _visual_run(&visual, "line");
}



int test_vislib_line_strip(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_LINE_STRIP, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 10000;
    uint32_t k = 16;

    double t = 0, r = 0;
    double aspect = dvz_canvas_aspect(canvas);

    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)n;
        r = .75 * t;
        pos[i][0] = r * cos(M_2PI * k * t);
        pos[i][1] = aspect * r * sin(M_2PI * k * t);

        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    return _visual_run(&visual, "line_strip");
}



int test_vislib_triangle_list(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_TRIANGLE, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 50;

    double t = 0;
    double ms = .1;
    double aspect = dvz_canvas_aspect(canvas);
    double r = .5;

    dvec3* p0 = calloc(n, sizeof(dvec3));
    dvec3* p1 = calloc(n, sizeof(dvec3));
    dvec3* p2 = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)n;

        p0[i][0] = r * cos(M_2PI * t);
        p0[i][1] = r * sin(M_2PI * t);

        dvz_colormap_scale(DVZ_CMAP_HSV, i, 0, n, color[i]);
        color[i][3] = 128;

        // Copy the 2 other points per triangle.
        memcpy(p1[i], p0[i], sizeof(dvec3));
        memcpy(p2[i], p0[i], sizeof(dvec3));

        // Shift the points.
        p0[i][0] -= ms;
        p0[i][1] -= ms;
        p1[i][0] += ms;
        p1[i][1] -= ms;
        p2[i][0] += 0;
        p2[i][1] += ms;

        p0[i][1] *= aspect;
        p1[i][1] *= aspect;
        p2[i][1] *= aspect;
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, p0);
    dvz_visual_data(&visual, DVZ_PROP_POS, 1, n, p1);
    dvz_visual_data(&visual, DVZ_PROP_POS, 2, n, p2);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(p0);
    FREE(p1);
    FREE(p2);
    FREE(color);

    return _visual_run(&visual, "triangle");
}



int test_vislib_triangle_strip(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_TRIANGLE_STRIP, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 40;

    double t = 0, a = 0;
    double m = .1;
    double aspect = dvz_canvas_aspect(canvas);

    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n - 1);
        a = M_2PI * t;
        pos[i][0] = (.5 + (i % 2 == 0 ? +m : -m)) * cos(a);
        pos[i][1] = aspect * (.5 + (i % 2 == 0 ? +m : -m)) * sin(a);
        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    return _visual_run(&visual, "triangle_strip");
}



int test_vislib_triangle_fan(TestContext* tc)
{
    DvzCanvas* canvas = tc->canvas;
    ASSERT(canvas != NULL);

    // Make visual.
    DvzVisual visual = dvz_visual(canvas);
    dvz_visual_builtin(&visual, DVZ_VISUAL_TRIANGLE_FAN, 0);
    _visual_common(&visual);

    // Create visual data.
    uint32_t n = 30;

    double t = 0, a = 0;
    double aspect = dvz_canvas_aspect(canvas);

    dvec3* pos = calloc(n, sizeof(dvec3));
    cvec4* color = calloc(n, sizeof(cvec4));

    for (uint32_t i = 0; i < n; i++)
    {
        t = i / (double)(n - 1);
        a = M_2PI * t;

        pos[i][0] = .5 * cos(a);
        pos[i][1] = aspect * .5 * sin(a);

        dvz_colormap_scale(DVZ_CMAP_HSV, t, 0, 1, color[i]);
    }

    // Set visual data.
    dvz_visual_data(&visual, DVZ_PROP_POS, 0, n, pos);
    dvz_visual_data(&visual, DVZ_PROP_COLOR, 0, n, color);

    // Free the arrays.
    FREE(pos);
    FREE(color);

    return _visual_run(&visual, "triangle_fan");
}



int test_vislib_marker(TestContext* tc) { return 0; }



int test_vislib_polygon(TestContext* tc) { return 0; }



int test_vislib_path(TestContext* tc) { return 0; }



int test_vislib_image(TestContext* tc) { return 0; }



int test_vislib_image_cmap(TestContext* tc) { return 0; }



int test_vislib_axes(TestContext* tc) { return 0; }



int test_vislib_mesh(TestContext* tc) { return 0; }



int test_vislib_volume(TestContext* tc) { return 0; }



int test_vislib_volume_slice(TestContext* tc) { return 0; }
