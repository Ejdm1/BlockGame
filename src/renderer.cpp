#include "renderer.hpp"

#pragma region build_shader {
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
#pragma endregion build_shader }
#pragma region build_textures {
/////////////////Load and create textures for copying to GPU///////////////////
void Renderer::build_textures(MTL::Device* device) {
    Texture_side_amounts texture_side_amounts;
    Texture_real_index texture_real_index;
    std::vector<std::string> texture_names = {};

    std::ifstream textureNamesFile("texture_names.txt");
    std::string textureNameData;
    while(std::getline(textureNamesFile, textureNameData)) {
        for(int i = 0; i < textureNameData.size(); i++) {
            if(textureNameData[i] == '%') {
                break;
            }

            if(textureNameData[i] == ',') {
                int counter = 0;
                std::string outStr = "";
                while(counter < textureNameData.size()) {
                    if(textureNameData[i + counter + 1] == ',' || textureNameData[i + counter + 1] == '%') {
                        break;
                    }
                    if(textureNameData[i + counter + 1] != '"') {
                        outStr += textureNameData[i + counter + 1];
                    }
                    counter++;
                }
                if(outStr != "") {
                    texture_names.push_back(outStr);
                }
            }
        } 
    }
    textureNamesFile.close();

    int size_x = 0;
    int size_y = 0;
    int num_channels = 0;
    int indexCounter = 0;
    int count = 1;

    std::ifstream textureAmountsFile("texture_amounts.txt");
    std::string indexAmountsData;
    int indexAmountsCounter = 0;
    while(std::getline(textureAmountsFile, indexAmountsData)) {
        for(int i = 0; i < indexAmountsData.size(); i++) {
            if(indexAmountsData[i] == '%') {
                break;
            }

            if(indexAmountsData[i] != ',') {
                int to_int = indexAmountsData[i] - '0';
                texture_side_amounts.texture_side_amounts[indexAmountsCounter] = to_int;
                indexAmountsCounter++;
            }
        } 
    }
    textureAmountsFile.close();

    std::ifstream textureRealIndexFile("texture_real_index.txt");
    std::string indexRealData;
    int indexRealCounter = 0;
    while(std::getline(textureRealIndexFile, indexRealData)) {
        for(int i = 0; i < indexRealData.size(); i++) {
            if(indexRealData[i] == ',') {
                std::string temp;
                temp += indexRealData[i+1];
                if(indexRealData[i+2] != ',' && indexRealData[i+2] != '%') {
                    temp += indexRealData[i+2];
                }
                texture_real_index.texture_real_index[indexRealCounter+1] = std::stoi(temp);
                indexRealCounter++;
            }
            if(indexRealData[i] != ',') {
                
            }
        }
    }
    textureRealIndexFile.close();

    // for(int i = 0; i < texture_names.size(); i++) {
    //     std::cout << texture_names[i] << " " << texture_side_amounts.texture_side_amounts[i] << " " << texture_real_index.texture_real_index[i] << std::endl;
    // }
    for (int i = 0; i < texture_names.size(); i++) {
        int texture_amount = texture_side_amounts.texture_side_amounts[i];
        std::string textureFileType = ".png";
        std::string texturePath = "./src/Textures/";
        texturesDict.insert({texture_names[i], i});
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

    ///////////////////////////texture_side_amounts to GPU////////////////////////////
    const size_t texture_side_amountsDataSize = sizeof(Texture_side_amounts);

    MTL::Buffer* ptexture_side_amountsBufferVertex = device->newBuffer(texture_side_amountsDataSize, MTL::ResourceStorageModeManaged);

    _ptexture_side_amountsBufferVertex = ptexture_side_amountsBufferVertex;

    memcpy(_ptexture_side_amountsBufferVertex->contents(), &texture_side_amounts, texture_side_amountsDataSize);

    _pNumberOfBlocksBufferVertex->didModifyRange(NS::Range::Make(0, _ptexture_side_amountsBufferVertex->length()));
    //////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////texture_real_index to GPU//////////////////////////////
    const size_t texture_real_indexDataSize = sizeof(Texture_real_index);

    MTL::Buffer* ptexture_real_indexBufferVertex = device->newBuffer(texture_real_indexDataSize, MTL::ResourceStorageModeManaged);

    _ptexture_real_indexBufferVertex = ptexture_real_indexBufferVertex;

    memcpy(_ptexture_real_indexBufferVertex->contents(), &texture_real_index, texture_real_indexDataSize);

    _ptexture_real_indexBufferVertex->didModifyRange(NS::Range::Make(0, _ptexture_real_indexBufferVertex->length()));
    //////////////////////////////////////////////////////////////////////////////////

    std::cout << "Textures built\n" << std::endl;
}
#pragma endregion build_textures }
/////////////////////////////////////////////////////////////////////////////////

#pragma region build_buffers {
void Renderer::build_buffers(MTL::Device* device) {
    using simd::float2;
    using simd::float3;

    chunkClass.generateChunk(texturesDict);

    /////////////Creating buffer for camera data and saveing them into it////////////
    const size_t cameraDataSize = kMaxFramesInFlight * sizeof( CameraData );
    for ( size_t i = 0; i < kMaxFramesInFlight; i++ ) {
        _pCameraDataBuffer[i] = device->newBuffer(cameraDataSize, MTL::ResourceStorageModeManaged);
    }
    /////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////Number of Blocks to GPU//////////////////////////
    const size_t numberOfBlocksDataSize = sizeof(NuberOfBlocksInChunk);

    MTL::Buffer* pNumberOfBlocksBufferVertex = device->newBuffer(numberOfBlocksDataSize, MTL::ResourceStorageModeManaged);

    _pNumberOfBlocksBufferVertex = pNumberOfBlocksBufferVertex;

    memcpy(_pNumberOfBlocksBufferVertex->contents(), &chunkClass.numberOfBlocksInChunk, numberOfBlocksDataSize);

    _pNumberOfBlocksBufferVertex->didModifyRange(NS::Range::Make(0, _pNumberOfBlocksBufferVertex->length()));
    /////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////Chunk data to GPU////////////////////////////////
    const size_t chunkDataSize = sizeof(ChunkToGPU) * chunkClass.chunkToGPU.size();

    MTL::Buffer* pArgumentBufferVertex = device->newBuffer(chunkDataSize, MTL::ResourceStorageModeManaged);

    _pArgumentBufferVertex = pArgumentBufferVertex;

    memcpy(_pArgumentBufferVertex->contents(), chunkClass.chunkToGPU.data(), chunkDataSize);

    _pArgumentBufferVertex->didModifyRange(NS::Range::Make(0, _pArgumentBufferVertex->length()));
    /////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////Buffer for textures//////////////////////////////
    pArgumentEncoderFragment = pFragmentFunction->newArgumentEncoder(0);

    size_t argumentBufferLengthFragment = pArgumentEncoderFragment->encodedLength();
    pArgumentBufferFragment = device->newBuffer(argumentBufferLengthFragment, MTL::ResourceStorageModeManaged);

    pArgumentEncoderFragment->setArgumentBuffer(pArgumentBufferFragment, 0);
    /////////////////////////////////////////////////////////////////////////////////

    std::cout << "Map seed: " << chunkClass.seed << std::endl;
    std::cout << "Buffers built\n" << std::endl;
}
#pragma endregion build_buffers }

#pragma region build_frame {
void Renderer::build_frame(MTL::Device* device) {
    for(int i = 0; i < kMaxFramesInFlight; i++) {
        _pFrameData[i] = device->newBuffer(sizeof(FrameData), MTL::ResourceStorageModeManaged);
    }

    std::cout << "Frame built" <<std::endl;
}
#pragma endregion build_frame }

#pragma  region build_depth {
void Renderer::build_spencil(MTL::Device* device) {
    MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);

    pDepthStencilDescriptor = device->newDepthStencilState( depthStencilDescriptor );

    depthStencilDescriptor->release();

    std::cout << "Depth built" << std::endl;
}
#pragma endregion build_depth }

