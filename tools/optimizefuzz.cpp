#include "../src/meshoptimizer.h"

#include <fuzzer/FuzzedDataProvider.h>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    FuzzedDataProvider fdp(data, size);

    uint32_t vertex_count = fdp.ConsumeIntegralInRange<uint32_t>(1, 1024);
    uint32_t index_count = fdp.ConsumeIntegralInRange<uint32_t>(1, 3072);
    index_count = (index_count / 3) * 3;

    if (fdp.remaining_bytes() < (vertex_count * 3 * sizeof(float) + index_count * sizeof(uint32_t))) {
        return 0;
    }

    std::vector<float> vertex_positions(vertex_count * 3);
    for (size_t i = 0; i < vertex_positions.size(); ++i) {
        vertex_positions[i] = fdp.ConsumeFloatingPoint<float>();
    }

    std::vector<uint32_t> indices(index_count);
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = fdp.ConsumeIntegralInRange<uint32_t>(0, vertex_count - 1);
    }

    std::vector<uint32_t> destination(index_count);

    // Vertex cache optimization
    meshopt_optimizeVertexCache(destination.data(), indices.data(), index_count, vertex_count);
    meshopt_optimizeVertexCacheFifo(destination.data(), indices.data(), index_count, vertex_count, 16);
    meshopt_optimizeVertexCacheStrip(destination.data(), indices.data(), index_count, vertex_count);

    // Overdraw optimization
    float threshold = fdp.ConsumeFloatingPointInRange<float>(1.0f, 1.1f);
    meshopt_optimizeOverdraw(destination.data(), indices.data(), index_count, vertex_positions.data(), vertex_count, 3 * sizeof(float), threshold);

    // Vertex fetch optimization
    std::vector<float> vertices_dest(vertex_count * 3);
    meshopt_optimizeVertexFetch(vertices_dest.data(), indices.data(), index_count, vertex_positions.data(), vertex_count, 3 * sizeof(float));

    // Overdraw/Coverage Analysis
    meshopt_analyzeOverdraw(indices.data(), index_count, vertex_positions.data(), vertex_count, 3 * sizeof(float));
    meshopt_analyzeCoverage(indices.data(), index_count, vertex_positions.data(), vertex_count, 3 * sizeof(float));

    // Stripifier
    if (fdp.ConsumeBool()) {
        uint32_t restart_index = fdp.ConsumeIntegral<uint32_t>();
        std::vector<uint32_t> strip(meshopt_stripifyBound(index_count));
        size_t strip_size = meshopt_stripify(strip.data(), indices.data(), index_count, vertex_count, restart_index);

        std::vector<uint32_t> unstrip(meshopt_unstripifyBound(strip_size));
        meshopt_unstripify(unstrip.data(), strip.data(), strip_size, restart_index);
    }

    if (fdp.ConsumeBool()) {
        std::vector<float> normals(vertex_count * 3);
        std::vector<float> uvs(vertex_count * 2);
        std::vector<float> tangents(index_count * 4);
        for (size_t i = 0; i < normals.size(); ++i) normals[i] = fdp.ConsumeFloatingPoint<float>();
        for (size_t i = 0; i < uvs.size(); ++i) uvs[i] = fdp.ConsumeFloatingPoint<float>();
        meshopt_generateTangents(tangents.data(), indices.data(), index_count, vertex_positions.data(), vertex_count, 3 * sizeof(float), normals.data(), 3 * sizeof(float), uvs.data(), 2 * sizeof(float), 0);
    }

    if (fdp.ConsumeBool()) {
        std::vector<unsigned int> remap(vertex_count);
        meshopt_generateVertexRemap(remap.data(), indices.data(), index_count, vertex_positions.data(), vertex_count, 3 * sizeof(float));
    }

    return 0;
}
