#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords; // pass UVs to the fragment shader
out vec3 Normal;
out vec3 FragPos;

uniform mat4 transform; // handles position, rotation, and scaling
uniform mat4 view; // view from camera
uniform mat4 projection; // maps the world to the viewport

void main()
{

    // world space position
    FragPos = vec3(transform * vec4(aPos, 1.0));

    // prevent non-uniform scaling from distorting normals
    Normal = mat3(transpose(inverse(transform))) * aNormal;

    gl_Position = projection * view* vec4(FragPos, 1.0);
    TexCoords = aTexCoords;
}