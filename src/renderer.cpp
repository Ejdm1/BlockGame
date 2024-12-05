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

    std::string tempShader;
    std::string shader = "";

    std::ifstream myShaderFile;
    myShaderFile.open("/Users/adamkapsa/Documents/Python_bruh/cpp_metal_blockgame/last_push_metal-cmake-glfw/src/shaders.metal");

    if (!myShaderFile.is_open()) {
        std::cout << "Error: Couldnt be opened" << std::endl;
    }

    while (getline (myShaderFile, tempShader)) {
        shader =  shader + tempShader + "\n";
    }
    myShaderFile.close();

    const char* shaderSrc = shader.c_str();

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = device->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError );
    if ( !pLibrary ) {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }

    MTL::Function* pVertexFn = pLibrary->newFunction( NS::String::string("vertexMain", UTF8StringEncoding) );
    pFragFn = pLibrary->newFunction( NS::String::string("fragmentMain", UTF8StringEncoding) );

    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction( pVertexFn );
    pDesc->setFragmentFunction( pFragFn );
    pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    pDesc->setDepthAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth16Unorm );

    _pPSO = device->newRenderPipelineState( pDesc, &pError );
    if ( !_pPSO ) {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }

    pVertexFn->release();
    pFragFn->release();
    pDesc->release();
    _pShaderLibrary = pLibrary;
    
    std::cout << "Shaders built" << std::endl;
}

