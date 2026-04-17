#version 460

layout (push_constant) uniform CameraData {
    mat4 view_projection;
} camera_data;

const vec3 cubeCorners[8] = vec3[8](
vec3(0, 0, 0), vec3(1, 0, 0), vec3(1, 1, 0), vec3(0, 1, 0),
vec3(0, 0, 1), vec3(1, 0, 1), vec3(1, 1, 1), vec3(0, 1, 1)
);

const uint cubeIndices[36] = uint[36](
0, 1, 2, 0, 2, 3,
4, 7, 6, 4, 6, 5,
0, 3, 7, 0, 7, 4,
1, 5, 6, 1, 6, 2,
3, 2, 6, 3, 6, 7,
0, 4, 5, 0, 5, 1
);

layout (location = 0) out vec3 out_color;

void main() {
    uint vert = cubeIndices[gl_VertexIndex];
    vec3 localPos = cubeCorners[vert];

    gl_Position = camera_data.view_projection * vec4(localPos, 1.0);
    out_color = vec3(localPos.x, localPos.y, localPos.z);
}
