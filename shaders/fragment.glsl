// Fragment shader
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;
uniform bool useTexture;

void main() {
    if (useTexture) {
        vec4 texColor = texture(texture1, TexCoord);

        // Check if texture coordinates are valid
        if (TexCoord.x < 0.0 || TexCoord.x > 1.0 || TexCoord.y < 0.0 || TexCoord.y > 1.0) {
            FragColor = vec4(1.0, 0.0, 1.0, 1.0); // Magenta for debugging
        } else {
            FragColor = texColor;
        }
    } else {
        FragColor = vec4(0.8, 0.8, 0.8, 1.0); // Default color
    }
}