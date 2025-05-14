#include "safpch.h"
#include "renderer2d.h"

#include <stb_truetype.h>
#include <stb_image_write.h>
#include <fstream>
#include "platform/vulkangraphics.h"

#include <glm/gtc/matrix_transform.hpp>

#include "globals.h"
#include <string>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

namespace saf {

    Font::Font(FontType _fonttype, float _scale, glm::vec4 _color)
        : fonttype(_fonttype), scale(_scale), color(_color)
    {

    }

    void Font::EnableCharRotate(float _char_rotate_angle)
    {
        char_rotate_angle = _char_rotate_angle;
    }

    void Font::EnableTranslate(glm::vec3 _translation)
    {
        translation = _translation;
    }

    void Font::EnableRotate(float _rotate_angle)
    {
        rotate_angle = _rotate_angle;
    }

    void Font::EnableFade(float _fade)
    {
        fade = _fade;
    }

    void Font::EnableScale(float _scale)
    {
        scale = _scale;
    }

    FontAtlas::FontAtlas(std::vector<std::string> files, int width, int height, int firstCode, int numofchars)
        : m_FirstCode(firstCode), m_NumOfCodes(numofchars), m_Width(width), m_Height(height)
	{
        int number_of_fonts = files.size();
        m_NumOfFonts = number_of_fonts;

        m_PackedChars = new stbtt_packedchar[m_NumOfCodes * number_of_fonts];
        m_AlignedQuads = new stbtt_aligned_quad[m_NumOfCodes * number_of_fonts];
        m_RawData = new uint8_t[m_Width * m_Height * number_of_fonts];

        // Read the font file

        for (int n = 0; n < number_of_fonts; ++n)
        {
            std::string file = files[n];
            std::ifstream inputStream(file.c_str(), std::ios::binary);

            inputStream.seekg(0, std::ios::end);
            auto&& fontFileSize = inputStream.tellg();
            inputStream.seekg(0, std::ios::beg);

            uint8_t* fontDataBuf = new uint8_t[fontFileSize];
            inputStream.read((char*)fontDataBuf, fontFileSize);

            stbtt_fontinfo fontInfo = {};

            uint32_t fontCount = stbtt_GetNumberOfFonts(fontDataBuf);
            IFX_TRACE("Font file: {0} contains {1} fonts", files[n], fontCount);

            if (!stbtt_InitFont(&fontInfo, fontDataBuf, 0))
                std::cerr << "stbtt_InitFont() Failed!\n";

            stbtt_pack_context ctx{};

           uint8_t* pPixelData = &m_RawData[n * m_Width * m_Height];
           stbtt_packedchar* pChardata = &m_PackedChars[n * m_NumOfCodes];

            stbtt_PackBegin(
                &ctx,                                     // stbtt_pack_context (this call will initialize it) 
                static_cast<unsigned char*>(pPixelData),                // Font Atlas texture data
                m_Width,                                  // Width of the font atlas texture
                m_Height,                                 // Height of the font atlas texture
                0,                                        // Stride in bytes
                1,                                        // Padding between the glyphs
                nullptr);

            stbtt_PackFontRange(
                &ctx,                                     // stbtt_pack_context
                fontDataBuf,                              // Font Atlas texture data
                0,                         // Font Index                                 
                64.f,                                     // Size of font in pixels. (Use STBTT_POINT_SIZE(fontSize) to use points) 
                firstCode,                                // Code point of the first charecter
                m_NumOfCodes,                             // No. of charecters to be included in the font atlas 
                pChardata                             // stbtt_packedchar array, this struct will contain the data to render a glyph
            );

            stbtt_PackEnd(&ctx);

            for (int i = 0; i < m_NumOfCodes; ++i)
            {
                float unusedX, unusedY;

                stbtt_GetPackedQuad(
                    pChardata,              // Array of stbtt_packedchar
                    m_Width,                           // Width of the font atlas texture
                    m_Height,                          // Height of the font atlas texture
                    i,                                 // Index of the glyph
                    &unusedX, &unusedY,                // current position of the glyph in screen pixel coordinates, (not required as we have a different corrdinate system)
                    &(m_AlignedQuads[i + n * m_NumOfCodes]),              // stbtt_alligned_quad struct. (this struct mainly consists of the texture coordinates)
                    0                                  // Allign X and Y position to a integer (doesn't matter because we are not using 'unusedX' and 'unusedY')
                );
            }

            delete[] fontDataBuf;

            stbi_write_png((std::string("fontAtlas") + std::to_string(n) + ".png").c_str(), m_Width, m_Height, 1, pPixelData, m_Width);
        }

        // Optionally write the font atlas texture as a png file.

        m_vkFontAtlas = vkhelper::CreateImage3D(global::g_Device, global::g_GraphicsQueueIndex, global::g_Allocator, m_Width, m_Height, number_of_fonts, 1, m_RawData, m_vmaFontAtlasAllocation);

        vk::ImageViewCreateInfo imageview_create_info({}, m_vkFontAtlas, vk::ImageViewType::e3D, vk::Format::eR8Unorm, {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        m_vkFontAtlasView = global::g_Device.createImageView(imageview_create_info);

        Free();
	}

    void FontAtlas::Shutdown()
    {
        if (m_vkFontAtlasView) global::g_Device.destroyImageView(m_vkFontAtlasView);
        if (m_vkFontAtlas) global::g_Allocator.destroyImage(m_vkFontAtlas, m_vmaFontAtlasAllocation);
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
        CreateSwapchain();

        m_vkVertexBuffer = vkhelper::create_buffer(sizeof(Vertex) * m_MaxQuads * 4, vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive, vma::MemoryUsage::eAutoPreferHost, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, global::g_Allocator, m_vmaVertexAllocation);
        if (global::g_Allocator.mapMemory(m_vmaVertexAllocation, reinterpret_cast<void**>(&m_VertexBuffer)) != vk::Result::eSuccess)
            IFX_ERROR("Failed to map vertex buffer");

        m_vkBasicVertexBuffer = vkhelper::create_buffer(sizeof(Vertex) * m_MaxQuads * 4, vk::BufferUsageFlagBits::eVertexBuffer, vk::SharingMode::eExclusive, vma::MemoryUsage::eAutoPreferHost, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, global::g_Allocator, m_vmaBasicVertexAllocation);
        if (global::g_Allocator.mapMemory(m_vmaBasicVertexAllocation, reinterpret_cast<void**>(&m_BasicVertexBuffer)) != vk::Result::eSuccess)
            IFX_ERROR("Failed to map vertex buffer");

        uint32_t indices[6]
        {
            0, 1, 2, 0, 2, 3
        };

        vma::Allocation allocation;
        vk::Buffer stage_buffer = vkhelper::create_buffer(sizeof(uint32_t) * m_MaxQuads * 6, vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eIndexBuffer, vk::SharingMode::eExclusive, vma::MemoryUsage::eAutoPreferHost, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, global::g_Allocator, allocation);
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

        m_vkIndexBuffer = vkhelper::create_buffer(sizeof(uint32_t) * m_MaxQuads * 6, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::SharingMode::eExclusive, vma::MemoryUsage::eAutoPreferDevice, vma::AllocationCreateFlagBits::eDedicatedMemory, global::g_Allocator, m_vmaIndexAllocation);

        saf::vkhelper::immediate_submit(global::g_Device, global::g_GraphicsQueueIndex, [&stage_buffer, this](vk::CommandBuffer cmd)
            {
                vk::BufferCopy buffer_copy(0, 0, m_MaxQuads * 6 * sizeof(uint32_t));
                cmd.copyBuffer(stage_buffer, m_vkIndexBuffer, buffer_copy);
            });

        global::g_Allocator.destroyBuffer(stage_buffer, allocation);

        {
            std::vector<std::string> files
            {
                "C:/Windows/Fonts/arial.ttf",
                "C:/Windows/Fonts/comic.ttf",
                "C:/Windows/Fonts/times.ttf",
                "C:/Windows/Fonts/inkfree.ttf",
                "C:/Windows/Fonts/impact.ttf",
                "assets/chiller.ttf"
            };
            m_FontAtlas = std::make_shared<FontAtlas>(files, 512, 512, 32, 96);

            vk::DescriptorSetLayoutBinding desc_layout_binding{};
            desc_layout_binding.binding = 0;
            desc_layout_binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            desc_layout_binding.descriptorCount = 1;
            desc_layout_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;
            desc_layout_binding.pImmutableSamplers = nullptr;

            vk::DescriptorSetLayoutCreateInfo desc_layout_info({}, { desc_layout_binding });
            m_vkAtlasDescriptorSetLayout = global::g_Device.createDescriptorSetLayout(desc_layout_info);

            vk::DescriptorSetAllocateInfo desc_alloc_info{};
            desc_alloc_info.descriptorPool = global::g_DescriptorPool;
            desc_alloc_info.descriptorSetCount = 1;
            desc_alloc_info.pSetLayouts = &m_vkAtlasDescriptorSetLayout;

            m_vkAtlasDescriptorSet = global::g_Device.allocateDescriptorSets(desc_alloc_info)[0];
            m_vkAtlasSampler = vkhelper::CreateFontSampler(global::g_Device);

            vk::DescriptorImageInfo desc_image_info;
            desc_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            desc_image_info.imageView = m_FontAtlas->m_vkFontAtlasView;
            desc_image_info.sampler = m_vkAtlasSampler;

            vk::WriteDescriptorSet desc_set_write(m_vkAtlasDescriptorSet, desc_layout_binding.binding, 0, vk::DescriptorType::eCombinedImageSampler, desc_image_info);
            global::g_Device.updateDescriptorSets(desc_set_write, {});

            vk::PushConstantRange pushconstant_range(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Uniform));

            vk::PipelineLayoutCreateInfo pipeline_layout_info({}, m_vkAtlasDescriptorSetLayout, pushconstant_range);
            m_vkAtlasPipelineLayout = global::g_Device.createPipelineLayout({ pipeline_layout_info });

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

            auto vertex_input_binding_descs = Vertex::getBindingDescription();
            auto vertex_input_attrib_descs = Vertex::getAttributeDescription();

            vk::PipelineVertexInputStateCreateInfo vertex_input({}, vertex_input_binding_descs, vertex_input_attrib_descs);

            // Our attachment will write to all color channels, but no blending is enabled.
            vk::PipelineColorBlendAttachmentState blend_attachment(
                vk::True,
                vk::BlendFactor::eSrcAlpha,              // src factor
                vk::BlendFactor::eDstAlpha,              // dst factor
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

            m_vkAtlasPipeline = vkhelper::CreateGraphicsPipeline(
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
                m_vkAtlasPipelineLayout,
                global::g_SurfaceFormat.format
            ); // We need to specify the pipeline layout

            if (!m_vkAtlasPipeline) IFX_ERROR("Vulkan failed to create atlas graphics pipeline");
            // Pipeline is baked, we can delete the shader modules now.
            global::g_Device.destroyShaderModule(shader_stages[0].module);
            global::g_Device.destroyShaderModule(shader_stages[1].module);
        }

        {

            vk::PushConstantRange pushconstant_range(vk::ShaderStageFlagBits::eVertex, 0, sizeof(Uniform));

            vk::PipelineLayoutCreateInfo pipeline_layout_info({}, {}, pushconstant_range);
            m_vkQuadPipelineLayout = global::g_Device.createPipelineLayout({ pipeline_layout_info });

            std::vector<vk::PipelineShaderStageCreateInfo> shader_stages
            {
                vk::PipelineShaderStageCreateInfo(
                    {},
                    vk::ShaderStageFlagBits::eVertex,
                    vkhelper::CreateShaderModule(global::g_Device, "assets/shaders/quad.vert"),
                    "main"
                ),
                vk::PipelineShaderStageCreateInfo(
                    {},
                    vk::ShaderStageFlagBits::eFragment,
                    vkhelper::CreateShaderModule(global::g_Device, "assets/shaders/quad.frag"),
                    "main"
                )
            };

            auto vertex_input_binding_descs = BasicVertex::getBindingDescription();
            auto vertex_input_attrib_descs = BasicVertex::getAttributeDescription();

            vk::PipelineVertexInputStateCreateInfo vertex_input({}, vertex_input_binding_descs, vertex_input_attrib_descs);

            // Our attachment will write to all color channels, but no blending is enabled.
            vk::PipelineColorBlendAttachmentState blend_attachment(
                vk::True,
                vk::BlendFactor::eSrcAlpha,              // src factor
                vk::BlendFactor::eDstAlpha,              // dst factor
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

            m_vkQuadPipeline = vkhelper::CreateGraphicsPipeline(
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
                m_vkQuadPipelineLayout,
                global::g_SurfaceFormat.format
            ); // We need to specify the pipeline layout

            if (!m_vkQuadPipeline) IFX_ERROR("Vulkan failed to create basic graphics pipeline");
            // Pipeline is baked, we can delete the shader modules now.
            global::g_Device.destroyShaderModule(shader_stages[0].module);
            global::g_Device.destroyShaderModule(shader_stages[1].module);
        }
        {

            std::array<vk::DescriptorSetLayoutBinding, 2> desc_bindings {};
            desc_bindings[0].binding = 0;
            desc_bindings[0].descriptorCount = 1;
            desc_bindings[0].descriptorType = vk::DescriptorType::eStorageImage;
            desc_bindings[0].pImmutableSamplers = nullptr;
            desc_bindings[0].stageFlags = vk::ShaderStageFlagBits::eCompute;

            desc_bindings[1].binding = 1;
            desc_bindings[1].descriptorCount = 1;
            desc_bindings[1].descriptorType = vk::DescriptorType::eStorageImage;
            desc_bindings[1].pImmutableSamplers = nullptr;
            desc_bindings[1].stageFlags = vk::ShaderStageFlagBits::eCompute;

            vk::DescriptorSetLayoutCreateInfo desc_create_info({}, desc_bindings);
            vk::DescriptorSetLayout desc_layout = global::g_Device.createDescriptorSetLayout(desc_create_info);
            if (!desc_layout) IFX_ERROR("Vulkan failed to create descriptor set layout");

            vk::DescriptorSetAllocateInfo desc_alloc_info(global::g_DescriptorPool, desc_layout);
            vk::DescriptorSet desc_set = global::g_Device.allocateDescriptorSets(desc_alloc_info)[0];
            if (!desc_set) IFX_ERROR("Vulkan failed to allocate descriptor set");

            int width, height;
            stbi_uc* rawimage = stbi_load("assets/test.png", &width, &height, nullptr, 4);

            vma::Allocation image_allocation;
            vk::Image image1 = vkhelper::CreateImage(global::g_Device, global::g_GraphicsQueueIndex, global::g_Allocator, width, height, 4, static_cast<uint8_t*>(rawimage), image_allocation, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst, vk::ImageLayout::eTransferDstOptimal);
            vk::Image image2 = vkhelper::CreateImage(global::g_Device, global::g_GraphicsQueueIndex, global::g_Allocator, width, height, 4, nullptr, image_allocation, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc, vk::ImageLayout::eGeneral);
            vk::ImageViewCreateInfo image_view_create_info1({}, image1, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm, {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
            vk::ImageViewCreateInfo image_view_create_info2({}, image2, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm, {}, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
            vk::ImageView image_view1 = global::g_Device.createImageView(image_view_create_info1);
            vk::ImageView image_view2 = global::g_Device.createImageView(image_view_create_info2);

            global::g_GraphicsQueue.waitIdle();
            stbi_image_free(rawimage);

            vk::DescriptorImageInfo image_info1{};
            image_info1.imageLayout = vk::ImageLayout::eGeneral;
            image_info1.imageView = image_view1;
            vk::DescriptorImageInfo image_info2{};
            image_info2.imageLayout = vk::ImageLayout::eGeneral;
            image_info2.imageView = image_view2;

            std::array<vk::WriteDescriptorSet, 2> desc_writes
            {
                vk::WriteDescriptorSet(desc_set, 0, 0, desc_bindings[0].descriptorType, image_info1, {}, {}),
                vk::WriteDescriptorSet(desc_set, 1, 0, desc_bindings[1].descriptorType, image_info2, {}, {})
            };

            global::g_Device.updateDescriptorSets(desc_writes, {});

            vk::PipelineLayoutCreateInfo pipeline_layout_info({}, desc_layout, {});
            m_vkComputePipelineLayout = global::g_Device.createPipelineLayout({pipeline_layout_info});

            vk::PipelineShaderStageCreateInfo shader_stage(
                {},
                vk::ShaderStageFlagBits::eCompute,
                vkhelper::CreateShaderModule(global::g_Device, "assets/shaders/test.comp"),
                "main"
            );

            m_vkComputePipeline = vkhelper::CreateComputePipeline(
                global::g_Device,
                nullptr,
                shader_stage,
                m_vkComputePipelineLayout
            ); // We need to specify the pipeline layout

            if (!m_vkComputePipelineLayout) IFX_ERROR("Vulkan failed to create compute pipeline");
            // Pipeline is baked, we can delete the shader modules now.
            global::g_Device.destroyShaderModule(shader_stage.module);

            vma::Allocation recieve_buffer_alloc;
            vk::Buffer recieve_buffer = vkhelper::create_buffer(width * height * 4, vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive, vma::MemoryUsage::eAutoPreferDevice, vma::AllocationCreateFlagBits::eHostAccessSequentialWrite, global::g_Allocator, recieve_buffer_alloc);

            vkhelper::immediate_submit(global::g_Device, global::g_ComputeQueueIndex, [this, desc_set, width, height, image1, image2, recieve_buffer](vk::CommandBuffer cmd)
            {
                cmd.bindPipeline(vk::PipelineBindPoint::eCompute, m_vkComputePipeline);

                vk::ImageMemoryBarrier image_memory_barrier_pre(
                    vk::AccessFlagBits::eNone,
                    vk::AccessFlagBits::eShaderRead,
                    vk::ImageLayout::eTransferDstOptimal,
                    vk::ImageLayout::eGeneral,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image1,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                );

                cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, image_memory_barrier_pre);

                vk::ImageMemoryBarrier image_memory_barrier_pre2(
                    vk::AccessFlagBits::eNone,
                    vk::AccessFlagBits::eShaderWrite,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eGeneral,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image2,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                );

                cmd.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {}, image_memory_barrier_pre2);

                cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_vkComputePipelineLayout, 0, desc_set, {});
                cmd.dispatch(width / 64, height, 1);

                vk::ImageMemoryBarrier image_memory_barrier_post(
                    vk::AccessFlagBits::eShaderWrite,
                    vk::AccessFlagBits::eTransferWrite,
                    vk::ImageLayout::eGeneral,
                    vk::ImageLayout::eTransferSrcOptimal,
                    VK_QUEUE_FAMILY_IGNORED,
                    VK_QUEUE_FAMILY_IGNORED,
                    image2,
                    vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
                );

                cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, image_memory_barrier_post);

                vk::BufferImageCopy buffer_image_copy(0, 0, 0, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), vk::Offset3D(0, 0, 0), vk::Extent3D(width, height, 1));

                cmd.copyImageToBuffer(image2, vk::ImageLayout::eTransferSrcOptimal, recieve_buffer, buffer_image_copy);
            });

            global::g_ComputeQueue.waitIdle();

            const void* recieve_data = static_cast<stbi_uc*>(global::g_Allocator.mapMemory(recieve_buffer_alloc));
            stbi_write_jpg("output.jpeg", width, height, 4, recieve_data, 0);

        }
	}

    void Renderer2D::Shutdown()
    {
        global::g_Device.waitIdle();

        if (m_vkAtlasDescriptorSetLayout) global::g_Device.destroyDescriptorSetLayout(m_vkAtlasDescriptorSetLayout);
        if (m_vkAtlasDescriptorSet) global::g_Device.freeDescriptorSets(global::g_DescriptorPool, { m_vkAtlasDescriptorSet });

        if (m_vkVertexBuffer)
        {
            global::g_Allocator.unmapMemory(m_vmaVertexAllocation);
            global::g_Allocator.destroyBuffer(m_vkVertexBuffer, m_vmaVertexAllocation);
        }
        if (m_vkBasicVertexBuffer)
        {
            global::g_Allocator.unmapMemory(m_vmaBasicVertexAllocation);
            global::g_Allocator.destroyBuffer(m_vkBasicVertexBuffer, m_vmaBasicVertexAllocation);
        }
        if (m_vkIndexBuffer) global::g_Allocator.destroyBuffer(m_vkIndexBuffer, m_vmaIndexAllocation);

        if (m_FontAtlas) m_FontAtlas->Shutdown();
        if (m_vkAtlasSampler) global::g_Device.destroySampler(m_vkAtlasSampler);

        if (m_vkAtlasPipeline) global::g_Device.destroy(m_vkAtlasPipeline);
        if (m_vkAtlasPipelineLayout) global::g_Device.destroyPipelineLayout(m_vkAtlasPipelineLayout);

        if (m_vkQuadPipeline) global::g_Device.destroy(m_vkQuadPipeline);
        if (m_vkQuadPipelineLayout) global::g_Device.destroyPipelineLayout(m_vkQuadPipelineLayout);

        for (auto semiphore : m_vkRecycleSemaphores) if (semiphore) global::g_Device.destroySemaphore(semiphore);
        if (m_vkSwapchainData.swapchain) DestroySwapchain(m_vkSwapchainData.swapchain);
    }

	void Renderer2D::BeginScene()
	{

	}

	void Renderer2D::Submit()
	{
        
	}

    bool Renderer2D::Flush(const glm::mat4& projection)
	{
        vk::Semaphore acquire_semaphore;
        if (m_vkRecycleSemaphores.empty())
        {
            acquire_semaphore = global::g_Device.createSemaphore({});
        }
        else
        {
            acquire_semaphore = m_vkRecycleSemaphores.back();
            m_vkRecycleSemaphores.pop_back();
        }

        vk::Result res;
        uint32_t   index;
        std::tie(res, index) = global::g_Device.acquireNextImageKHR(m_vkSwapchainData.swapchain, UINT64_MAX, acquire_semaphore);

        if (res != vk::Result::eSuccess)
        {
            m_vkRecycleSemaphores.push_back(acquire_semaphore);
            IFX_WARN("Vulkan failed acquireNextImage");
            global::g_GraphicsQueue.waitIdle();
            return false;
        }
        else
        {
            // If we have outstanding fences for this swapchain image, wait for them to complete first. Then reset them.
            if (m_vkFramesData[index].queue_submit_fence)
            {
                (void)global::g_Device.waitForFences(m_vkFramesData[index].queue_submit_fence, true, UINT64_MAX);
                global::g_Device.resetFences(m_vkFramesData[index].queue_submit_fence);
            }

            if (m_vkFramesData[index].primary_command_pool)
            {
                global::g_Device.resetCommandPool(m_vkFramesData[index].primary_command_pool);
            }

            // Recycle the old semaphore back into the semaphore manager.
            vk::Semaphore old_semaphore = m_vkFramesData[index].swapchain_acquire_semaphore;

            if (old_semaphore)
            {
                m_vkRecycleSemaphores.push_back(old_semaphore);
            }

            m_vkFramesData[index].swapchain_acquire_semaphore = acquire_semaphore;
        }

        vk::CommandBuffer cmd = m_vkFramesData[index].primary_command_buffer;

        // We will only submit this once before it's recycled.
        vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        // Begin command recording

        vk::Image swapchain_image = global::g_Device.getSwapchainImagesKHR(m_vkSwapchainData.swapchain)[index];

        cmd.begin(begin_info);

        vk::ImageMemoryBarrier image_memory_barrier(
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            0U,
            0U,
            swapchain_image,
            vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,0U, 1U, 0U, 1U
            });

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, { image_memory_barrier });

