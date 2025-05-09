#version 330 core

in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec4 uColor;

out vec4 FragColor;

void main() {
    vec4 tex = texture(texture1, TexCoord);
    FragColor = tex * uColor;
}
