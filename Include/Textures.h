#pragma once
#ifndef TEXTURES_H
#define TEXTURES_H

#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include <iostream>
#include <filesystem>

class Texture {
public:
    // Constructor with option to disable caching
    Texture(const char* path, bool useCache = true);
    ~Texture();

    void Bind(unsigned int slot) const;
    void Unbind() const;

    // Add reload method
    bool Reload(const char* path);

    // Static methods for debugging
    static void EnableDebug(bool enable);
    static void PrintBindStats();

    // Static method to clear texture cache
    static void ClearCache();

    // Static method to force reload all textures
    static void ForceReloadAll();

    // Static variables for tracking
    static int totalBindCalls;
    static int activeBindings;
    unsigned int ID;
    // Static texture cache
    static std::unordered_map<std::string, unsigned int> textureCache;

    // Static timestamp cache for file modification tracking
    static std::unordered_map<std::string, std::filesystem::file_time_type> textureTimestamps;

private:

    std::string texturePath;
    mutable bool isBound;
    mutable unsigned int lastBoundSlot;

    static bool debugEnabled;

    // Helper method to load texture data
    bool LoadTextureData(const char* path, bool addToCache);
};

#endif // TEXTURES_H