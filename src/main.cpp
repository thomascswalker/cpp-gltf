#include <iostream>
#include "gltf.h"

using namespace GLTF;

int main() {
    std::vector<int> indices;
    std::vector<Vector3> positions;
    bool result = loadGltf(indices, positions);
    return 0;
}
