#ifndef DVZ_AXIS_HEADER
#define DVZ_AXIS_HEADER

#include "../include/datoviz/canvas.h"
#include "../include/datoviz/panel.h"
#include "../include/datoviz/scene.h"
#include "../include/datoviz/ticks_types.h"
#include "../include/datoviz/transforms.h"
#include "../include/datoviz/visuals.h"
#include "../include/datoviz/vklite.h"
#include "ticks.h"
#include "transforms_utils.h"
#include "visuals_utils.h"
#include "vklite_utils.h"



/*************************************************************************************************/
/*  Axes parameters                                                                              */
/*************************************************************************************************/

// TODO: customizable params
#define DVZ_DEFAULT_AXES_MARGINS                                                                  \
    (vec4) { 20, 20, 50, 100 }
#define DVZ_DEFAULT_AXES_LINE_WIDTH_MINOR  2.0f
#define DVZ_DEFAULT_AXES_LINE_WIDTH_MAJOR  4.0f
#define DVZ_DEFAULT_AXES_LINE_WIDTH_GRID   1.0f
#define DVZ_DEFAULT_AXES_LINE_WIDTH_LIM    2.0f
#define DVZ_DEFAULT_AXES_TICK_LENGTH_MINOR 10.0f
#define DVZ_DEFAULT_AXES_TICK_LENGTH_MAJOR 15.0f
#define DVZ_DEFAULT_AXES_FONT_SIZE         10.0f
#define DVZ_DEFAULT_AXES_COLOR_BLACK                                                              \
    (cvec4) { 0, 0, 0, 255 }
#define DVZ_DEFAULT_AXES_COLOR_GRAY                                                               \
    (cvec4) { 128, 128, 128, 255 }



/*************************************************************************************************/
/*  Axes functions                                                                               */
/*************************************************************************************************/

// Create the ticks context.
static DvzAxesContext _axes_context(DvzController* controller, DvzAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == DVZ_CONTROLLER_AXES_2D);

    ASSERT(controller->panel != NULL);
    ASSERT(controller->panel->grid != NULL);

    // Canvas size, used in tick computation.
    DvzCanvas* canvas = controller->panel->grid->canvas;
    ASSERT(canvas != NULL);
    float dpi_scaling = canvas->dpi_scaling;
    DvzViewport* viewport = &controller->panel->viewport;
    uint32_t size = 0;
    if (coord == DVZ_AXES_COORD_X)
        size = viewport->size_framebuffer[0];
    else if (coord == DVZ_AXES_COORD_Y)
        size = viewport->size_framebuffer[1];
    vec4 m = {0};
    glm_vec4_copy(viewport->margins, m);

    // Make axes context.
    DvzAxesContext ctx = {0};
    ctx.coord = coord;
    ctx.extensions = 1; // extend the range on the left/right and top/bottom
    ctx.size_viewport = size - m[1 - coord] - m[3 - coord]; // remove the margins
    ctx.scale_orig = controller->interacts[0].u.p.zoom[coord];

    // TODO: improve determination of glyph size
    float font_size = controller->u.axes_2D.font_size;
    ASSERT(font_size > 0);
    DvzFontAtlas* atlas = &canvas->gpu->context->font_atlas;
    ASSERT(atlas->glyph_width > 0);
    ASSERT(atlas->glyph_height > 0);
    ctx.size_glyph = coord == DVZ_AXES_COORD_X
                         ? font_size * atlas->glyph_width / atlas->glyph_height
                         : font_size;
    ctx.size_glyph *= dpi_scaling;

    return ctx;
}



// Recompute the tick locations as a function of the current axis range in data coordinates.
static void _axes_ticks(DvzController* controller, DvzAxisCoord coord, dvec2 range)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == DVZ_CONTROLLER_AXES_2D);

    DvzAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    DvzPanel* panel = controller->panel;
    ASSERT(panel != NULL);

    // Prepare context for tick computation.
    DvzAxesContext ctx = _axes_context(controller, coord);

    double vmin = range[0];
    double vmax = range[1];
    double vlen = vmax - vmin;
    ASSERT(vlen > 0);

    // Free the existing ticks.
    if (axes->ticks[coord].values != NULL)
        dvz_ticks_destroy(&axes->ticks[coord]);

    // Determine the tick number and positions.
    axes->ticks[coord] = dvz_ticks(vmin, vmax, ctx);

    // We keep track of the context.
    axes->ctx[coord] = ctx;
}



