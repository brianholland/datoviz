# cython: c_string_type=unicode, c_string_encoding=ascii

# -------------------------------------------------------------------------------------------------
# Imports
# -------------------------------------------------------------------------------------------------

from functools import wraps, partial
import logging

cimport numpy as np
import numpy as np
from cpython.ref cimport Py_INCREF
from libc.string cimport memcpy
from libc.stdio cimport printf

cimport datoviz.cydatoviz as cv


logger = logging.getLogger(__name__)



# -------------------------------------------------------------------------------------------------
# Types
# -------------------------------------------------------------------------------------------------

ctypedef np.float32_t FLOAT
ctypedef np.double_t DOUBLE
ctypedef np.uint8_t CHAR
ctypedef np.uint8_t[4] CVEC4
ctypedef np.int16_t SHORT
ctypedef np.uint16_t USHORT
ctypedef np.int32_t INT
ctypedef np.uint32_t UINT



# -------------------------------------------------------------------------------------------------
# Constants
# -------------------------------------------------------------------------------------------------

DEFAULT_WIDTH = 1024
DEFAULT_HEIGHT = 768


# TODO: add more keys
_KEYS = {
    cv.DVZ_KEY_LEFT: 'left',
    cv.DVZ_KEY_RIGHT: 'right',
    cv.DVZ_KEY_UP: 'up',
    cv.DVZ_KEY_DOWN: 'down',
    cv.DVZ_KEY_HOME: 'home',
    cv.DVZ_KEY_END: 'end',
    cv.DVZ_KEY_KP_ADD: '+',
    cv.DVZ_KEY_KP_SUBTRACT: '-',
    cv.DVZ_KEY_A: 'a',
    cv.DVZ_KEY_B: 'b',
    cv.DVZ_KEY_C: 'c',
    cv.DVZ_KEY_D: 'd',
    cv.DVZ_KEY_E: 'e',
    cv.DVZ_KEY_F: 'f',
    cv.DVZ_KEY_G: 'g',
    cv.DVZ_KEY_H: 'h',
    cv.DVZ_KEY_I: 'i',
    cv.DVZ_KEY_J: 'j',
    cv.DVZ_KEY_K: 'k',
    cv.DVZ_KEY_L: 'l',
    cv.DVZ_KEY_M: 'm',
    cv.DVZ_KEY_N: 'n',
    cv.DVZ_KEY_O: 'o',
    cv.DVZ_KEY_P: 'p',
    cv.DVZ_KEY_Q: 'q',
    cv.DVZ_KEY_R: 'r',
    cv.DVZ_KEY_S: 's',
    cv.DVZ_KEY_T: 't',
    cv.DVZ_KEY_U: 'u',
    cv.DVZ_KEY_V: 'v',
    cv.DVZ_KEY_W: 'w',
    cv.DVZ_KEY_X: 'x',
    cv.DVZ_KEY_Y: 'y',
    cv.DVZ_KEY_Z: 'z',
}

# HACK: these keys do not raise a Python key event
_EXCLUDED_KEYS = (
    cv.DVZ_KEY_NONE,
    cv.DVZ_KEY_LEFT_SHIFT,
    cv.DVZ_KEY_LEFT_CONTROL,
    cv.DVZ_KEY_LEFT_ALT,
    cv.DVZ_KEY_LEFT_SUPER,
    cv.DVZ_KEY_RIGHT_SHIFT,
    cv.DVZ_KEY_RIGHT_CONTROL,
    cv.DVZ_KEY_RIGHT_ALT,
    cv.DVZ_KEY_RIGHT_SUPER,
)

_BUTTONS = {
    cv.DVZ_MOUSE_BUTTON_LEFT: 'left',
    cv.DVZ_MOUSE_BUTTON_MIDDLE: 'middle',
    cv.DVZ_MOUSE_BUTTON_RIGHT: 'right',
}

_EVENTS ={
    'mouse_press': cv.DVZ_EVENT_MOUSE_PRESS,
    'mouse_release': cv.DVZ_EVENT_MOUSE_RELEASE,
    'mouse_move': cv.DVZ_EVENT_MOUSE_MOVE,
    'mouse_wheel': cv.DVZ_EVENT_MOUSE_WHEEL,
    'mouse_drag_begin': cv.DVZ_EVENT_MOUSE_DRAG_BEGIN,
    'mouse_drag_end': cv.DVZ_EVENT_MOUSE_DRAG_END,
    'mouse_click': cv.DVZ_EVENT_MOUSE_CLICK,
    'mouse_double_click': cv.DVZ_EVENT_MOUSE_DOUBLE_CLICK,
    'key_press': cv.DVZ_EVENT_KEY_PRESS,
    'key_release': cv.DVZ_EVENT_KEY_RELEASE,
    'frame': cv.DVZ_EVENT_FRAME,
    'timer': cv.DVZ_EVENT_TIMER,
    'gui': cv.DVZ_EVENT_GUI,
}

_VISUALS = {
    'point': cv.DVZ_VISUAL_POINT,
    'marker': cv.DVZ_VISUAL_MARKER,
    'mesh': cv.DVZ_VISUAL_MESH,
    'path': cv.DVZ_VISUAL_PATH,
    'polygon': cv.DVZ_VISUAL_POLYGON,
    'image': cv.DVZ_VISUAL_IMAGE,
    'image_cmap': cv.DVZ_VISUAL_IMAGE_CMAP,
    'volume': cv.DVZ_VISUAL_VOLUME,
    'volume_slice': cv.DVZ_VISUAL_VOLUME_SLICE,
    'line_strip': cv.DVZ_VISUAL_LINE_STRIP,
}

_CONTROLLERS = {
    'panzoom': cv.DVZ_CONTROLLER_PANZOOM,
    'axes': cv.DVZ_CONTROLLER_AXES_2D,
    'arcball': cv.DVZ_CONTROLLER_ARCBALL,
    'camera': cv.DVZ_CONTROLLER_CAMERA,
}

_TRANSPOSES = {
    None: cv.DVZ_CDS_TRANSPOSE_NONE,
    'xfyrzu': cv.DVZ_CDS_TRANSPOSE_XFYRZU,
    'xbydzl': cv.DVZ_CDS_TRANSPOSE_XBYDZL,
    'xlybzd': cv.DVZ_CDS_TRANSPOSE_XLYBZD,
}

_COORDINATE_SYSTEMS = {
    'data': cv.DVZ_CDS_DATA,
    'scene': cv.DVZ_CDS_SCENE,
    'vulkan': cv.DVZ_CDS_VULKAN,
    'framebuffer': cv.DVZ_CDS_FRAMEBUFFER,
    'window': cv.DVZ_CDS_WINDOW,
}

