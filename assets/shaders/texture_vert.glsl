#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform vec2 uOffset;     // screen position
uniform vec2 uUVOffset;   // top-left UV for this sprite
uniform vec2 uUVScale;    // size of this sprite in UV space

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos + uOffset, 0.0, 1.0);
    TexCoord = uUVOffset + aUV * uUVScale;
}
