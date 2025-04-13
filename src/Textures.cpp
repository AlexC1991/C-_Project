#include "Textures.h"
#include <iostream>
#include <fstream>
#include <direct.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Static members initialization
unsigned int Texture::activeBindings = 0;
unsigned int Texture::totalBindCalls = 0;
bool Texture::debugEnabled = false;

void Texture::EnableDebug(bool enable) {
    debugEnabled = enable;
    if (enable) {
        std::cout << "Texture debug enabled" << std::endl;
        ResetBindStats();
    } else {
        std::cout << "Texture debug disabled" << std::endl;
    }
}

bool Texture::IsDebugEnabled() {
    return debugEnabled;
}

void Texture::ResetBindStats() {
    activeBindings = 0;
    totalBindCalls = 0;
    std::cout << "Texture bind stats reset" << std::endl;
}

void Texture::PrintBindStats() {
    std::cout << "Texture Stats: " << std::endl;
    std::cout << "  Total bind calls: " << totalBindCalls << std::endl;
    std::cout << "  Currently bound textures: " << activeBindings << std::endl;
}

Texture::Texture(const char* path) {
    texturePath = path;
    std::cout << "Attempting to load texture from: " << path << std::endl;

    // Check if file exists
    std::ifstream file(path);
    if (!file.good()) {
        std::cerr << "Texture file does not exist or cannot be opened: " << path << std::endl;
        throw std::runtime_error("Texture file not found: " + std::string(path));
    }

    // Print current working directory for debugging
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        std::cout << "Current working directory: " << cwd << std::endl;
    }

    // Generate texture
    glGenTextures(1, &ID);

    // Load texture from file
    if (!LoadFromFile(path)) {
        // Create a default texture if loading fails
        CreateDefaultTexture();
    }
}

Texture::~Texture() {
    if (ID != 0) {
        if (debugEnabled && isBound) {
            std::cout << "Warning: Deleting texture that is still bound: " << texturePath << std::endl;
            activeBindings--;
        }
        glDeleteTextures(1, &ID);
        ID = 0;
    }
}

void Texture::Bind(unsigned int slot) const {
    if (debugEnabled) {
        totalBindCalls++;

        // Check if already bound to this slot
        if (isBound && lastBoundSlot == slot) {
            std::cout << "Warning: Texture already bound to slot " << slot << ": " << texturePath << std::endl;
            return; // Skip redundant binding
        }

        // If bound to a different slot, update tracking
        if (isBound) {
            std::cout << "Rebinding texture from slot " << lastBoundSlot << " to " << slot << ": " << texturePath << std::endl;
        } else {
            std::cout << "Binding texture to slot " << slot << ": " << texturePath << std::endl;
            activeBindings++;
        }
    }

    // Actual binding
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Update tracking
    isBound = true;
    lastBoundSlot = slot;

    // Verify binding worked
    if (debugEnabled) {
        GLint currentTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
        if ((unsigned int)currentTexture != ID) {
            std::cerr << "Error: Texture binding failed. Expected: " << ID
                      << ", Got: " << currentTexture << std::endl;
        }
    }
}

void Texture::Unbind() const {
    if (!isBound) {
        if (debugEnabled) {
            std::cout << "Warning: Attempting to unbind texture that is not bound: " << texturePath << std::endl;
        }
        return;
    }

    glActiveTexture(GL_TEXTURE0 + lastBoundSlot);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (debugEnabled) {
        std::cout << "Unbinding texture from slot " << lastBoundSlot << ": " << texturePath << std::endl;
        activeBindings--;
    }

    isBound = false;
}

bool Texture::LoadFromFile(const char* path) {
    // Bind texture
    glBindTexture(GL_TEXTURE_2D, ID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load image
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // Flip y-axis during loading
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        // Determine format based on number of channels
        GLenum internalFormat, dataFormat;
        if (nrChannels == 1) {
            internalFormat = GL_RED;
            dataFormat = GL_RED;
        }
        else if (nrChannels == 3) {
            internalFormat = GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrChannels == 4) {
            internalFormat = GL_RGBA;
            dataFormat = GL_RGBA;
        }
        else {
            std::cerr << "Unsupported number of channels: " << nrChannels << std::endl;
            stbi_image_free(data);
            return false;
        }

        // Generate texture
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Free image data
        stbi_image_free(data);

        if (debugEnabled) {
            std::cout << "Successfully loaded texture: " << path << std::endl;
            std::cout << "  Dimensions: " << width << "x" << height << std::endl;
            std::cout << "  Channels: " << nrChannels << std::endl;
            std::cout << "  Format: " << (nrChannels == 1 ? "RED" : (nrChannels == 3 ? "RGB" : "RGBA")) << std::endl;
        }

        return true;
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
        return false;
    }
}

void Texture::CreateDefaultTexture() {
    // Create a simple 2x2 checkerboard texture
    unsigned char checkerboard[] = {
        255, 0, 255, 255,   0, 0, 0, 255,
        0, 0, 0, 255,   255, 0, 255, 255
    };

    glBindTexture(GL_TEXTURE_2D, ID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Generate texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, checkerboard);

    if (debugEnabled) {
        std::cout << "Created default checkerboard texture" << std::endl;
    }
}

unsigned int Texture::GetID() const {
    return ID;
}

bool Texture::IsValid() const {
    return ID != 0;
}