_PROPS = {
    'pos': cv.DVZ_PROP_POS,
    'color': cv.DVZ_PROP_COLOR,
    'alpha': cv.DVZ_PROP_ALPHA,
    'ms': cv.DVZ_PROP_MARKER_SIZE,
    'marker_type': cv.DVZ_PROP_MARKER_TYPE,
    'normal': cv.DVZ_PROP_NORMAL,
    'texcoords': cv.DVZ_PROP_TEXCOORDS,
    'index': cv.DVZ_PROP_INDEX,
    'range': cv.DVZ_PROP_RANGE,
    'length': cv.DVZ_PROP_LENGTH,
    'scale': cv.DVZ_PROP_SCALE,
    'cap_type': cv.DVZ_PROP_CAP_TYPE,
    'light_params': cv.DVZ_PROP_LIGHT_PARAMS,
    'light_pos': cv.DVZ_PROP_LIGHT_POS,
    'texcoefs': cv.DVZ_PROP_TEXCOEFS,
    'linewidth': cv.DVZ_PROP_LINE_WIDTH,
    'colormap': cv.DVZ_PROP_COLORMAP,
    'transferx': cv.DVZ_PROP_TRANSFER_X,
    'transfery': cv.DVZ_PROP_TRANSFER_Y,
    'clip': cv.DVZ_PROP_CLIP,
}

_DTYPES = {
    cv.DVZ_DTYPE_CHAR: (np.uint8, 1),
    cv.DVZ_DTYPE_CVEC2: (np.uint8, 2),
    cv.DVZ_DTYPE_CVEC3: (np.uint8, 3),
    cv.DVZ_DTYPE_CVEC4: (np.uint8, 4),

    cv.DVZ_DTYPE_USHORT: (np.uint16, 1),
    cv.DVZ_DTYPE_USVEC2: (np.uint16, 2),
    cv.DVZ_DTYPE_USVEC3: (np.uint16, 3),
    cv.DVZ_DTYPE_USVEC4: (np.uint16, 4),

    cv.DVZ_DTYPE_SHORT: (np.int16, 1),
    cv.DVZ_DTYPE_SVEC2: (np.int16, 2),
    cv.DVZ_DTYPE_SVEC3: (np.int16, 3),
    cv.DVZ_DTYPE_SVEC4: (np.int16, 4),

    cv.DVZ_DTYPE_UINT: (np.uint32, 1),
    cv.DVZ_DTYPE_UVEC2: (np.uint32, 2),
    cv.DVZ_DTYPE_UVEC3: (np.uint32, 3),
    cv.DVZ_DTYPE_UVEC4: (np.uint32, 4),

    cv.DVZ_DTYPE_INT: (np.int32, 1),
    cv.DVZ_DTYPE_IVEC2: (np.int32, 2),
    cv.DVZ_DTYPE_IVEC3: (np.int32, 3),
    cv.DVZ_DTYPE_IVEC4: (np.int32, 4),

    cv.DVZ_DTYPE_FLOAT: (np.float32, 1),
    cv.DVZ_DTYPE_VEC2: (np.float32, 2),
    cv.DVZ_DTYPE_VEC3: (np.float32, 3),
    cv.DVZ_DTYPE_VEC4: (np.float32, 4),

    cv.DVZ_DTYPE_DOUBLE: (np.double, 1),
    cv.DVZ_DTYPE_DVEC2: (np.double, 2),
    cv.DVZ_DTYPE_DVEC3: (np.double, 3),
    cv.DVZ_DTYPE_DVEC4: (np.double, 4),

    cv.DVZ_DTYPE_MAT2: (np.float32, (2, 2)),
    cv.DVZ_DTYPE_MAT3: (np.float32, (3, 3)),
    cv.DVZ_DTYPE_MAT4: (np.float32, (4, 4)),
}

_TRANSFORMS = {
    'earth': cv.DVZ_TRANSFORM_EARTH_MERCATOR_WEB,
}

_CONTROLS = {
    'slider_float': cv.DVZ_GUI_CONTROL_SLIDER_FLOAT,
    'slider_float2': cv.DVZ_GUI_CONTROL_SLIDER_FLOAT2,
    'slider_int': cv.DVZ_GUI_CONTROL_SLIDER_INT,
    'input_float': cv.DVZ_GUI_CONTROL_INPUT_FLOAT,
    'checkbox': cv.DVZ_GUI_CONTROL_CHECKBOX,
    'button': cv.DVZ_GUI_CONTROL_BUTTON,
    'label': cv.DVZ_GUI_CONTROL_LABEL,
}

