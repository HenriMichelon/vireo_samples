/*
* Copyright (c) 2025-present Henri Michelon
*
* This software is released under the MIT License.
* https://opensource.org/licenses/MIT
*/
struct Global {
    float3   cameraPosition;
    float4x4 projection;
    float4x4 view;
    float4x4 viewInverse;
    float4   ambientLight;
}

struct Model {
    float4x4 transform;
}

struct Light {
    float3 direction;
    float4 color;
}

struct Material {
    float shininess;
    int   diffuseTextureIndex;
    int   normalTextureIndex;
    int   aoTextureIndex;
}
