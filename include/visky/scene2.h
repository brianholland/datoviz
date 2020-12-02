#ifndef VKL_SCENE_HEADER
#define VKL_SCENE_HEADER

#include "context.h"
#include "graphics.h"
#include "visuals2.h"
#include "vklite2.h"



/*************************************************************************************************/
/*  Constants                                                                                    */
/*************************************************************************************************/



/*************************************************************************************************/
/*  Enums                                                                                        */
/*************************************************************************************************/

typedef enum
{
    VKL_VISUAL_VARIANT_NONE,
    VKL_VISUAL_VARIANT_RAW,
    VKL_VISUAL_VARIANT_AGG,
    VKL_VISUAL_VARIANT_TEXTURED,
    VKL_VISUAL_VARIANT_TEXTURED_MULTI,
    VKL_VISUAL_VARIANT_SHADED,
} VklVisualVariant;


typedef enum
{
    VKL_VISUAL_NONE,

    VKL_VISUAL_SCATTER, // raw, agg
    VKL_VISUAL_SEGMENT, // raw, agg
    VKL_VISUAL_ARROW,
    VKL_VISUAL_PATH, // raw, agg
    VKL_VISUAL_TEXT, // raw, agg
    VKL_VISUAL_TRIANGLE,
    VKL_VISUAL_RECTANGLE,
    VKL_VISUAL_IMAGE, // single, multi
    VKL_VISUAL_DISC,
    VKL_VISUAL_SECTOR,
    VKL_VISUAL_MESH, // raw, textured, textured_multi, shaded
    VKL_VISUAL_POLYGON,
    VKL_VISUAL_PSLG,
    VKL_VISUAL_HISTOGRAM,
    VKL_VISUAL_AREA,
    VKL_VISUAL_CANDLE,
    VKL_VISUAL_GRAPH,
    VKL_VISUAL_SURFACE,
    VKL_VISUAL_VOLUME,
    VKL_VISUAL_FAKE_SPHERE,
} VklVisualBuiltin;



#endif



/*************************************************************************************************/
/*  Functions                                                                                    */
/*************************************************************************************************/

VKY_EXPORT void vkl_visual_builtin(VklVisual* visual, VklVisualBuiltin builtin);

VKY_EXPORT void vkl_visual_variant(VklVisual* visual, VklVisualVariant variant);

VKY_EXPORT void vkl_visual_transform(VklVisual* visual, VklTransformAxis transform_axis);