_COLORMAPS = {
    'binary': cv.DVZ_CMAP_BINARY,
    'hsv': cv.DVZ_CMAP_HSV,
    'cividis': cv.DVZ_CMAP_CIVIDIS,
    'inferno': cv.DVZ_CMAP_INFERNO,
    'magma': cv.DVZ_CMAP_MAGMA,
    'plasma': cv.DVZ_CMAP_PLASMA,
    'viridis': cv.DVZ_CMAP_VIRIDIS,
    'blues': cv.DVZ_CMAP_BLUES,
    'bugn': cv.DVZ_CMAP_BUGN,
    'bupu': cv.DVZ_CMAP_BUPU,
    'gnbu': cv.DVZ_CMAP_GNBU,
    'greens': cv.DVZ_CMAP_GREENS,
    'greys': cv.DVZ_CMAP_GREYS,
    'oranges': cv.DVZ_CMAP_ORANGES,
    'orrd': cv.DVZ_CMAP_ORRD,
    'pubu': cv.DVZ_CMAP_PUBU,
    'pubugn': cv.DVZ_CMAP_PUBUGN,
    'purples': cv.DVZ_CMAP_PURPLES,
    'rdpu': cv.DVZ_CMAP_RDPU,
    'reds': cv.DVZ_CMAP_REDS,
    'ylgn': cv.DVZ_CMAP_YLGN,
    'ylgnbu': cv.DVZ_CMAP_YLGNBU,
    'ylorbr': cv.DVZ_CMAP_YLORBR,
    'ylorrd': cv.DVZ_CMAP_YLORRD,
    'afmhot': cv.DVZ_CMAP_AFMHOT,
    'autumn': cv.DVZ_CMAP_AUTUMN,
    'bone': cv.DVZ_CMAP_BONE,
    'cool': cv.DVZ_CMAP_COOL,
    'copper': cv.DVZ_CMAP_COPPER,
    'gist_heat': cv.DVZ_CMAP_GIST_HEAT,
    'gray': cv.DVZ_CMAP_GRAY,
    'hot': cv.DVZ_CMAP_HOT,
    'pink': cv.DVZ_CMAP_PINK,
    'spring': cv.DVZ_CMAP_SPRING,
    'summer': cv.DVZ_CMAP_SUMMER,
    'winter': cv.DVZ_CMAP_WINTER,
    'wistia': cv.DVZ_CMAP_WISTIA,
    'brbg': cv.DVZ_CMAP_BRBG,
    'bwr': cv.DVZ_CMAP_BWR,
    'coolwarm': cv.DVZ_CMAP_COOLWARM,
    'piyg': cv.DVZ_CMAP_PIYG,
    'prgn': cv.DVZ_CMAP_PRGN,
    'puor': cv.DVZ_CMAP_PUOR,
    'rdbu': cv.DVZ_CMAP_RDBU,
    'rdgy': cv.DVZ_CMAP_RDGY,
    'rdylbu': cv.DVZ_CMAP_RDYLBU,
    'rdylgn': cv.DVZ_CMAP_RDYLGN,
    'seismic': cv.DVZ_CMAP_SEISMIC,
    'spectral': cv.DVZ_CMAP_SPECTRAL,
    'twilight_shifted': cv.DVZ_CMAP_TWILIGHT_SHIFTED,
    'twilight': cv.DVZ_CMAP_TWILIGHT,
    'brg': cv.DVZ_CMAP_BRG,
    'cmrmap': cv.DVZ_CMAP_CMRMAP,
    'cubehelix': cv.DVZ_CMAP_CUBEHELIX,
    'flag': cv.DVZ_CMAP_FLAG,
    'gist_earth': cv.DVZ_CMAP_GIST_EARTH,
    'gist_ncar': cv.DVZ_CMAP_GIST_NCAR,
    'gist_rainbow': cv.DVZ_CMAP_GIST_RAINBOW,
    'gist_stern': cv.DVZ_CMAP_GIST_STERN,
    'gnuplot2': cv.DVZ_CMAP_GNUPLOT2,
    'gnuplot': cv.DVZ_CMAP_GNUPLOT,
    'jet': cv.DVZ_CMAP_JET,
    'nipy_spectral': cv.DVZ_CMAP_NIPY_SPECTRAL,
    'ocean': cv.DVZ_CMAP_OCEAN,
    'prism': cv.DVZ_CMAP_PRISM,
    'rainbow': cv.DVZ_CMAP_RAINBOW,
    'terrain': cv.DVZ_CMAP_TERRAIN,
    'bkr': cv.DVZ_CMAP_BKR,
    'bky': cv.DVZ_CMAP_BKY,
    'cet_d10': cv.DVZ_CMAP_CET_D10,
    'cet_d11': cv.DVZ_CMAP_CET_D11,
    'cet_d8': cv.DVZ_CMAP_CET_D8,
    'cet_d13': cv.DVZ_CMAP_CET_D13,
    'cet_d3': cv.DVZ_CMAP_CET_D3,
    'cet_d1a': cv.DVZ_CMAP_CET_D1A,
    'bjy': cv.DVZ_CMAP_BJY,
    'gwv': cv.DVZ_CMAP_GWV,
    'bwy': cv.DVZ_CMAP_BWY,
    'cet_d12': cv.DVZ_CMAP_CET_D12,
    'cet_r3': cv.DVZ_CMAP_CET_R3,
    'cet_d9': cv.DVZ_CMAP_CET_D9,
    'cwr': cv.DVZ_CMAP_CWR,
    'cet_cbc1': cv.DVZ_CMAP_CET_CBC1,
    'cet_cbc2': cv.DVZ_CMAP_CET_CBC2,
    'cet_cbl1': cv.DVZ_CMAP_CET_CBL1,
    'cet_cbl2': cv.DVZ_CMAP_CET_CBL2,
    'cet_cbtc1': cv.DVZ_CMAP_CET_CBTC1,
    'cet_cbtc2': cv.DVZ_CMAP_CET_CBTC2,
    'cet_cbtl1': cv.DVZ_CMAP_CET_CBTL1,
    'bgy': cv.DVZ_CMAP_BGY,
    'bgyw': cv.DVZ_CMAP_BGYW,
    'bmw': cv.DVZ_CMAP_BMW,
    'cet_c1': cv.DVZ_CMAP_CET_C1,
    'cet_c1s': cv.DVZ_CMAP_CET_C1S,
    'cet_c2': cv.DVZ_CMAP_CET_C2,
    'cet_c4': cv.DVZ_CMAP_CET_C4,
    'cet_c4s': cv.DVZ_CMAP_CET_C4S,
    'cet_c5': cv.DVZ_CMAP_CET_C5,
    'cet_i1': cv.DVZ_CMAP_CET_I1,
    'cet_i3': cv.DVZ_CMAP_CET_I3,
    'cet_l10': cv.DVZ_CMAP_CET_L10,
    'cet_l11': cv.DVZ_CMAP_CET_L11,
    'cet_l12': cv.DVZ_CMAP_CET_L12,
    'cet_l16': cv.DVZ_CMAP_CET_L16,
    'cet_l17': cv.DVZ_CMAP_CET_L17,
    'cet_l18': cv.DVZ_CMAP_CET_L18,
    'cet_l19': cv.DVZ_CMAP_CET_L19,
    'cet_l4': cv.DVZ_CMAP_CET_L4,
    'cet_l7': cv.DVZ_CMAP_CET_L7,
    'cet_l8': cv.DVZ_CMAP_CET_L8,
    'cet_l9': cv.DVZ_CMAP_CET_L9,
    'cet_r1': cv.DVZ_CMAP_CET_R1,
    'cet_r2': cv.DVZ_CMAP_CET_R2,
    'colorwheel': cv.DVZ_CMAP_COLORWHEEL,
    'fire': cv.DVZ_CMAP_FIRE,
    'isolum': cv.DVZ_CMAP_ISOLUM,
    'kb': cv.DVZ_CMAP_KB,
    'kbc': cv.DVZ_CMAP_KBC,
    'kg': cv.DVZ_CMAP_KG,
    'kgy': cv.DVZ_CMAP_KGY,
    'kr': cv.DVZ_CMAP_KR,
    'black_body': cv.DVZ_CMAP_BLACK_BODY,
    'kindlmann': cv.DVZ_CMAP_KINDLMANN,
    'extended_kindlmann': cv.DVZ_CMAP_EXTENDED_KINDLMANN,
    'glasbey': cv.DVZ_CPAL256_GLASBEY,
    'glasbey_cool': cv.DVZ_CPAL256_GLASBEY_COOL,
    'glasbey_dark': cv.DVZ_CPAL256_GLASBEY_DARK,
    'glasbey_hv': cv.DVZ_CPAL256_GLASBEY_HV,
    'glasbey_light': cv.DVZ_CPAL256_GLASBEY_LIGHT,
    'glasbey_warm': cv.DVZ_CPAL256_GLASBEY_WARM,
    'accent': cv.DVZ_CPAL032_ACCENT,
    'dark2': cv.DVZ_CPAL032_DARK2,
    'paired': cv.DVZ_CPAL032_PAIRED,
    'pastel1': cv.DVZ_CPAL032_PASTEL1,
    'pastel2': cv.DVZ_CPAL032_PASTEL2,
    'set1': cv.DVZ_CPAL032_SET1,
    'set2': cv.DVZ_CPAL032_SET2,
    'set3': cv.DVZ_CPAL032_SET3,
    'tab10': cv.DVZ_CPAL032_TAB10,
    'tab20': cv.DVZ_CPAL032_TAB20,
    'tab20b': cv.DVZ_CPAL032_TAB20B,
    'tab20c': cv.DVZ_CPAL032_TAB20C,
    'category10_10': cv.DVZ_CPAL032_CATEGORY10_10,
    'category20_20': cv.DVZ_CPAL032_CATEGORY20_20,
    'category20b_20': cv.DVZ_CPAL032_CATEGORY20B_20,
    'category20c_20': cv.DVZ_CPAL032_CATEGORY20C_20,
    'colorblind8': cv.DVZ_CPAL032_COLORBLIND8,
}

