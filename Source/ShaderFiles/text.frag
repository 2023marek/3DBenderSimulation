#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D fontTex;
uniform vec4 textColor;

void main()
{
    vec4 sampled = texture(fontTex, TexCoord);
    FragColor = textColor * sampled;
}
