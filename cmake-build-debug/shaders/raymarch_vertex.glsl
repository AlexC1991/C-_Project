#version 330 core
layout (location = 0) in vec2 aPos; // Simple 2D position for the quad vertices

out vec2 TexCoords; // Pass texture coordinates (or screen coords) to fragment shader

void main()
{
    // Output vertex position directly in Normalized Device Coordinates (NDC)
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);

    // Calculate UV coordinates (0.0 to 1.0) based on NDC position
    TexCoords = aPos * 0.5 + 0.5;
}