_TEXTURE_FILTERS = {
    None: cv.VK_FILTER_NEAREST,
    'nearest': cv.VK_FILTER_NEAREST,
    'linear': cv.VK_FILTER_LINEAR,
}

_SOURCE_TYPES = {
    'transfer': (cv.DVZ_SOURCE_TYPE_TRANSFER, 1),
    'image': (cv.DVZ_SOURCE_TYPE_IMAGE, 2),
    'volume': (cv.DVZ_SOURCE_TYPE_VOLUME, 3),
}

_FORMATS = {
    (np.dtype(np.uint8), 1): cv.VK_FORMAT_R8_UNORM,
    (np.dtype(np.uint8), 3): cv.VK_FORMAT_R8G8B8_UNORM,
    (np.dtype(np.uint8), 4): cv.VK_FORMAT_R8G8B8A8_UNORM,
    (np.dtype(np.uint16), 1): cv.VK_FORMAT_R16_UNORM,
    (np.dtype(np.int16), 1): cv.VK_FORMAT_R16_SNORM,
    (np.dtype(np.uint32), 1): cv.VK_FORMAT_R32_UINT,
    (np.dtype(np.int32), 1): cv.VK_FORMAT_R32_SINT,
    (np.dtype(np.float32), 1): cv.VK_FORMAT_R32_SFLOAT,
}

_MARKER_TYPES = {
    'disc': cv.DVZ_MARKER_DISC,
    'vbar': cv.DVZ_MARKER_VBAR,
}

_CUSTOM_COLORMAPS = {}



# -------------------------------------------------------------------------------------------------
# Constant utils
# -------------------------------------------------------------------------------------------------

def _key_name(key):
    return _KEYS.get(key, key)

def _button_name(button):
    return _BUTTONS.get(button, None)

def _get_modifiers(mods):
    mods_py = []
    if mods & cv.DVZ_KEY_MODIFIER_SHIFT:
        mods_py.append('shift')
    if mods & cv.DVZ_KEY_MODIFIER_CONTROL:
        mods_py.append('control')
    if mods & cv.DVZ_KEY_MODIFIER_ALT:
        mods_py.append('alt')
    if mods & cv.DVZ_KEY_MODIFIER_SUPER:
        mods_py.append('super')
    return tuple(mods_py)

# def _mouse_state(state):
#     return _MOUSE_STATES.get(state, None)

def _get_prop(name):
    return _PROPS[name]



# -------------------------------------------------------------------------------------------------
# Util functions
# -------------------------------------------------------------------------------------------------

def _validate_data(dt, nc, data):
    data = data.astype(dt)
    if not data.flags['C_CONTIGUOUS']:
        data = np.ascontiguousarray(data)
    if not hasattr(nc, '__len__'):
        nc = (nc,)
    nd = len(nc)  # expected dimension of the data - 1
    if nc[0] == 1 and data.ndim == 1:
        data = data.reshape((-1, 1))
    if data.ndim < nd + 1:
        data = data[np.newaxis, :]
    assert data.ndim == nd + 1, f"Incorrect array dimension {data.shape}, nc={nc}"
    assert data.shape[1:] == nc, f"Incorrect array shape {data.shape} instead of {nc}"
    return data



cdef _get_ev_args(cv.DvzEvent c_ev):
    cdef float* fvalue
    cdef int* ivalue
    cdef bint* bvalue
    dt = c_ev.type
    # GUI events.
    if dt == cv.DVZ_EVENT_GUI:
        if c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_SLIDER_FLOAT:
            fvalue = <float*>c_ev.u.g.control.value
            return (fvalue[0],), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_SLIDER_FLOAT2:
            fvalue = <float*>c_ev.u.g.control.value
            return (fvalue[0], fvalue[1]), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_SLIDER_INT:
            ivalue = <int*>c_ev.u.g.control.value
            return (ivalue[0],), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_INPUT_FLOAT:
            fvalue = <float*>c_ev.u.g.control.value
            return (fvalue[0],), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_CHECKBOX:
            bvalue = <bint*>c_ev.u.g.control.value
            return (bvalue[0],), {}
        elif c_ev.u.g.control.type == cv.DVZ_GUI_CONTROL_BUTTON:
            bvalue = <bint*>c_ev.u.g.control.value
            return (bvalue[0],), {}
    # Key events.
    elif dt == cv.DVZ_EVENT_KEY_PRESS or dt == cv.DVZ_EVENT_KEY_RELEASE:
        key = _key_name(c_ev.u.k.key_code)
        modifiers = _get_modifiers(c_ev.u.k.modifiers)
        return (key, modifiers), {}
    # Mouse button events.
    elif dt == cv.DVZ_EVENT_MOUSE_PRESS or dt == cv.DVZ_EVENT_MOUSE_RELEASE:
        button = _button_name(c_ev.u.b.button)
        modifiers = _get_modifiers(c_ev.u.b.modifiers)
        return (button, modifiers), {}
    # Mouse button events.
    elif dt == cv.DVZ_EVENT_MOUSE_CLICK or dt == cv.DVZ_EVENT_MOUSE_DOUBLE_CLICK:
        x = c_ev.u.c.pos[0]
        y = c_ev.u.c.pos[1]
        button = _button_name(c_ev.u.c.button)
        modifiers = _get_modifiers(c_ev.u.c.modifiers)
        dbl = c_ev.u.c.double_click
        return (x, y), dict(button=button, modifiers=modifiers)  #, double_click=dbl)
    # Mouse move event.
    elif dt == cv.DVZ_EVENT_MOUSE_MOVE:
        x = c_ev.u.m.pos[0]
        y = c_ev.u.m.pos[1]
        modifiers = _get_modifiers(c_ev.u.m.modifiers)
        return (x, y), dict(modifiers=modifiers)
    # Mouse wheel event.
    elif dt == cv.DVZ_EVENT_MOUSE_WHEEL:
        x = c_ev.u.w.pos[0]
        y = c_ev.u.w.pos[1]
        dx = c_ev.u.w.dir[0]
        dy = c_ev.u.w.dir[1]
        modifiers = _get_modifiers(c_ev.u.w.modifiers)
        return (x, y, dx, dy), dict(modifiers=modifiers)
    return (), {}



