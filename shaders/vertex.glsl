#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 transform; // handles position, rotation, and scaling
uniform mat4 view; // view from camera
uniform mat4 projection; // maps the world to the viewport

void main()
{
    gl_Position = projection * view* transform * vec4(aPos, 1.0);
}