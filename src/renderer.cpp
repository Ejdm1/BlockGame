#include "renderer.hpp"

#pragma region build_shader {
void Renderer::build_shaders(MTL::Device* device) {
    using NS::StringEncoding::UTF8StringEncoding;

    //////////////////////////////////////////////////////Get shader and load its functons/////////////////////////////////////////////////////
    std::ifstream shadeingFile;
    shadeingFile.open("./src/shaders.metal");

    if (!shadeingFile.is_open()) {
        std::cout << "Shader file could not be opened" << std::endl;
        exit(1);
    }

    std::string tempShaderStr;
    std::string shaderStr = "";

    while (getline (shadeingFile, tempShaderStr)) {
        shaderStr =  shaderStr + tempShaderStr + "\n";
    }
    shadeingFile.close();

    const char* ShaderStr = shaderStr.c_str();

    NS::Error* Error = nullptr;
    MTL::Library* Library = device->newLibrary(NS::String::string(ShaderStr, UTF8StringEncoding), nullptr, &Error);
    if (!Library) {
        __builtin_printf("%s", Error->localizedDescription()->utf8String());
        assert(false);
    }

    ///////////Get vertext and fragment functions///////////////////
    VertexFunction = Library->newFunction(NS::String::string("vertexMain", UTF8StringEncoding));
    FragmentFunction = Library->newFunction(NS::String::string("fragmentMain", UTF8StringEncoding));
    ///////////////////////////////////////////////////////////////
    ////////////////Set vertex and fragment functions for GPUs pipeline descriptor////////////////////////////////
    MTL::RenderPipelineDescriptor* RenderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    RenderPipelineDescriptor->setVertexFunction(VertexFunction);
    RenderPipelineDescriptor->setFragmentFunction(FragmentFunction);
    RenderPipelineDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatRGBA8Unorm_sRGB);
    RenderPipelineDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatDepth16Unorm);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////And finaly set it into pipeline state and check for errors//////////////////////////////////
    RenderPipelineState = device->newRenderPipelineState(RenderPipelineDescriptor, &Error);
    if ( !RenderPipelineState ) {
        __builtin_printf("%s", Error->localizedDescription()->utf8String());
        assert(false);
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VertexFunction->release();
    FragmentFunction->release();
    RenderPipelineDescriptor->release();
    
    std::cout << "Shaders built" << std::endl;
}
#pragma endregion build_shader }