# NOTE: this function may run in a background thread if using async callbacks
# It should not acquire the GIL
cdef _wrapped_callback(cv.DvzCanvas* c_canvas, cv.DvzEvent c_ev):
    cdef object tup
    if c_ev.user_data != NULL:
        tup = <object>c_ev.user_data

        # For each type of event, get the arguments to the function
        ev_args, ev_kwargs = _get_ev_args(c_ev)

        f, args = tup

        # This is the control type the callback was registered for.
        name = args[0] if args else None

        # NOTE: only call the callback if the raised GUI event is for that control.
        dt = c_ev.type
        if dt == cv.DVZ_EVENT_GUI:
            if c_ev.u.g.control.name != name:
                return

        try:
            f(*ev_args, **ev_kwargs)
        except Exception as e:
            print("Error: %s" % e)



cdef _add_event_callback(
    cv.DvzCanvas* c_canvas, cv.DvzEventType evtype, double param, f, args,
    cv.DvzEventMode mode=cv.DVZ_EVENT_MODE_SYNC):

    cdef void* ptr_to_obj
    tup = (f, args)

    # IMPORTANT: need to either keep a reference of this tuple object somewhere in the class,
    # or increase the ref, otherwise this tuple will be deleted by the time we call it in the
    # C callback function.
    Py_INCREF(tup)

    ptr_to_obj = <void*>tup
    cv.dvz_event_callback(
        c_canvas, evtype, param, mode,
        <cv.DvzEventCallback>_wrapped_callback, ptr_to_obj)



# -------------------------------------------------------------------------------------------------
# Public functions
# -------------------------------------------------------------------------------------------------

def colormap(np.ndarray[DOUBLE, ndim=1] values, vmin=None, vmax=None, cmap=None, alpha=None):
    N = values.size
    if cmap in _COLORMAPS:
        cmap_ = _COLORMAPS[cmap]
    elif cmap in _CUSTOM_COLORMAPS:
        cmap_ = _CUSTOM_COLORMAPS[cmap]
    else:
        cmap_ = cv.DVZ_CMAP_VIRIDIS
    # TODO: ndarrays
    cdef np.ndarray out = np.zeros((N, 4), dtype=np.uint8)
    if vmin is None:
        vmin = values.min()
    if vmax is None:
        vmax = values.max()
    cv.dvz_colormap_array(cmap_, N, <double*>&values.data[0], vmin, vmax, <cv.cvec4*>&out.data[0])
    if alpha is not None:
        if not isinstance(alpha, np.ndarray):
            alpha = np.array(alpha)
        alpha = (alpha * 255).astype(np.uint8)
        out[:, 3] = alpha
    return out



# -------------------------------------------------------------------------------------------------
# App
# -------------------------------------------------------------------------------------------------

cdef class App:

    cdef cv.DvzApp* _c_app
    cdef cv.DvzGpu* _c_gpu

    _canvases = []

    def __cinit__(self):
        self._c_app = cv.dvz_app(cv.DVZ_BACKEND_GLFW)
        if self._c_app is NULL:
            raise MemoryError()
        self._c_gpu = cv.dvz_gpu_best(self._c_app);
        if self._c_gpu is NULL:
            raise MemoryError()

    def __dealloc__(self):
        self.destroy()

    def destroy(self):
        if self._c_app is not NULL:
            for c in self._canvases:
                c.destroy()
            cv.dvz_app_destroy(self._c_app)
            self._c_app = NULL

    def canvas(
            self, int width=DEFAULT_WIDTH, int height=DEFAULT_HEIGHT, int rows=1, int cols=1,
            bint show_fps=False, clear_color=None, bint pick=False, bint high_dpi=False):
        cdef int flags = 0
        flags |= cv.DVZ_CANVAS_FLAGS_IMGUI
        if show_fps:
            flags |= cv.DVZ_CANVAS_FLAGS_FPS
        if pick:
            flags |= cv.DVZ_CANVAS_FLAGS_PICK
        if high_dpi:
            flags |= cv.DVZ_CANVAS_FLAGS_DPI_SCALE_200
        c_canvas = cv.dvz_canvas(self._c_gpu, width, height, flags)

        # Canvas clear color.
        if clear_color == 'white':
            cv.dvz_canvas_clear_color(c_canvas, 1, 1, 1)

        if c_canvas is NULL:
            raise MemoryError()
        c = Canvas()
        c.create(self, c_canvas, rows, cols, clear_color)
        self._canvases.append(c)
        return c

    def run(self, int n_frames=0, unicode screenshot=None, unicode video=None):
        # HACK: run a few frames to render the image, make a screenshot, and run the event loop.
        if screenshot and self._canvases:
            cv.dvz_app_run(self._c_app, 5)
            self._canvases[0].screenshot(screenshot)
        if video and self._canvases:
            self._canvases[0].video(video)
        cv.dvz_app_run(self._c_app, n_frames)

    def run_one_frame(self):
        cv.dvz_app_run(self._c_app, 1)



# -------------------------------------------------------------------------------------------------
# Canvas
# -------------------------------------------------------------------------------------------------

