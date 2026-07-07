#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLMesh.hpp>
#include <LuminolRenderEngine/Graphics/OpenGL/OpenGLTextureManager.hpp>
#include <LuminolRenderEngine/Utilities/ModelLoader.hpp>
#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>

#include <LuminolMaths/Vector.hpp>

#include <gsl/gsl>
#include <glad/gl.h>

namespace {

using namespace Luminol::Graphics;

auto load_first_texture_or_nothing(
    const std::vector<std::filesystem::path>& texture_paths,
    const std::unordered_map<
        std::filesystem::path,
        Luminol::Utilities::ImageLoader::Image>& textures_map
) -> std::optional<Luminol::Utilities::ImageLoader::Image> {
    if (texture_paths.empty()) {
        return std::nullopt;
    }

    return textures_map.at(texture_paths[0]);
}

constexpr auto create_vertex_attributes() {
    constexpr auto texture_coordinates_offset = 3 * sizeof(float);
    constexpr auto normal_offset = 5 * sizeof(float);
    constexpr auto tangent_offset = 8 * sizeof(float);

    return std::array{
        VertexAttribute{
            .component_count = 3,
            .normalized = false,
            .relative_offset = 0,
        },
        VertexAttribute{
            .component_count = 2,
            .normalized = false,
            .relative_offset = texture_coordinates_offset,
        },
        VertexAttribute{
            .component_count = 3,
            .normalized = false,
            .relative_offset = normal_offset,
        },
        VertexAttribute{
            .component_count = 3,
            .normalized = false,
            .relative_offset = tangent_offset,
        },
    };
}

auto load_texture_from_path(
    const std::optional<std::filesystem::path>& path, ColorSpace color_space
) -> OpenGLMesh::TextureRefOptional {
    if (!path.has_value()) {
        return std::nullopt;
    }

    const auto& path_value = path.value();

    auto& texture_manager = OpenGLTextureManager::get_instance();
    texture_manager.load_texture(path_value, color_space);
    return texture_manager.get_texture(path_value);
}

auto load_texture_from_image(
    const std::optional<Luminol::Utilities::ImageLoader::Image>& image,
    ColorSpace color_space
) -> OpenGLMesh::TextureRefOptional {
    if (!image.has_value()) {
        return std::nullopt;
    }

    const auto& image_value = image.value();

    auto& texture_manager = OpenGLTextureManager::get_instance();
    texture_manager.load_texture(image_value, color_space);
    return texture_manager.get_texture(image_value.path);
}

}  // namespace

namespace Luminol::Graphics {

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices, gsl::span<const uint32_t> indices
)
    : vertex_array_object{vertices, indices, create_vertex_attributes()} {}

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
)
    : vertex_array_object{vertices, indices, create_vertex_attributes()},
      diffuse_texture{load_texture_from_path(
          texture_paths.diffuse_texture_path, ColorSpace::SRGB
      )},
      emissive_texture{load_texture_from_path(
          texture_paths.emissive_texture_path, ColorSpace::SRGB
      )},
      normal_texture{load_texture_from_path(
          texture_paths.normal_texture_path, ColorSpace::Linear
      )},
      metallic_texture{load_texture_from_path(
          texture_paths.metallic_texture_path, ColorSpace::Linear
      )},
      roughness_texture{load_texture_from_path(
          texture_paths.roughness_texture_path, ColorSpace::Linear
      )},
      ambient_occlusion_texture{load_texture_from_path(
          texture_paths.ambient_occlusion_texture_path, ColorSpace::Linear
      )} {}

OpenGLMesh::OpenGLMesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TextureImages& texture_images
)
    : vertex_array_object{vertices, indices, create_vertex_attributes()},
      diffuse_texture{load_texture_from_image(
          texture_images.diffuse_texture, ColorSpace::SRGB
      )},
      emissive_texture{load_texture_from_image(
          texture_images.emissive_texture, ColorSpace::SRGB
      )},
      normal_texture{load_texture_from_image(
          texture_images.normal_texture, ColorSpace::Linear
      )},
      metallic_texture{load_texture_from_image(
          texture_images.metallic_texture, ColorSpace::Linear
      )},
      roughness_texture{load_texture_from_image(
          texture_images.roughness_texture, ColorSpace::Linear
      )},
      ambient_occlusion_texture{load_texture_from_image(
          texture_images.ambient_occlusion_texture, ColorSpace::Linear
      )} {}

