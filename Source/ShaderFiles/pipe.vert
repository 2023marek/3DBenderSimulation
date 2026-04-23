#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 MVP;
uniform mat4 Model;
uniform mat3 NormalMatrix;

out VS_OUT {
    vec3 normal;
    vec3 fragPos;
} vs_out;

void main()
{
    gl_Position = MVP * vec4(aPos, 1.0);
    vs_out.normal = normalize(NormalMatrix * aNormal);
    vs_out.fragPos = vec3(Model * vec4(aPos, 1.0));
}