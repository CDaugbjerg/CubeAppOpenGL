#version 330 core
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoords;

out v2f 
{
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} OUT;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;


void main()
{
	gl_Position = projection * view * model * vec4(inPos, 1.0f);

    OUT.FragPos = vec3(model * vec4(inPos, 1.0));
    OUT.Normal = mat3(transpose(inverse(model))) * inNormal;  
    OUT.TexCoords = inTexCoords;
    OUT.FragPosLightSpace = lightSpaceMatrix * vec4(OUT.FragPos, 1.0);
}