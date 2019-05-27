#include "../src/meshoptimizer.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

const char* getError(cgltf_result result)
{
	switch (result)
	{
	case cgltf_result_file_not_found:
		return "file not found";

	case cgltf_result_io_error:
		return "I/O error";

	case cgltf_result_invalid_json:
		return "invalid JSON";

	case cgltf_result_invalid_gltf:
		return "invalid GLTF";

	case cgltf_result_out_of_memory:
		return "out of memory";

	default:
		return "unknown error";
	}
}

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "Usage: gltfpack [options] input output\n");
		return 1;
	}

	const char* input = argv[argc - 2];
	const char* output = argv[argc - 1];

	cgltf_options options = {};
	cgltf_data* data = 0;
	cgltf_result result = cgltf_parse_file(&options, input, &data);
	result = (result == cgltf_result_success) ? cgltf_validate(data) : result;
	result = (result == cgltf_result_success) ? cgltf_load_buffers(&options, data, input) : result;

	if (result != cgltf_result_success)
	{
		cgltf_free(data);

		fprintf(stderr, "Error loading %s: %s\n", input, getError(result));
		return 2;
	}

	FILE* out = fopen(output, "wb");
	if (!out)
	{
		cgltf_free(data);

		fprintf(stderr, "Error saving %s\n", output);
		return 3;
	}

	fclose(out);
	cgltf_free(data);

	return 0;
}
