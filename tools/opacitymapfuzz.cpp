#include "../src/meshoptimizer.h"

#include <fuzzer/FuzzedDataProvider.h>
#include <vector>
#include <string.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    FuzzedDataProvider fdp(data, size);

    uint32_t vertex_count = fdp.ConsumeIntegralInRange<uint32_t>(1, 1024);
    uint32_t index_count = fdp.ConsumeIntegralInRange<uint32_t>(3, 3072);
    index_count = (index_count / 3) * 3;

    if (fdp.remaining_bytes() < (vertex_count * 2 * sizeof(float) + index_count * sizeof(uint32_t))) {
        return 0;
    }

    std::vector<float> vertex_uvs(vertex_count * 2);
    for (size_t i = 0; i < vertex_uvs.size(); ++i) {
        vertex_uvs[i] = fdp.ConsumeFloatingPoint<float>();
    }

    std::vector<uint32_t> indices(index_count);
    for (size_t i = 0; i < index_count; ++i) {
        indices[i] = fdp.ConsumeIntegralInRange<uint32_t>(0, vertex_count - 1);
    }

    uint32_t texture_width = fdp.ConsumeIntegralInRange<uint32_t>(1, 512);
    uint32_t texture_height = fdp.ConsumeIntegralInRange<uint32_t>(1, 512);
    int max_level = fdp.ConsumeIntegralInRange<int>(0, 4);
    float target_edge = fdp.ConsumeFloatingPointInRange<float>(0.0f, 10.0f);

    size_t triangle_count = index_count / 3;
    std::vector<unsigned char> levels(triangle_count);
    std::vector<unsigned int> sources(triangle_count);
    std::vector<int> omm_indices(triangle_count);

    size_t omm_count = meshopt_opacityMapMeasure(levels.data(), sources.data(), omm_indices.data(), indices.data(), index_count, vertex_uvs.data(), vertex_count, 2 * sizeof(float), texture_width, texture_height, max_level, target_edge);

    if (omm_count > 0 && fdp.remaining_bytes() > 0) {
        int states = fdp.ConsumeBool() ? 2 : 4;
        
        // Rasterize some maps
        std::vector<unsigned char> texture_data(texture_width * texture_height, 128);
        size_t total_omm_size = 0;
        for (size_t i = 0; i < omm_count; ++i) {
            total_omm_size += meshopt_opacityMapEntrySize(levels[i], states);
        }
        
        if (total_omm_size < 1000000) { // Limit memory usage
            std::vector<unsigned char> omm_data(total_omm_size);
            std::vector<unsigned int> offsets(omm_count);
            size_t current_offset = 0;
            
            for (size_t i = 0; i < omm_count; ++i) {
                offsets[i] = (unsigned int)current_offset;
                
                unsigned int tri_idx = sources[i];
                float uv0[2] = {vertex_uvs[indices[tri_idx * 3 + 0] * 2 + 0], vertex_uvs[indices[tri_idx * 3 + 0] * 2 + 1]};
                float uv1[2] = {vertex_uvs[indices[tri_idx * 3 + 1] * 2 + 0], vertex_uvs[indices[tri_idx * 3 + 1] * 2 + 1]};
                float uv2[2] = {vertex_uvs[indices[tri_idx * 3 + 2] * 2 + 0], vertex_uvs[indices[tri_idx * 3 + 2] * 2 + 1]};
                
                meshopt_opacityMapRasterize(omm_data.data() + current_offset, levels[i], states, uv0, uv1, uv2, texture_data.data(), 1, texture_width, texture_width, texture_height);
                
                current_offset += meshopt_opacityMapEntrySize(levels[i], states);
            }
            
            // Compact
            meshopt_opacityMapCompact(omm_data.data(), total_omm_size, levels.data(), offsets.data(), omm_count, omm_indices.data(), triangle_count, states);
        }
    }

    return 0;
}
