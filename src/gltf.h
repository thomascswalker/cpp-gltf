#ifndef GLTF_H
#define GLTF_H

#ifdef _WIN32
#include "windows.h"
#elif defined __APPLE__
// How do you do the equivalent of GetOpenFilename?
#endif

#include <exception>
#include <string>
#include <vector>
#include "json.h"

constexpr auto GLTF_FILE_FILTER = "glTF Files (.gltf, .glb)\0*.gltf;*.glb\0";

namespace GLTF {

    class FileReadError
        : public std::exception {
        std::string m_message = "File read error!";
    public:
        FileReadError() = default;
        FileReadError(std::string message)
            : m_message(message) {
        };
        std::string what() {
            return m_message;
        }
    };

    struct Vector3 {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    enum AccessorDataTypes {
        GL_SIGNED_BYTE = 5120,
        GL_UNSIGNED_BYTE = 5121,
        GL_SIGNED_SHORT = 5122,
        GL_UNSIGNED_SHORT = 5123,
        GL_UNSIGNED_INT = 5125,
        GL_FLOAT = 5126,
    };

    enum AccessorComponentTypes {
        SCALAR
    };

    struct Accessor {
        int bufferView;
        int byteOffset;
        int componentType;
        int count;
        int max;
        int min;
        int type;
    };

    bool getOpenFilename(std::string& filename);
    void openBinaryFile(const std::string& filename, std::vector<char>& buffer);
    bool parseBinary(std::vector<char>& buffer);
    bool loadGltf(std::vector<int>& indices, std::vector<Vector3>& positions);
}

#endif