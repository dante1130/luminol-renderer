#include "SDL_GPUCullingUtils.hpp"

#include <algorithm>

#include <LuminolRenderEngine/Graphics/Frustum.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUFactory.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMesh.hpp>

namespace Luminol::Graphics::SDL_GPU {

namespace {

using namespace Luminol::Maths;

auto transform_point(const Matrix4x4f& matrix, const Vector3f& point)
    -> Vector3f {
    return Vector3f{
        (point.x() * matrix[0][0]) + (point.y() * matrix[1][0]) +
            (point.z() * matrix[2][0]) + matrix[3][0],
        (point.x() * matrix[0][1]) + (point.y() * matrix[1][1]) +
            (point.z() * matrix[2][1]) + matrix[3][1],
        (point.x() * matrix[0][2]) + (point.y() * matrix[1][2]) +
            (point.z() * matrix[2][2]) + matrix[3][2],
    };
}

}  // namespace

auto compute_mesh_world_bounds(
    const SDL_GPUMesh& mesh, gsl::span<const Matrix4x4f> model_matrices
) -> BoundingBox {
    const auto& local_bounds = mesh.get_local_bounds();

    if (model_matrices.empty()) {
        return local_bounds;
    }

    const auto local_corners = std::array{
        Vector3f{local_bounds.min.x(), local_bounds.min.y(), local_bounds.min.z()},
        Vector3f{local_bounds.max.x(), local_bounds.min.y(), local_bounds.min.z()},
        Vector3f{local_bounds.min.x(), local_bounds.max.y(), local_bounds.min.z()},
        Vector3f{local_bounds.max.x(), local_bounds.max.y(), local_bounds.min.z()},
        Vector3f{local_bounds.min.x(), local_bounds.min.y(), local_bounds.max.z()},
        Vector3f{local_bounds.max.x(), local_bounds.min.y(), local_bounds.max.z()},
        Vector3f{local_bounds.min.x(), local_bounds.max.y(), local_bounds.max.z()},
        Vector3f{local_bounds.max.x(), local_bounds.max.y(), local_bounds.max.z()},
    };

    auto world_min = Vector3f{0.0F, 0.0F, 0.0F};
    auto world_max = Vector3f{0.0F, 0.0F, 0.0F};
    auto bounds_initialized = false;

    for (const auto& transform : model_matrices) {
        for (const auto& corner : local_corners) {
            const auto world_corner = transform_point(transform, corner);

            if (!bounds_initialized) {
                world_min = world_corner;
                world_max = world_corner;
                bounds_initialized = true;
                continue;
            }

            world_min = Vector3f{
                std::min(world_min.x(), world_corner.x()),
                std::min(world_min.y(), world_corner.y()),
                std::min(world_min.z(), world_corner.z()),
            };
            world_max = Vector3f{
                std::max(world_max.x(), world_corner.x()),
                std::max(world_max.y(), world_corner.y()),
                std::max(world_max.z(), world_corner.z()),
            };
        }
    }

    return BoundingBox{.min = world_min, .max = world_max};
}

auto compute_batch_mesh_world_bounds(
    const SDL_GPUFactory& graphics_factory,
    gsl::span<const InstanceBatch> instance_batches,
    const std::unordered_map<RenderableId, std::vector<Matrix4x4f>>&
        queued_draws
) -> BatchMeshBounds {
    auto result = BatchMeshBounds{};
    result.reserve(instance_batches.size());

    for (const auto& batch : instance_batches) {
        const auto meshes = graphics_factory.get_meshes(batch.renderable_id);
        auto mesh_bounds = std::vector<BoundingBox>{};
        mesh_bounds.reserve(meshes.size());

        const auto transforms_it = queued_draws.find(batch.renderable_id);
        const auto model_matrices = transforms_it != queued_draws.end()
            ? gsl::span<const Matrix4x4f>{transforms_it->second}
            : gsl::span<const Matrix4x4f>{};

        for (const auto& mesh : meshes) {
            mesh_bounds.push_back(
                compute_mesh_world_bounds(mesh, model_matrices)
            );
        }

        result.push_back(std::move(mesh_bounds));
    }

    return result;
}

auto append_batch_indirect_commands(
    const SDL_GPUFactory& graphics_factory,
    const InstanceBatch& batch,
    const std::vector<BoundingBox>& mesh_bounds,
    const std::array<Vector4f, 6>& frustum_planes,
    std::optional<Utilities::ModelLoader::AlphaMode> alpha_mode_filter,
    std::vector<IndirectDrawCommand>& out_commands
) -> IndirectDrawRange {
    const auto meshes = graphics_factory.get_meshes(batch.renderable_id);
    const auto range_offset = static_cast<uint32_t>(out_commands.size());
    auto range_count = uint32_t{0};

    for (auto mesh_index = std::size_t{0}; mesh_index < meshes.size();
         ++mesh_index) {
        const auto& mesh = meshes[mesh_index];

        if (alpha_mode_filter.has_value() &&
            mesh.alpha_mode() != *alpha_mode_filter) {
            continue;
        }

        const auto& bounds = mesh_bounds[mesh_index];
        if (!aabb_in_frustum(frustum_planes, bounds.min, bounds.max)) {
            continue;
        }

        out_commands.push_back(IndirectDrawCommand{
            .num_indices = mesh.get_index_count(),
            .num_instances = batch.instance_count,
            .first_index = mesh.get_first_index(),
            .vertex_offset = mesh.get_vertex_offset(),
            .first_instance = 0U,
        });
        ++range_count;
    }

    return IndirectDrawRange{.offset = range_offset, .count = range_count};
}

}  // namespace Luminol::Graphics::SDL_GPU
