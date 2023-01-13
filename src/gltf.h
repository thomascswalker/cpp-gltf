#ifndef GLTF_H
#define GLTF_H

#ifdef _WIN32
#include "windows.h"
#elif defined __APPLE__
// How do you do the equivalent of GetOpenFilename?
#endif

#include <exception>
#include <map>
#include <string>
#include <variant>
#include <vector>

#include "json.h"

namespace GLTF {

typedef char byte;
typedef unsigned char unsigned_byte;

constexpr auto GLTF_FILE_FILTER = "glTF Files (.gltf, .glb)\0*.gltf;*.glb\0";
constexpr auto GL_SIGNED_BYTE = 5120;
constexpr auto GL_UNSIGNED_BYTE = 5121;
constexpr auto GL_SIGNED_SHORT = 5122;
constexpr auto GL_UNSIGNED_SHORT = 5123;
constexpr auto GL_UNSIGNED_INT = 5125;
constexpr auto GL_FLOAT = 5126;

// Variant for binary storage types
typedef std::variant<byte, unsigned_byte, short, unsigned short, unsigned int, float> value_t;

// Map of Component Type to number of values expected
static std::map<std::string, int> GL_COMPONENT_TYPE = {{"SCALAR", 1}, {"VEC2", 2}, {"VEC3", 3}, {"VEC4", 4},
                                                       {"MAT2", 4},   {"MAT3", 9}, {"MAT4", 16}};

class FileReadError : public std::exception {
    std::string m_message = "File read error!";

   public:
    FileReadError() { };
    FileReadError(std::string message) : m_message(message){};
    using std::exception::what;
    const char* what() { return m_message.c_str(); }
    ;
};

// Struct for GLTF accessors
struct Accessor {
    std::string name;
    int bufferView = 0;
    int byteOffset = 0;
    int byteStride = 0;
    int componentType = 0;  // SIGNED_BYTE, UNSIGNED_BYTE, etc.
    int count = 0;
    std::vector<int> max;
    std::vector<int> min;
    int type = 0;  // SCALAR, VEC2, VEC3, etc.
};

// Struct for holding abstract Gltf indices, positions, and other data
template <typename indices_t, typename positions_t>
struct GltfObject {
    std::vector<indices_t> indices;
    std::vector<positions_t> positions;
};

/// <summary>
/// Calls the OS-specific 'get open file' dialog.
/// </summary>
/// <param name="filename">The output filename.</param>
/// <returns>Whether the filename was retrieved or not.</returns>
bool getOpenFilename(std::string& filename) {
#ifdef _WIN32  // Boilerplate windows code
    OPENFILENAME ofn = {0};
    TCHAR szFile[256] = {0};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = GLTF_FILE_FILTER;  // Accept either .gltf or .glb
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn) == TRUE) {
        std::string dest(ofn.lpstrFile);
        filename = dest;
        return true;
    }

#elif defined __APPLE__
    // https://stackoverflow.com/questions/22068159/launching-an-open-dialog-on-mac-osx-from-c
    filename = std::string([[nsurl path] UTF8String]);
    return true;
#endif

    return false;
};

/// <summary>
/// Reads a binary file into memory in the form of a char array.
/// </summary>
/// <param name="filename">The file to read.</param>
/// <param name="buffer">The resulting char array.</param>
void openBinaryFile(const std::string& filename, std::vector<char>& buffer) {
    /// Open file
    std::cout << "Opening file " << filename << "..." << std::endl;
    std::ifstream file(filename, std::ios::binary);  // Read as binary file

    // Get file size
    file.seekg(0, std::ios::end);             // Move to end
    std::streamsize fileSize = file.tellg();  // Get byte position at this point
    buffer.resize(fileSize);
    file.seekg(0, std::ios::beg);  // Move back to beginning
    file.read(&buffer.at(0),
              fileSize);  // Read all contents into memory into the address at the

    std::cout << "File is " << fileSize << " bytes" << std::endl;

    // beginning of the buffer
    std::cout << "Closing file " << filename << std::endl;
    file.close();
};

/// <summary>
/// Given the JSON index for the accessor and the JSON object itself, retrieve
/// the respective accessor's properties.
/// - bufferView
/// - byteOffset
/// - byteStride
/// - componentType
/// - count
/// - type
/// </summary>
/// <param name="index">The accessor index.</param>
/// <param name="json">The JSON object to visit.</param>
/// <returns>The accessor (as a struct) with its respective values.</returns>
Accessor parseAccessor(int index, JSON::JsonObject& json) {
    Accessor accessor;
    auto accessorJson = json["accessors"][index];

    accessor.bufferView = (int)accessorJson["bufferView"];
    accessor.byteOffset = (int)accessorJson["byteOffset"];

    if (accessorJson.hasKey("byteStride")) {
        accessor.byteStride = (int)accessorJson["byteStride"];
    }

    accessor.componentType = (int)accessorJson["componentType"];
    accessor.count = (int)accessorJson["count"];

    std::string type = (std::string)accessorJson["type"];
    accessor.type = GL_COMPONENT_TYPE[type];

    return accessor;
};

