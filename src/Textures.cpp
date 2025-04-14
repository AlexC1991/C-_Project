#include "Textures.h"
#include <iostream>
#include "stb_image.h"
#include <chrono>

// Initialize static members
bool Texture::debugEnabled = false;
int Texture::totalBindCalls = 0;
int Texture::activeBindings = 0;
std::unordered_map<std::string, unsigned int> Texture::textureCache;
std::unordered_map<std::string, std::filesystem::file_time_type> Texture::textureTimestamps;

Texture::Texture(const char* path, bool useCache) : ID(0), isBound(false), lastBoundSlot(0) {
    texturePath = path;

    // Check if file exists
    if (!std::filesystem::exists(path)) {
        std::cerr << "Texture file does not exist: " << path << std::endl;
        throw std::runtime_error("Failed to find texture file");
    }

    // Get current file timestamp
    auto currentTimestamp = std::filesystem::last_write_time(path);

    // Check if texture is already in cache and we want to use cache
    if (useCache) {
        auto it = textureCache.find(path);
        auto timeIt = textureTimestamps.find(path);

        if (it != textureCache.end() && timeIt != textureTimestamps.end()) {
            // If file hasn't been modified, use cached texture
            if (timeIt->second == currentTimestamp) {
                std::cout << "Using cached texture: " << path << " (ID: " << it->second << ")" << std::endl;
                ID = it->second;
                return;
            } else {
                std::cout << "Texture file modified, reloading: " << path << std::endl;
                // File modified, need to reload
                if (it->second != 0) {
                    glDeleteTextures(1, &it->second);
                }
                textureCache.erase(it);
            }
        }
    }

    // Update timestamp
    textureTimestamps[path] = currentTimestamp;

    // Load the texture data
    if (!LoadTextureData(path, useCache)) {
        throw std::runtime_error("Failed to load texture data");
    }
}

bool Texture::LoadTextureData(const char* path, bool addToCache) {
    // Load image
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // Flip images vertically

    // Force stb_image to reload from disk
    stbi_image_free(nullptr); // This helps reset some internal stb_image state
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;
        else {
            std::cerr << "Unsupported number of channels: " << nrChannels << std::endl;
            format = GL_RGB;
        }

        // Generate texture
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Load texture data
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Add to cache if caching is enabled
        if (addToCache) {
            textureCache[path] = ID;
        }

        // Print texture info
        std::cout << "Successfully loaded texture: " << path << std::endl;
        std::cout << "  Dimensions: " << width << "x" << height << std::endl;
        std::cout << "  Channels: " << nrChannels << std::endl;
        std::cout << "  Format: " << (format == GL_RGB ? "RGB" : (format == GL_RGBA ? "RGBA" : "RED")) << std::endl;
        std::cout << "  Texture ID: " << ID << std::endl;

        stbi_image_free(data);
        return true;
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
        return false;
    }
}

Texture::~Texture() {
    // Don't delete the texture ID here, as it might be shared in the cache
    // The cache will be cleared when the application exits
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

        // Check texture dimensions
        GLint width, height;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        std::cout << "Bound texture dimensions: " << width << "x" << height << std::endl;

        // Check texture parameters
        GLint wrapS, wrapT, minFilter, magFilter;
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrapS);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrapT);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minFilter);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &magFilter);

        std::cout << "Texture parameters: Wrap S: " << wrapS << ", Wrap T: " << wrapT
                  << ", Min Filter: " << minFilter << ", Mag Filter: " << magFilter << std::endl;
    }
}

void Texture::Unbind() const {
    if (debugEnabled && isBound) {
        std::cout << "Unbinding texture from slot " << lastBoundSlot << ": " << texturePath << std::endl;
        activeBindings--;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    isBound = false;
}

void Texture::EnableDebug(bool enable) {
    debugEnabled = enable;
    std::cout << "Texture debug " << (enable ? "enabled" : "disabled") << std::endl;

    if (enable) {
        // Reset stats when enabling debug
        totalBindCalls = 0;
        activeBindings = 0;
        std::cout << "Texture bind stats reset" << std::endl;
    }
}

void Texture::PrintBindStats() {
    if (debugEnabled) {
        std::cout << "Texture Stats:" << std::endl;
        std::cout << "  Total bind calls: " << totalBindCalls << std::endl;
        std::cout << "  Currently bound textures: " << activeBindings << std::endl;
        std::cout << "  Cached textures: " << textureCache.size() << std::endl;
    }
}

bool Texture::Reload(const char* path) {
    // Delete the old texture if it's not shared
    if (ID != 0) {
        // Check if other textures are using this ID
        bool shared = false;
        for (const auto& pair : textureCache) {
            if (pair.first != texturePath && pair.second == ID) {
                shared = true;
                break;
            }
        }

        if (!shared) {
            glDeleteTextures(1, &ID);
        }
        ID = 0;
    }

    // Remove from cache
    textureCache.erase(texturePath);
    textureTimestamps.erase(texturePath);

    // Update path
    texturePath = path;

    try {
        // Check if file exists
        if (!std::filesystem::exists(path)) {
            std::cerr << "Texture file does not exist: " << path << std::endl;
            return false;
        }

        // Update timestamp
        textureTimestamps[path] = std::filesystem::last_write_time(path);

        // Load the texture data (don't add to cache yet)
        if (!LoadTextureData(path, true)) {
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to reload texture: " << e.what() << std::endl;
        return false;
    }
}

void Texture::ClearCache() {
    // Delete all textures in the cache
    for (auto& pair : textureCache) {
        if (pair.second != 0) {
            glDeleteTextures(1, &pair.second);
        }
    }

    // Clear the cache
    textureCache.clear();
    textureTimestamps.clear();
    std::cout << "Texture cache cleared" << std::endl;
}

void Texture::ForceReloadAll() {
    // Clear all textures
    ClearCache();

    // Clear stb_image internal cache
    stbi_image_free(nullptr);

    std::cout << "Forced reload of all textures" << std::endl;
}