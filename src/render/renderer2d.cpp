#include "safpch.h"
#include "renderer2d.h"

#include <stb_truetype.h>
#include <stb_image_write.h>
#include <fstream>
#include "platform/vulkangraphics.h"

#include "globals.h"

namespace saf {

    FontAtlas::FontAtlas(std::string file, int width, int height, int firstCode, int numofchars)
        : m_FirstCode(firstCode), m_NumOfCodes(numofchars), m_Width(width), m_Height(height), m_RawData(new uint8_t[m_Width * m_Height]), m_PackedChars{}, m_AlignedQuads{}
	{
        m_PackedChars = new stbtt_packedchar[m_NumOfCodes];
        m_AlignedQuads = new stbtt_aligned_quad[m_NumOfCodes];

        // Read the font file
        std::ifstream inputStream(file.c_str(), std::ios::binary);

        inputStream.seekg(0, std::ios::end);
        auto&& fontFileSize = inputStream.tellg();
        inputStream.seekg(0, std::ios::beg);

        uint8_t* fontDataBuf = new uint8_t[fontFileSize];

        inputStream.read((char*)fontDataBuf, fontFileSize);

        stbtt_fontinfo fontInfo = {};

        uint32_t fontCount = stbtt_GetNumberOfFonts(fontDataBuf);
        std::cout << "Font File: " << file << " has " << fontCount << " fonts\n";

        IFX_TRACE("Font file: {0} contains {1} fonts", file, fontCount);

        if (!stbtt_InitFont(&fontInfo, fontDataBuf, 0))
            std::cerr << "stbtt_InitFont() Failed!\n";

        stbtt_pack_context ctx;

        stbtt_PackBegin(
            &ctx,                                     // stbtt_pack_context (this call will initialize it) 
            (unsigned char*)m_RawData,     // Font Atlas texture data
            m_Width,                                  // Width of the font atlas texture
            m_Height,                                 // Height of the font atlas texture
            0,                                        // Stride in bytes
            1,                                        // Padding between the glyphs
            nullptr);

        stbtt_PackFontRange(
            &ctx,                                     // stbtt_pack_context
            fontDataBuf,                              // Font Atlas texture data
            0,                                        // Font Index                                 
            64.f,                                     // Size of font in pixels. (Use STBTT_POINT_SIZE(fontSize) to use points) 
            firstCode,                                // Code point of the first charecter
            m_NumOfCodes,                             // No. of charecters to be included in the font atlas 
            m_PackedChars                     // stbtt_packedchar array, this struct will contain the data to render a glyph
        );
        stbtt_PackEnd(&ctx);

        for (int i = 0; i < m_NumOfCodes; i++)
        {
            float unusedX, unusedY;

            stbtt_GetPackedQuad(
                m_PackedChars,              // Array of stbtt_packedchar
                m_Width,                           // Width of the font atlas texture
                m_Height,                          // Height of the font atlas texture
                i,                                 // Index of the glyph
                &unusedX, &unusedY,                // current position of the glyph in screen pixel coordinates, (not required as we have a different corrdinate system)
                &(m_AlignedQuads[i]),              // stbtt_alligned_quad struct. (this struct mainly consists of the texture coordinates)
                0                                  // Allign X and Y position to a integer (doesn't matter because we are not using 'unusedX' and 'unusedY')
            );
        }

        delete[] fontDataBuf;

        // Optionally write the font atlas texture as a png file.
        stbi_write_png("fontAtlas.png", m_Width, m_Height, 1, m_RawData, m_Width);
	}

    FontAtlas::~FontAtlas()
    {
        Free();
        delete[] m_PackedChars;
        delete[] m_AlignedQuads;
    }

    Renderer2D::Renderer2D(uint32_t width, uint32_t height)
        : m_Width(width), m_Height(height)
    {

    }

