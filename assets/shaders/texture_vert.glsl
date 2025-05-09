#version 330 core

layout(location = 0) in vec2 aPos;   // [-1,1] quad coordinates
layout(location = 1) in vec2 aUV;    // 0–1 texture coordinates

uniform mat4 uProjection;
uniform vec2 uOffset;   // screen-space center position (pixels)
uniform vec2 uSize;     // half-size in pixels (width/2, height/2)

out vec2 TexCoord;

void main() {
    vec2 scaledPos = aPos * uSize;
    vec2 worldPos = scaledPos + uOffset;
    gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);
    TexCoord = aUV;
}
