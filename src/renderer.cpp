#include "renderer.hpp"
#include "camera.hpp"
#include "context.hpp"
#include "window.hpp"

#include "math.hpp"

#include <iostream>
#include <bitset>
#include <fstream>

float MoveBits(int bitAmount, int bitsRight, int data) {
    int shiftedRight = data >> bitsRight;
    int bitMask = (1 << bitAmount) -1;
    int temp = shiftedRight & bitMask;
    if (temp > 15) {
        temp = -temp + 16;
    }
    return static_cast<float>(temp);
}

simd::float3 GetPos(int data) {
    return simd::float3{MoveBits(5, 0, data), MoveBits(5, 5, data), MoveBits(5, 10, data)};
}

int GetSide(int data) {
    return MoveBits(3, 15, data);
}

int GetBlockId(int data) {
    return MoveBits(14, 18, data);
}

void Renderer::build_shaders(MTL::Device* device) {
    using NS::StringEncoding::UTF8StringEncoding;

    std::string tempShaderStr;
    std::string shaderStr = "";

    std::ifstream shadeingFile;
    shadeingFile.open("./src/shaders.metal");

    if (!shadeingFile.is_open()) {
        std::cout << "Error: Couldnt be opened" << std::endl;
    }

    while (getline (shadeingFile, tempShaderStr)) {
        shaderStr =  shaderStr + tempShaderStr + "\n";
    }
    shadeingFile.close();

    const char* pShaderStr = shaderStr.c_str();

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = device->newLibrary( NS::String::string(pShaderStr, UTF8StringEncoding), nullptr, &pError );
    if ( !pLibrary ) {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }

    pVertexFunction = pLibrary->newFunction( NS::String::string("vertexMain", UTF8StringEncoding) );
    pFragmentFunction = pLibrary->newFunction( NS::String::string("fragmentMain", UTF8StringEncoding) );

    MTL::RenderPipelineDescriptor* pRenderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    pRenderPipelineDescriptor->setVertexFunction( pVertexFunction );
    pRenderPipelineDescriptor->setFragmentFunction( pFragmentFunction );
    pRenderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm );
    pRenderPipelineDescriptor->setDepthAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth16Unorm );

    pRenderPipelineState = device->newRenderPipelineState( pRenderPipelineDescriptor, &pError );
    if ( !pRenderPipelineState ) {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }

    pVertexFunction->release();
    pFragmentFunction->release();
    pRenderPipelineDescriptor->release();
    _pShaderLibrary = pLibrary;
    
    std::cout << "Shaders built" << std::endl;
}

void Renderer::build_textures(MTL::Device* device)
{
    int size_x = 0;
    int size_y = 0;
    int num_channels = 0;
    stbi_uc* grass_top_texture = stbi_load("./src/Textures/Grass/Grass_top.png", &size_x, &size_y, &num_channels, 4);
    if(grass_top_texture == nullptr) {
        throw std::runtime_error("Textures couldn't be found");
    }

    stbi_uc* grass_side_texture = stbi_load("./src/Textures/Grass/Grass_side.png", &size_x, &size_y, &num_channels, 4);
    if(grass_side_texture == nullptr) {
        throw std::runtime_error("Textures couldn't be found");
    }

    stbi_uc* grass_bottom_texture = stbi_load("./src/Textures/Grass/Grass_bottom.png", &size_x, &size_y, &num_channels, 4);
    if(grass_bottom_texture == nullptr) {
        throw std::runtime_error("Textures couldn't be found");
    }

    MTL::TextureDescriptor* pTextureDescriptor = MTL::TextureDescriptor::alloc()->init();
    pTextureDescriptor->setWidth( static_cast<NS::UInteger>(size_x) );
    pTextureDescriptor->setHeight( static_cast<NS::UInteger>(size_y) );
    pTextureDescriptor->setPixelFormat( MTL::PixelFormatRGBA8Unorm );
    pTextureDescriptor->setTextureType( MTL::TextureType2D );
    pTextureDescriptor->setStorageMode( MTL::StorageModeManaged );
    pTextureDescriptor->setUsage( MTL::ResourceUsageSample | MTL::ResourceUsageRead );

    pTextureArr[0] = device->newTexture( pTextureDescriptor );
    pTextureArr[1] = device->newTexture( pTextureDescriptor );
    pTextureArr[2] = device->newTexture( pTextureDescriptor );

    pTextureArr[0]->replaceRegion( MTL::Region( 0, 0, 0, size_x, size_y, 1 ), 0, grass_top_texture, size_x * 4 );
    pTextureArr[1]->replaceRegion( MTL::Region( 0, 0, 0, size_x, size_y, 1 ), 0, grass_side_texture, size_x * 4 );
    pTextureArr[2]->replaceRegion( MTL::Region( 0, 0, 0, size_x, size_y, 1 ), 0, grass_bottom_texture, size_x * 4 );

    stbi_image_free(grass_top_texture);
    stbi_image_free(grass_side_texture);
    stbi_image_free(grass_bottom_texture);
    pTextureDescriptor->release();

    std::cout << "Textures built" << std::endl;
}

