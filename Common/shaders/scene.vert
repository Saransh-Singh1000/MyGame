#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform Viewport {
    vec2 size;
} viewport;

layout(location = 0) out vec4 color;

void main() {
    vec2 normalized = inPosition / viewport.size;
    gl_Position = vec4(normalized.x * 2.0 - 1.0, normalized.y * 2.0 - 1.0, 0.0, 1.0);
    color = inColor;
}
