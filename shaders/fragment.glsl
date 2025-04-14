// Fragment shader
#version 330 core
out vec4 FragColor;

in vec3 vertexColor;
in vec2 TexCoord;

uniform sampler2D texture1;
uniform bool useTexture;

void main() {
    // Debug visualization - show texture coordinates as colors
    vec3 coordColor = vec3(TexCoord.x, TexCoord.y, 0.0);

    if (useTexture) {
        // Try to sample the texture
        vec4 texColor = texture(texture1, TexCoord);

        // Mix with vertex color for better visibility
        FragColor = mix(texColor, vec4(vertexColor, 1.0), 0.3);
    } else {
        // If no texture, show the texture coordinates as colors
        FragColor = vec4(coordColor, 1.0);
    }
}