cdef class Canvas:
    cdef cv.DvzCanvas* _c_canvas
    cdef cv.DvzScene* _c_scene
    cdef cv.DvzGrid* _c_grid
    cdef object _app
    cdef bint _video_recording
    cdef object _clear_color

    _panels = []

    cdef create(self, app, cv.DvzCanvas* c_canvas, int rows, int cols, clear_color):
        self._c_canvas = c_canvas
        self._c_scene = cv.dvz_scene(c_canvas, rows, cols)
        self._c_grid = &self._c_scene.grid
        self._app = app
        self._clear_color = clear_color
        # _add_close_callback(self._c_canvas, self._destroy_wrapper, ())

    def screenshot(self, unicode path):
        cdef char* _c_path = path
        cv.dvz_screenshot_file(self._c_canvas, _c_path);

    def video(self, unicode path):
        cdef char* _c_path = path
        cv.dvz_canvas_video(self._c_canvas, 30, 10000000, _c_path, False)
        self._video_recording = True

    def pause(self):
        self._video_recording = not self._video_recording
        cv.dvz_canvas_pause(self._c_canvas, self._video_recording)

    def stop(self):
        cv.dvz_canvas_stop(self._c_canvas)

    def colormap(self, unicode name, np.ndarray[CHAR, ndim=2] colors):
        assert colors.shape[1] == 4
        color_count = colors.shape[0]
        assert color_count > 0
        assert color_count <= 256
        colors = colors.astype(np.uint8)
        if not colors.flags['C_CONTIGUOUS']:
            colors = np.ascontiguousarray(colors)

        # TODO: use constant CMAP_CUSTOM instead of hard-coded value
        cmap = 160 + len(_CUSTOM_COLORMAPS)
        _CUSTOM_COLORMAPS[name] = cmap
        cv.dvz_colormap_custom(cmap, color_count, <cv.cvec4*>&colors.data[0])
        cv.dvz_context_colormap(self._c_canvas.gpu.context)

    def panel(self, int row=0, int col=0, controller='axes', transform=None, transpose=None, **kwargs):
        cdef int flags
        flags = 0
        if controller == 'axes':
            if kwargs.pop('hide_minor_ticks', False):
                flags |= cv.DVZ_AXES_FLAGS_HIDE_MINOR
            if kwargs.pop('hide_grid', False):
                flags |= cv.DVZ_AXES_FLAGS_HIDE_GRID

        if controller == 'axes' and self._clear_color is None:
            cv.dvz_canvas_clear_color(self._c_canvas, 1, 1, 1)

        ctl = _CONTROLLERS.get(controller, cv.DVZ_CONTROLLER_NONE)
        trans = _TRANSPOSES.get(transpose, cv.DVZ_CDS_TRANSPOSE_NONE)
        transf = _TRANSFORMS.get(transform, cv.DVZ_TRANSFORM_CARTESIAN)
        c_panel = cv.dvz_scene_panel(self._c_scene, row, col, ctl, flags)
        if c_panel is NULL:
            raise MemoryError()
        c_panel.data_coords.transform = transf
        cv.dvz_panel_transpose(c_panel, trans)
        p = Panel()
        p.create(self._c_scene, c_panel)
        self._panels.append(p)
        return p

    def pick(self, cv.uint32_t x, cv.uint32_t y):
        cdef cv.uvec2 xy
        cdef cv.ivec4 rgba
        xy[0] = x
        xy[1] = y
        cv.dvz_canvas_pick(self._c_canvas, xy, rgba)

        cdef cv.int32_t r, g, b, a
        r = rgba[0]
        g = rgba[1]
        b = rgba[2]
        a = rgba[3]

        return (r, g, b, a)

    def gui(self, unicode title):
        c_gui = cv.dvz_gui(self._c_canvas, title, 0)
        gui = Gui()
        gui.create(self._c_canvas, c_gui)
        return gui

    def demo_gui(self):
        cv.dvz_imgui_demo(self._c_canvas)

    def _texture(self, source_type, arr, filtering='nearest'):
        tex = Texture()
        tex.create(self._c_canvas, source_type, arr)
        tex.set_filter(filtering)
        return tex

    def transfer(self, arr, **kwargs):
        return self._texture('transfer', arr, **kwargs)

    def image(self, arr, **kwargs):
        return self._texture('image', arr, **kwargs)

    def volume(self, arr, **kwargs):
        return self._texture('volume', arr, **kwargs)

    def __dealloc__(self):
        self.destroy()

    def _destroy_wrapper(self):
        # This is called when the user presses Esc, Datoviz organizes the canvas closing and
        # destruction, but we need the Python object to be destroyed as well and the
        # canvas to be removed from the canvas list in the App.
        self._c_canvas = NULL
        self._app._canvases.remove(self)

    def destroy(self):
        # This is called when the canvas is closed from Python.
        # The event loop will close the canvas and destroy it at the next frame.
        # However this doesn't work when the canvas is closed from C (for example by pressing Esc)
        # because then the C object will be destroyed, but not the Python one. We need to
        # destroy the Python via the close callback, which is called when the C library
        # is about to destroy the canvas, to give Python a chance to destroy the Python wrapper
        # as well.
        if self._c_scene is not NULL:
            cv.dvz_scene_destroy(self._c_scene)
        if self._c_canvas is not NULL:
            cv.dvz_canvas_to_close(self._c_canvas)
            self._c_canvas = NULL

    def panel_at(self, x, y):
        # Find the panel
        cdef cv.vec2 pos
        pos[0] = x
        pos[1] = y
        c_panel = cv.dvz_panel_at(self._c_grid, pos)

        cdef Panel panel
        for p in self._panels:
            panel = p
            if panel._c_panel == c_panel:
                return panel

    def _connect(self, evtype_py, f, param=0, cv.DvzEventMode mode=cv.DVZ_EVENT_MODE_SYNC):
        cdef cv.DvzEventType evtype
        evtype = _EVENTS.get(evtype_py, 0)
        _add_event_callback(self._c_canvas, evtype, param, f, (), mode=mode)

    def connect(self, f):
        assert f.__name__.startswith('on_')
        ev_name = f.__name__[3:]
        self._connect(ev_name, f)

    # def connect_async(self, f):
    #     assert f.__name__.startswith('on_')
    #     ev_name = f.__name__[3:]
    #     self._connect(ev_name, f, mode=cv.DVZ_EVENT_MODE_ASYNC)



# -------------------------------------------------------------------------------------------------
# Panel
# -------------------------------------------------------------------------------------------------

