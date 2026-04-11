#version 330 core
out vec4 FragColor;

in vec2 TexCoords; // receives UVs from vertex shader

uniform vec3 objectColor;
uniform sampler2D mytexture;
uniform bool hasTexture;

void main()
{
    if (hasTexture) {
        FragColor = texture(mytexture, TexCoords) * vec4(objectColor, 1.0f);
    } else {
        FragColor = vec4(objectColor, 1.0f);
    }
}