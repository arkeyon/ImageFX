#pragma once

#include "shader.h"

#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>
#include <glm/glm/glm.hpp>

#include <stb_truetype.h>

namespace saf {

	struct Vertex {
		glm::vec3 pos;
		glm::vec4 color;
		glm::vec2 tex_coord;

		static std::array<vk::VertexInputBindingDescription, 1> getBindingDescription() {
			return { vk::VertexInputBindingDescription(0, sizeof(Vertex)) };
		}
	};

	struct Uniform {
		glm::mat4 projection_view;
		glm::mat4 model;
	};

	enum class FontType
	{
		Arial, ComicSans
	};

	struct Font
	{
		FontType fonttype = FontType::Arial;
		float scale = 1.f;
		glm::vec4 color = glm::vec4(1.f, 1.f, 1.f, 1.f);
	};

	class FontAtlas
	{
	public:
		FontAtlas(std::string file, int width, int height, int firstCode, int numofchars);
		~FontAtlas();
		FontAtlas(const FontAtlas&) = delete;
		FontAtlas(FontAtlas&&) = delete;
		FontAtlas& operator=(const FontAtlas&) = delete;
		FontAtlas& operator=(FontAtlas&&) = delete;

		inline const uint8_t* GetData() { return m_RawData; }
		inline void Free()
		{
			if (m_RawData)
			{
				delete[] m_RawData;
				m_RawData = nullptr;
			}
		}
	private:
		friend class Renderer2D;

		std::string m_File;
		uint32_t m_Width, m_Height;
		const float m_FontSize = 64.f;
		int m_FirstCode;
		int m_NumOfCodes;
		uint8_t* m_RawData;
		stbtt_packedchar* m_PackedChars;
		stbtt_aligned_quad* m_AlignedQuads;
	};

	class Terminal : std::stringbuf
	{
	public:
		virtual int sync() override;
	private:

	};

	class Renderer2D
	{
	public:
		Renderer2D(uint32_t width, uint32_t height);

		void Init();
		void Shutdown();
		void BeginScene();
		void Submit();
		void Flush(vk::CommandBuffer cmd);
		void EndScene();

		void DrawString(std::string str, float pixelscale, glm::vec2 bounding_first = { -1.f, -1.f }, glm::vec2 bounding_second = { 1.f, 1.f }, Font font = {});

	private:
		uint32_t m_Width, m_Height;

		std::shared_ptr<FontAtlas> m_FontAtlas = nullptr;
		vk::Image m_vkFontAtlas = nullptr;
		vma::Allocation m_vmaFontAtlasAllocation = nullptr;
		vk::ImageView m_vkFontAtlasView = nullptr;
		vk::DescriptorSetLayout m_vkFontDescriptorSetLayout = nullptr;
		vk::DescriptorSet m_vkFontAtlasDescriptorSet = nullptr;
		vk::Sampler m_vkFontAtlasSampler = nullptr;

		vk::PipelineLayout m_vkPipelineLayout = nullptr;
		vk::Pipeline m_vkPipeline = nullptr;

		const uint32_t m_MaxQuads = 0x4000;
		uint32_t m_QuadCount = 0;

		Vertex* m_VertexBuffer = nullptr;

		vma::Allocation m_vmaVertexAllocation = nullptr;
		vk::Buffer m_vkVertexBuffer = nullptr;
		vma::Allocation m_vmaIndexAllocation = nullptr;
		vk::Buffer m_vkIndexBuffer = nullptr;
	};

}