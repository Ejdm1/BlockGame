#include "renderer.hpp"
#include "camera.hpp"
#include "context.hpp"
#include "window.hpp"

#include "math.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <FastNoise/FastNoise.h>

//////////////////Move bits in integer to get stored values//////////////////////
float MoveBits(int bitAmount, int bitsRight, int data) {
    int shiftedRight = data >> bitsRight;
    int bitMask = (1 << bitAmount) -1;
    int temp = shiftedRight & bitMask;
    return static_cast<float>(temp);
}
/////////////////////////////////////////////////////////////////////////////////
////////Get blocks position in chunk x,y,z(4bits x, 8bits y, 4bits z)////////////
simd::float3 GetPos(int data) {
    return simd::float3{MoveBits(4, 0, data), MoveBits(8, 4, data), MoveBits(4, 12, data)};
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////Get texture ID(max 14bits)//////////////////////////////
int GetBlockId(int data) {
    return MoveBits(14, 18, data);
}
/////////////////////////////////////////////////////////////////////////////////
////////////////////////Create sign bit for chunks position//////////////////////
int ChunkPosition(int posInX, int posInY) {
    if(posInX < 0) {
        posInX = (posInX * -1) + 128;
    }
    if(posInY < 0) {
        posInY = (posInY * -1) + 128;
    }
    int pos = 0;
    pos |= (posInX & 0xff) << 0;
    pos |= (posInY & 0xff) << 8;
    
    return pos;
}
/////////////////////////////////////////////////////////////////////////////////
//////////////Get chunks position in world space x,y(8bits each)/////////////////
glm::vec2 GetChunkPosition(int chunkData) {
    glm::vec2 posVec;
    posVec.y = MoveBits(8,0,chunkData);
    posVec.x = MoveBits(8,8,chunkData);
    if(posVec.x > 128) {
        posVec.x = (posVec.x * -1) + 128;
    };
    if(posVec.y > 128) {
        posVec.y = (posVec.y * -1) + 128;
    };
    return posVec;
}
/////////////////////////////////////////////////////////////////////////////////
#pragma region generate_terrain {

FastNoise::SmartNode<> Generate_Terrain() {
    auto OpenSimplex = FastNoise::New<FastNoise::OpenSimplex2>();
    auto FractalFBm = FastNoise::New<FastNoise::FractalFBm>();
    FractalFBm->SetSource(OpenSimplex);
    FractalFBm->SetGain(0.06f);
    FractalFBm->SetOctaveCount(4);
    FractalFBm->SetLacunarity(4.0f);
    auto DomainScale = FastNoise::New<FastNoise::DomainScale>();
    DomainScale->SetSource(FractalFBm);
    DomainScale->SetScale(1.64f);//0.86
    auto PosationOutput = FastNoise::New<FastNoise::PositionOutput>();
    PosationOutput->Set<FastNoise::Dim::Y>(6.72f);
    auto add = FastNoise::New<FastNoise::Add>();
    add->SetLHS(DomainScale);
    add->SetRHS(PosationOutput);

    return add;
}

#pragma endregion generate_terrain }

#pragma region build_shader {
void Renderer::build_shaders(MTL::Device* device) {
    using NS::StringEncoding::UTF8StringEncoding;

    std::string tempShaderStr;
    std::string shaderStr = "";

    std::ifstream shadeingFile;
    shadeingFile.open("./src/shaders.metal");//"./src/shaders.metal");

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
#pragma endregion build_textures }
/////////////////////////////////////////////////////////////////////////////////
#include <fstream>
#pragma region build_buffers {
void Renderer::build_buffers(MTL::Device* device) {
    using simd::float2;
    using simd::float3;

    ////////////////////////Block verts to GPU///////////////////////////////////////

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
    /////////////////////////////////////////////////////////////////////////////////
    /////////////Creating buffer for camera data and saveing them into it////////////
    const size_t cameraDataSize = kMaxFramesInFlight * sizeof( CameraData );
    for ( size_t i = 0; i < kMaxFramesInFlight; i++ ) {
        _pCameraDataBuffer[ i ] = device->newBuffer( cameraDataSize, MTL::ResourceStorageModeManaged );
    }
    /////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////Chunk creator//////////////////////////////////////
    std::vector<Chunk> chunk = {};
    chunk.resize(chunkCount);

    std::memset(chunk.data(), 0, sizeof( Chunk )* chunk.size());

    ////////////////////////////////////////Generate new map/////////////////////////////////////////
    bool loadMap;
    std::ifstream mapFileTest(mapFileName + ".txt");
    if(mapFileTest.good()) {
        loadMap = true;
    }
    else {
        loadMap = false;
    }
    mapFileTest.close();
    if(!loadMap) {
        auto generator = Generate_Terrain();
        std::vector<float> noiseMap(256 * 16 * 16);
        std::vector<glm::vec2> positions;
        int count = 0;

        for(int i = 0; i < chunkLine;i++) {
            for(int j = 0; j < chunkLine; j++) {
                glm::vec2 position = glm::vec2{i, j};
                positions.push_back(position);
                generator->GenUniformGrid3D(noiseMap.data(), 16 * position.x, -20,16 * position.y, 16, 256, 16, 0.01f, 1337);
                // std::cout << position.x << " " << position.y << std::endl;
                int index = 0;
                for(int x = 0; x < 16; x++) {
                    for(int y = 0; y < 256; y++) {
                        for(int z = 0; z < 16; z++) {
                            if(noiseMap[index] >= 0.0f) {
                                chunk[count].blocks[y][x][z] = 0;
                            }else {
                                if(noiseMap[index + 16] >= 0) {
                                    chunk[count].blocks[y][x][z] = blockFace(glm::vec3 {x,y,z}, 4);
                                } 
                                else if(noiseMap[index + 80] >= 0) {
                                    chunk[count].blocks[y][x][z] = blockFace(glm::vec3 {x,y,z}, 3);
                                } 
                                else if(noiseMap[index] < 0) {
                                    chunk[count].blocks[y][x][z] = blockFace(glm::vec3 {x,y,z}, 15);
                                }
                            }
                            index++;
                        }
                    }
                }
                count++;
            }
        }

        std::ofstream chunkFileWrite(mapFileName +".txt");
        for(int k = 0; k < chunkCount; k++) {
            chunkFileWrite << "-" << k << "~";
            chunkFileWrite << ChunkPosition(positions[k].x, positions[k].y) << " ";
            for(int i = 0; i < 256; i++) {
                for(int l = 0; l < 16; l++) {
                    for(int m = 0; m < 16; m++) {
                        int temp = chunk[k].blocks[i][l][m];
                        if(i == 255 && l == 15 && m > 14) {
                            chunkFileWrite << temp;
                        }
                        else {
                            chunkFileWrite << temp << " ";
                        }
                    }
                }
            }
            chunkFileWrite << "%\n";
        }
        chunkFileWrite.close();
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////

    std::ifstream chunkFileRead(mapFileName + ".txt");

    std::string data;
    bool startLoading = false;
    std::vector<std::vector<int>> loadedChunks;
    std::vector<int> loadedBlocks;
    std::vector<int> loadedChunkPos;
    while(std::getline(chunkFileRead, data)) {
        for(int i = 0; i < data.size(); i++) {
            if(data[i] == '-') {
                int counter = 1;
                std::cout << "Loading chunk: ";
                while(data[counter] != '~') {
                    std::cout << data[i+counter];
                    counter++;
                }
                std::cout << std::endl;
                startLoading = true;
            }
            if(data[i] == '~') {
                std::string tempPos = "";
                for(int f = 0; f < 32; f++) {
                    if(data[i+f+1] == ' ') {
                        break;
                    }
                    tempPos += data[i+f+1];
                }
                loadedChunkPos.push_back(std::stoi(tempPos));
            }
            if(data[i] == '%') {
                startLoading = false;
                loadedChunks.push_back(loadedBlocks);
                loadedBlocks.clear();
            }
            if(startLoading) {
                if(data[i] == ' ') {
                    std::string tempBlock = "";
                    for(int j = 0; j < 7; j++) {
                        tempBlock += data[i+j+1];
                    }
                    loadedBlocks.push_back(std::stoi(tempBlock));
                }
            }
        }
    }

    for(int i = 0; i < loadedChunks.size(); i++) {
        int i256 = 0;
        int l16 = 0;
        int m16 = 0;
        for(int data : loadedChunks[i]) {
            if(m16 == 16) {
                m16 = 0;
                l16++;
            }
            if(l16 == 16) {
                l16 = 0;
                i256++;
            }
            chunk[i].blocks[i256][l16][m16] = data;
            m16++;
        }
    }
    chunkFileRead.close();

    NuberOfBlocksInChunk numberOfBlocksInChunk;
    std::vector<Block> blocks;
    bool side1 = false, side2 = false, side3 = false, side4 = false, top = false, bottom = false, checkBlock = false;
    for(int k = 0; k < chunkCount; k++) {
        for(int i = 0; i < 256; i++) {
            for(int l = 0; l < 16; l++) {
                for(int m = 0; m < 16; m++) {
                    if(m != 15 && chunk[k].blocks[i][l][m+1] != 0) {
                        side1 = true;
                    }
                    if(m != 0  && chunk[k].blocks[i][l][m-1] != 0) {
                        side2 = true;
                    }
                    if(l != 15 && chunk[k].blocks[i][l+1][m] != 0) {
                        side3 = true;
                    }
                    if(l != 0  && chunk[k].blocks[i][l-1][m] != 0) {
                        side4 = true;
                    }
                    if(i != 255 && chunk[k].blocks[i+1][l][m] != 0) {
                        top = true;
                    }
                    if(i != 0 && chunk[k].blocks[i-1][l][m] != 0) {
                        bottom = true;
                    }
                    /////deletes lowest layer
                    if(i == 0 && chunk[k].blocks[i+1][l][m] != 0) {
                        bottom = true;
                    }
                    /////////////////////////
                    if(chunk[k].blocks[i][l][m] != 0) {
                        checkBlock = true;
                    }
                    if(!side1 || !side2 || !side3 || !side4 || !top || !bottom) {
                        if(checkBlock) {
                            Block temp;
                            temp.block = chunk[k].blocks[i][l][m];
                            blocks.push_back(temp);
                            blockCounter++;
                        }
                    }//std::unordered_map<class Key, class Tp>
                    side1 = false, side2 = false, side3 = false, side4 = false, top = false, bottom = false, checkBlock = false;
                }
            }
        }
        numberOfBlocksInChunk.nuberOfBlocks[k] = blockCounter;
        blockCounter = 0;
    }
    std::vector<ChunkToGPU> chunkToGPU = {};
    chunkToGPU.resize(chunkCount);
    std::memset(chunkToGPU.data(), 0, sizeof(ChunkToGPU) * chunkToGPU.size());

    int aijfgoiusdagfui = 0;
    for(int c = 0; c < chunkCount; c++) {
        for(int i = 0; i < numberOfBlocksInChunk.nuberOfBlocks[c]; i++) {
            chunkToGPU[c].blocks[i] = blocks[i + aijfgoiusdagfui].block;
        }
        aijfgoiusdagfui += numberOfBlocksInChunk.nuberOfBlocks[c];
        chunkToGPU[c].chunkPos = loadedChunkPos[c];
    }
    /////////////////////////////////////////////////////////////////////////////////
    for(int i = 0; i < sizeof(numberOfBlocksInChunk.nuberOfBlocks)/sizeof(numberOfBlocksInChunk.nuberOfBlocks[0]); i++) {
        blockCounter += numberOfBlocksInChunk.nuberOfBlocks[i];
    }
    std::cout << blockCounter << std::endl;
    ////////////////////////////////Number of Blocks to GPU//////////////////////////
    const size_t numberOfBlocksDataSize = sizeof(NuberOfBlocksInChunk);

    MTL::Buffer* pNumberOfBlocksBufferVertex = device->newBuffer(numberOfBlocksDataSize, MTL::ResourceStorageModeManaged);

    _pNumberOfBlocksBufferVertex = pNumberOfBlocksBufferVertex;

    memcpy(_pNumberOfBlocksBufferVertex->contents(), &numberOfBlocksInChunk, numberOfBlocksDataSize);

    _pNumberOfBlocksBufferVertex->didModifyRange(NS::Range::Make(0, _pNumberOfBlocksBufferVertex->length()));
    /////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////Chunk data to GPU////////////////////////////////
    const size_t chunkDataSize = sizeof(ChunkToGPU) * chunkToGPU.size();

    MTL::Buffer* pArgumentBufferVertex = device->newBuffer(chunkDataSize, MTL::ResourceStorageModeManaged);

    _pArgumentBufferVertex = pArgumentBufferVertex;

    memcpy(_pArgumentBufferVertex->contents(), chunkToGPU.data(), chunkDataSize);

    _pArgumentBufferVertex->didModifyRange(NS::Range::Make(0, _pArgumentBufferVertex->length()));

    pArgumentEncoderFragment = pFragmentFunction->newArgumentEncoder(0);

    size_t argumentBufferLengthFragment = pArgumentEncoderFragment->encodedLength();
    pArgumentBufferFragment = device->newBuffer(argumentBufferLengthFragment, MTL::ResourceStorageModeManaged);

    pArgumentEncoderFragment->setArgumentBuffer(pArgumentBufferFragment, 0);
    /////////////////////////////////////////////////////////////////////////////////

    std::cout << "Buffers built" << std::endl;
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

        MTL::RenderCommandEncoder* pCommandEncoder = pCommandBuffer->renderCommandEncoder( pRenderPassDescriptor );

        pCommandEncoder->setRenderPipelineState( pRenderPipelineState );
        pCommandEncoder->setDepthStencilState( pDepthStencilDescriptor );

        //////////////////////////////////////Set buffers//////////////////////////////////////////
        pCommandEncoder->setVertexBuffer( _pVertexDataBuffer, 0, 0 );
        pCommandEncoder->setVertexBuffer( pCameraDataBuffer, 0, 1 );
        pCommandEncoder->setVertexBuffer( _pArgumentBufferVertex, 0, 2 );
        pCommandEncoder->setVertexBuffer( _pNumberOfBlocksBufferVertex, 0, 3 );

        pArgumentEncoderFragment->setTextures( pTextureArr, textureRange);
        pCommandEncoder->setFragmentBuffer(pArgumentBufferFragment, 0, 0);
        //////////////////////////////////////////////////////////////////////////////////////////
        //////////////Creating FPS counter or menu if active(imGui)////////////////
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
        pCommandEncoder->setCullMode(MTL::CullModeBack);
        pCommandEncoder->setFrontFacingWinding(MTL::Winding::WindingCounterClockwise);

        pCommandEncoder->drawPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle, 0, 36, blockCounter);

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