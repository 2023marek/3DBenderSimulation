#version 330 core

layout (location = 0) in vec2 aPos;

uniform vec2 uScreenSize;
uniform vec4 uRect; // x, y, width, height

void main()
{
    vec2 pos = uRect.xy + aPos * uRect.zw;

    vec2 ndc;
    ndc.x = (pos.x / uScreenSize.x) * 2.0 - 1.0;
    ndc.y = 1.0 - (pos.y / uScreenSize.y) * 2.0;

    gl_Position = vec4(ndc, 0.0, 1.0);
}