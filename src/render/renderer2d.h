#pragma once

#include "shader.h"

#include <vk_mem_alloc.hpp>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <stb_truetype.h>

#include <deque>

#include <stb_image.h>

#define BIT(x) (1U << x)

namespace saf {

	struct Vertex {
		glm::vec3 pos;
		glm::vec4 color;
		glm::vec2 tex_coord;
		float samplerid;

		inline static std::array<vk::VertexInputBindingDescription, 1> getBindingDescription() {
			return { vk::VertexInputBindingDescription(0, sizeof(Vertex)) };
		}

		inline static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescription()
		{
			return std::array<vk::VertexInputAttributeDescription, 4>
			{
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)),
				vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord)),
				vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32Sfloat, offsetof(Vertex, samplerid))
			};
		}
	};

	struct BasicVertex {
		glm::vec3 pos;
		glm::vec4 color;

		inline static std::array<vk::VertexInputBindingDescription, 1> getBindingDescription() {
			return { vk::VertexInputBindingDescription(0, sizeof(BasicVertex)) };
		}

		inline static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription()
		{
			return std::array<vk::VertexInputAttributeDescription, 2>
			{
				vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(BasicVertex, pos)),
				vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(BasicVertex, color)),
			};
		}
	};

	struct Uniform {
		glm::mat4 projection_view;
		glm::mat4 model;
	};

	enum class FontType
	{
		Arial,
		ComicSans,
		TimesNewRoman,
		InkFree,
		Impact,
		Chiller
	};

	enum class FontFlags
	{

		rotate = BIT(0),
		rotate_chars = BIT(1),
		scale_chars = BIT(2),
		translate_chars = BIT(3),

		fade = BIT(4),
		gradiant = BIT(5)

	};

	struct Font
	{
		Font(FontType _fonttype = FontType::Arial, float _scale = 1.f, glm::vec4 _color = glm::vec4(1.f, 1.f, 1.f, 1.f));

		void EnableCharRotate(float _char_rotate_angle);
		void EnableTranslate(glm::vec3 _translation);
		void EnableRotate(float _rotate_angle);
		void EnableFade(float _fade);
		void EnableScale(float _scale);

		float fade = 0.f;
		float scale = 1.f;
		float rotate_angle = 0.f;
		float char_rotate_angle = 0.f;
		glm::vec3 translation = glm::vec3(0.f, 0.f, 0.f);

		FontType fonttype = FontType::Arial;
		glm::vec4 color = glm::vec4(1.f, 1.f, 1.f, 1.f);
	};


	struct GraphicalString
	{
		struct GraphicalChar
		{
			GraphicalChar(char _code, Font _font = {})
				: code(_code), font(_font)
			{

			}
			char code;
			Font font;
		};

		GraphicalString(std::string str, Font font = Font(saf::FontType::ComicSans, 0.5f))
		{
			m_GChars.reserve(str.length());
			for (char ch : str)
			{
				m_GChars.emplace_back(ch, font);
			}
		}

		inline size_t Length() const { return m_GChars.size(); }

		inline std::string ToString()
		{
			std::string str = "";
			for (const GraphicalChar& ch : m_GChars)
			{
				str += ch.code;
			}
			return str;
		}

		inline const GraphicalChar& operator[](uint32_t index) const { return m_GChars[index]; }

		std::vector<GraphicalChar> m_GChars;
	};


	class StringAnimator
	{
	public:
		virtual GraphicalString Get(std::string input, float delta) = 0;
		inline virtual bool ShouldDelete(float delta) { return false; }
	};

	class FontAtlas
	{
	public:
		FontAtlas(std::vector<std::string> files, int width, int height, int firstCode, int numofchars);
		~FontAtlas();
		FontAtlas(const FontAtlas&) = delete;
		FontAtlas(FontAtlas&&) = delete;
		FontAtlas& operator=(const FontAtlas&) = delete;
		FontAtlas& operator=(FontAtlas&&) = delete;

		void Shutdown();

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
		friend class TextRenderer;

		uint32_t m_Width, m_Height;
		const float m_FontSize = 64.f;
		int m_FirstCode;
		int m_NumOfCodes;
		int m_NumOfFonts;

		std::string m_File;
		uint8_t* m_RawData;
		stbtt_packedchar* m_PackedChars;
		stbtt_aligned_quad* m_AlignedQuads;

		vk::Image m_vkFontAtlas = nullptr;
		vma::Allocation m_vmaFontAtlasAllocation = nullptr;
		vk::ImageView m_vkFontAtlasView = nullptr;

	};

	//struct Image
	//{
	//	Image(std::string file)
	//	{
	//		vk::ImageCreateInfo image_info;
	//		vk::ImageCreateFlagBits::
	//		//vkhelper::CreateImage();
	//	}
	//
	//	~Image()
	//	{
	//		global::g_Device.destroyImageView(m_vkImageView);
	//		global::g_Allocator.destroyImage(m_vkImage, m_vmaAllocation);
	//	}
	//
	//	Image(const Image&) = delete;
	//	Image(Image&&) = delete;
	//	Image& operator=(const Image&) = delete;
	//	Image& operator=(Image&&) = delete;
	//
	//	vk::Image m_vkImage;
	//	vk::ImageView m_vkImageView;
	//	vma::Allocation m_vmaAllocation;
	//};

	class Renderer
	{
	public:
		Renderer(glm::mat4 projection);

		inline void SetProjection(glm::mat4 projection) { m_Projection = projection; }

		virtual void Init() = 0;
		virtual void Shutdown() = 0;
		virtual void BeginScene() = 0;
		virtual void Flush(vk::CommandBuffer cmd) = 0;
		virtual void EndScene() = 0;

		/* DrawCalls
		 *
		 * Pipeline
			 * Aligned Rect, Image2DArray FontAtlas + ImageAtlas (512x512xN)	| Common Vertex / Index Buffers
			 * Bind Descriptors, Bind VertexBuffer, Bind IndexBuffer			| Common Vertex / Index Buffers
			 * Set ImageAttachment MemoryBarrier, DrawCMD						| Common Vertex / Index Buffers
		 * Pipeline																| Common Vertex / Index Buffers
			 * Aligned Rect, Image2DArray Small (256x256)xN						| Common Vertex / Index Buffers
			 * Bind Descriptors, Bind VertexBuffer, Bind IndexBuffer			| Common Vertex / Index Buffers
			 * Set ImageAttachment MemoryBarrier, DrawCMD						| Common Vertex / Index Buffers
		 * Pipeline																| Common Vertex / Index Buffers
			 * Aligned Rect, Image2DArray Large (1024x1024)xN					| Common Vertex / Index Buffers
			 * Bind Descriptors, Bind VertexBuffer, Bind IndexBuffer			| Common Vertex / Index Buffers
			 * Set ImageAttachment MemoryBarrier, DrawCMD						| Common Vertex / Index Buffers
		 * Pipeline
		 * 
		 * Pipeline						TODO
			 * Filled TriangleMesh		TODO
			 *							TODO
			 * Lines					TODO
		 * 
		 */

	protected:

		glm::mat4 m_Projection;
		vk::PipelineLayout m_vkPipelineLayout = nullptr;
		vk::Pipeline m_vkPipeline = nullptr;

		Vertex* m_VertexBuffer = nullptr;

		vma::Allocation m_vmaVertexAllocation = nullptr;
		vk::Buffer m_vkVertexBuffer = nullptr;
		vma::Allocation m_vmaIndexAllocation = nullptr;
		vk::Buffer m_vkIndexBuffer = nullptr;

		vk::DescriptorSetLayout m_vkDescriptorSetLayout = nullptr;
		vk::DescriptorSet m_vkDescriptorSet = nullptr;

		friend class FrameManager;
	};

	class TextRenderer : public Renderer
	{
	public:
		TextRenderer(glm::mat4 projection);

		void Init() override;
		void Shutdown() override;
		void BeginScene() override;
		void Flush(vk::CommandBuffer cmd) override;
		void EndScene() override;

		glm::vec2 DrawString(const GraphicalString& str, glm::vec2 bounding_first = { -1.f, -1.f }, glm::vec2 bounding_second = { 1.f, 1.f }, int cursor = -1);
		glm::vec2 DrawString(const std::string& str, glm::vec2 bounding_first = { -1.f, -1.f }, glm::vec2 bounding_second = { 1.f, 1.f }, int cursor = -1, Font font = Font(saf::FontType::ComicSans, 0.5f));

		glm::vec2 DrawStringCenter(const GraphicalString& str, glm::vec2 bounding_first = { -1.f, -1.f }, glm::vec2 bounding_second = { 1.f, 1.f }, int cursor = -1);
		glm::vec2 DrawStringCenter(const std::string& str, glm::vec2 bounding_first = { -1.f, -1.f }, glm::vec2 bounding_second = { 1.f, 1.f }, int cursor = -1, Font font = Font(saf::FontType::ComicSans, 0.5f));

	private:
		glm::mat4 m_Projection;
		vk::PipelineLayout m_vkPipelineLayout = nullptr;
		vk::Pipeline m_vkPipeline = nullptr;

		Vertex* m_VertexBuffer = nullptr;

		vma::Allocation m_vmaVertexAllocation = nullptr;
		vk::Buffer m_vkVertexBuffer = nullptr;
		vma::Allocation m_vmaIndexAllocation = nullptr;
		vk::Buffer m_vkIndexBuffer = nullptr;

		uint32_t m_QuadCount;
		uint32_t m_MaxQuads;

		vk::DescriptorSetLayout m_vkDescriptorSetLayout = nullptr;
		vk::DescriptorSet m_vkDescriptorSet = nullptr;
		vk::Sampler m_vkSampler = nullptr;

		std::shared_ptr<FontAtlas> m_FontAtlas;
	};

	class QuadRenderer : public Renderer
	{
	public:
		void Init() override;
		void Shutdown() override;
		void BeginScene() override;
		void Flush(vk::CommandBuffer cmd) override;
		void EndScene() override;

		void FillRect(glm::vec3 position, glm::vec2 size, glm::vec4 color = glm::vec4(1.f, 1.f, 1.f, 1.f));
		void FillRectCenter(glm::vec3 position, glm::vec2 size, glm::vec4 color = glm::vec4(1.f, 1.f, 1.f, 1.f));

	private:
		glm::mat4 m_Projection;
		vk::PipelineLayout m_vkPipelineLayout = nullptr;
		vk::Pipeline m_vkPipeline = nullptr;

		Vertex* m_VertexBuffer = nullptr;

		vma::Allocation m_vmaVertexAllocation = nullptr;
		vk::Buffer m_vkVertexBuffer = nullptr;
		vma::Allocation m_vmaIndexAllocation = nullptr;
		vk::Buffer m_vkIndexBuffer = nullptr;

		uint32_t m_QuadCount;
		uint32_t m_MaxQuads;

		vk::DescriptorSetLayout m_vkDescriptorSetLayout = nullptr;
		vk::DescriptorSet m_vkDescriptorSet = nullptr;
	};

	class Renderer2D : public Renderer
	{
	public:
		Renderer2D();
		void Init() override;
		void Shutdown() override;
		void BeginScene() override;
		void Flush(vk::CommandBuffer cmd) override;
		void EndScene() override;
	private:
		std::vector<std::shared_ptr<Renderer>> m_Renderers;
	};

}