        // Set clear color values.
        vk::ClearValue clear_value;
        clear_value.color = vk::ClearColorValue(std::array<float, 4>({ {0.01f, 0.01f, 0.033f, 1.0f} }));

        const vk::RenderingAttachmentInfo color_attachment_info(m_vkSwapchainData.image_views[index], vk::ImageLayout::eAttachmentOptimalKHR, vk::ResolveModeFlagBits::eNone, {}, vk::ImageLayout::eUndefined, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, clear_value);

        vk::RenderingInfo rendering_info({}, { {0, 0}, {m_vkSwapchainData.extent.width, m_vkSwapchainData.extent.height} }, 1, 0, { color_attachment_info }, nullptr, nullptr);

        cmd.beginRenderingKHR(rendering_info);

        vk::Viewport vp(0.0f, 0.0f, static_cast<float>(m_vkSwapchainData.extent.width), static_cast<float>(m_vkSwapchainData.extent.height), 0.0f, 1.0f);
        // Set viewport dynamically
        cmd.setViewport(0, vp);

        vk::Rect2D scissor({ 0, 0 }, { m_vkSwapchainData.extent.width, m_vkSwapchainData.extent.height });
        // Set scissor dynamically
        cmd.setScissor(0, scissor);

        //Atlas Draw
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_vkAtlasPipeline);

        cmd.bindVertexBuffers(0, m_vkVertexBuffer, { 0UL });
        cmd.bindIndexBuffer(m_vkIndexBuffer, { 0UL }, vk::IndexType::eUint32);

        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_vkAtlasPipelineLayout, 0, { m_vkAtlasDescriptorSet }, {});

        Uniform uniform{};
        uniform.projection_view = projection;
        uniform.model = glm::mat4(1.f);
        cmd.pushConstants<Uniform>(m_vkAtlasPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, uniform);

        cmd.drawIndexed(m_AtlasQuadCount * 6, 1, 0, 0, 0);
        m_AtlasQuadCount = 0;

        //Basic Draw

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_vkQuadPipeline);

        cmd.bindVertexBuffers(0, m_vkBasicVertexBuffer, { 0UL });
        cmd.bindIndexBuffer(m_vkIndexBuffer, { 0UL }, vk::IndexType::eUint32);

        //cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_vkQuadPipelineLayout, 0, { m_vkAtlasDescriptorSet }, {});
        cmd.pushConstants<Uniform>(m_vkQuadPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, uniform);

        cmd.drawIndexed(m_BasicQuadCount * 6, 1, 0, 0, 0);
        m_BasicQuadCount = 0;

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        cmd.endRenderingKHR();

        vk::ImageMemoryBarrier image_memory_barrier2(
            vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eNone,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            0U,
            0U,
            swapchain_image,
            vk::ImageSubresourceRange{
                vk::ImageAspectFlagBits::eColor,0U, 1U, 0U, 1U
            });

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, { image_memory_barrier2 });

        // Complete the command buffer.
        cmd.end();

        // Submit it to the queue with a release semaphore.
        if (!m_vkFramesData[index].swapchain_release_semaphore)
        {
            m_vkFramesData[index].swapchain_release_semaphore = global::g_Device.createSemaphore({});
        }

        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo info(
            m_vkFramesData[index].swapchain_acquire_semaphore,
            wait_stage,
            cmd,
            m_vkFramesData[index].swapchain_release_semaphore
        );
        // Submit command buffer to graphics queue
        global::g_GraphicsQueue.submit(info, m_vkFramesData[index].queue_submit_fence);

        // Present swapchain image
        vk::PresentInfoKHR present_info(m_vkFramesData[index].swapchain_release_semaphore, m_vkSwapchainData.swapchain, index);
        res = global::g_GraphicsQueue.presentKHR(present_info);

        if (res != vk::Result::eSuccess)
        {
            IFX_ERROR("Failed to present swapchain image.");
        }

        return true;
	}

	void Renderer2D::EndScene()
	{

    }

    void Renderer2D::Resize(uint32_t width, uint32_t height)
    {
        if (width == m_Width && height == m_Height) return;
        m_Width = width;
        m_Height = height;
        global::g_Device.waitIdle();
        CreateSwapchain();
    }

    void Renderer2D::CreateSwapchain()
    {
        IFX_TRACE("Window CreateSwapchain");
        vk::SurfaceCapabilitiesKHR capabilities = global::g_PhysicalDevice.getSurfaceCapabilitiesKHR(global::g_Surface);
        std::vector<vk::PresentModeKHR> supported_preset_modes = global::g_PhysicalDevice.getSurfacePresentModesKHR(global::g_Surface);

        if (supported_preset_modes.size() == 0) IFX_ERROR("Vulkan surface doesnt support any presentModes?");

        m_vkSwapchainData.format = global::g_SurfaceFormat.format;

        uint32_t desired_swapchain_images = capabilities.minImageCount + 1;
        if ((capabilities.maxImageCount > 0) && (desired_swapchain_images > capabilities.maxImageCount))
        {
            // Application must settle for fewer images than desired.
            desired_swapchain_images = capabilities.maxImageCount;
            IFX_WARN("Vulkan settling for {0} swapchain images, {1} desired", capabilities.maxImageCount, desired_swapchain_images);
        }

        vk::SurfaceTransformFlagBitsKHR pre_transform =
            (capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ?
            vk::SurfaceTransformFlagBitsKHR::eIdentity : capabilities.currentTransform;

        vk::CompositeAlphaFlagBitsKHR composite = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eOpaque)
        {
            composite = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        }
        else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit)
        {
            composite = vk::CompositeAlphaFlagBitsKHR::eInherit;
        }
        else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
        {
            composite = vk::CompositeAlphaFlagBitsKHR::ePreMultiplied;
        }
        else if (capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
        {
            composite = vk::CompositeAlphaFlagBitsKHR::ePostMultiplied;
        }

        vk::Extent2D extent{
            std::min(m_Width, capabilities.currentExtent.width),
            std::min(m_Height, capabilities.currentExtent.height) };

        m_vkSwapchainData.extent = extent;
        vk::SwapchainKHR old_swapchain = m_vkSwapchainData.swapchain;

        vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
        for (const auto mode : supported_preset_modes)
        {
            if (mode == vk::PresentModeKHR::eMailbox)
            {
                present_mode = mode;
                break;
            }
        }

        if (present_mode == vk::PresentModeKHR::eFifo) IFX_TRACE("Vulkan physical device doesnt support mailbox present mode, defaulting to FiFo");
        else IFX_TRACE("Vulkan using mailbox present mode");

        vk::SwapchainCreateInfoKHR swapchain_create_info(
            {},                                                 //flags_                 = {}
            global::g_Surface,                                        //surface_               = {}
            desired_swapchain_images,                           //minImageCount_         = {}
            global::g_SurfaceFormat.format,                                //imageFormat_           = VULKAN_HPP_NAMESPACE::Format::eUndefined
            global::g_SurfaceFormat.colorSpace,                            //imageColorSpace_       = VULKAN_HPP_NAMESPACE::ColorSpaceKHR::eSrgbNonlinear
            extent,                                             //imageExtent_           = {}
            1,                                                  //imageArrayLayers_      = {}
            vk::ImageUsageFlagBits::eColorAttachment,           //imageUsage_            = {},
            vk::SharingMode::eExclusive,                        //imageSharingMode_      = VULKAN_HPP_NAMESPACE::SharingMode::eExclusive,
            {},                                                 //queueFamilyIndexCount_ = {},
            {},                                                 //pQueueFamilyIndices_   = {},
            pre_transform,                                      //preTransform_          = VULKAN_HPP_NAMESPACE::SurfaceTransformFlagBitsKHR::eIdentity,
            composite,                                          //compositeAlpha_        = VULKAN_HPP_NAMESPACE::CompositeAlphaFlagBitsKHR::eOpaque,
            present_mode,                                       //presentMode_           = VULKAN_HPP_NAMESPACE::PresentModeKHR::eImmediate,
            VK_TRUE,                                            //clipped_               = {},
            old_swapchain                                       //oldSwapchain_          = {},
        );

        m_vkSwapchainData.swapchain = global::g_Device.createSwapchainKHR(swapchain_create_info);
        if (!m_vkSwapchainData.swapchain) IFX_ERROR("Vulkan failed to create swapchain");

        DestroySwapchain(old_swapchain);

        std::vector<vk::Image> swapchain_images = global::g_Device.getSwapchainImagesKHR(m_vkSwapchainData.swapchain);
        size_t image_count = swapchain_images.size();

        // Initialize per-frame resources.
        // Every swapchain image has its own command pool and fence manager.
        // This makes it very easy to keep track of when we can reset command buffers and such.
        m_vkFramesData.clear();
        m_vkFramesData.resize(image_count);

        for (auto& frame_data : m_vkFramesData)
        {
            frame_data.queue_submit_fence = global::g_Device.createFence({ vk::FenceCreateFlagBits::eSignaled });
            frame_data.primary_command_pool = global::g_Device.createCommandPool({ vk::CommandPoolCreateFlagBits::eTransient, global::g_GraphicsQueueIndex });
            vk::CommandBufferAllocateInfo command_buffer_allocate_info(frame_data.primary_command_pool, vk::CommandBufferLevel::ePrimary, 1);
            frame_data.primary_command_buffer = global::g_Device.allocateCommandBuffers(command_buffer_allocate_info).front();
        }

        for (size_t i = 0; i < image_count; i++)
        {
            // Create an image view which we can render into.
            vk::ImageViewCreateInfo image_view_create_info({},
                swapchain_images[i],
                vk::ImageViewType::e2D,
                global::g_SurfaceFormat.format,
                { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA },
                { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

            vk::ImageView image_view = global::g_Device.createImageView(image_view_create_info);
            if (!image_view) IFX_ERROR("Vulkan failed to create imageview");

            m_vkSwapchainData.image_views.push_back(image_view);
        }
    }

    void Renderer2D::DestroySwapchain(vk::SwapchainKHR swapchain)
    {
        if (!swapchain) return;

        for (vk::ImageView image_view : m_vkSwapchainData.image_views) { global::g_Device.destroyImageView(image_view); }
        m_vkSwapchainData.image_views.clear();

        for (auto& frame_data : m_vkFramesData) {
            if (frame_data.queue_submit_fence)
            {
                global::g_Device.destroyFence(frame_data.queue_submit_fence);
                frame_data.queue_submit_fence = nullptr;
            }

            if (frame_data.primary_command_buffer)
            {
                global::g_Device.freeCommandBuffers(frame_data.primary_command_pool, frame_data.primary_command_buffer);
                frame_data.primary_command_buffer = nullptr;
            }

            if (frame_data.primary_command_pool)
            {
                global::g_Device.destroyCommandPool(frame_data.primary_command_pool);
                frame_data.primary_command_pool = nullptr;
            }

            if (frame_data.swapchain_acquire_semaphore)
            {
                global::g_Device.destroySemaphore(frame_data.swapchain_acquire_semaphore);
                frame_data.swapchain_acquire_semaphore = nullptr;
            }

            if (frame_data.swapchain_release_semaphore)
            {
                global::g_Device.destroySemaphore(frame_data.swapchain_release_semaphore);
                frame_data.swapchain_release_semaphore = nullptr;
            }
        }
        m_vkFramesData.clear();

        global::g_Device.destroySwapchainKHR(swapchain);
    }

    glm::vec2 Renderer2D::DrawString(const GraphicalString& str, glm::vec2 bounding_first, glm::vec2 bounding_second, int cursor)
    {
        glm::vec2 position(std::min(bounding_first.x, bounding_second.x), std::min(bounding_first.y, bounding_second.y));

        glm::vec2 cursor_pos = position;
        glm::vec2 localPosition = position;
        float firstchar = position.x;
        int lastspaceindex = 0;
        int lastspacequadindex = m_AtlasQuadCount;

        int retries = 0;

        for (int i = 0; i < str.Length(); ++i)
        {
            char ch = str[i].code;
            if (ch == '\0') return cursor_pos;

            Font font = str[i].font;

            uint32_t ifontid = static_cast<uint32_t>(font.fonttype);

            // Check if the charecter glyph is in the font atlas.
            if (ch >= m_FontAtlas->m_FirstCode && ch <= m_FontAtlas->m_FirstCode + m_FontAtlas->m_NumOfCodes)
            {

                // Retrive the data that is used to render a glyph of charecter 'ch'
                stbtt_packedchar* packedChar = &(m_FontAtlas->m_PackedChars[ch - m_FontAtlas->m_FirstCode + m_FontAtlas->m_NumOfCodes * ifontid]);
                stbtt_aligned_quad* alignedQuad = &(m_FontAtlas->m_AlignedQuads[ch - m_FontAtlas->m_FirstCode + m_FontAtlas->m_NumOfCodes * ifontid]);

                // The units of the fields of the above structs are in pixels, 
                // convert them to a unit of what we want be multilplying to pixelScale  
                glm::vec2 glyphSize =
                {
                    (packedChar->x1 - packedChar->x0) * font.scale,
                    (packedChar->y1 - packedChar->y0) * font.scale
                };

                glm::vec2 glyphBoundingBoxBottomLeft =
                {
                    packedChar->xoff * font.scale,
                    (packedChar->yoff + m_FontAtlas->m_FontSize) * font.scale
                };

                float char_right = localPosition.x + glyphBoundingBoxBottomLeft.x + glyphSize.x;
                float char_left = localPosition.x + glyphBoundingBoxBottomLeft.x;
                if (char_right >= std::max(bounding_first.x, bounding_second.x))
                {
                    if (retries < 1)
                    {
                        float word_width = char_right - firstchar;
                        float line_space = std::abs(bounding_second.x - bounding_first.x);

                        ++retries;

                        if (word_width < line_space)
                        {
                            localPosition.y += m_FontAtlas->m_FontSize * font.scale;
                            localPosition.x = position.x;
                            m_AtlasQuadCount = lastspacequadindex;

                            i = lastspaceindex;
                            continue;
                        }
                        else
                        {
                            localPosition.y += m_FontAtlas->m_FontSize * font.scale;
                            localPosition.x = position.x;


                            --i;
                            continue;
                        }
                    }
                    else
                    {
                        return cursor_pos;
                    }
                }

                retries = 0;

                uint32_t vertex_offs = m_AtlasQuadCount * 4;

                //glm::mat4 scale = glm::scale(glm::mat4(1.f), glm::vec3(1.f, 1.f, 1.f));

                glm::mat4 rotation = glm::rotate(glm::mat4(1.f), font.char_rotate_angle, glm::vec3(0.f, 0.f, 1.f));
                //glm::mat3 rotation = glm::mat3(1.f);
                glm::mat4 translation = glm::translate(glm::mat4(1.f), glm::vec3(localPosition.x + glyphSize.x / 2.f, localPosition.y + glyphSize.y / 2.f, 0.f) + font.translation);

                m_VertexBuffer[vertex_offs + 0].pos = translation * rotation * glm::vec4(glyphBoundingBoxBottomLeft.x + glyphSize.x / 2.f, glyphBoundingBoxBottomLeft.y + glyphSize.y / 2.f, 0.f, 1.f);
                m_VertexBuffer[vertex_offs + 1].pos = translation * rotation * glm::vec4(glyphBoundingBoxBottomLeft.x - glyphSize.x / 2.f, glyphBoundingBoxBottomLeft.y + glyphSize.y / 2.f, 0.f, 1.f);
                m_VertexBuffer[vertex_offs + 2].pos = translation * rotation * glm::vec4(glyphBoundingBoxBottomLeft.x - glyphSize.x / 2.f, glyphBoundingBoxBottomLeft.y - glyphSize.y / 2.f, 0.f, 1.f);
                m_VertexBuffer[vertex_offs + 3].pos = translation * rotation * glm::vec4(glyphBoundingBoxBottomLeft.x + glyphSize.x / 2.f, glyphBoundingBoxBottomLeft.y - glyphSize.y / 2.f, 0.f, 1.f);

                m_VertexBuffer[vertex_offs + 0].tex_coord = glm::vec2(alignedQuad->s1, alignedQuad->t1);
                m_VertexBuffer[vertex_offs + 1].tex_coord = glm::vec2(alignedQuad->s0, alignedQuad->t1);
                m_VertexBuffer[vertex_offs + 2].tex_coord = glm::vec2(alignedQuad->s0, alignedQuad->t0);
                m_VertexBuffer[vertex_offs + 3].tex_coord = glm::vec2(alignedQuad->s1, alignedQuad->t0);

                float fade = 1.f - font.fade;
                m_VertexBuffer[vertex_offs + 0].color = font.color * fade;
                m_VertexBuffer[vertex_offs + 1].color = font.color * fade;
                m_VertexBuffer[vertex_offs + 2].color = font.color * fade;
                m_VertexBuffer[vertex_offs + 3].color = font.color * fade;

                float fontz = (static_cast<float>(font.fonttype) + 0.5f) / static_cast<float>(m_FontAtlas->m_NumOfFonts);

                m_VertexBuffer[vertex_offs + 0].samplerid = fontz;
                m_VertexBuffer[vertex_offs + 1].samplerid = fontz;
                m_VertexBuffer[vertex_offs + 2].samplerid = fontz;
                m_VertexBuffer[vertex_offs + 3].samplerid = fontz;

                // Update the position to render the next glyph specified by packedChar->xadvance.
                localPosition.x += packedChar->xadvance * font.scale;

                if (i == cursor)
                {
                    cursor_pos = localPosition;
                }

                if (i > 0)
                {
                    char lastchar = str[i - 1].code;
                    if (lastchar == ' ' || lastchar == '\t' || lastchar == '\n')
                    {
                        firstchar = char_left;
                        lastspaceindex = i - 1;
                        lastspacequadindex = m_AtlasQuadCount;
                    }
                }

                ++m_AtlasQuadCount;
            }
            else if (ch == '\n')
            {
                // advance y by fontSize, reset x-coordinate
                localPosition.y += m_FontAtlas->m_FontSize * font.scale;
                localPosition.x = position.x;
            }
            else if (ch == '\t')
            {
                stbtt_packedchar* packedChar = &(m_FontAtlas->m_PackedChars[' ' - m_FontAtlas->m_FirstCode + m_FontAtlas->m_NumOfCodes * ifontid]);
                float tabwidth = (packedChar->x1 - packedChar->x0) * font.scale * 4.f;
                localPosition.x = (floor(localPosition.x / tabwidth) + 1.f) * (m_FontAtlas->m_FontSize * font.scale * 4.f);
            }
        }

        if (cursor == -1) return localPosition;
        return cursor_pos;
    }

    glm::vec2 Renderer2D::DrawString(const std::string& str, glm::vec2 bounding_first, glm::vec2 bounding_second, int cursor, Font font)
    {
        return DrawString(GraphicalString(str, font), bounding_first, bounding_second, cursor);
    }

    glm::vec2 Renderer2D::DrawStringCenter(const GraphicalString& str, glm::vec2 bounding_first, glm::vec2 bounding_second, int cursor)
    {
        uint32_t startquad = m_AtlasQuadCount;
        glm::vec2 endpos = DrawString(str, bounding_first, bounding_second, cursor);
        for (int i = startquad * 4; i < m_AtlasQuadCount * 4; ++i)
        {
            m_VertexBuffer[i].pos -= (glm::vec3(endpos - bounding_first, 0.f) + glm::vec3(0.f, m_FontAtlas->m_FontSize * str[0].font.scale, 0.f)) / 2.f;
        }
        return (endpos + bounding_first) / 2.f;
    }

    glm::vec2 Renderer2D::DrawStringCenter(const std::string& str, glm::vec2 bounding_first, glm::vec2 bounding_second, int cursor, Font font)
    {
        return DrawStringCenter(GraphicalString(str, font), bounding_first, bounding_second, cursor);
    }

    //void Renderer2D::DrawImage(std::shared_ptr<Image> image, glm::vec2 position)
    //{
    //    DrawImage(image, glm::translate(glm::mat4(1.f), glm::vec3(position.x, position.y, 0.f)));
    //}
    //
    //void Renderer2D::DrawImage(std::shared_ptr<Image> image, glm::mat4 transform)
    //{
    //    int index = m_ImageList.size();
    //    for (int i = 0; i < m_ImageList.size(); ++i)
    //    {
    //        if (m_ImageList[i] == image)
    //        {
    //            index = i;
    //            break;
    //        }
    //    }
    //    if (index = m_ImageList.size()) m_ImageList.push_back(image);
    //
    //
    //    static glm::vec2 quad[4] = {
    //        glm::vec2(+1.f, +1.f),
    //        glm::vec2(-1.f, +1.f),
    //        glm::vec2(-1.f, -1.f),
    //        glm::vec2(+1.f, -1.f)
    //    };
    //
    //    int vertexoffs = (m_MaxQuads - m_ImageQuadCount) * 4 - 1;
    //    ++m_ImageQuadCount;
    //
    //    m_VertexBuffer[vertexoffs - 3].pos = glm::vec3(glm::vec4(quad[0], 0.f, 1.f) * transform);
    //    m_VertexBuffer[vertexoffs - 2].pos = glm::vec3(glm::vec4(quad[1], 0.f, 1.f) * transform);
    //    m_VertexBuffer[vertexoffs - 1].pos = glm::vec3(glm::vec4(quad[2], 0.f, 1.f) * transform);
    //    m_VertexBuffer[vertexoffs - 0].pos = glm::vec3(glm::vec4(quad[3], 0.f, 1.f) * transform);
    //
    //    m_VertexBuffer[vertexoffs - 3].color = glm::vec4(1.f, 1.f, 1.f, 1.f);
    //    m_VertexBuffer[vertexoffs - 2].color = glm::vec4(1.f, 1.f, 1.f, 1.f);
    //    m_VertexBuffer[vertexoffs - 1].color = glm::vec4(1.f, 1.f, 1.f, 1.f);
    //    m_VertexBuffer[vertexoffs - 0].color = glm::vec4(1.f, 1.f, 1.f, 1.f);
    //
    //    m_VertexBuffer[vertexoffs - 3].tex_coord = quad[0];
    //    m_VertexBuffer[vertexoffs - 2].tex_coord = quad[1];
    //    m_VertexBuffer[vertexoffs - 1].tex_coord = quad[2];
    //    m_VertexBuffer[vertexoffs - 0].tex_coord = quad[3];
    //
    //    m_VertexBuffer[vertexoffs - 3].samplerid = static_cast<float>(index);
    //    m_VertexBuffer[vertexoffs - 2].samplerid = static_cast<float>(index);
    //    m_VertexBuffer[vertexoffs - 1].samplerid = static_cast<float>(index);
    //    m_VertexBuffer[vertexoffs - 0].samplerid = static_cast<float>(index);
    //}
    //
    //void DrawRect(glm::vec3 position, glm::vec2 size, glm::vec4 color = glm::vec4(1.f, 1.f, 1.f, 1.f))
    //{
    //
    //}

    void Renderer2D::FillRect(glm::vec3 position, glm::vec2 size, glm::vec4 color)
    {

        uint32_t vertex = m_BasicQuadCount * 4;
        ++m_BasicQuadCount;

        m_BasicVertexBuffer[vertex + 0].pos = glm::vec3(+size.x, +size.y, 0.f) + position;
        m_BasicVertexBuffer[vertex + 1].pos = glm::vec3(0.f, +size.y, 0.f) + position;
        m_BasicVertexBuffer[vertex + 2].pos = glm::vec3(0.f, 0.f, 0.f) + position;
        m_BasicVertexBuffer[vertex + 3].pos = glm::vec3(+size.x, 0.f, 0.f) + position;

        m_BasicVertexBuffer[vertex + 0].color = color;
        m_BasicVertexBuffer[vertex + 1].color = color;
        m_BasicVertexBuffer[vertex + 2].color = color;
        m_BasicVertexBuffer[vertex + 3].color = color;
    }

    void Renderer2D::FillRectCenter(glm::vec3 position, glm::vec2 size, glm::vec4 color)
    {
        FillRect(position - glm::vec3(size, 0.f) / 2.f, size, color);
    }

    //void DrawCircle(glm::vec3 position, float radius, glm::vec4 color = glm::vec4(1.f, 1.f, 1.f, 1.f))
    //{
    //
    //}
    //
    //void FillCircle(glm::vec3 position, float radius, glm::vec4 color = glm::vec4(1.f, 1.f, 1.f, 1.f))
    //{
    //
    //}

}