#pragma region build_textures {
////////////////////////////////////////////////Load and create textures for copying to GPU/////////////////////////////////////////////////
void Renderer::build_textures(MTL::Device* device) {
    Texture_side_amounts texture_side_amounts;
    Texture_real_index texture_real_index;
    std::vector<std::string> texture_names = {};
    ////////////////Get all texture names from premade text file///////////////////
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
    ///////////////////////////////////////////////////////////////////////////////

    int size_x = 0;
    int size_y = 0;
    int num_channels = 0;
    int indexCounter = 0;
    int count = 1;

    ////////////Load how many textures does specific block have 1 or 3/////////////
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
    ///////////////////////////////////////////////////////////////////////////////
    /////////////////Load textures real indexes which is used on GPU because I need to determine////////////////
    /////////////////right index since I have blocks with one texture or three textures/////////////////////////
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
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////Load textures and create array of them/////////////////////////////////////////////////////
    for (int i = 0; i < texture_names.size(); i++) {
        int texture_amount = texture_side_amounts.texture_side_amounts[i];
        std::string textureFileType = ".png";
        std::string texturePath = "./src/Textures/";
        texturesDict.insert({texture_names[i], i});
        for(int j = 0; j < texture_amount;j++) {
            std::string textureName = texture_names[i];
            ////////////////////Check if block has three texture and add right end(top,bottom,side)/////////////////
            if(texture_amount == 3) {
                textureName += texture_end_names[j];
            }
            ////////////////////////////////////////////////////////////////////////////////////////////////////////

            std::string textureFullName = texturePath + textureName + textureFileType;

            const char* constTexturePath = textureFullName.c_str();
            stbi_uc* texture = stbi_load(constTexturePath, &size_x, &size_y, &num_channels, 4);


            size_t spaces = 32 - textureName.length();
            if(texture != nullptr) {
                MTL::TextureDescriptor* TextureDescriptor = MTL::TextureDescriptor::alloc()->init();

                for (int k = 0; k < spaces; k++) {
                    textureName += "-";
                }
                std::cout << textureName << "> Number: " << count << " Found" << std::endl;

                ///////////////////Setup texture descriptor(width, height, color format, texture type and usage)///////////////////
                TextureDescriptor->setWidth( static_cast<NS::UInteger>(size_x) );
                TextureDescriptor->setHeight( static_cast<NS::UInteger>(size_y) );
                TextureDescriptor->setPixelFormat( MTL::PixelFormatRGBA8Unorm_sRGB );
                TextureDescriptor->setTextureType( MTL::TextureType2D );
                TextureDescriptor->setStorageMode( MTL::StorageModeManaged );
                TextureDescriptor->setUsage( MTL::ResourceUsageSample | MTL::ResourceUsageRead );
                ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                TextureArr[indexCounter] = device->newTexture( TextureDescriptor );
                TextureArr[indexCounter]->replaceRegion( MTL::Region( 0, 0, 0, size_x, size_y, 1 ), 0, texture, size_x * 4 );
                stbi_image_free(texture);
                TextureDescriptor->release();
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
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////texture_side_amounts to GPU////////////////////////////
    const size_t texture_side_amountsDataSize = sizeof(Texture_side_amounts);

    MTL::Buffer* texture_side_amounts_buffer_vertexL = device->newBuffer(texture_side_amountsDataSize, MTL::ResourceStorageModeManaged);

    texture_side_amounts_buffer_vertex = texture_side_amounts_buffer_vertexL;

    memcpy(texture_side_amounts_buffer_vertex->contents(), &texture_side_amounts, texture_side_amountsDataSize);

    texture_side_amounts_buffer_vertex->didModifyRange(NS::Range::Make(0, texture_side_amounts_buffer_vertex->length()));
    //////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////texture_real_index to GPU//////////////////////////////
    const size_t texture_real_indexDataSize = sizeof(Texture_real_index);

    MTL::Buffer* texture_real_index_buffer_vertexL = device->newBuffer(texture_real_indexDataSize, MTL::ResourceStorageModeManaged);

    texture_real_index_buffer_vertex = texture_real_index_buffer_vertexL;

    memcpy(texture_real_index_buffer_vertex->contents(), &texture_real_index, texture_real_indexDataSize);

    texture_real_index_buffer_vertex->didModifyRange(NS::Range::Make(0, texture_real_index_buffer_vertex->length()));
    //////////////////////////////////////////////////////////////////////////////////

    std::cout << "Textures built\n" << std::endl;
}
#pragma endregion build_textures }
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region build_buffers {
void Renderer::build_buffers(MTL::Device* device, bool first, int chunkLine) {
    using simd::float2;
    using simd::float3;

    if(first && regenerate) {
        srand(time(0));
        chunkClass.seed = rand() % 100000;
    }
    if(inAppRegenerate && !loadMap) {
        regenerate = true;
        srand(time(0));
        chunkClass.seed = rand() % 100000;
    }

    chunkClass.generateChunk(texturesDict, regenerate, chunkLine);
    regenerate = false;
    loadMap = false;

    //////////////////////Creating buffers for tripple buffering/////////////////////
    chunkIndexToKeep.resize(chunkLine*chunkLine);
    for (int i = 0; i < kMaxFramesInFlight; i++) {
        CameraDataBuffer[i] = device->newBuffer(kMaxFramesInFlight * sizeof(CameraDataS), MTL::ResourceStorageModeManaged);
        NumberOfBlocksBufferVertex[i] = device->newBuffer(sizeof(int)*chunkLine*chunkLine, MTL::ResourceStorageModeManaged);
        ChunkIndexesToBeRenderd[i] = device->newBuffer(sizeof(int)*chunkIndexToKeep.size(), MTL::ResourceStorageModeManaged);
    }
    /////////////////////////////////////////////////////////////////////////////////
    for(int j = 0; j < 2; j++) {
        mapBuffers[j] = device->newBuffer(sizeof(ChunkToGPU) * chunkClass.chunkToGPU.size(), MTL::ResourceStorageModeManaged);
    }
    ////////////////////////////////Buffer for textures//////////////////////////////
    if(first) {
        ArgumentEncoderFragment = FragmentFunction->newArgumentEncoder(0);

        ArgumentBufferFragment = device->newBuffer(ArgumentEncoderFragment->encodedLength(), MTL::ResourceStorageModeManaged);

        ArgumentEncoderFragment->setArgumentBuffer(ArgumentBufferFragment, 0);
    }
    /////////////////////////////////////////////////////////////////////////////////

    std::cout << "Map seed: " << chunkClass.seed << std::endl;
    std::cout << "Buffers built\n" << std::endl;
}
#pragma endregion build_buffers }

#pragma  region build_depth {
void Renderer::build_spencil(MTL::Device* device) {
    MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
    depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
    depthStencilDescriptor->setDepthWriteEnabled(true);

    DepthStencilState = device->newDepthStencilState( depthStencilDescriptor );

    depthStencilDescriptor->release();

    std::cout << "Depth built" << std::endl;
}
#pragma endregion build_depth }

void Renderer::GetMaps() {
    names.clear();
    seeds.clear();
    for(int i = 4; i < 512; i = i * 2) {
        if(std::filesystem::exists("map" + std::to_string(i) + "x" + std::to_string(i) + ".txt")) {
            names.push_back("map" + std::to_string(i) + "x" + std::to_string(i));
        }
    }
    std::string data;
    for(int j = 0; j < names.size(); j++) {
        std::ifstream chunkFileRead(names[j] + ".txt");
        std::getline(chunkFileRead, data);
        int counter = 0;
        std::string tempSeed = "";
        while(data[counter] != '-') {
            tempSeed += data[counter];
            counter++;
        }
        seeds.push_back(std::stoi(tempSeed));
    }
}

#pragma region render {
void Renderer::run() {
    using simd::float4x4;
    using simd::float4;
    using Clock = std::chrono::high_resolution_clock;

    Context* context = new Context;
    Window* window = new Window;
    Camera* camera = new Camera;

    bool first = true;
    int chunkLine = 8;
    int newChunkLine = 8;

    window->create_window();
    window->cursor_setup(true, window->glfwWindow, window->window_size);

    context->setup(window->glfwWindow);

    Renderer::build_shaders(context->device);
    Renderer::build_spencil(context->device);
    Renderer::build_textures(context->device);
    Renderer::build_buffers(context->device, first, chunkLine);

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

    NS::Range textureRange = NS::Range(0, (sizeof(TextureArr)/sizeof(TextureArr[0])));

    bool renderDist = true;
    bool culling = true;
    bool winding = true;
    bool depth = true;
    bool distanceFog = true;
    int blockCount = 0;
    int renderDistance = 10;

    GetMaps();

    ///////////ImGui font setup////////////
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig config1;
    config1.SizePixels = 24.0f;

    ImFontConfig config2;
    config2.SizePixels = 16.0f;

    ImFont* FPSFont = io.Fonts->AddFontDefault(&config1);
    ImFont* evrElse = io.Fonts->AddFontDefault(&config2);
    ///////////////////////////////////////

    std::cout << "Rendering" << std::endl;

    while (!glfwWindowShouldClose(window->glfwWindow)) {
        glfwPollEvents();

        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
        context->MetalDrawable = context->metalLayer->nextDrawable();

        frame = (frame + 1) % kMaxFramesInFlight;

        MTL::CommandBuffer* CommandBuffer = context->commandQueue->commandBuffer();
        dispatch_semaphore_wait(_semaphore, DISPATCH_TIME_FOREVER);
        CommandBuffer->addCompletedHandler(^void(MTL::CommandBuffer* pCommandBuffer){
            dispatch_semaphore_signal(_semaphore);
        });

        ////////////////////////Update camera state////////////////////////
        MTL::Buffer* CameraDataBufferL = CameraDataBuffer[frame];
        if(glfwGetWindowAttrib(window->glfwWindow, GLFW_FOCUSED) || first) {
            camera->update(CameraDataBufferL,window->glfwWindow,window->window_size,delta_time, window);
            first = false;
        }

        camera->CameraData->worldTransform = camera->view_mat;
        camera->CameraData->worldNormalTransform = math::discardTranslation(camera->CameraData->worldTransform);
        camera->CameraData->cameraPosition.x = camera->position.x; camera->CameraData->cameraPosition.y = camera->position.y; camera->CameraData->cameraPosition.z = camera->position.z;
        CameraDataBufferL->didModifyRange(NS::Range::Make(0, sizeof(CameraDataS)));
        ///////////////////////////////////////////////////////////////////

        MTL::RenderPassDescriptor* RenderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

        ///////////////////////////////////////Setup drawing plane//////////////////////////////////////////
        MTL::RenderPassColorAttachmentDescriptor* RenderPassColorAttachmentDescriptor = RenderPassDescriptor->colorAttachments()->object(0);
        RenderPassColorAttachmentDescriptor->setTexture(context->MetalDrawable->texture());
        RenderPassColorAttachmentDescriptor->setLoadAction(MTL::LoadActionClear);
        RenderPassColorAttachmentDescriptor->setClearColor(MTL::ClearColor(.4f, .6f, .9f, 1.0f));
        RenderPassColorAttachmentDescriptor->setStoreAction(MTL::StoreActionStore);
        ///////////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////Create imGui frame for drawing//////////////////////////////
        ImGui::GetIO().DisplayFramebufferScale = ImVec2(2,2);
        ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(window->window_size.x), static_cast<float>(window->window_size.y));

        ImGui_ImplMetal_NewFrame(RenderPassDescriptor);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        /////////////////////////////////////////////////////////////////////////////////
        //////////////Creating FPS counter or menu if active(using imGui)////////////////
        ImGui::PushFont(evrElse);
        if(window->menu) {
            if(window->options) {
                ImGui::SetWindowSize("Options",ImVec2(menu_size.x, menu_size.y), 0);
                ImGui::SetWindowPos("Options", ImVec2(window->window_size.x*0.5f - menu_size.x * 0.5f, window->window_size.y*0.5f - menu_size.y * 0.5f), 0);
                ImGui::Begin("Options", open , window_flags);
                ImGui::SetWindowFontScale(2);
                if(ImGui::Button("Back", ImVec2(menu_size.x, menu_size.y/3))) {
                    window->options = false;
                }

                ImGui::PushItemWidth(menu_size.x/3);
                if(ImGui::Button("Regenerate", ImVec2(menu_size.x/2 - 5, 35))) {
                    mapIndex++;
                    if(mapIndex > 1) {
                        mapIndex = 0;
                    }
                    regenerate = true;
                    int tempSeed = chunkClass.seed;
                    ChunkClass newChunkClass;
                    chunkClass = newChunkClass;
                    chunkClass.seed = tempSeed;
                    chunkLine = newChunkLine;
                    build_buffers(context->device, first, chunkLine);
                    GetMaps();
                }
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x/2 + 5);
                if(ImGui::Button("Load(if exists)", ImVec2(menu_size.x/2 - 5, 35))) {
                    mapIndex++;
                    if(mapIndex > 1) {
                        mapIndex = 0;
                    }
                    loadMap = true;
                    int tempSeed = chunkClass.seed;
                    ChunkClass newChunkClass;
                    chunkClass = newChunkClass;
                    chunkClass.seed = tempSeed;
                    chunkLine = newChunkLine;
                    build_buffers(context->device, first, chunkLine);
                }

                ImGui::Text("Chunk amount: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x-menu_size.x/3);
                const char* items2[] = {"4", "8", "16", "32", "64", "128"};
                static const char* current_item2 = "8";
                if (ImGui::BeginCombo("##1", current_item2))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(items2); n++)
                    {
                        bool is_selected2 = (current_item2 == items2[n]);
                        if (ImGui::Selectable(items2[n], is_selected2)) {
                            current_item2 = items2[n];
                            newChunkLine = std::atoi(current_item2);
                        }
                        if (is_selected2) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Text("Seed: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x-menu_size.x/3);
                ImGui::InputInt("##2", &chunkClass.seed, 1, 1);

                ImGui::Text("Generate new seed: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x - 33);
                ImGui::Checkbox("##3", &inAppRegenerate);

                ImGui::Text("Render distance: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x - 33);
                ImGui::Checkbox("##4", &renderDist);

                ImGui::Text("Distance fog: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x - 33);
                ImGui::Checkbox("##5", &distanceFog);

                ImGui::Text("Culling: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x - 33);
                ImGui::Checkbox("##6", &culling);

                ImGui::Text("Depth: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x - 33);
                ImGui::Checkbox("##7", &depth);

                ImGui::Text("Winding: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x-menu_size.x/3);
                const char* items[] = { "CounterClockWise", "ClockWise"};
                static const char* current_item = "CounterClockWise";
                if (ImGui::BeginCombo("##8", current_item))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                    {
                        bool is_selected = (current_item == items[n]);
                        if (ImGui::Selectable(items[n], is_selected)) {
                            current_item = items[n];
                            winding = !winding;
                        }
                        if (is_selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Text("Render distance(chunks): ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(menu_size.x-menu_size.x/3);
                const char* items1[] = { "4", "8", "10", "12", "16", "32", "100000"};
                static const char* current_item1 = "10";
                if (ImGui::BeginCombo("##9", current_item1))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(items1); n++)
                    {
                        bool is_selected1 = (current_item1 == items1[n]);
                        if (ImGui::Selectable(items1[n], is_selected1)) {
                            current_item1 = items1[n];
                            renderDistance = std::atoi(current_item1);
                        }
                        if (is_selected1) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                std::string namesSeedsOut = "";
                for(int i = 0; i < names.size(); i++) {
                    namesSeedsOut = namesSeedsOut + names[i] + " - Seed: " + std::to_string(seeds[i]) + "\n";
                }
                ImGui::Text("%s",namesSeedsOut.c_str());
                ImGui::PopItemWidth();
                ImGui::End();
            }
            else {
                ImGui::SetWindowSize("Menu",ImVec2(menu_size.x, menu_size.y), 0);
                ImGui::SetWindowPos("Menu", ImVec2(window->window_size.x*0.5f - menu_size.x * 0.5f, window->window_size.y*0.5f - menu_size.y * 0.5f), 0);

                ImGui::Begin("Menu", open , window_flags);
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
                    CommandEncoder->endEncoding();
                    pool->release();
                    context->commandQueue->release();
                    context->ns_window->release();
                    context->metalLayer->release();
                    glfwTerminate();
                    context->device->release();
                    RenderPipelineState->release();
                    DepthStencilState->release();

                    ImGui_ImplMetal_Shutdown();
                    ImGui_ImplGlfw_Shutdown();
                    ImGui::DestroyContext();
                    exit(0);
                    /////////////////////////////////////////////////////////////////////////////////
                }
                ImGui::End();
            }
        }
        ImGui::PopFont();
        ImGui::PushFont(FPSFont);
        ImGui::Text("X: %.0f, Y: %.0f, Z: %.0f\n%.1fFPS, %.3fms\nInstance count: %i", camera->position.x, camera->position.y, camera->position.z, ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate, blockCount);
        ImGui::PopFont();
        /////////////////////////////////////////////////////////////////////////////////
        ////////////////////////////////Set some things and render to screen////////////////////////////

        ////////////////////////////////Chunk data to GPU////////////////////////////////
        memcpy(mapBuffers[mapIndex]->contents(), chunkClass.chunkToGPU.data(), sizeof(ChunkToGPU) * chunkClass.chunkToGPU.size());
        mapBuffers[mapIndex]->didModifyRange(NS::Range::Make(0, mapBuffers[mapIndex]->length()));
        /////////////////////////////////////////////////////////////////////////////////
        ImGui::Render();
        NuberOfBlocksInChunk numberOfBlocksInChunk = {};
        numberOfBlocksInChunk.nuberOfBlocks.resize(chunkLine*chunkLine);
        int counter = 0;
        blockCount = 0;
        for(int i = 0; i < chunkLine*chunkLine;i++) {
            float temp = glm::distance(glm::vec3{camera->position.x, 0.0, camera->position.z}, glm::vec3{GetChunkPosition(chunkClass.chunkToGPU[i].chunkPos).x * 16 + 8, 0.0, GetChunkPosition(chunkClass.chunkToGPU[i].chunkPos).y * 16 + 8});
            if(renderDist) {
                if(temp <= renderDistance*16) {
                    chunkIndexToKeep[counter] = i;
                    numberOfBlocksInChunk.nuberOfBlocks[counter] = chunkClass.numberOfBlocksInChunk.nuberOfBlocks[i];
                    blockCount += chunkClass.numberOfBlocksInChunk.nuberOfBlocks[i];
                    counter++;
                }
            }
            else {
                chunkIndexToKeep[counter] = i;
                numberOfBlocksInChunk.nuberOfBlocks[counter] = chunkClass.numberOfBlocksInChunk.nuberOfBlocks[i];
                blockCount += chunkClass.numberOfBlocksInChunk.nuberOfBlocks[i];
                counter++;
            }
        }
        MTL::Buffer* chunkIndexBuffer = ChunkIndexesToBeRenderd[frame];

        memcpy(chunkIndexBuffer->contents(), chunkIndexToKeep.data(), sizeof(int) * chunkIndexToKeep.size());

        chunkIndexBuffer->didModifyRange(NS::Range::Make(0, chunkIndexBuffer->length()));



        MTL::Buffer* numberOfBlocksBuffer = NumberOfBlocksBufferVertex[frame];

        memcpy(numberOfBlocksBuffer->contents(), numberOfBlocksInChunk.nuberOfBlocks.data(), sizeof(int)*chunkLine*chunkLine);

        numberOfBlocksBuffer->didModifyRange(NS::Range::Make(0, numberOfBlocksBuffer->length()));



        MTL::Buffer* fogBuff = context->device->newBuffer(sizeof(distanceFog), MTL::ResourceStorageModeManaged);

        memcpy(fogBuff->contents(), &distanceFog, sizeof(distanceFog));

        fogBuff->didModifyRange(NS::Range::Make(0, fogBuff->length()));

        MTL::RenderCommandEncoder* CommandEncoder = CommandBuffer->renderCommandEncoder( RenderPassDescriptor);

        CommandEncoder->setRenderPipelineState( RenderPipelineState );
        if(depth) {
            CommandEncoder->setDepthStencilState(DepthStencilState);
        }

        //////////////////////////////////////Set buffers//////////////////////////////////////////
        CommandEncoder->setVertexBuffer(CameraDataBufferL, 0, 1);
        CommandEncoder->setVertexBuffer(mapBuffers[mapIndex], 0, 2);
        CommandEncoder->setVertexBuffer(numberOfBlocksBuffer, 0, 3 );
        CommandEncoder->setVertexBuffer(texture_real_index_buffer_vertex, 0, 4);
        CommandEncoder->setVertexBuffer(texture_side_amounts_buffer_vertex, 0, 5);
        CommandEncoder->setVertexBuffer(chunkIndexBuffer, 0, 6);
        CommandEncoder->setVertexBuffer(fogBuff, 0, 7);

        ArgumentEncoderFragment->setTextures(TextureArr, textureRange);
        CommandEncoder->setFragmentBuffer(ArgumentBufferFragment, 0, 0);
        //////////////////////////////////////////////////////////////////////////////////////////

        if(culling) {
            CommandEncoder->setCullMode(MTL::CullModeBack);
        }
        if(winding) {
            CommandEncoder->setFrontFacingWinding(MTL::Winding::WindingCounterClockwise);
        }
        else {
            CommandEncoder->setFrontFacingWinding(MTL::Winding::WindingClockwise);
        }

        CommandEncoder->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, 0, 36, blockCount);

        CommandEncoder->setFrontFacingWinding(MTL::Winding::WindingClockwise);
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), CommandBuffer, CommandEncoder);
        

        CommandEncoder->endEncoding();
        CommandBuffer->presentDrawable(context->MetalDrawable);
        CommandBuffer->commit();
        CommandBuffer->waitUntilCompleted();

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
    RenderPipelineState->release();
    DepthStencilState->release();

    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    /////////////////////////////////////////////////////////////////////////////////
}
#pragma endregion render }