#pragma region render {
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

        MTL::Buffer* pCameraDataBuffer = _pCameraDataBuffer[ _frame ];
        if (glfwGetWindowAttrib(window->glfwWindow, GLFW_FOCUSED)) {
            camera->update(pCameraDataBuffer,window->glfwWindow,window->window_size,delta_time, window);
        }

        camera->pCameraData->worldTransform = camera->view_mat;
        camera->pCameraData->worldNormalTransform = math::discardTranslation( camera->pCameraData->worldTransform );
        pCameraDataBuffer->didModifyRange( NS::Range::Make( 0, sizeof( CameraData ) ) );

        MTL::RenderPassDescriptor* pRenderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

        ///////////////////////////////////////Setup drawing plane//////////////////////////////////////////
        MTL::RenderPassColorAttachmentDescriptor* pRenderPassColorAttachmentDescriptor = pRenderPassDescriptor->colorAttachments()->object(0);
        pRenderPassColorAttachmentDescriptor->setTexture(context->pMetalDrawable->texture());
        pRenderPassColorAttachmentDescriptor->setLoadAction(MTL::LoadActionClear);
        pRenderPassColorAttachmentDescriptor->setClearColor(MTL::ClearColor(.2f, .2f, .3f, 1.0f));
        pRenderPassColorAttachmentDescriptor->setStoreAction(MTL::StoreActionStore);
        ///////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////Create imGui frame for drawing//////////////////////////////
        ImGui::GetIO().DisplayFramebufferScale = ImVec2(2,2);
        ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(window->window_size.x), static_cast<float>(window->window_size.y));

        ImGui_ImplMetal_NewFrame(pRenderPassDescriptor);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        /////////////////////////////////////////////////////////////////////////////////
        //////////////Creating FPS counter or menu if active(imGui)////////////////
        if(window->menu) {
            if(window->options) {
                ImGui::SetWindowSize("Options",ImVec2(menu_size.x, menu_size.y), 0);
                ImGui::SetWindowPos("Options", ImVec2(window->window_size.x*0.5f - menu_size.x * 0.5f, window->window_size.y*0.5f - menu_size.y * 0.5f), 0);
                ImGui::Begin("Options", p_open , window_flags);
                ImGui::SetWindowFontScale(2);
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
                    ///////////////////////Releasing everything created/////////////////////////////
                    pCommandEncoder->endEncoding();
                    pool->release();
                    context->commandQueue->release();
                    context->ns_window->release();
                    context->metalLayer->release();
                    glfwTerminate();
                    context->device->release();
                    _pShaderLibrary->release();
                    pRenderPipelineState->release();

                    ImGui_ImplMetal_Shutdown();
                    ImGui_ImplGlfw_Shutdown();
                    ImGui::DestroyContext();
                    exit(0);
                    /////////////////////////////////////////////////////////////////////////////////
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
        /////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////Set some things and rende to screen////////////////////////////
        ImGui::Render();

        MTL::RenderCommandEncoder* pCommandEncoder = pCommandBuffer->renderCommandEncoder( pRenderPassDescriptor );

        pCommandEncoder->setRenderPipelineState( pRenderPipelineState );
        pCommandEncoder->setDepthStencilState( pDepthStencilDescriptor );
        
        //////////////////////////////////////Set buffers//////////////////////////////////////////
        pCommandEncoder->setVertexBuffer( pCameraDataBuffer, 0, 1 );
        pCommandEncoder->setVertexBuffer( _pArgumentBufferVertex, 0, 2 );
        pCommandEncoder->setVertexBuffer( _pNumberOfBlocksBufferVertex, 0, 3 );
        pCommandEncoder->setVertexBuffer( _ptexture_real_indexBufferVertex, 0, 4 );
        pCommandEncoder->setVertexBuffer( _ptexture_side_amountsBufferVertex, 0, 5 );

        pArgumentEncoderFragment->setTextures( pTextureArr, textureRange);
        pCommandEncoder->setFragmentBuffer(pArgumentBufferFragment, 0, 0);
        //////////////////////////////////////////////////////////////////////////////////////////

        pCommandEncoder->setCullMode(MTL::CullModeBack);
        pCommandEncoder->setFrontFacingWinding(MTL::Winding::WindingCounterClockwise);

        pCommandEncoder->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, 0, 36, chunkClass.blockCounter);

        pCommandEncoder->setCullMode(MTL::CullModeBack);
        pCommandEncoder->setFrontFacingWinding(MTL::Winding::WindingClockwise);
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), pCommandBuffer, pCommandEncoder);
        

        pCommandEncoder->endEncoding();
        pCommandBuffer->presentDrawable(context->pMetalDrawable);
        pCommandBuffer->commit();
        pCommandBuffer->waitUntilCompleted();

        pool->release();
        /////////////////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////Capture delta time////////////////////////////
        auto now = Clock::now();
        delta_time = std::chrono::duration<float>(now - prev_time).count();
        prev_time = now;
        /////////////////////////////////////////////////////////////////////////////
    }
    ///////////////////////Releasing everything created hopefully////////////////////
    context->commandQueue->release();
    context->ns_window->release();
    context->metalLayer->release();
    glfwTerminate();
    context->device->release();
    _pShaderLibrary->release();
    pRenderPipelineState->release();

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    /////////////////////////////////////////////////////////////////////////////////
}
#pragma endregion render }