// Update the axes visual's data as a function of the computed ticks.
static void _axes_upload(DvzController* controller, DvzAxisCoord coord)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == DVZ_CONTROLLER_AXES_2D);
    DvzAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    ASSERT(controller->visual_count == 2);

    DvzVisual* visual = controller->visuals[coord];
    ASSERT(visual != NULL);

    DvzPanel* panel = controller->panel;
    ASSERT(panel != NULL);

    DvzDataCoords* coords = &panel->data_coords;
    ASSERT(coords != NULL);

    DvzAxesTicks* axticks = &axes->ticks[coord];
    uint32_t N = axticks->value_count;
    ASSERT(N > 0);

    // Range used for normalization of the ticks (corresponds to init panzoom).
    double vmin = axes->box.p0[coord];
    double vmax = axes->box.p1[coord];
    ASSERT(vmin < vmax);

    ASSERT(axticks->values != NULL);
    // Normalize the tick values to fit in NDC range.
    double* ticks = (double*)calloc(N, sizeof(double));
    ASSERT(ticks != NULL);
    for (uint32_t i = 0; i < N; i++)
    {
        ticks[i] = -1 + 2 * (axticks->values[i] - vmin) / (vmax - vmin);
        // log_info("%d %f vmin=%f vmax=%f", i, ticks[i], vmin, vmax);
    }

    // Minor ticks.
    double* minor_ticks = (double*)calloc((N - 1) * 4, sizeof(double));
    uint32_t k = 0;
    for (uint32_t i = 0; i < N - 1; i++)
        for (uint32_t j = 1; j <= 4; j++)
            minor_ticks[k++] = ticks[i] + j * (ticks[i + 1] - ticks[i]) / 5.;
    ASSERT(k == (N - 1) * 4);

    // Prepare text values.
    char** text = (char**)calloc(N, sizeof(char*));
    for (uint32_t i = 0; i < N; i++)
    {
        text[i] = &axticks->labels[i * MAX_GLYPHS_PER_TICK];
        // log_info("%f %s", ticks[i], text[i]);
    }

    // Set visual data.
    double lim[] = {-1};
    dvz_visual_data(visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_MINOR, 4 * (N - 1), minor_ticks);
    dvz_visual_data(visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_MAJOR, N, ticks);
    dvz_visual_data(visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_GRID, N, ticks);
    dvz_visual_data(visual, DVZ_PROP_POS, DVZ_AXES_LEVEL_LIM, 1, lim);
    dvz_visual_data(visual, DVZ_PROP_TEXT, 0, N, text);

    FREE(minor_ticks);
    FREE(ticks);
    FREE(text);
}



// Update the axes to the extent defined by the DvzDataCoords struct in the DvzPanel
static void _axes_set(DvzController* controller, DvzBox box)
{
    // WARNING: the panzoom must be reset when calling this function, so that axes->box corresponds
    // to the current view.

    ASSERT(controller != NULL);
    ASSERT(controller->type == DVZ_CONTROLLER_AXES_2D);

    DvzAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    // Initial data coordinates from the panel
    // log_info(
    //     "set axes range to x=[%f, %f], y=[%f, %f]", box.p0[0], box.p1[0], box.p0[1], box.p1[1]);
    _check_box(box);
    axes->box = box;

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        // Compute the ticks for these ranges.
        // NOTE: the range for computing the ticks is the initial range with reset panzoom.
        _axes_ticks(controller, coord, (dvec2){box.p0[coord], box.p1[coord]});

        // Upload the data.
        _axes_upload(controller, coord);
    }
}



// Initialize the ticks positions and visual.
static void _axes_ticks_init(DvzController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == DVZ_CONTROLLER_AXES_2D);
    ASSERT(controller->panel != NULL);
    DvzAxes2D* axes = &controller->u.axes_2D;

    // NOTE: get the font size which was set by in vislib.c as a prop.
    DvzProp* prop = dvz_prop_get(controller->visuals[0], DVZ_PROP_TEXT_SIZE, 0);
    ASSERT(prop != NULL);
    float* font_size = dvz_prop_item(prop, 0);
    axes->font_size = *font_size;
    ASSERT(axes->font_size > 0);

    ASSERT(axes != NULL);
    _axes_set(controller, controller->panel->data_coords.box);
}



