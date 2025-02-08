
#include <vulkan/vulkan.hpp>

namespace saf {

	class Shader {
	public:
		Shader(std::string path);
		Shader(std::string code, vk::ShaderStageFlags type);

		vk::ShaderEXT m_vkShader;

	};

}