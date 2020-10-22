#include <math.h>

#include "../include/visky/visky.h"

#define RANDOM_POS true


static void frame_callback(VkyCanvas* canvas, void* data)
{
    VkyMouse* mouse = canvas->event_controller->mouse;
    if (mouse->cur_state == VKY_MOUSE_STATE_CLICK && mouse->button == VKY_MOUSE_BUTTON_LEFT)
    {
        if (vky_panel_from_mouse(canvas->scene, mouse->cur_pos)->row != 1)
        {
            return;
        }
        VkyPick pick = vky_pick(canvas->scene, mouse->cur_pos, NULL);
        printf("CLICKED AT:\n");
        printf("pos canvas px %f %f\n", pick.pos_canvas_px[0], pick.pos_canvas_px[1]);
        printf("pos canvas ndc %f %f\n", pick.pos_canvas_ndc[0], pick.pos_canvas_ndc[1]);
        printf("pos panel %f %f\n", pick.pos_panel[0], pick.pos_panel[1]);
        printf("pos panzoom %f %f\n", pick.pos_panzoom[0], pick.pos_panzoom[1]);
        printf("pos gpu %f %f\n", pick.pos_gpu[0], pick.pos_gpu[1]);
        printf("pos data %f %f\n\n", pick.pos_data[0], pick.pos_data[1]);
    }

    VkyKeyboard* keyboard = canvas->event_controller->keyboard;
    if (keyboard->key == VKY_KEY_G)
    {
        for (uint32_t i = 0; i < canvas->scene->grid->panel_count; i++)
        {
            if (canvas->scene->grid->panels[i].controller_type == VKY_CONTROLLER_AXES_2D)
            {
                vky_axes_toggle_tick(
                    (VkyAxes*)canvas->scene->grid->panels[i].controller, VKY_AXES_TICK_GRID);
            }
        }
    }
}


int main()
{
    log_set_level_env();

    VkyApp* app = vky_create_app(VKY_DEFAULT_BACKEND);
    VkyCanvas* canvas = vky_create_canvas(app, VKY_DEFAULT_WIDTH, VKY_DEFAULT_HEIGHT);
    VkyScene* scene = vky_create_scene(canvas, VKY_CLEAR_COLOR_BLACK, 2, 1);
    vky_set_grid_heights(scene, (float[]){1, 2});

    vky_add_vertex_buffer(canvas->gpu, 1e7);
    vky_add_index_buffer(canvas->gpu, 1e7);

    VkyPanel* panel = vky_get_panel(scene, 1, 0);

    // Create the visual.
    VkyMarkersParams params = (VkyMarkersParams){{0, 0, 0, 1}, 1.0f, false};
    VkyVisual* visual = vky_visual(scene, VKY_VISUAL_MARKER, &params, NULL);
    vky_add_visual_to_panel(visual, panel, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    // Set the panel controller.
    VkyAxes2DParams axparams = vky_default_axes_2D_params();

    // Set the yscale.
    axparams.yscale.vmin = -25;
    axparams.yscale.vmax = +75;

    // x label
    strcpy(axparams.xlabel.label, "Scatter plot");
    axparams.xlabel.axis = VKY_AXIS_X;
    axparams.xlabel.color.rgb[0] = 255;
    axparams.xlabel.color.rgb[1] = 0;
    axparams.xlabel.color.rgb[2] = 0;
    axparams.xlabel.color.alpha = TO_BYTE(VKY_AXES_LABEL_COLOR_A);
    axparams.xlabel.font_size = 12;

    // y label
    strcpy(axparams.ylabel.label, "Vertical axis");
    axparams.ylabel.axis = VKY_AXIS_Y;
    axparams.ylabel.color.rgb[0] = 0;
    axparams.ylabel.color.rgb[1] = 255;
    axparams.ylabel.color.rgb[2] = 0;
    axparams.ylabel.color.alpha = TO_BYTE(VKY_AXES_LABEL_COLOR_A);
    axparams.ylabel.font_size = 12;

    axparams.colorbar.cmap = VKY_CMAP_VIRIDIS;
    vky_set_controller(panel, VKY_CONTROLLER_AXES_2D, &axparams);

    // Upload the data.
    const uint32_t n0 = 100;
    const uint32_t n = n0 * n0;
    VkyMarkersVertex* data = calloc(n, sizeof(VkyMarkersVertex));
    for (uint32_t i = 0; i < n; i++)
    {
        data[i] = (VkyMarkersVertex)
        {
#if RANDOM_POS
            {.25f * randn(), -.5 + .25f * randn(), 0.0f},
#else
            {-1 + .02 * (i % n0), -1 + .02 * (i / n0), 0},
#endif
                vky_color(VKY_CMAP_VIRIDIS, i % n0, 0, n0, .5 + .5 * rand_float()),
                RAND_MARKER_SIZE, VKY_MARKER_ARROW, i % 256
        };
    }
    visual->data.item_count = n;
    visual->data.items = data;
    vky_visual_data_raw(visual);
    FREE(data);

    vky_add_frame_callback(canvas, frame_callback, NULL);

    // Second panel.
    VkyPanel* panel2 = vky_get_panel(scene, 0, 0);

    axparams.xlabel.label[0] = 0;
    // axparams.margins[2] = 100;
    // axparams.margins[3] = 100;
    // axparams.ylabel.label[0] = 0;

    vky_set_controller(panel2, VKY_CONTROLLER_AXES_2D, &axparams);
    vky_add_visual_to_panel(visual, panel2, VKY_VIEWPORT_INNER, VKY_VISUAL_PRIORITY_NONE);

    vky_link_panels(panel, panel2, VKY_PANEL_LINK_ALL);

    vky_run_app(app);
    vky_destroy_app(app);
    return 0;
}
