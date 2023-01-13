#include <iostream>

#include "gltf.h"

int main() {
    GLTF::GltfObject<int, double> model;
    bool result = GLTF::loadGltf(model);

    std::cout << "Result: " << result << std::endl;
    std::cout << "Indices: " << model.indices.size() << std::endl;
    std::cout << "Vertices: " << (model.positions.size() / 3) << std::endl;

    return 0;
}
                               