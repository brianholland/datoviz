#version 450
#include "common.glsl"

layout (std140, binding = USER_BINDING) uniform Params {
    mat4 lights_pos_0; // lights 0-3
    mat4 lights_params_0; // for each light, coefs for ambient, diffuse, specular
    vec4 view_pos;
    vec4 tex_coefs; // blending coefficients for the textures
    vec4 clip_coefs;
} params;

layout(binding = (USER_BINDING+1)) uniform sampler2D tex_0;
layout(binding = (USER_BINDING+2)) uniform sampler2D tex_1;
layout(binding = (USER_BINDING+3)) uniform sampler2D tex_2;
layout(binding = (USER_BINDING+4)) uniform sampler2D tex_3;

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in float in_clip;
layout (location = 4) in float in_alpha;

layout (location = 0) out vec4 out_color;

const float eps = .00001;

void main() {
    if (in_clip < -eps)
        discard;

    vec3 normal, light_dir, ambient, diffuse, view_dir, reflect_dir, specular, color;
    vec3 lpar, lpos;
    vec3 light_color = vec3(1); // TODO
    float diff, spec;

    normal = normalize(in_normal);
    out_color = vec4(0, 0, 0, 1);
    diffuse = vec3(0);
    specular = vec3(0);

    // Color.
    color = vec3(0);
    color += params.tex_coefs.x * texture(tex_0, in_uv).xyz;
    color += params.tex_coefs.y * texture(tex_1, in_uv).xyz;
    color += params.tex_coefs.z * texture(tex_2, in_uv).xyz;
    color += params.tex_coefs.t * texture(tex_3, in_uv).xyz;

    // Light position and params.
    for (int i = 0; i < 3; i++) {
        lpos = params.lights_pos_0[i].xyz;
        lpar = params.lights_params_0[i].xyz;

        // Light direction.
        light_dir = normalize(lpos - in_pos);

        // Ambient component.
        ambient = light_color;

        // Diffuse component.
        // HACK: normals on both faces
        diff = max(dot(light_dir, normal), 0.0);
        diff = max(diff, max(dot(light_dir, -normal), 0.0));
        diffuse = diff * light_color;

        // Specular component.
        view_dir = normalize(params.view_pos.xyz - in_pos);
        reflect_dir = reflect(-light_dir, normal);
        spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);
        specular = spec * light_color;

        // Total color.
        out_color.xyz += (lpar.x * ambient + lpar.y * diffuse + lpar.z * specular) * color;
    }

    out_color.a = in_alpha;
}
