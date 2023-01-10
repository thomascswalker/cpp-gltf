#include "gltf.h"

bool GLTF::getOpenFilename(std::string& filename) {
#ifdef _WIN32 // Boilerplate windows code
    LPCSTR typeFilter;
    OPENFILENAME ofn = { 0 };
    TCHAR szFile[256] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = GLTF_FILE_FILTER; // Accept either .gltf or .glb
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
    filename = std::string([[nsurl path]UTF8String] );
    return true;
#endif

    return false;
}

void GLTF::openBinaryFile(const std::string& filename, std::vector<char>& buffer) {
    /// Open file
    std::cout << "Opening file " << filename << "..." << std::endl;
    std::ifstream file(filename, std::ios::binary);    // Read as binary file

    // Get file size
    file.seekg(0, std::ios::end);                // Move to end
    std::streamsize fileSize = file.tellg();    // Get byte position at this point
    buffer.resize(fileSize);
    file.seekg(0, std::ios::beg);                // Move back to beginning
    file.read(&buffer.at(0), fileSize);            // Read all contents into memory into the address at the

    std::cout << "File is " << fileSize << " bytes" << std::endl;

    // beginning of the buffer
    std::cout << "Closing file " << filename << std::endl;
    file.close();
}

bool GLTF::parseBinary(std::vector<char>& buffer) {
    for (int i = 0; i < buffer.size(); i++) {
        std::cout << buffer[i] << std::endl;
    }
    return false;
}

bool GLTF::loadGltf(std::vector<int>& indices, std::vector<Vector3>& positions) {
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
        // .gltf is a JSON file itself, with a companion .bin file for the geometry.
        // We can load the .gltf file itself as a JSON object
        json = JSON::loadFile(filename);

        // We'll need to get the companion .bin file, which is in the 'buffers' element
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
        ptr++; // Go to next four bytes
        auto version = *ptr;
        if (version == 2) {
            std::cout << "Valid version" << std::endl;
        } else {
            std::string msg("Invalid version: " + std::to_string(version));
            throw FileReadError(msg);
        }

        // Last four bytes should match the file size of the .glb file
        ptr++; // Go to next four bytes
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
        auto chunk = reinterpret_cast<const char *>(&buffer.at(20));
        std::string jsonString(chunk, jsonSize);
        json = JSON::loadString(jsonString);

        // Extract the rest of the binary data
        auto dataOffset = 20 + jsonSize;
        auto dataSize = fileSize - dataOffset;
        std::vector<char> temp(buffer.begin() + dataOffset, buffer.end());
        buffer = temp;
        
    } else {
        // If we've somehow gotten here, we've got the wrong filetype
        std::string msg("Unable to read file of type " + filetype);
        throw FileReadError(msg);
    }

    std::cout << json.format() << std::endl;

    // https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.pdf

    auto meshes = json["meshes"];
    for (int i = 0; i < meshes.size(); i++) {
        auto mesh = json["meshes"][i];
        auto primitives = mesh["primitives"];
        for (int j = 0; j < primitives.size(); j++) {
            auto primitive = mesh["primitives"][j];

            // Extract buffer entities
            std::map<std::string, int> entities;
            for (auto& [k, v] : (JSON::JsonDict) primitive["attributes"]) {
                entities[k] = (int) v;
            }
            if (primitive.hasKey("indices")) {
                entities["indices"] = (int) primitive["indices"];
            }
            if (primitive.hasKey("material")) {
                entities["material"] = (int) primitive["material"];
            }
            if (primitive.hasKey("mode")) {
                entities["mode"] = (int) primitive["mode"];
            }

            for (auto& [k, v] : entities) {
                std::cout << k << ": " << std::to_string(v) << std::endl;
            }
        }
    }
    //bool result = parseBinary(buffer);
    return true;
}