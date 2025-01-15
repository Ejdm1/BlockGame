#include "renderer.hpp"
#include "camera.hpp"
#include "context.hpp"
#include "window.hpp"

#include "math.hpp"

#include <iostream>
#include <bitset>
#include <fstream>
#include <string>

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
    shadeingFile.open("/Users/adamkapsa/Documents/Python_bruh/cpp_metal_blockgame/last_push_metal-cmake-glfw/src/shaders.metal");//"./src/shaders.metal");

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
    pRenderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatRGBA8Unorm_sRGB );
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

void Renderer::build_textures(MTL::Device* device) {
    int size_x = 0;
    int size_y = 0;
    int num_channels = 0;
    int indexCounter = 0;
    int count = 1;

    for (int i = 0; i < texture_names.size(); i++) {
        int texture_amount = texture_side_amounts[i];
        std::string textureFileType = ".png";
        std::string texturePath = "./src/Textures/";
        for(int j = 0; j < texture_amount;j++) {
            std::string textureName = texture_names[i];
            if(texture_amount == 3) {
                textureName += texture_end_names[j];
            }
            
            std::string textureFullName = texturePath + textureName + textureFileType;

            const char* constTexturePath = textureFullName.c_str();
            stbi_uc* texture = stbi_load(constTexturePath, &size_x, &size_y, &num_channels, 4);

            size_t spaces = 32 - textureName.length();
            if(texture != nullptr) {
                MTL::TextureDescriptor* pTextureDescriptor = MTL::TextureDescriptor::alloc()->init();

                for (int k = 0; k < spaces; k++) {
                    textureName += "-";
                }
                std::cout << textureName << "> Number: " << count << " Found" << std::endl;

                pTextureDescriptor->setWidth( static_cast<NS::UInteger>(size_x) );
                pTextureDescriptor->setHeight( static_cast<NS::UInteger>(size_y) );
                pTextureDescriptor->setPixelFormat( MTL::PixelFormatRGBA8Unorm_sRGB );
                pTextureDescriptor->setTextureType( MTL::TextureType2D );
                pTextureDescriptor->setStorageMode( MTL::StorageModeManaged );
                pTextureDescriptor->setUsage( MTL::ResourceUsageSample | MTL::ResourceUsageRead );

                pTextureArr[indexCounter] = device->newTexture( pTextureDescriptor );
                pTextureArr[indexCounter]->replaceRegion( MTL::Region( 0, 0, 0, size_x, size_y, 1 ), 0, texture, size_x * 4 );
                stbi_image_free(texture);
                pTextureDescriptor->release();
                indexCounter++;
            }
            else {
                for (int k = 0; k < spaces; k++) {
                    textureName += "-";
                }
                std::cout << textureName << "> Number: " << count << " Not found" << std::endl;
            }
            count++;
        }
    }

    std::cout << "Textures built" << std::endl;
}