void Renderer::build_buffers(MTL::Device* device) {
    using simd::float2;
    using simd::float3;

    const float s = 0.5f;

    VertexData verts[] = {
        //                                         Texture
        //   Positions           Normals         Coordinates
        { { -s, -s, +s }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },
        { { +s, -s, +s }, {  0.f,  0.f,  1.f }, { 1.f, 1.f } },
        { { +s, +s, +s }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },
        { { -s, +s, +s }, {  0.f,  0.f,  1.f }, { 0.f, 0.f } },

        { { +s, -s, +s }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },
        { { +s, -s, -s }, {  1.f,  0.f,  0.f }, { 1.f, 1.f } },
        { { +s, +s, -s }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { +s, +s, +s }, {  1.f,  0.f,  0.f }, { 0.f, 0.f } },

        { { +s, -s, -s }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
        { { -s, -s, -s }, {  0.f,  0.f, -1.f }, { 1.f, 1.f } },
        { { -s, +s, -s }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },
        { { +s, +s, -s }, {  0.f,  0.f, -1.f }, { 0.f, 0.f } },

        { { -s, -s, -s }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },
        { { -s, -s, +s }, { -1.f,  0.f,  0.f }, { 1.f, 1.f } },
        { { -s, +s, +s }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { -s, +s, -s }, { -1.f,  0.f,  0.f }, { 0.f, 0.f } },

        { { -s, +s, +s }, {  0.f,  1.f,  0.f }, { 0.f, 1.f } },
        { { +s, +s, +s }, {  0.f,  1.f,  0.f }, { 1.f, 1.f } },
        { { +s, +s, -s }, {  0.f,  1.f,  0.f }, { 1.f, 0.f } },
        { { -s, +s, -s }, {  0.f,  1.f,  0.f }, { 0.f, 0.f } },

        { { -s, -s, -s }, {  0.f, -1.f,  0.f }, { 0.f, 1.f } },
        { { +s, -s, -s }, {  0.f, -1.f,  0.f }, { 1.f, 1.f } },
        { { +s, -s, +s }, {  0.f, -1.f,  0.f }, { 1.f, 0.f } },
        { { -s, -s, +s }, {  0.f, -1.f,  0.f }, { 0.f, 0.f } }
    };

    uint16_t indices[] = {
        12, 13, 14, 14, 15, 12, //front
        0,  1,  2,  2,  3,  0, //right
        4,  5,  6,  6,  7,  4, //back
        8,  9, 10, 10, 11,  8, //left
        16, 17, 18, 18, 19, 16, //top
        20, 21, 22, 22, 23, 20 //bot
    };
    
    const size_t vertexDataSize = sizeof( verts );
    const size_t indexDataSize = sizeof( indices );

    MTL::Buffer* pVertexBuffer = device->newBuffer( vertexDataSize, MTL::ResourceStorageModeManaged );
    MTL::Buffer* pIndexBuffer = device->newBuffer( indexDataSize, MTL::ResourceStorageModeManaged );

    _pVertexDataBuffer = pVertexBuffer;
    _pIndexBuffer = pIndexBuffer;

    memcpy( _pVertexDataBuffer->contents(), verts, vertexDataSize );
    memcpy( _pIndexBuffer->contents(), indices, indexDataSize );

    _pVertexDataBuffer->didModifyRange( NS::Range::Make( 0, _pVertexDataBuffer->length() ) );
    _pIndexBuffer->didModifyRange( NS::Range::Make( 0, _pIndexBuffer->length() ) );

    const size_t cameraDataSize = kMaxFramesInFlight * sizeof( CameraData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i ) {
        _pCameraDataBuffer[ i ] = device->newBuffer( cameraDataSize, MTL::ResourceStorageModeManaged );
    }

    std::cout << "Buffers built" << std::endl;
}

void Renderer::build_frame(MTL::Device* device) {
    for ( int i = 0; i < kMaxFramesInFlight; ++i ) {
        _pFrameData[ i ]= device->newBuffer( sizeof( FrameData ), MTL::ResourceStorageModeManaged );
    }

    std::cout << "Frame built" <<std::endl;
}

void Renderer::build_spencil(MTL::Device* device) {
    MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction( MTL::CompareFunction::CompareFunctionLess );
    depthStencilDescriptor->setDepthWriteEnabled( true );

    pDepthStencilDescriptor = device->newDepthStencilState( depthStencilDescriptor );

    depthStencilDescriptor->release();

    std::cout << "Depth built" << std::endl;
}

void Renderer::run() {
    using simd::float4x4;
    using simd::float4;
    using Clock = std::chrono::high_resolution_clock;

    Context* context = new Context;
    Window* window = new Window;
    Camera* camera = new Camera;

    window->create_window();
    window->cursor_setup(true, window->glfwWindow, window->window_size);

    context->setup(window->glfwWindow);

    Renderer::build_frame(context->device);
    Renderer::build_shaders(context->device);
    Renderer::build_spencil(context->device);
    Renderer::build_textures(context->device);
    Renderer::build_buffers(context->device);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window->glfwWindow, true);
    ImGui_ImplGlfw_SetCallbacksChainForAllWindows(true);
    ImGui_ImplMetal_Init(context->device);

    _semaphore = dispatch_semaphore_create( kMaxFramesInFlight );

    Clock::time_point start = Clock::now(), prev_time = start;
    delta_time = 1.0f;

    bool printBlock = true;
    BlockData* bd = new BlockData;

    int blockDataArr[6] = {bd->sideFront, bd->sideRight, bd->sideBack, bd->sideLeft ,bd->top, bd->bot};

    for (int mix = 0; mix < 6;mix++) {
        std::cout << "pos: " <<GetPos(blockDataArr[mix])[0] << ", " << GetPos(blockDataArr[mix])[1] << ", " << GetPos(blockDataArr[mix])[2] << " side: " << GetSide(blockDataArr[mix]) << " blockID: " << GetBlockId(blockDataArr[mix]) << std::endl;
        std::cout << "binary: " << std::bitset<32>(blockDataArr[mix]) << std::endl;
        std::cout << GetSide(blockDataArr[mix]) << std::endl;
    }

    glm::vec2 menu_size = {window->window_size.x*0.5, window->window_size.y*0.5};

    std::cout << "Rendering" << std::endl;

    while (!glfwWindowShouldClose(window->glfwWindow)) {
        glfwPollEvents();

        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
        context->pMetalDrawable = context->metalLayer->nextDrawable();

        _frame = (_frame + 1) % kMaxFramesInFlight;

        MTL::CommandBuffer* pCommandBuffer = context->commandQueue->commandBuffer();
        dispatch_semaphore_wait( _semaphore, DISPATCH_TIME_FOREVER );
        pCommandBuffer->addCompletedHandler( ^void( MTL::CommandBuffer* pCommandBuffer ){
            dispatch_semaphore_signal( _semaphore );
        });

            
        // Update camera state:
        MTL::Buffer* pCameraDataBuffer = _pCameraDataBuffer[ _frame ];
        if (glfwGetWindowAttrib(window->glfwWindow, GLFW_FOCUSED)) {
            camera->update(pCameraDataBuffer,window->glfwWindow,window->window_size,delta_time, window);
        }

        camera->pCameraData->worldTransform = camera->view_mat;
        camera->pCameraData->worldNormalTransform = math::discardTranslation( camera->pCameraData->worldTransform );
        pCameraDataBuffer->didModifyRange( NS::Range::Make( 0, sizeof( CameraData ) ) );

        MTL::RenderPassDescriptor* pRenderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

        MTL::RenderPassColorAttachmentDescriptor* pRenderPassColorAttachmentDescriptor = pRenderPassDescriptor->colorAttachments()->object(0);
        pRenderPassColorAttachmentDescriptor->setTexture(context->pMetalDrawable->texture());
        pRenderPassColorAttachmentDescriptor->setLoadAction(MTL::LoadActionClear);
        pRenderPassColorAttachmentDescriptor->setClearColor(MTL::ClearColor(.2f, .2f, .3f, 1.0f));
        pRenderPassColorAttachmentDescriptor->setStoreAction(MTL::StoreActionStore);


        ImGui::GetIO().DisplayFramebufferScale = ImVec2(2,2);
        ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(window->window_size.x), static_cast<float>(window->window_size.y));

        ImGui_ImplMetal_NewFrame(pRenderPassDescriptor);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        MTL::RenderCommandEncoder* pCommandEncoder = pCommandBuffer->renderCommandEncoder( pRenderPassDescriptor );

        pCommandEncoder->setRenderPipelineState( pRenderPipelineState );
        pCommandEncoder->setDepthStencilState( pDepthStencilDescriptor );

        pCommandEncoder->setVertexBuffer( _pVertexDataBuffer, 0, 0 );
        pCommandEncoder->setVertexBuffer( pCameraDataBuffer, 0, 1 );

        MTL::ArgumentEncoder* pArgumentEncoderFragment = pFragmentFunction->newArgumentEncoder(0);

        size_t argumentBufferLengthFragment = pArgumentEncoderFragment->encodedLength();
        MTL::Buffer* pArgumentBufferFragment = context->device->newBuffer(argumentBufferLengthFragment, MTL::ResourceStorageModeShared);

        pArgumentEncoderFragment->setArgumentBuffer(pArgumentBufferFragment, 0);

        NS::Range textureRange = NS::Range(0, 127);
        pArgumentEncoderFragment->setTextures( pTextureArr, textureRange);

        pCommandEncoder->setFragmentBuffer(pArgumentBufferFragment, 0, 0);

        MTL::ArgumentEncoder* pArgumentEncoderVertex = pVertexFunction->newArgumentEncoder(2);

        size_t argumentBufferLengthVertex = pArgumentEncoderVertex->encodedLength();
        const size_t blockDataSize = sizeof( blockDataArr );
        MTL::Buffer* pArgumentBufferVertex = context->device->newBuffer(blockDataSize, MTL::ResourceStorageModeShared);

        memcpy( pArgumentBufferVertex->contents(), blockDataArr,  blockDataSize);

        pArgumentEncoderFragment->setArgumentBuffer(pArgumentBufferVertex, 0);

        pCommandEncoder->setVertexBuffer(pArgumentBufferVertex, 0, 2);

        if(window->menu) {
            if(window->options) {
                ImGui::SetWindowSize("Options",ImVec2(menu_size.x, menu_size.y), 0);
                ImGui::SetWindowPos("Options", ImVec2(window->window_size.x*0.5f - menu_size.x * 0.5f, window->window_size.y*0.5f - menu_size.y * 0.5f), 0);
                ImGui::Begin("Options", p_open , window_flags);
                ImGui::SetWindowFontScale(2);
                if(ImGui::Button("Generate terrain", ImVec2(menu_size.x, menu_size.y/3))) {
                    
                }
                if(ImGui::Button("Back", ImVec2(menu_size.x, menu_size.y/3))) {
                    window->options = false;
                }
                ImGui::End();
            }
            else {
                ImGui::SetWindowSize("Menu",ImVec2(menu_size.x, menu_size.y), 0);
                ImGui::SetWindowPos("Menu", ImVec2(window->window_size.x*0.5f - menu_size.x * 0.5f, window->window_size.y*0.5f - menu_size.y * 0.5f), 0);

                ImGui::Begin("Menu", p_open , window_flags);
                ImGui::SetWindowFontScale(2);
                if(ImGui::Button("Options", ImVec2(menu_size.x, menu_size.y/3))) {
                    window->options = true;
                }
                if(ImGui::Button("Back", ImVec2(menu_size.x, menu_size.y/3))) {
                    window->menu = false;
                    window->cursor_setup(true, window->glfwWindow, window->window_size);
                }
                if(ImGui::Button("Quit", ImVec2(menu_size.x, menu_size.y/3))) {
                    pCommandEncoder->endEncoding();
                    pool->release();
                    context->commandQueue->release();
                    context->ns_window->release();
                    context->metalLayer->release();
                    glfwTerminate();
                    context->device->release();
                    _pShaderLibrary->release();
                    _pVertexColorsBuffer->release();
                    _pIndexBuffer->release();
                    pRenderPipelineState->release();

                    ImGui_ImplMetal_Shutdown();
                    ImGui_ImplGlfw_Shutdown();
                    ImGui::DestroyContext();
                    exit(0);
                }
                ImGui::End();
            }
        }
        else {
            ImGui::SetWindowPos(" ", ImVec2(5,5), 0);
            ImGui::Begin(" ", p_open, window_flags);
            ImGui::SetWindowFontScale(2);
            ImGui::Text("%.1fFPS, %.3fms", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            ImGui::End();
        }
        ImGui::Render();

        pCommandEncoder->setCullMode( MTL::CullModeBack );
        pCommandEncoder->setFrontFacingWinding( MTL::Winding::WindingCounterClockwise );

        pCommandEncoder->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
                                                6 * 6, MTL::IndexType::IndexTypeUInt16,
                                                _pIndexBuffer,
                                                0,
                                                1 );

        pCommandEncoder->setCullMode( MTL::CullModeBack);
        pCommandEncoder->setFrontFacingWinding( MTL::Winding::WindingClockwise );
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), pCommandBuffer, pCommandEncoder);
        

        pCommandEncoder->endEncoding();
        pCommandBuffer->presentDrawable(context->pMetalDrawable);
        pCommandBuffer->commit();
        pCommandBuffer->waitUntilCompleted();

        pool->release();
        auto now = Clock::now();
        delta_time = std::chrono::duration<float>(now - prev_time).count();
        prev_time = now;
        //std::cout << "\r" << (1.0f / delta_time) << std::flush;
    }
    context->commandQueue->release();
    context->ns_window->release();
    context->metalLayer->release();
    glfwTerminate();
    context->device->release();
    _pShaderLibrary->release();
    _pVertexColorsBuffer->release();
    _pIndexBuffer->release();
    pRenderPipelineState->release();

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}