auto OpenGLMesh::draw() const -> void {
    this->bind_textures();
    this->vertex_array_object.bind();
    glDrawElements(
        GL_TRIANGLES,
        this->vertex_array_object.get_index_count(),
        GL_UNSIGNED_INT,
        nullptr
    );
    this->vertex_array_object.unbind();
    this->unbind_textures();
}

auto OpenGLMesh::draw_instanced(int32_t instance_count) const -> void {
    this->bind_textures();
    this->vertex_array_object.bind();
    glDrawElementsInstanced(
        GL_TRIANGLES,
        this->vertex_array_object.get_index_count(),
        GL_UNSIGNED_INT,
        nullptr,
        instance_count
    );
    this->vertex_array_object.unbind();
    this->unbind_textures();
}

auto OpenGLMesh::bind_textures() const -> void {
    if (this->diffuse_texture.has_value()) {
        this->diffuse_texture->get().bind(SamplerBindingPoint::TextureDiffuse);
    }

    if (this->emissive_texture.has_value()) {
        this->emissive_texture->get().bind(SamplerBindingPoint::TextureEmissive
        );
    }

    if (this->normal_texture.has_value()) {
        this->normal_texture->get().bind(SamplerBindingPoint::TextureNormal);
    }

    if (this->metallic_texture.has_value()) {
        this->metallic_texture->get().bind(SamplerBindingPoint::TextureMetallic
        );
    }

    if (this->roughness_texture.has_value()) {
        this->roughness_texture->get().bind(
            SamplerBindingPoint::TextureRoughness
        );
    }

    if (this->ambient_occlusion_texture.has_value()) {
        this->ambient_occlusion_texture->get().bind(
            SamplerBindingPoint::TextureAO
        );
    }
}

auto OpenGLMesh::unbind_textures() const -> void {
    if (this->emissive_texture.has_value()) {
        this->emissive_texture->get().unbind(
            SamplerBindingPoint::TextureEmissive
        );
    }

    if (this->diffuse_texture.has_value()) {
        this->diffuse_texture->get().unbind(SamplerBindingPoint::TextureDiffuse
        );
    }

    if (this->normal_texture.has_value()) {
        this->normal_texture->get().unbind(SamplerBindingPoint::TextureNormal);
    }

    if (this->metallic_texture.has_value()) {
        this->metallic_texture->get().unbind(
            SamplerBindingPoint::TextureMetallic
        );
    }

    if (this->roughness_texture.has_value()) {
        this->roughness_texture->get().unbind(
            SamplerBindingPoint::TextureRoughness
        );
    }

    if (this->ambient_occlusion_texture.has_value()) {
        this->ambient_occlusion_texture->get().unbind(
            SamplerBindingPoint::TextureAO
        );
    }
}

