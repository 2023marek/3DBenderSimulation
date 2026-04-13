#version 330 core

in VS_OUT {
    vec3 normal;
    vec3 fragPos;
} fs_in;

uniform vec3 objectColor = vec3(0.05, 0.05, 0.0);
uniform vec3 lightPos = vec3(10.0, 10.0, 10.0);
uniform vec3 viewPos = vec3(0.0, 0.0, 0.0);

out vec4 FragColor;

void main()
{
    // Ambient
    vec3 ambient = 0.3 * objectColor;
    
    // Diffuse
    vec3 norm = normalize(fs_in.normal);
    vec3 lightDir = normalize(lightPos - fs_in.fragPos);
    float diff = max(dot(norm, lightDir), 0.1);
    vec3 diffuse = diff * objectColor;
    
    // Specular
    vec3 viewDir = normalize(viewPos - fs_in.fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * vec3(1.0);
    
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}