// Determine the coords that need to be updated during panzoom because of overlapping labels.
// range is the current range, in data coordinates, that is visible
static bool _axes_collision(DvzController* controller, DvzAxisCoord coord, dvec2 range)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == DVZ_CONTROLLER_AXES_2D);
    DvzAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);
    // ASSERT(update != NULL);

    // Determine whether the ticks are overlapping, if so we should recompute the ticks (zooming)
    // Same if there are less than N visible labels (dezooming)
    // for (uint32_t i = 0; i < 2; i++)
    // {
    // DvzAxisCoord coord = (DvzAxisCoord)i;
    DvzAxesTicks* ticks = &axes->ticks[coord];
    ASSERT(ticks != NULL);

    // NOTE: make a copy because we'll use a temporary context object when computing the
    // overlap.
    DvzAxesContext ctx = axes->ctx[coord];
    // ctx.labels = ticks->labels;
    ASSERT(controller->interacts != NULL);
    ASSERT(controller->interact_count >= 1);
    float scale = controller->interacts[0].u.p.zoom[coord] / ctx.scale_orig;
    ASSERT(scale > 0);
    ctx.size_viewport *= scale;
    if (ctx.size_viewport <= 0)
        return false;
    ASSERT(ctx.size_viewport > 0);
    // ASSERT(ctx.labels != NULL);

    // Check whether there are overlapping labels (dezooming).
    double min_distance = min_distance_labels(ticks, &ctx);

    // Check whether the current view is outside the computed ticks (panning);
    bool outside = range[0] <= ticks->lmin_in || range[1] >= ticks->lmax_in;
    // if (coord == 0)
    //     log_info("%f %f %d", ticks->lmin_ex, ticks->lmax_ex, outside);

    double rel_space = min_distance / (ctx.size_viewport / scale);

    // if (i == 0)
    //     log_info(
    //         "coord %d min_d %.3f, rel_space %.3f, outside %d", //
    //         i, min_distance, rel_space, outside);
    // Recompute the ticks on the current axis?
    // }

    return min_distance <= 0 || rel_space >= .5 || outside;
}



// // Update axes->range struct as a function of the current panzoom.
// static void _axes_range(DvzController* controller, DvzAxisCoord coord)
// {
//     ASSERT(controller != NULL);
//     ASSERT(controller->type == DVZ_CONTROLLER_AXES_2D);
//     DvzPanel* panel = controller->panel;
//     ASSERT(panel != NULL);
//     DvzAxes2D* axes = &controller->u.axes_2D;
//     ASSERT(axes != NULL);

//     // set axes->range depending on coord
//     dvec3 in_bl = {-1, +1, .5}, out_bl;
//     dvec3 in_tr = {+1, -1, .5}, out_tr;

//     DvzTransformChain tc = _transforms_cds(panel, DVZ_CDS_VULKAN, DVZ_CDS_SCENE);
//     _transforms_apply(&tc, in_bl, out_bl);
//     _transforms_apply(&tc, in_tr, out_tr);

//     axes->panzoom_range[coord][0] = out_bl[coord];
//     axes->panzoom_range[coord][1] = out_tr[coord];
// }



// Callback called at every frame.
static void _axes_refresh(DvzController* controller, bool force)
{
    ASSERT(controller != NULL);
    ASSERT(controller->interacts != NULL);
    ASSERT(controller->interact_count >= 1);
    ASSERT(controller->panel != NULL);
    ASSERT(controller->panel->grid != NULL);

    DvzCanvas* canvas = controller->panel->grid->canvas;
    ASSERT(canvas != NULL);

    DvzPanel* panel = controller->panel;
    ASSERT(panel != NULL);

    if (!force && !controller->interacts[0].is_active && !canvas->resized)
        return;

    // Check label collision
    // DEBUG
    // bool update[2] = {true, true}; // whether X and Y axes must be updated or not
    bool update[2] = {false, false}; // whether X and Y axes must be updated or not

    dvec2 range[2];
    // Compute current visible range in data coordinates in range[coord]
    // Determine collision on each axis.

    dvec3 in_bl = {-1, +1, .5}, out_bl;
    dvec3 in_tr = {+1, -1, .5}, out_tr;

    DvzTransformChain tc = _transforms_cds(panel, DVZ_CDS_VULKAN, DVZ_CDS_DATA);
    _transforms_apply(&tc, in_bl, out_bl);
    _transforms_apply(&tc, in_tr, out_tr);

    for (uint32_t i = 0; i < 2; i++)
    {
        range[i][0] = out_bl[i];
        range[i][1] = out_tr[i];

        update[i] = _axes_collision(controller, (DvzAxisCoord)i, range[i]);
    }

    // Force axes ticks refresh when resizing.
    if (canvas->resized || force)
    {
        update[0] = true;
        update[1] = true;
    }

    for (uint32_t coord = 0; coord < 2; coord++)
    {
        if (!update[coord])
            continue;
        _axes_ticks(controller, (DvzAxisCoord)coord, range[coord]);
        _axes_upload(controller, (DvzAxisCoord)coord);

        // TODO: what else to do here? update a request??
        // canvas->obj.status = DVZ_OBJECT_STATUS_NEED_UPDATE;
    }
}