void Renderer::build_textures(MTL::Device* device)
{
    int size_x = 0;
    int size_y = 0;
    int num_channels = 0;
    stbi_uc* grass_top_texture = stbi_load("/Users/adamkapsa/Documents/Python_bruh/cpp_metal_blockgame/last_push_metal-cmake-glfw/src/Textures/Grass/Grass_top.png", &size_x, &size_y, &num_channels, 4);
    if(grass_top_texture == nullptr) {
        throw std::runtime_error("Textures couldn't be found");
    }

    stbi_uc* grass_side_texture = stbi_load("/Users/adamkapsa/Documents/Python_bruh/cpp_metal_blockgame/last_push_metal-cmake-glfw/src/Textures/Grass/Grass_side.png", &size_x, &size_y, &num_channels, 4);
    if(grass_side_texture == nullptr) {
        throw std::runtime_error("Textures couldn't be found");
    }

    stbi_uc* grass_bottom_texture = stbi_load("/Users/adamkapsa/Documents/Python_bruh/cpp_metal_blockgame/last_push_metal-cmake-glfw/src/Textures/Grass/Grass_bottom.png", &size_x, &size_y, &num_channels, 4);
    if(grass_bottom_texture == nullptr) {
        throw std::runtime_error("Textures couldn't be found");
    }

    MTL::TextureDescriptor* pTextureDesc = MTL::TextureDescriptor::alloc()->init();
    pTextureDesc->setWidth( static_cast<NS::UInteger>(size_x) );
    pTextureDesc->setHeight( static_cast<NS::UInteger>(size_y) );
    pTextureDesc->setPixelFormat( MTL::PixelFormatRGBA8Unorm_sRGB );
    pTextureDesc->setTextureType( MTL::TextureType2D );
    pTextureDesc->setStorageMode( MTL::StorageModeManaged );
    pTextureDesc->setUsage( MTL::ResourceUsageSample | MTL::ResourceUsageRead );

    _pTexture[0] = device->newTexture( pTextureDesc );
    _pTexture[1] = device->newTexture( pTextureDesc );
    _pTexture[2] = device->newTexture( pTextureDesc );

    _pTexture[0]->replaceRegion( MTL::Region( 0, 0, 0, size_x, size_y, 1 ), 0, grass_top_texture, size_x * 4 );
    _pTexture[1]->replaceRegion( MTL::Region( 0, 0, 0, size_x, size_y, 1 ), 0, grass_side_texture, size_x * 4 );
    _pTexture[2]->replaceRegion( MTL::Region( 0, 0, 0, size_x, size_y, 1 ), 0, grass_bottom_texture, size_x * 4 );

    stbi_image_free(grass_top_texture);
    stbi_image_free(grass_side_texture);
    stbi_image_free(grass_bottom_texture);
    pTextureDesc->release();

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
         0,  1,  2,  2,  3,  0, /* front */
         4,  5,  6,  6,  7,  4, /* right */
         8,  9, 10, 10, 11,  8, /* back */
        12, 13, 14, 14, 15, 12, /* left */
        16, 17, 18, 18, 19, 16, /* top */
        20, 21, 22, 22, 23, 20, /* bottom */
    };

    BlockData* blockData = new BlockData;

    
    int temp = blockData->sideFront;
    std::cout << "pos: " <<GetPos(temp)[0] << ", " << GetPos(temp)[1] << ", " << GetPos(temp)[2] << " side: " << GetSide(temp) << " blockID: " << GetBlockId(temp) << std::endl;
    std::cout << "binary: " << std::bitset<32>(temp) << std::endl;
    std::cout << GetSide(temp) << std::endl;

    const size_t blockDataSize = sizeof( &blockData );

    const size_t vertexDataSize = sizeof( verts );
    const size_t indexDataSize = sizeof( indices );

    MTL::Buffer* pBlockBuffer = device->newBuffer( blockDataSize, MTL::ResourceStorageModeManaged );
    MTL::Buffer* pVertexBuffer = device->newBuffer( vertexDataSize, MTL::ResourceStorageModeManaged );
    MTL::Buffer* pIndexBuffer = device->newBuffer( indexDataSize, MTL::ResourceStorageModeManaged );

    _pBlockDataBuffer = pBlockBuffer;
    _pVertexDataBuffer = pVertexBuffer;
    _pIndexBuffer = pIndexBuffer;

    memcpy( _pBlockDataBuffer->contents(), verts, blockDataSize );
    memcpy( _pVertexDataBuffer->contents(), verts, vertexDataSize );
    memcpy( _pIndexBuffer->contents(), indices, indexDataSize );

    _pBlockDataBuffer->didModifyRange(NS::Range::Make(0, _pBlockDataBuffer->length() ) );
    _pVertexDataBuffer->didModifyRange( NS::Range::Make( 0, _pVertexDataBuffer->length() ) );
    _pIndexBuffer->didModifyRange( NS::Range::Make( 0, _pIndexBuffer->length() ) );

    const size_t instanceDataSize = kMaxFramesInFlight * kNumInstances * sizeof( InstanceData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i ) {
        _pInstanceDataBuffer[ i ] = device->newBuffer( instanceDataSize, MTL::ResourceStorageModeManaged );
    }

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
    MTL::DepthStencilDescriptor* pDsDesc = MTL::DepthStencilDescriptor::alloc()->init();
    pDsDesc->setDepthCompareFunction( MTL::CompareFunction::CompareFunctionLess );
    pDsDesc->setDepthWriteEnabled( true );

    _pDepthStencilState = device->newDepthStencilState( pDsDesc );

    pDsDesc->release();

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

    _semaphore = dispatch_semaphore_create( kMaxFramesInFlight );

    Clock::time_point start = Clock::now(), prev_time = start;
    delta_time = 1.0f;

    std::cout << "Rendering" << std::endl;

    while (!glfwWindowShouldClose(window->glfwWindow)) {
        glfwPollEvents();

        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
        context->metalDrawable = context->metalLayer->nextDrawable();

        _frame = (_frame + 1) % kMaxFramesInFlight;
        MTL::Buffer* pInstanceDataBuffer = _pInstanceDataBuffer[ _frame ];

        MTL::CommandBuffer* pCmd = context->commandQueue->commandBuffer();
        dispatch_semaphore_wait( _semaphore, DISPATCH_TIME_FOREVER );
        pCmd->addCompletedHandler( ^void( MTL::CommandBuffer* pCmd ){
            dispatch_semaphore_signal( _semaphore );
        });

        const float scl = 1.f;
        InstanceData* pInstanceData = reinterpret_cast< InstanceData *>( pInstanceDataBuffer->contents() );

        glm::vec3 objectPosition = { 0.f, 0.f, 0.f };

        float4x4 rr1 = math::makeYRotate( 0.0f );
        float4x4 rr0 = math::makeXRotate( 0.0f );
        float4x4 fullObjectRot = rr1 * rr0;

        // Update instance positions:
        float4x4 scale = math::makeScale( (glm::vec3){ scl, scl, scl } );
        float4x4 zrot = math::makeZRotate( 0.0f );
        float4x4 yrot = math::makeYRotate( 0.0f);

        float4x4 translate = math::makeTranslate( objectPosition );

        pInstanceData[ 0 ].instanceTransform = fullObjectRot * translate * yrot * zrot * scale;
        pInstanceData[ 0 ].instanceNormalTransform = math::discardTranslation( *reinterpret_cast<glm::mat4*>(&pInstanceData[ 0 ].instanceTransform));

        pInstanceData[ 0 ].instanceColor = (float4){ 1.0f, 1.0f, 1.0f, 1.0f };
        
        pInstanceDataBuffer->didModifyRange( NS::Range::Make( 0, pInstanceDataBuffer->length() ) );
            
        // Update camera state:
        MTL::Buffer* pCameraDataBuffer = _pCameraDataBuffer[ _frame ];

        camera->update(pCameraDataBuffer,window->glfwWindow,window->window_size,delta_time, window);
        
        camera->pCameraData->worldTransform = camera->view_mat;
        camera->pCameraData->worldNormalTransform = math::discardTranslation( camera->pCameraData->worldTransform );
        pCameraDataBuffer->didModifyRange( NS::Range::Make( 0, sizeof( CameraData ) ) );

        MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

        MTL::RenderPassColorAttachmentDescriptor* colorAttachment = renderPassDescriptor->colorAttachments()->object(0);
        colorAttachment->setTexture(context->metalDrawable->texture());
        colorAttachment->setLoadAction(MTL::LoadActionClear);
        colorAttachment->setClearColor(MTL::ClearColor(.2f, .2f, .3f, 1.0f));
        colorAttachment->setStoreAction(MTL::StoreActionStore);

        MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( renderPassDescriptor );

        pEnc->setRenderPipelineState( _pPSO );
        pEnc->setDepthStencilState( _pDepthStencilState );

        pEnc->setVertexBuffer( _pVertexDataBuffer, 0, 0 );
        pEnc->setVertexBuffer( pInstanceDataBuffer, 0, 1 );
        pEnc->setVertexBuffer( pCameraDataBuffer, 0, 2 );
        pEnc->setVertexBuffer( _pBlockDataBuffer, 0, 3);

        MTL::ArgumentEncoder* argEncoder = pFragFn->newArgumentEncoder(0);

        size_t argBufferLength = argEncoder->encodedLength();
        MTL::Buffer* argumentBuffer = context->device->newBuffer(argBufferLength, MTL::ResourceStorageModeShared);

        argEncoder->setArgumentBuffer(argumentBuffer, 0);

        argEncoder->setTexture( _pTexture[0], 0);
        argEncoder->setTexture( _pTexture[1], 1);
        argEncoder->setTexture( _pTexture[2], 2);

        pEnc->setFragmentBuffer(argumentBuffer, 0, 0);





        

        pEnc->setCullMode( MTL::CullModeBack );
        pEnc->setFrontFacingWinding( MTL::Winding::WindingCounterClockwise );

        pEnc->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
                                    6 * 6, MTL::IndexType::IndexTypeUInt16,
                                    _pIndexBuffer,
                                    0,
                                    kNumInstances );

        pEnc->endEncoding();
        pCmd->presentDrawable(context->metalDrawable);
        pCmd->commit();
        pCmd->waitUntilCompleted();

        pool->release();
        auto now = Clock::now();
        delta_time = std::chrono::duration<float>(now - prev_time).count();
        prev_time = now;
        //std::cout << "\r" << (1.0f / delta_time) << std::flush;
    }
    context->commandQueue->release();
    context->window->release();
    context->metalLayer->release();
    glfwTerminate();
    context->device->release();
    _pShaderLibrary->release();
    _pVertexColorsBuffer->release();
    for ( int i = 0; i <  kMaxFramesInFlight; ++i ) {
        _pInstanceDataBuffer[i]->release();
    }
    _pIndexBuffer->release();
    _pPSO->release();
}