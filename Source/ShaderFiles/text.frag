const char* textFrag = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D fontTex;
uniform vec4 textColor;

void main()
{
    vec4 sampleColor = texture(fontTex, TexCoord);
    float alpha = sampleColor.a;

    FragColor = vec4(textColor.rgb, textColor.a * alpha);
}
)";