// Callback called at every frame.
static void _axes_callback(DvzController* controller, DvzEvent ev)
{
    ASSERT(controller != NULL);
    _default_controller_callback(controller, ev);
    _axes_refresh(controller, false);
}



static bool _is_white_background(DvzCanvas* canvas)
{
    // TODO: refactor this and put elsewhere
    ASSERT(canvas != NULL);
    float* color = canvas->render.renderpass.clear_values->color.float32;
    return color[0] == 1 && color[1] == 1 && color[2] == 1;
}



static void _axes_visual(DvzController* controller, DvzAxisCoord coord)
{
    ASSERT(controller != NULL);
    DvzPanel* panel = controller->panel;
    ASSERT(panel != NULL);
    ASSERT(panel->scene != NULL);
    DvzCanvas* canvas = panel->scene->canvas;
    ASSERT(canvas != NULL);
    DvzContext* ctx = panel->grid->canvas->gpu->context;

    // Axes visual flags
    // 0x000X: coordinate (X=0/1)
    // 0x00X0: no CPU pos normalization
    // 0xX0000: interact fixed axis
    int flags = DVZ_VISUAL_FLAGS_TRANSFORM_NONE |
                (coord == 0 ? DVZ_INTERACT_FIXED_AXIS_Y : DVZ_INTERACT_FIXED_AXIS_X) | //
                (int)coord;

    // NOTE: here, we take the controller flags, in the 0x0X00 range, and we shift them to the
    // visual-specific bit range in 0x000X, noting that the first bit is reserved to the axis
    // coordinate.
    flags |= (controller->flags >> 8);

    ASSERT((flags & DVZ_VISUAL_FLAGS_TRANSFORM_NONE) != 0);

    DvzVisual* visual = dvz_scene_visual(panel, DVZ_VISUAL_AXES_2D, flags);
    dvz_controller_visual(controller, visual);
    visual->priority = DVZ_MAX_VISUAL_PRIORITY;

    visual->clip[0] = DVZ_VIEWPORT_OUTER;
    visual->clip[1] = coord == 0 ? DVZ_VIEWPORT_OUTER_BOTTOM : DVZ_VIEWPORT_OUTER_LEFT;

    visual->interact_axis[0] = visual->interact_axis[1] =
        (coord == 0 ? DVZ_INTERACT_FIXED_AXIS_Y : DVZ_INTERACT_FIXED_AXIS_X) >> 12;

    // Text params.
    DvzFontAtlas* atlas = &ctx->font_atlas;
    ASSERT(strlen(atlas->font_str) > 0);
    dvz_visual_texture(visual, DVZ_SOURCE_TYPE_FONT_ATLAS, 0, atlas->texture);

    DvzGraphicsTextParams params = {0};
    params.grid_size[0] = (int32_t)atlas->rows;
    params.grid_size[1] = (int32_t)atlas->cols;
    params.tex_size[0] = (int32_t)atlas->width;
    params.tex_size[1] = (int32_t)atlas->height;
    dvz_visual_data_source(visual, DVZ_SOURCE_TYPE_PARAM, 0, 0, 1, 1, &params);

    if (!_is_white_background(canvas))
    {
        log_debug("dark background detected, putting axes in white");
        for (uint32_t i = 0; i < 4; i++)
        {
            if (i == 2)
                continue;
            dvz_visual_data(visual, DVZ_PROP_COLOR, i, 1, (cvec4[]){{255, 255, 255, 255}});
        }
        dvz_visual_data(visual, DVZ_PROP_COLOR, 4, 1, (cvec4[]){{255, 255, 255, 255}});
    }
}



// Add axes to a panel.
static void _add_axes(DvzController* controller)
{
    ASSERT(controller != NULL);
    DvzPanel* panel = controller->panel;
    ASSERT(panel != NULL);
    ASSERT(panel->scene != NULL);
    panel->controller = controller;
    DvzContext* ctx = panel->grid->canvas->gpu->context;
    ASSERT(ctx != NULL);

    dvz_panel_margins(panel, DVZ_DEFAULT_AXES_MARGINS);

    for (uint32_t coord = 0; coord < 2; coord++)
        _axes_visual(controller, (DvzAxisCoord)coord);

    // Add the axes data.
    _axes_ticks_init(controller);
}



// Destroy the axes objects.
static void _axes_destroy(DvzController* controller)
{
    ASSERT(controller != NULL);
    ASSERT(controller->type == DVZ_CONTROLLER_AXES_2D);
    DvzAxes2D* axes = &controller->u.axes_2D;
    ASSERT(axes != NULL);

    for (uint32_t i = 0; i < 2; i++)
    {
        dvz_ticks_destroy(&axes->ticks[i]);
    }
}



#endif
