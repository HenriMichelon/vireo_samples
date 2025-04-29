/* Copyright (c) 2025-present Henri Michelon
*
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */
module;
#include "Libraries.h"
module samples.deferred.global;

namespace samples {

    bool LoadOBJ(const std::string& filepath, Mesh& outMesh) {
        std::vector<vec3> positions;
        std::vector<vec3> normals;
        std::vector<vec2> uvs;
        outMesh.vertices.clear();
        outMesh.indices.clear();

        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string tag;
            iss >> tag;
            if (tag == "v") {
                vec3 p;
                iss >> p.x >> p.y >> p.z;
                positions.push_back(p);
            }
            else if (tag == "vn") {
                vec3 n;
                iss >> n.x >> n.y >> n.z;
                normals.push_back(n);
            }
            else if (tag == "vt") {
                vec2 t;
                iss >> t.x >> t.y;
                uvs.push_back(t);
            }
            else if (tag == "f") {
                for (int i = 0; i < 3; ++i) {
                    std::string vertRef;
                    iss >> vertRef;              // ex: "12/34/56"
                    std::replace(vertRef.begin(), vertRef.end(), '/', ' ');
                    std::istringstream viss(vertRef);

                    uint32_t vi = 0, uvi = 0, ni = 0;
                    viss >> vi >> uvi >> ni;

                    Vertex v{};
                    if (vi  > 0 && vi  <= positions.size()) v.position  = positions[vi - 1];
                    if (uvi > 0 && uvi <= uvs      .size()) v.uv   = uvs[uvi - 1];
                    if (ni  > 0 && ni  <= normals .size()) v.normal = normals[ni - 1];

                    outMesh.indices.push_back(static_cast<uint32_t>(outMesh.vertices.size()));
                    outMesh.vertices.emplace_back(v);
                }
            }
            // xx other tags
        }
        return true;
    }
}