cdef class Panel:

    cdef cv.DvzScene* _c_scene
    cdef cv.DvzPanel* _c_panel

    _visuals = []

    cdef create(self, cv.DvzScene* c_scene, cv.DvzPanel* c_panel):
        self._c_panel = c_panel
        self._c_scene = c_scene

    @property
    def row(self):
        return self._c_panel.row

    @property
    def col(self):
        return self._c_panel.col

    def visual(self, vtype, depth_test=None, transform='auto'):
        visual_type = _VISUALS.get(vtype, 0)
        if not visual_type:
            raise ValueError("unknown visual type")
        flags = 0
        if depth_test:
            flags |= cv.DVZ_GRAPHICS_FLAGS_DEPTH_TEST
        if transform is None:
            flags |= cv.DVZ_VISUAL_FLAGS_TRANSFORM_NONE
        # This keyword means that the panel box will NOT be recomputed every time the POS prop
        # changes
        elif transform == 'init':
            flags |= cv.DVZ_VISUAL_FLAGS_TRANSFORM_BOX_INIT
        c_visual = cv.dvz_scene_visual(self._c_panel, visual_type, flags)
        if c_visual is NULL:
            raise MemoryError()
        v = Visual()
        v.create(self._c_panel, c_visual, vtype)
        self._visuals.append(v)
        return v

    def pick(self, x, y, target_cds='data'):
        cdef cv.dvec3 pos_in
        cdef cv.dvec3 pos_out
        pos_in[0] = x
        pos_in[1] = y
        pos_in[2] = 0
        source = cv.DVZ_CDS_WINDOW
        target = _COORDINATE_SYSTEMS[target_cds]
        cv.dvz_transform(self._c_panel, source, pos_in, target, pos_out)
        return pos_out[0], pos_out[1]



# -------------------------------------------------------------------------------------------------
# Texture
# -------------------------------------------------------------------------------------------------

cdef _get_tex_info(unicode source_type, np.ndarray arr):
    # Find the Vulkan format for the texture
    c_source_type, ndim = _SOURCE_TYPES[source_type]

    # Find the format from the array shape and dtype
    assert 1 <= ndim <= 3
    if ndim <= arr.ndim - 1:
        nc = arr.shape[ndim]
    else:
        nc = 1
    # assert (arr.dtype, nc) in _FORMATS, ((arr.dtype, nc), ndim, arr.shape)
    assert nc <= 4
    c_format = _FORMATS[arr.dtype, nc]
    return c_source_type, c_format, ndim



cdef class Texture:
    cdef cv.DvzCanvas* _c_canvas
    cdef cv.DvzContext* _c_context
    cdef cv.DvzTexture* _c_texture
    cdef cv.VkFormat _c_format
    cdef cv.DvzSourceType _c_source_type
    cdef unicode source_type

    cdef create(self, cv.DvzCanvas* c_canvas, unicode source_type, np.ndarray arr):
        self._c_canvas = c_canvas
        self._c_context = c_canvas.gpu.context
        self.source_type = source_type

        # Find the texture information from the NumPy array spec.
        self._c_source_type, self._c_format, ndim = _get_tex_info(source_type, arr)

        # Find the shape
        cdef np.uint32_t shape[3]
        for i in range(ndim):
            shape[i] = arr.shape[i]
        for i in range(ndim, 3):
            shape[i] = 1

        if ndim > 1:
            # NOTE: Vulkan considers textures as (width, height, depth) whereas NumPy
            # considers them as (height, width, depth), hence the need to transpose here.
            shape[0], shape[1] = shape[1], shape[0]

        # Create the Datoviz texture.
        self._c_texture = cv.dvz_ctx_texture(self._c_context, ndim, &shape[0], self._c_format)

        self.set_data(arr)

    def set_filter(self, name):
        cv.dvz_texture_filter(self._c_texture, cv.DVZ_FILTER_MIN, _TEXTURE_FILTERS[name])
        cv.dvz_texture_filter(self._c_texture, cv.DVZ_FILTER_MAG, _TEXTURE_FILTERS[name])

    def set_data(self, np.ndarray arr):
        # if not arr.flags['C_CONTIGUOUS']:
        #     arr = np.ascontiguousarray(arr)

        c_source_type, c_format, ndim = _get_tex_info(self.source_type, arr)
        assert c_source_type == self._c_source_type
        assert c_format == self._c_format

        cdef cv.uvec3 DVZ_ZERO_OFFSET = [0, 0, 0]

        cdef size = arr.size
        cdef item_size = np.dtype(arr.dtype).itemsize

        cdef int s
        s = size * item_size
        # printf("canvas=%d, tex=%d, size=%d, data=%d\n", self._c_canvas, self._c_texture, s, &value.data[0])
        cv.dvz_upload_texture(
            self._c_canvas, self._c_texture, DVZ_ZERO_OFFSET, DVZ_ZERO_OFFSET,
            size * item_size, &arr.data[0])


# -------------------------------------------------------------------------------------------------
# Visual
# -------------------------------------------------------------------------------------------------

