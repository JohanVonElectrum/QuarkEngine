#version 450

layout(push_constant) uniform CameraData {
    mat4 view_projection;
} camera_data;

layout(location = 0) out vec3 out_color;

void main() {
    const vec2 positions[3] = vec2[](
        vec2(0.0, -0.5),
        vec2(0.5, 0.5),
        vec2(-0.5, 0.5)
    );

    const vec3 colors[3] = vec3[](
        vec3(1.0, 0.1, 0.1),
        vec3(0.1, 1.0, 0.1),
        vec3(0.1, 0.3, 1.0)
    );

    gl_Position = camera_data.view_projection * vec4(positions[gl_VertexIndex], 0.0, 1.0);
    out_color = colors[gl_VertexIndex];
}