/// <summary>
/// Given an accessor, parse its respective data from the given binary buffer.
/// </summary>
/// <typeparam name="T">The type the binary data is stored as.</typeparam>
/// <param name="accessor">The accessor to get values from.</param>
/// <param name="buffer">The buffer to retrieve data from.</param>
/// <returns>A vector of the data type the buffer was stored as.</returns>
template <typename T>
std::vector<T> parseEntity(Accessor accessor, std::vector<char>& buffer) {
    auto ptr = reinterpret_cast<T*>(&buffer.at(accessor.byteOffset));
    std::vector<T> arr;
    int i = 0;
    while (i < accessor.count) {
        for (int i = 0; i < accessor.type; i++) {
            arr.push_back(*ptr++);
        }
        i++;
    }
    return arr;
};

/// <summary>
/// Prompts a GetOpenFileName dialog and, with the resulting filename,
/// constructs a Gltf object. This includes reading the binary geometry
/// information and storing it in a relatively abstract way.
/// </summary>
/// <typeparam name="positions_t">Type to store position vector.</typeparam>
/// <typeparam name="indices_t">Type to store index vector.</typeparam>
/// <param name="gltf">The resulting Gltf object.</param>
/// <returns>Whether the file was read and parsed or not.</returns>
template <typename indices_t, typename positions_t>
bool loadGltf(GltfObject<indices_t, positions_t>& gltf) {
    // Initial variables
    JSON::JsonObject json;
    std::vector<char> buffer;
    size_t size;

    // Get the file to load
    std::string filename;
    if (!getOpenFilename(filename)) {
        std::cout << "No file selected." << std::endl;
        return false;
    }

    // Extract file type
    std::string filetype = filename.substr(filename.find_last_of(".") + 1);
    std::cout << "Loading " << filetype << " file: " << filename << std::endl;

    // Read JSON and BIN components into memory
    if (filetype == "gltf") {
        // .gltf is a JSON file itself, with a companion .bin file for the
        // geometry. We can load the .gltf file itself as a JSON object
        json = JSON::loadFile(filename);

        // We'll need to get the companion .bin file, which is in the 'buffers'
        // element
        std::string filepath = filename.substr(0, filename.find_last_of('\\'));
        std::string binFilename;
        binFilename = filepath + "\\" + (std::string)json["buffers"][0]["uri"];
        std::cout << "Companion .bin file: " << binFilename << std::endl;

        // Load the .bin file into our memory buffer
        openBinaryFile(binFilename, buffer);
    } else if (filetype == "glb") {
        // Otherwise we've loaded a .glb which has three components:
        // 1. The 12-byte header
        // 2. The JSON data
        // 3. One or more chunks of geometry data
        std::cout << ".glb loaded, no companion .bin file needed." << std::endl;

        // Load the .glb file into our memory buffer
        openBinaryFile(filename, buffer);
        size = buffer.size();

        // Read the header
        auto ptr = reinterpret_cast<unsigned int*>(&buffer.at(0));

        // First four bytes should spell out 'glTF'
        auto magic = std::string(reinterpret_cast<const char*>(ptr), 4);
        if (magic == "glTF") {
            std::cout << "Valid magic header" << std::endl;
        } else {
            std::string msg("Magic header malformed: " + magic);
            throw FileReadError(msg);
        }

        // Next four bytes should indicate the version, at the moment set to 2
        ptr++;  // Go to next four bytes
        auto version = *ptr;
        if (version == 2) {
            std::cout << "Valid version" << std::endl;
        } else {
            std::string msg("Invalid version: " + std::to_string(version));
            throw FileReadError(msg);
        }

        // Last four bytes should match the file size of the .glb file
        ptr++;  // Go to next four bytes
        auto fileSize = *ptr;
        if (fileSize == size) {
            std::cout << "Valid size" << std::endl;
        } else {
            std::string msg("Size mismatch; wanted " + std::to_string(size) + ", got " + std::to_string(fileSize));
            throw FileReadError(msg);
        }

        // Next we'll extract the JSON data
        ptr++;
        size_t jsonSize = *ptr;
        std::cout << "JSON is " << jsonSize << " bytes" << std::endl;

        // Extract JSON content as a string
        auto chunk = reinterpret_cast<const char*>(&buffer.at(20));
        std::string jsonString(chunk, jsonSize);
        json = JSON::loadString(jsonString);

        // Extract the rest of the binary data
        auto dataOffset = 20 + jsonSize;
        std::vector<char> temp(buffer.begin() + dataOffset, buffer.end());
        buffer = temp;
    } else {
        // If we've somehow gotten here, we've got the wrong filetype
        std::string msg("Unable to read file of type " + filetype);
        throw FileReadError(msg);
    }

    std::cout << json.format() << std::endl;

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.pdf
    std::map<std::string, int> entities;
    auto meshes = json["meshes"];
    for (int i = 0; i < meshes.size(); i++) {
        auto mesh = json["meshes"][i];
        auto primitives = mesh["primitives"];
        for (int j = 0; j < primitives.size(); j++) {
            auto primitive = mesh["primitives"][j];

            // Extract buffer entities where the 'value' in for these entities
            // corresponds to the index of an accessor which contains the
            // corresponding data.
            for (auto& [k, v] : (JSON::JsonDict)primitive["attributes"]) {
                entities[k] = (int)v;
            }
            if (primitive.hasKey("indices")) {
                entities["indices"] = (int)primitive["indices"];
            }

            // TODO: Implement mode, material entities
        }
    }

    // For each entity
    for (auto& [k, v] : entities) {
        // Parse the accessor data from the JSON
        Accessor accessor = parseAccessor(v, json);
        accessor.name = k;

        // Parse the data from the binary, given the Component Type
        std::vector<value_t> data;
        switch (accessor.componentType) {
            case GL_SIGNED_BYTE:
                for (auto& v : parseEntity<byte>(accessor, buffer)) {
                    data.emplace_back(v);
                }
                break;
            case GL_UNSIGNED_BYTE:
                for (auto& v : parseEntity<unsigned_byte>(accessor, buffer)) {
                    data.emplace_back(v);
                }
                break;
            case GL_SIGNED_SHORT:
                for (auto& v : parseEntity<short>(accessor, buffer)) {
                    data.emplace_back(v);
                }
                break;
            case GL_UNSIGNED_SHORT:
                for (auto& v : parseEntity<unsigned short>(accessor, buffer)) {
                    data.emplace_back(v);
                }
                break;
            case GL_UNSIGNED_INT:
                for (auto& v : parseEntity<unsigned int>(accessor, buffer)) {
                    data.emplace_back(v);
                }
                break;
            case GL_FLOAT:
                for (auto& v : parseEntity<float>(accessor, buffer)) {
                    data.emplace_back(v);
                }
                break;
            default:
                continue;
        }

        // Convert parsed index data to given index data type
        if (accessor.name == "indices") {
            for (auto& v : data) {
                switch (v.index()) {
                    case 0:
                        gltf.indices.emplace_back((indices_t)std::get<byte>(v));
                        break;
                    case 1:
                        gltf.indices.emplace_back((indices_t)std::get<unsigned_byte>(v));
                        break;
                    case 2:
                        gltf.indices.emplace_back((indices_t)std::get<short>(v));
                        break;
                    case 3:
                        gltf.indices.emplace_back((indices_t)std::get<unsigned short>(v));
                        break;
                    case 4:
                        gltf.indices.emplace_back((indices_t)std::get<unsigned int>(v));
                        break;
                    case 5:
                        gltf.indices.emplace_back((indices_t)std::get<float>(v));
                        break;
                    default:
                        break;
                }
            }
        }

        // Convert parsed position data to given position data type
        if (accessor.name == "POSITION") {
            for (auto& v : data) {
                switch (v.index()) {
                    case 0:  // Char
                        gltf.positions.emplace_back((positions_t)std::get<byte>(v));
                        break;
                    case 1:  // Unsigned char
                        gltf.positions.emplace_back((positions_t)std::get<unsigned_byte>(v));
                        break;
                    case 2:  // Short
                        gltf.positions.emplace_back((positions_t)std::get<short>(v));
                        break;
                    case 3:  // Unsigned short
                        gltf.positions.emplace_back((positions_t)std::get<unsigned short>(v));
                        break;
                    case 4:  // Unsigned int
                        gltf.positions.emplace_back((positions_t)std::get<unsigned int>(v));
                        break;
                    case 5:  // Float
                        gltf.positions.emplace_back((positions_t)std::get<float>(v));
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return true;
}
}

#endif  // !GLTF_H