cdef class Visual:
    cdef cv.DvzPanel* _c_panel
    cdef cv.DvzVisual* _c_visual
    cdef cv.DvzContext* _c_context
    cdef unicode vtype
    _textures = {}

    cdef create(self, cv.DvzPanel* c_panel, cv.DvzVisual* c_visual, unicode vtype):
        self._c_panel = c_panel
        self._c_visual = c_visual
        self._c_context = c_visual.canvas.gpu.context
        self.vtype = vtype

    def data(self, name, np.ndarray value, idx=0):
        prop_type = _get_prop(name)
        c_prop = cv.dvz_prop_get(self._c_visual, prop_type, idx)
        dtype, nc = _DTYPES[c_prop.dtype]
        value = _validate_data(dtype, nc, value)
        N = value.shape[0]
        cv.dvz_visual_data(self._c_visual, prop_type, idx, N, &value.data[0])

    def texture(self, Texture tex, idx=0):
        # Bind the texture with the visual for the specified source.
        cv.dvz_visual_texture(
            self._c_visual, tex._c_source_type, idx, tex._c_texture)

    def load_obj(self, unicode path, compute_normals=False):
        # TODO: check that it is a mesh visual

        cdef cv.DvzMesh mesh = cv.dvz_mesh_obj(path);

        if compute_normals:
            print("computing normals")
            cv.dvz_mesh_normals(&mesh)

        nv = mesh.vertices.item_count;
        ni = mesh.indices.item_count;

        cv.dvz_visual_data_source(self._c_visual, cv.DVZ_SOURCE_TYPE_VERTEX, 0, 0, nv, nv, mesh.vertices.data);
        cv.dvz_visual_data_source(self._c_visual, cv.DVZ_SOURCE_TYPE_INDEX, 0, 0, ni, ni, mesh.indices.data);

    # def surface(
    #     self, np.ndarray[DOUBLE, ndim=2] x, np.ndarray[DOUBLE, ndim=2] y, np.ndarray[DOUBLE, ndim=2] z,
    #     # np.ndarray[DOUBLE, ndim=1] values=None, cmap=None, vmin=None, vmax=None):
    #     np.ndarray[DOUBLE, ndim=3] uv=None,
    #     ):

    #     # cmap_ = _COLORMAPS.get(cmap, cv.DVZ_CMAP_VIRIDIS)
    #     # colormap(values, vmin=vmin, vmax=vmax, cmap=None, alpha=None):

    #     # TODO: check that it is a mesh visual
    #     row_count = x.shape[0]
    #     col_count = x.shape[1]

    #     cdef np.ndarray positions = np.empty((row_count, col_count, 3), dtype=np.float32)
    #     positions[..., 0] = x
    #     positions[..., 1] = y
    #     positions[..., 2] = z
    #     # assert colors.shape[0] == row_count

    #     cdef np.ndarray texcoords = np.empty((row_count, col_count, 2), dtype=np.float32)

    #     cdef const cv.vec2* p_uv = NULL
    #     if uv is not None:
    #         texcoords[..., 0] = uv[..., 0]
    #         texcoords[..., 1] = uv[..., 1]
    #         p_uv = <const cv.vec2*>(&texcoords.data[0])

    #     cdef cv.DvzMesh mesh = cv.dvz_mesh_grid(
    #         row_count, col_count, <const cv.vec3*>(&positions.data[0]), p_uv)

    #     nv = mesh.vertices.item_count;
    #     ni = mesh.indices.item_count;

    #     cv.dvz_visual_data_source(self._c_visual, cv.DVZ_SOURCE_TYPE_VERTEX, 0, 0, nv, nv, mesh.vertices.data);
    #     cv.dvz_visual_data_source(self._c_visual, cv.DVZ_SOURCE_TYPE_INDEX, 0, 0, ni, ni, mesh.indices.data);



# -------------------------------------------------------------------------------------------------
# GUI
# -------------------------------------------------------------------------------------------------

cdef class GuiControl:
    cdef cv.DvzGui* _c_gui
    cdef cv.DvzGuiControl* _c_control
    cdef unicode ctype
    cdef bytes str_ascii

    cdef create(self, cv.DvzGui* c_gui, cv.DvzGuiControl* c_control, unicode ctype):
        self._c_gui = c_gui
        self._c_control = c_control
        self.ctype = ctype

    def get(self):
        cdef void* ptr
        ptr = cv.dvz_gui_value(self._c_control)
        if self.ctype == 'input_float' or self.ctype == 'slider_float':
            return (<float*>ptr)[0]

    def set(self, obj):
        cdef void* ptr
        cdef char* c_str
        ptr = cv.dvz_gui_value(self._c_control)
        if self.ctype == 'input_float' or self.ctype == 'slider_float':
            (<float*>ptr)[0] = <float>float(obj)
        elif self.ctype == 'label':
            self.str_ascii = obj.encode('ascii')
            if len(self.str_ascii) >= 1024:
                self.str_ascii = self.str_ascii[:1024]
            c_str = self.str_ascii
            # HACK: +1 for string null termination
            memcpy(ptr, c_str, len(self.str_ascii) + 1)


cdef class Gui:
    cdef cv.DvzCanvas* _c_canvas
    cdef cv.DvzGui* _c_gui
    _controls = {}

    cdef create(self, cv.DvzCanvas* c_canvas, cv.DvzGui* c_gui):
        self._c_canvas = c_canvas
        self._c_gui = c_gui

    def control(self, unicode ctype, unicode name, **kwargs):
        cdef cv.DvzEventMode mode
        # is_async = kwargs.pop('mode', None) == 'async'
        mode = cv.DVZ_EVENT_MODE_SYNC #  if is_async else cv.DVZ_EVENT_MODE_SYNC
        ctrl = _CONTROLS.get(ctype, 0)
        cdef char* c_name = name

        cdef cv.DvzGuiControl* c
        cdef cv.vec2 vec2_value

        if (ctype == 'slider_float'):
            c_vmin = kwargs.get('vmin', 0)
            c_vmax = kwargs.get('vmax', 1)
            c_value = kwargs.get('value', (c_vmin + c_vmax) / 2.0)
            c = cv.dvz_gui_slider_float(self._c_gui, c_name, c_vmin, c_vmax, c_value)
        elif (ctype == 'slider_float2'):
            c_vmin = kwargs.get('vmin', 0)
            c_vmax = kwargs.get('vmax', 1)
            c_value = kwargs.get('value', (c_vmin, c_vmax))
            c_force = kwargs.get('force_increasing', False)
            vec2_value[0] = c_value[0]
            vec2_value[1] = c_value[1]
            c = cv.dvz_gui_slider_float2(self._c_gui, c_name, c_vmin, c_vmax, vec2_value, c_force)
        elif (ctype == 'slider_int'):
            c_vmin = kwargs.get('vmin', 0)
            c_vmax = kwargs.get('vmax', 1)
            c_value = kwargs.get('value', c_vmin)
            c = cv.dvz_gui_slider_int(self._c_gui, c_name, c_vmin, c_vmax, c_value)
        elif (ctype == 'input_float'):
            c_step = kwargs.get('step', .1)
            c_step_fast = kwargs.get('step_fast', 1)
            c_value = kwargs.get('value', 0)
            c = cv.dvz_gui_input_float(self._c_gui, c_name, c_step, c_step_fast, c_value)
        elif (ctype == 'checkbox'):
            c_value = kwargs.get('value', 0)
            c = cv.dvz_gui_checkbox(self._c_gui, c_name, c_value)
        elif (ctype == 'button'):
            c = cv.dvz_gui_button(self._c_gui, c_name, 0)
        elif (ctype == 'label'):
            c_value = kwargs.get('value', "")
            c = cv.dvz_gui_label(self._c_gui, c_name, c_value)

        # Gui control object
        w = GuiControl()
        w.create(self._c_gui, c, ctype)
        self._controls[name] = w

        def wrap(f):
            cdef cv.DvzEventType evtype
            evtype = cv.DVZ_EVENT_GUI
            _add_event_callback(self._c_canvas, evtype, 0, f, (name,), mode=mode)

        return wrap

    def get_value(self, name):
        if name not in self._controls:
            raise ValueError("unknown control %s", name)
        c = self._controls[name]
        return c.get()

    def set_value(self, name, value):
        if name not in self._controls:
            raise ValueError("unknown control %s", name)
        c = self._controls[name]
        return c.set(value)
