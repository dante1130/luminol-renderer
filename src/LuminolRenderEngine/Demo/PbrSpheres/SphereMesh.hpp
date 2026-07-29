#pragma once

#include <cstdint>
#include <vector>

namespace Luminol::Demo::PbrSpheres {

struct SphereMesh {
    // Interleaved: position.xyz, uv.xy, normal.xyz, tangent.xyz (11
    // floats/vertex), matching the engine's fixed procedural vertex layout
    // (see SDL_GPUMesh.cpp's vertex_stride_in_floats).
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
};

// Builds a UV sphere of the given radius, centered at the origin, with
// latitude_segments rings and longitude_segments slices per ring.
[[nodiscard]] auto make_uv_sphere(
    float radius, uint32_t latitude_segments, uint32_t longitude_segments
) -> SphereMesh;

}  // namespace Luminol::Demo::PbrSpheres
