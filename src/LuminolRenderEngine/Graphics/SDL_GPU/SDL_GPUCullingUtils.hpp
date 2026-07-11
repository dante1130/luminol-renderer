#pragma once

#include <array>
#include <optional>
#include <unordered_map>
#include <vector>

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/BoundingBox.hpp>
#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBatch.hpp>
#include <LuminolRenderEngine/Utilities/ModelLoader.hpp>

namespace Luminol::Graphics::SDL_GPU {

class SDL_GPUFactory;
class SDL_GPUMesh;

// Memory layout must exactly match SDL_GPUIndexedIndirectDrawCommand (see
// SDL_gpu.h) - SDL_GPU only requires the buffer's bytes to match that
// layout, not the C++ type name, so this local struct avoids leaking a raw
// SDL3 include into callers.
struct IndirectDrawCommand {
    uint32_t num_indices;
    uint32_t num_instances;
    uint32_t first_index;
    int32_t vertex_offset;
    uint32_t first_instance;
};

struct IndirectDrawRange {
    uint32_t offset;
    uint32_t count;
};

// One world-space AABB per mesh (submesh) within a batch, covering the union
// of all of that batch's current-frame instances. Shared across every pass
// that culls the same frame's batches against a frustum (shadow faces, main
// camera, AO prepass), since it doesn't depend on which frustum is used.
using BatchMeshBounds = std::vector<std::vector<BoundingBox>>;

[[nodiscard]] auto compute_batch_mesh_world_bounds(
    const SDL_GPUFactory& graphics_factory,
    gsl::span<const InstanceBatch> instance_batches,
    const std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>&
        queued_draws
) -> BatchMeshBounds;

// World-space AABB for a single submesh, covering the union of every given
// instance transform. Lets callers compute bounds on demand for just the
// submeshes they need (e.g. only blend-mode submeshes in a batch that
// otherwise has no need for CPU-side bounds - see
// SDL_GPUMeshRenderPass::draw's transparent bin), rather than eagerly for
// every submesh in every batch like compute_batch_mesh_world_bounds.
[[nodiscard]] auto compute_mesh_world_bounds(
    const SDL_GPUMesh& mesh,
    gsl::span<const Maths::Matrix4x4f> model_matrices
) -> BoundingBox;

// Appends one IndirectDrawCommand per mesh in this batch that survives the
// frustum test (and, if alpha_mode_filter is set, matches that alpha mode),
// and records where in out_commands this batch's commands start/how many
// there are (0 if none survive - callers skip the indirect draw call
// entirely in that case).
auto append_batch_indirect_commands(
    const SDL_GPUFactory& graphics_factory,
    const InstanceBatch& batch,
    const std::vector<BoundingBox>& mesh_bounds,
    const std::array<Maths::Vector4f, 6>& frustum_planes,
    std::optional<Utilities::ModelLoader::AlphaMode> alpha_mode_filter,
    std::vector<IndirectDrawCommand>& out_commands
) -> IndirectDrawRange;

}  // namespace Luminol::Graphics::SDL_GPU