	void Renderer2D::Init()
	{
        m_FontAtlas = std::make_shared<FontAtlas>("C:/Windows/Fonts/comicz.ttf", 512, 512, 32, 96);

        m_vkVertexBuffer = vkhelper::create_buffer(sizeof(Vertex) * m_MaxQuads * 4, vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive, vma::MemoryUsage::eCpuToGpu, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, global::g_Allocator, m_vmaVertexAllocation);
        if (global::g_Allocator.mapMemory(m_vmaVertexAllocation, reinterpret_cast<void**>(&m_VertexBuffer)) != vk::Result::eSuccess)
            IFX_ERROR("Failed to map vertex buffer");

        uint32_t indices[6]
        {
            0, 1, 2, 0, 2, 3
        };

        vma::Allocation allocation;
        vk::Buffer stage_buffer = vkhelper::create_buffer(sizeof(uint32_t) * m_MaxQuads * 6, vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eIndexBuffer, vk::SharingMode::eExclusive, vma::MemoryUsage::eCpuToGpu, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, global::g_Allocator, allocation);
        uint32_t* data;
        if (global::g_Allocator.mapMemory(allocation, reinterpret_cast<void**>(&data)) != vk::Result::eSuccess)
            IFX_ERROR("Failed to map index staging buffer");

        for (int i = 0; i < m_MaxQuads; ++i)
        {
            data[i * 6 + 0] = indices[0] + i * 4;
            data[i * 6 + 1] = indices[1] + i * 4;
            data[i * 6 + 2] = indices[2] + i * 4;
            data[i * 6 + 3] = indices[3] + i * 4;
            data[i * 6 + 4] = indices[4] + i * 4;
            data[i * 6 + 5] = indices[5] + i * 4;
        }
        global::g_Allocator.unmapMemory(allocation);

        m_vkIndexBuffer = vkhelper::create_buffer(sizeof(uint32_t) * m_MaxQuads * 6, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::SharingMode::eExclusive, vma::MemoryUsage::eGpuOnly, vma::AllocationCreateFlagBits::eDedicatedMemory, global::g_Allocator, m_vmaIndexAllocation);

        saf::vkhelper::immediate_submit(global::g_Device, global::g_GraphicsQueueIndex, [&stage_buffer, this](vk::CommandBuffer cmd)
            {
                vk::BufferCopy buffer_copy(0, 0, m_MaxQuads * 6 * sizeof(uint32_t));
                cmd.copyBuffer(stage_buffer, m_vkIndexBuffer, buffer_copy);
            });

        global::g_Allocator.destroyBuffer(stage_buffer, allocation);

        m_vkFontAtlas = vkhelper::CreateImage(global::g_Device, global::g_GraphicsQueueIndex, global::g_Allocator, m_FontAtlas->m_Width, m_FontAtlas->m_Height, 1, m_FontAtlas->m_RawData, m_vmaFontAtlasAllocation);

        vk::ImageViewCreateInfo imageview_create_info({}, m_vkFontAtlas, vk::ImageViewType::e2D, vk::Format::eR8Unorm, {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        m_vkFontAtlasView = global::g_Device.createImageView(imageview_create_info);

        vk::DescriptorSetLayoutBinding desc_layout_binding{};
        desc_layout_binding.binding = 0;
        desc_layout_binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
        desc_layout_binding.descriptorCount = 1;
        desc_layout_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
        desc_layout_binding.pImmutableSamplers = nullptr;

        vk::DescriptorSetLayoutCreateInfo desc_layout_info({}, { desc_layout_binding });
        m_vkFontDescriptorSetLayout = global::g_Device.createDescriptorSetLayout(desc_layout_info);

        vk::DescriptorSetAllocateInfo desc_alloc_info{};
        desc_alloc_info.descriptorPool = global::g_DescriptorPool;
        desc_alloc_info.descriptorSetCount = 1;
        desc_alloc_info.pSetLayouts = &m_vkFontDescriptorSetLayout;

        m_vkFontAtlasDescriptorSet = global::g_Device.allocateDescriptorSets(desc_alloc_info)[0];
        m_vkFontAtlasSampler = vkhelper::CreateFontSampler(global::g_Device);

        vk::DescriptorImageInfo desc_image_info{};
        desc_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        desc_image_info.imageView = m_vkFontAtlasView;
        desc_image_info.sampler = m_vkFontAtlasSampler;

        vk::WriteDescriptorSet desc_set_write(m_vkFontAtlasDescriptorSet, desc_layout_binding.binding, 0, vk::DescriptorType::eCombinedImageSampler, desc_image_info);
        global::g_Device.updateDescriptorSets(desc_set_write, {});

        vk::PushConstantRange pushconstant_range(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Uniform));

        vk::PipelineLayoutCreateInfo pipeline_layout_info({}, m_vkFontDescriptorSetLayout, pushconstant_range);
        m_vkPipelineLayout = global::g_Device.createPipelineLayout({ pipeline_layout_info });

        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages
        {
            vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eVertex,
                vkhelper::CreateShaderModule(global::g_Device, "assets/shaders/fonts.vert"),
                "main"
            ),
            vk::PipelineShaderStageCreateInfo(
                {},
                vk::ShaderStageFlagBits::eFragment,
                vkhelper::CreateShaderModule(global::g_Device, "assets/shaders/fonts.frag"),
                "main"
            )
        };

        std::array<vk::VertexInputAttributeDescription, 3> vertex_input_attrb_descs{
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, tex_coord))
        };

        auto vertex_input_binding_descs = Vertex::getBindingDescription();

        vk::PipelineVertexInputStateCreateInfo vertex_input({}, vertex_input_binding_descs, vertex_input_attrb_descs);

        // Our attachment will write to all color channels, but no blending is enabled.
        vk::PipelineColorBlendAttachmentState blend_attachment(
            vk::True,
            vk::BlendFactor::eOne,              // src factor
            vk::BlendFactor::eOne,              // dst factor
            vk::BlendOp::eAdd,                  // color blend op
            vk::BlendFactor::eOne,              // src alpha factor
            vk::BlendFactor::eOne,              // dst alpha factor
            vk::BlendOp::eAdd,                  // alpha blend op
            vk::ColorComponentFlagBits::eR |    // blend mask
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA
        );

        // Disable all depth testing.
        vk::PipelineDepthStencilStateCreateInfo depth_stencil;

        m_vkPipeline = vkhelper::CreateGraphicsPipeline(
            global::g_Device,
            nullptr,
            shader_stages,
            vertex_input,
            vk::PrimitiveTopology::eTriangleList,        // We will use triangle lists to draw geometry.
            0,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            vk::FrontFace::eClockwise,
            { blend_attachment },
            depth_stencil,
            m_vkPipelineLayout,
            global::g_SurfaceFormat.format
        ); // We need to specify the pipeline layout

        if (!m_vkPipeline) IFX_ERROR("Vulkan failed to create graphics pipeline");
        // Pipeline is baked, we can delete the shader modules now.
        global::g_Device.destroyShaderModule(shader_stages[0].module);
        global::g_Device.destroyShaderModule(shader_stages[1].module);
	}

    void Renderer2D::Shutdown()
    {

        if (m_vkFontDescriptorSetLayout) global::g_Device.destroyDescriptorSetLayout(m_vkFontDescriptorSetLayout);
        if (m_vkFontAtlasDescriptorSet) global::g_Device.freeDescriptorSets(global::g_DescriptorPool, { m_vkFontAtlasDescriptorSet });

        if (m_vkVertexBuffer)
        {
            global::g_Allocator.unmapMemory(m_vmaVertexAllocation);
            global::g_Allocator.destroyBuffer(m_vkVertexBuffer, m_vmaVertexAllocation);
        }
        if (m_vkIndexBuffer) global::g_Allocator.destroyBuffer(m_vkIndexBuffer, m_vmaIndexAllocation);
        if (m_vkFontAtlasView) global::g_Device.destroyImageView(m_vkFontAtlasView);
        if (m_vkFontAtlas) global::g_Allocator.destroyImage(m_vkFontAtlas, m_vmaFontAtlasAllocation);
        if (m_vkFontAtlasSampler) global::g_Device.destroySampler(m_vkFontAtlasSampler);

        if (m_vkPipeline) global::g_Device.destroy(m_vkPipeline);
        if (m_vkPipelineLayout) global::g_Device.destroyPipelineLayout(m_vkPipelineLayout);
    }

	void Renderer2D::BeginScene()
	{

	}

	void Renderer2D::Submit()
	{
        
	}

    void Renderer2D::Flush(vk::CommandBuffer cmd)
	{
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_vkPipeline);

		cmd.bindVertexBuffers(0, m_vkVertexBuffer, { 0UL });
		cmd.bindIndexBuffer(m_vkIndexBuffer, { 0UL }, vk::IndexType::eUint32);

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_vkPipelineLayout, 0, { m_vkFontAtlasDescriptorSet }, {});

        Uniform uniform{};
        uniform.projection_view = glm::mat4(1.f);
        uniform.model = glm::mat4(1.f);
        cmd.pushConstants<Uniform>(m_vkPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, uniform);

        cmd.drawIndexed(m_QuadCount * 6, 1, 0, 0, 0);

        m_QuadCount = 0;
	}

	void Renderer2D::EndScene()
	{

	}

    void Renderer2D::DrawString(std::string str, float pixelscale, glm::vec2 bounding_first, glm::vec2 bounding_second, Font font)
    {
        glm::vec2 position(std::min(bounding_first.x, bounding_second.x), std::min(bounding_first.y, bounding_second.y));


        glm::vec2 localPosition = position;
        for (char ch : str)
        {
            // Check if the charecter glyph is in the font atlas.
            if (ch >= m_FontAtlas->m_FirstCode && ch <= m_FontAtlas->m_FirstCode + m_FontAtlas->m_NumOfCodes)
            {

                // Retrive the data that is used to render a glyph of charecter 'ch'
                stbtt_packedchar* packedChar = &m_FontAtlas->m_PackedChars[ch - m_FontAtlas->m_FirstCode];
                stbtt_aligned_quad* alignedQuad = &m_FontAtlas->m_AlignedQuads[ch - m_FontAtlas->m_FirstCode];

                // The units of the fields of the above structs are in pixels, 
                // convert them to a unit of what we want be multilplying to pixelScale  
                glm::vec2 glyphSize =
                {
                    (packedChar->x1 - packedChar->x0) * pixelscale * font.scale,
                    (packedChar->y1 - packedChar->y0) * pixelscale * font.scale
                };

                glm::vec2 glyphBoundingBoxBottomLeft =
                {
                    localPosition.x + packedChar->xoff * pixelscale * font.scale,
                    localPosition.y + (packedChar->yoff + m_FontAtlas->m_FontSize) * pixelscale * font.scale
                };
                //

                if (glyphBoundingBoxBottomLeft.x + glyphSize.x >= std::max(bounding_first.x, bounding_second.x))
                {
                    localPosition.y += m_FontAtlas->m_FontSize * pixelscale * font.scale;
                    localPosition.x = position.x;

                    glyphSize =
                    {
                        (packedChar->x1 - packedChar->x0) * pixelscale * font.scale,
                        (packedChar->y1 - packedChar->y0) * pixelscale * font.scale
                    };

                    glyphBoundingBoxBottomLeft =
                    {
                        localPosition.x + packedChar->xoff * pixelscale * font.scale,
                        localPosition.y + (packedChar->yoff + m_FontAtlas->m_FontSize) * pixelscale * font.scale
                    };
                }

                uint32_t vertex_offs = m_QuadCount * 4;
                ++m_QuadCount;
                
                m_VertexBuffer[vertex_offs + 0].pos = glm::vec3(glyphBoundingBoxBottomLeft.x + glyphSize.x, glyphBoundingBoxBottomLeft.y + glyphSize.y, 0.f);
                m_VertexBuffer[vertex_offs + 1].pos = glm::vec3(glyphBoundingBoxBottomLeft.x, glyphBoundingBoxBottomLeft.y + glyphSize.y, 0.f);
                m_VertexBuffer[vertex_offs + 2].pos = glm::vec3(glyphBoundingBoxBottomLeft.x, glyphBoundingBoxBottomLeft.y, 0.f);
                m_VertexBuffer[vertex_offs + 3].pos = glm::vec3(glyphBoundingBoxBottomLeft.x + glyphSize.x, glyphBoundingBoxBottomLeft.y, 0.f);

                m_VertexBuffer[vertex_offs + 0].tex_coord = glm::vec2(alignedQuad->s1, alignedQuad->t1);
                m_VertexBuffer[vertex_offs + 1].tex_coord = glm::vec2(alignedQuad->s0, alignedQuad->t1);
                m_VertexBuffer[vertex_offs + 2].tex_coord = glm::vec2(alignedQuad->s0, alignedQuad->t0);
                m_VertexBuffer[vertex_offs + 3].tex_coord = glm::vec2(alignedQuad->s1, alignedQuad->t0);

                m_VertexBuffer[vertex_offs + 0].color = glm::vec4(1.f, 1.f, 1.f, 1.f);
                m_VertexBuffer[vertex_offs + 1].color = glm::vec4(1.f, 1.f, 1.f, 1.f);
                m_VertexBuffer[vertex_offs + 2].color = glm::vec4(1.f, 1.f, 1.f, 1.f);
                m_VertexBuffer[vertex_offs + 3].color = glm::vec4(1.f, 1.f, 1.f, 1.f);

                // Update the position to render the next glyph specified by packedChar->xadvance.
                localPosition.x += packedChar->xadvance * pixelscale * font.scale;
            }
            else if (ch == '\n')
            {
                // advance y by fontSize, reset x-coordinate
                localPosition.y += m_FontAtlas->m_FontSize * pixelscale * font.scale;
                localPosition.x = position.x;
            }
            else if (ch == '\t')
            {
                localPosition.x = (floor(localPosition.x / (m_FontAtlas->m_FontSize * pixelscale * font.scale * 4.f)) + 1.f) * (m_FontAtlas->m_FontSize * pixelscale * font.scale * 4.f);
            }
        }
    }

}