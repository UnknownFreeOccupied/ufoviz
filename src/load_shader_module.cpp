// UFO
#include <ufo/viz/load_shader_module.hpp>

// STL
#include <fstream>

namespace ufo
{
WGPUShaderModule loadShaderModule(WGPUDevice device, std::filesystem::path const& path)
{
	std::ifstream file(path);
	if (!file.is_open()) {
		return nullptr;
	}

	file.seekg(0, std::ios::end);
	std::size_t size = file.tellg();
	std::string shader_source(size, ' ');
	file.seekg(0);
	file.read(shader_source.data(), size);

	return loadShaderModule(device, shader_source);
}

WGPUShaderModule loadShaderModule(WGPUDevice device, std::string const& shader_source)
{
	WGPUShaderModuleWGSLDescriptor shader_code_desc{};
	shader_code_desc.chain.next  = nullptr;
	shader_code_desc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	shader_code_desc.code        = shader_source.c_str();

	WGPUShaderModuleDescriptor shader_desc{};
	shader_desc.nextInChain = nullptr;
#ifdef WEBGPU_BACKEND_WGPU
	shader_desc.hintCount = 0;
	shader_desc.hints     = nullptr;
#endif
	shader_desc.nextInChain = &shader_code_desc.chain;
	return wgpuDeviceCreateShaderModule(device, &shader_desc);
}

// WGPUShaderModule frmwrk_load_shader_module(WGPUDevice device, char const* name)
// {
// 	FILE*            file          = NULL;
// 	char*            buf           = NULL;
// 	WGPUShaderModule shader_module = NULL;

// 	file = fopen(name, "rb");
// 	if (!file) {
// 		perror("fopen");
// 		goto cleanup;
// 	}

// 	if (fseek(file, 0, SEEK_END) != 0) {
// 		perror("fseek");
// 		goto cleanup;
// 	}
// 	long length = ftell(file);
// 	if (length == -1) {
// 		perror("ftell");
// 		goto cleanup;
// 	}
// 	if (fseek(file, 0, SEEK_SET) != 0) {
// 		perror("fseek");
// 		goto cleanup;
// 	}

// 	buf = malloc(length + 1);
// 	assert(buf);
// 	fread(buf, 1, length, file);
// 	buf[length] = 0;

// 	shader_module = wgpuDeviceCreateShaderModule(
// 	    device, &(WGPUShaderModuleDescriptor const){
// 	                .label = name,
// 	                .nextInChain =
// 	                    (WGPUChainedStruct const*)&(WGPUShaderModuleWGSLDescriptor const){
// 	                        .chain =
// 	                            (WGPUChainedStruct const){
// 	                                .sType = WGPUSType_ShaderModuleWGSLDescriptor,
// 	                            },
// 	                        .code = buf,
// 	                    },
// 	            });

// cleanup:
// 	if (file) fclose(file);
// 	if (buf) free(buf);
// 	return shader_module;
// }
}  // namespace ufo