void Renderer::build_buffers(MTL::Device* device) {
    using simd::float2;
    using simd::float3;

    VertexData verts[] = {
        //                                                                               Texture
        //                   Positions                        Normals                   Coordinates
        { { +0.5, -0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
        { { -0.5, -0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 1.f, 1.f } },
        { { -0.5, +0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },
        { { -0.5, +0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 1.f, 0.f } },
        { { +0.5, +0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 0.f, 0.f } },
        { { +0.5, -0.5, -0.5 }, {  0.f,  0.f, -1.f }, { 0.f, 1.f } },
        
        { { -0.5, -0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },
        { { +0.5, -0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 1.f, 1.f } },
        { { +0.5, +0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },
        { { +0.5, +0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 1.f, 0.f } },
        { { -0.5, +0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 0.f, 0.f } },
        { { -0.5, -0.5, +0.5 }, {  0.f,  0.f,  1.f }, { 0.f, 1.f } },

        { { +0.5, -0.5, +0.5 }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },
        { { +0.5, -0.5, -0.5 }, {  1.f,  0.f,  0.f }, { 1.f, 1.f } },
        { { +0.5, +0.5, -0.5 }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { +0.5, +0.5, -0.5 }, {  1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { +0.5, +0.5, +0.5 }, {  1.f,  0.f,  0.f }, { 0.f, 0.f } },
        { { +0.5, -0.5, +0.5 }, {  1.f,  0.f,  0.f }, { 0.f, 1.f } },

        { { -0.5, -0.5, -0.5 }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },
        { { -0.5, -0.5, +0.5 }, { -1.f,  0.f,  0.f }, { 1.f, 1.f } },
        { { -0.5, +0.5, +0.5 }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { -0.5, +0.5, +0.5 }, { -1.f,  0.f,  0.f }, { 1.f, 0.f } },
        { { -0.5, +0.5, -0.5 }, { -1.f,  0.f,  0.f }, { 0.f, 0.f } },
        { { -0.5, -0.5, -0.5 }, { -1.f,  0.f,  0.f }, { 0.f, 1.f } },

        { { -0.5, +0.5, +0.5 }, {  0.f,  1.f,  0.f }, { 0.f, 1.f } },
        { { +0.5, +0.5, +0.5 }, {  0.f,  1.f,  0.f }, { 1.f, 1.f } },
        { { +0.5, +0.5, -0.5 }, {  0.f,  1.f,  0.f }, { 1.f, 0.f } },
        { { +0.5, +0.5, -0.5 }, {  0.f,  1.f,  0.f }, { 1.f, 0.f } },
        { { -0.5, +0.5, -0.5 }, {  0.f,  1.f,  0.f }, { 0.f, 0.f } },
        { { -0.5, +0.5, +0.5 }, {  0.f,  1.f,  0.f }, { 0.f, 1.f } },

        { { -0.5, -0.5, -0.5 }, {  0.f, -1.f,  0.f }, { 0.f, 1.f } },
        { { +0.5, -0.5, -0.5 }, {  0.f, -1.f,  0.f }, { 1.f, 1.f } },
        { { +0.5, -0.5, +0.5 }, {  0.f, -1.f,  0.f }, { 1.f, 0.f } },
        { { +0.5, -0.5, +0.5 }, {  0.f, -1.f,  0.f }, { 1.f, 0.f } },
        { { -0.5, -0.5, +0.5 }, {  0.f, -1.f,  0.f }, { 0.f, 0.f } },
        { { -0.5, -0.5, -0.5 }, {  0.f, -1.f,  0.f }, { 0.f, 1.f } }
    };
    
    const size_t vertexDataSize = sizeof( verts );

    MTL::Buffer* pVertexBuffer = device->newBuffer( vertexDataSize, MTL::ResourceStorageModeManaged );

    _pVertexDataBuffer = pVertexBuffer;

    memcpy( _pVertexDataBuffer->contents(), verts, vertexDataSize );

    _pVertexDataBuffer->didModifyRange( NS::Range::Make( 0, _pVertexDataBuffer->length() ) );

    const size_t cameraDataSize = kMaxFramesInFlight * sizeof( CameraData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i ) {
        _pCameraDataBuffer[ i ] = device->newBuffer( cameraDataSize, MTL::ResourceStorageModeManaged );
    }

Blocks blocks[instanceCount];

std::memset(blocks, 0, sizeof(blocks));

for(int i = 0; i < instanceCount; i++) {
    blocks[i].block[0] = blockFace(glm::vec3 {31,-2,31}, 0, blockID);
    blocks[i].block[1] = blockFace(glm::vec3 {31,-2,31}, 1, blockID);
    blocks[i].block[2] = blockFace(glm::vec3 {31,-2,31}, 2, blockID);
    blocks[i].block[3] = blockFace(glm::vec3 {31,-2,31}, 3, blockID);
    blocks[i].block[4] = blockFace(glm::vec3 {31,-2,31}, 4, blockID);
    blocks[i].block[5] = blockFace(glm::vec3 {31,-2,31}, 5, blockID);
}

// blockFace(glm::vec3 {0,1,0}, 0, blockID)
// blockFace(glm::vec3 {0,1,0}, 1, blockID)
// blockFace(glm::vec3 {0,1,0}, 2, blockID)
// blockFace(glm::vec3 {0,1,0}, 3, blockID)
// blockFace(glm::vec3 {0,1,0}, 4, blockID)
// blockFace(glm::vec3 {0,1,0}, 5, blockID)

// blockFace(glm::vec3 {0,0,0}, 0, blockID)
// blockFace(glm::vec3 {0,0,0}, 1, blockID)
// blockFace(glm::vec3 {0,0,0}, 2, blockID)
// blockFace(glm::vec3 {0,0,0}, 3, blockID)
// blockFace(glm::vec3 {0,0,0}, 4, blockID)
// blockFace(glm::vec3 {0,0,0}, 5, blockID)


                            

    // ########### Block data print ###########
    // for(int i = 0; i < 5;i++) {
    //     for(int j = 0; j < 6; j++) {
    //         std::cout << "Block pos x " << GetPos(Blocks[i][j]).x << " Block pos y " << GetPos(Blocks[i][j]).y << " Block pos z " << GetPos(Blocks[i][j]).z << GetPos(Blocks[i][j]).x << " Block ID " << GetBlockId(Blocks[i][j]) << std::endl;
    //     }
    //     std::cout << std::endl;
    // }

    const size_t blockDataSize = sizeof( blocks );

    MTL::Buffer* pArgumentBufferVertex = device->newBuffer(blockDataSize, MTL::ResourceStorageModeManaged);

    _pArgumentBufferVertex = pArgumentBufferVertex;

    memcpy( _pArgumentBufferVertex->contents(), blocks,  blockDataSize);

    _pArgumentBufferVertex->didModifyRange( NS::Range::Make( 0, _pArgumentBufferVertex->length() ) );



    // const size_t vertexDataSize = sizeof( verts );

    // MTL::Buffer* pVertexBuffer = device->newBuffer( vertexDataSize, MTL::ResourceStorageModeManaged );

    // _pVertexDataBuffer = pVertexBuffer;

    // memcpy( _pVertexDataBuffer->contents(), verts, vertexDataSize );

    // _pVertexDataBuffer->didModifyRange( NS::Range::Make( 0, _pVertexDataBuffer->length() ) );
    

    pArgumentEncoderFragment = pFragmentFunction->newArgumentEncoder(0);

    size_t argumentBufferLengthFragment = pArgumentEncoderFragment->encodedLength();
    pArgumentBufferFragment = device->newBuffer(argumentBufferLengthFragment, MTL::ResourceStorageModeManaged);

    pArgumentEncoderFragment->setArgumentBuffer(pArgumentBufferFragment, 0);

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

    Renderer::build_shaders(context->device);
    Renderer::build_frame(context->device);
    Renderer::build_buffers(context->device);
    Renderer::build_spencil(context->device);
    Renderer::build_textures(context->device);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window->glfwWindow, true);
    ImGui_ImplGlfw_SetCallbacksChainForAllWindows(true);
    ImGui_ImplMetal_Init(context->device);

    _semaphore = dispatch_semaphore_create( kMaxFramesInFlight );

    Clock::time_point start = Clock::now(), prev_time = start;
    delta_time = 1.0f;

    glm::vec2 menu_size = {window->window_size.x*0.5, window->window_size.y*0.5};

    NS::Range textureRange = NS::Range(0, (sizeof(pTextureArr) / sizeof(pTextureArr[0])));

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
        pCommandEncoder->setVertexBuffer( _pArgumentBufferVertex, 0, 2 );

        pArgumentEncoderFragment->setTextures( pTextureArr, textureRange);
        pCommandEncoder->setFragmentBuffer(pArgumentBufferFragment, 0, 0);

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

        pCommandEncoder->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, 0, 36, instanceCount);

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
    pRenderPipelineState->release();

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
