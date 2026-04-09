#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 transform; // handles position, rotation, and scaling
uniform mat4 projection; // maps the world to the viewport
uniform mat4 view; // view from camera

void main()
{
    gl_Position = projection * view* transform * vec4(aPos, 1.0);
}