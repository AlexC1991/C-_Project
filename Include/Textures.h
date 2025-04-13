#pragma once
#include <GL/glew.h>
#include <string>

class Texture {
public:
    Texture(const char* path);
    ~Texture();

    void Bind(unsigned int slot = 0) const;
    void Unbind() const;

    unsigned int GetID() const;
    bool IsValid() const;
    bool IsCurrentlyBound() const { return isBound; }
    const std::string& GetPath() const { return texturePath; }
    static unsigned int totalBindCalls;
    // Debug methods
    static void EnableDebug(bool enable = true);
    static bool IsDebugEnabled();
    static void PrintBindStats();

private:
    unsigned int ID = 0;
    std::string texturePath;

    // Binding state tracking
    mutable bool isBound = false;
    mutable unsigned int lastBoundSlot = 0;

    // Static tracking for all textures
    static unsigned int activeBindings;
    static bool debugEnabled;

    // Helper methods
    bool LoadFromFile(const char* path);
    void CreateDefaultTexture();
    static void ResetBindStats();
};