auto create_quad_mesh() -> OpenGLMesh {
    struct Vertex {
        Maths::Vector3f position;
        Maths::Vector2f tex_coords;
        Maths::Vector3f normal;
        Maths::Vector3f tangent;
    };

    constexpr auto vertices = std::array{
        Vertex{
            .position = Maths::Vector3f{-1.0f, 1.0f, 0.0f},
            .tex_coords = Maths::Vector2f{0.0f, 1.0f},
            .normal = Maths::Vector3f{0.0f, 0.0f, 1.0f},
            .tangent = Maths::Vector3f{1.0f, 0.0f, 0.0f},
        },
        Vertex{
            .position = Maths::Vector3f{-1.0f, -1.0f, 0.0f},
            .tex_coords = Maths::Vector2f{0.0f, 0.0f},
            .normal = Maths::Vector3f{0.0f, 0.0f, 1.0f},
            .tangent = Maths::Vector3f{1.0f, 0.0f, 0.0f},
        },
        Vertex{
            .position = Maths::Vector3f{1.0f, -1.0f, 0.0f},
            .tex_coords = Maths::Vector2f{1.0f, 0.0f},
            .normal = Maths::Vector3f{0.0f, 0.0f, 1.0f},
            .tangent = Maths::Vector3f{1.0f, 0.0f, 0.0f},
        },
        Vertex{
            .position = Maths::Vector3f{1.0f, 1.0f, 0.0f},
            .tex_coords = Maths::Vector2f{1.0f, 1.0f},
            .normal = Maths::Vector3f{0.0f, 0.0f, 1.0f},
            .tangent = Maths::Vector3f{1.0f, 0.0f, 0.0f},
        }
    };

    // Draw in counter-clockwise order
    constexpr auto indices = std::array{0u, 3u, 2u, 2u, 1u, 0u};

    constexpr auto component_count = 11u;

    auto vertices_float = std::vector<float>{};
    vertices_float.reserve(vertices.size() * component_count);

    for (const auto& vertex : vertices) {
        vertices_float.push_back(vertex.position.x());
        vertices_float.push_back(vertex.position.y());
        vertices_float.push_back(vertex.position.z());

        vertices_float.push_back(vertex.tex_coords.x());
        vertices_float.push_back(vertex.tex_coords.y());

        vertices_float.push_back(vertex.normal.x());
        vertices_float.push_back(vertex.normal.y());
        vertices_float.push_back(vertex.normal.z());

        vertices_float.push_back(vertex.tangent.x());
        vertices_float.push_back(vertex.tangent.y());
        vertices_float.push_back(vertex.tangent.z());
    }

    return OpenGLMesh{vertices_float, indices};
}

auto load_meshes_from_model(const std::filesystem::path& model_path)
    -> std::vector<OpenGLMesh> {
    const auto model_data_opt =
        Luminol::Utilities::ModelLoader::load_model(model_path);

    Expects(model_data_opt.has_value());

    const auto& model_data = model_data_opt.value();

    auto meshes = std::vector<OpenGLMesh>{};
    meshes.reserve(model_data.meshes.size());

    for (const auto& mesh_data : model_data.meshes) {
        Expects(
            mesh_data.vertices.size() == mesh_data.texture_coordinates.size()
        );

        constexpr auto vertex_components = 11;

        auto mesh_vertices = std::vector<float>{};
        mesh_vertices.reserve(mesh_data.vertices.size() * vertex_components);

        for (size_t i = 0; i < mesh_data.vertices.size(); ++i) {
            mesh_vertices.push_back(mesh_data.vertices[i].x());
            mesh_vertices.push_back(mesh_data.vertices[i].y());
            mesh_vertices.push_back(mesh_data.vertices[i].z());
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].x());
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].y());
            mesh_vertices.push_back(mesh_data.normals[i].x());
            mesh_vertices.push_back(mesh_data.normals[i].y());
            mesh_vertices.push_back(mesh_data.normals[i].z());
            mesh_vertices.push_back(mesh_data.tangents[i].x());
            mesh_vertices.push_back(mesh_data.tangents[i].y());
            mesh_vertices.push_back(mesh_data.tangents[i].z());
        }

        const auto texture_images = TextureImages{
            .diffuse_texture = load_first_texture_or_nothing(
                mesh_data.diffuse_texture_paths, model_data.textures_map
            ),
            .emissive_texture = load_first_texture_or_nothing(
                mesh_data.emissive_texture_paths, model_data.textures_map
            ),
            .normal_texture = load_first_texture_or_nothing(
                mesh_data.normal_texture_paths, model_data.textures_map
            ),
            .metallic_texture = load_first_texture_or_nothing(
                mesh_data.metallic_texture_paths, model_data.textures_map
            ),
            .roughness_texture = load_first_texture_or_nothing(
                mesh_data.roughness_texture_paths, model_data.textures_map
            ),
            .ambient_occlusion_texture = load_first_texture_or_nothing(
                mesh_data.ambient_occlusion_texture_paths,
                model_data.textures_map
            )
        };

        meshes.emplace_back(mesh_vertices, mesh_data.indices, texture_images);
    }

    return meshes;
}

}  // namespace Luminol::Graphics
