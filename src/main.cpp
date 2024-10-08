#include <cstdio>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION

#include <Metal/Metal.hpp>
#include <MetalKit/MetalKit.hpp>

#include "backend/glfw_adapter.h"
#include "namespaces.hpp"
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
using Clock = std::chrono::high_resolution_clock;


static constexpr size_t kInstanceRows = 1;
static constexpr size_t kInstanceColumns = 1;
static constexpr size_t kInstanceDepth = 1;
static constexpr size_t kNumInstances = (kInstanceRows * kInstanceColumns * kInstanceDepth);
static constexpr size_t kMaxFramesInFlight = 3;

const float window_size_x = 512;
const float window_size_y = 512;

float mouse_sens = 0.5f;
float fov = 45.f;


MTL::Device* device;
MTL::RenderPipelineState* _pPSO;
MTL::Buffer* _pVertexColorsBuffer;
MTL::Library* _pShaderLibrary;
MTL::Buffer* _pFrameData[3];
int _frame = 0;
dispatch_semaphore_t _semaphore;
MTL::DepthStencilState* _pDepthStencilState;
MTL::Buffer* _pInstanceDataBuffer[kMaxFramesInFlight];
MTL::Buffer* _pCameraDataBuffer[kMaxFramesInFlight];
MTL::Buffer* _pIndexBuffer;
MTL::Buffer* _pVertexDataBuffer;
MTL::Texture* _pTexture;
GLFWwindow* glfwWindow;
glm::mat4 vtrn_mat{1.f};
glm::mat4 vrot_mat{1.f};

glm::vec3 position;
glm::vec2 rotation = {0.f,0.f};



namespace math {
    constexpr glm::vec3 add( const glm::vec3& a, const glm::vec3& b ) {
        return a + b;
    }

    constexpr glm::mat4 makeIdentity() {
        return glm::mat4(1.0f);
    }

    glm::mat4 viewMat() {
        glm::mat4 mp = vtrn_mat * vrot_mat;
        return mp;
    }

    simd::float4x4 makePerspective( float fovRadians, float aspect, float znear, float zfar ) {
        glm::mat4 mp = glm::perspective(fovRadians, aspect, znear, zfar);
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    simd::float4x4 makeXRotate( float angleRadians ) {
        glm::mat4 mp = glm::rotate(glm::mat4(1.0f), -angleRadians, glm::vec3{1.0f,0.0f,0.0f});
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    simd::float4x4 makeYRotate( float angleRadians ) {
        glm::mat4 mp = glm::rotate(glm::mat4(1.0f), angleRadians, glm::vec3{0.0f,1.0f,0.0f});
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    simd::float4x4 makeZRotate( float angleRadians ) {
        glm::mat4 mp = glm::rotate(glm::mat4(1.0f), -angleRadians, glm::vec3{0.0f,0.0f,1.0f});
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    simd::float4x4 makeTranslate( const glm::vec3& v ) {
        glm::mat4 mp = glm::translate(glm::mat4(1), v);
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    simd::float4x4 makeScale( const glm::vec3& v ) {
        glm::mat4 mp = glm::scale(glm::mat4(1.f),v);
        return *reinterpret_cast<simd::float4x4*>(&mp);
    }

    simd::float3x3 discardTranslation( const glm::mat4& m ) {
        glm::mat3 mp = glm::mat3(glm::mat4(m));
        return *reinterpret_cast<simd::float3x3*>(&mp);
    }
}

void on_mouse_move(GLFWwindow* window, double x, double y ) {
    x = window_size_x/2 - x;
    y = window_size_y/2 - y;
    if(glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
        rotation.x += static_cast<float>(x) * mouse_sens * 0.0001f * fov;
        rotation.y += static_cast<float>(y) * mouse_sens * 0.0001f * fov;
    }
    
    constexpr auto MAX_ROT = 1.56825555556f;
    if (rotation.y > MAX_ROT) { rotation.y = MAX_ROT; }
    if (rotation.y < -MAX_ROT) { rotation.y = -MAX_ROT; }
}

void on_key(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch (key) {
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}

struct VertexData {
    simd::float3 position;
    simd::float3 normal;
};

void buildShader() {
    using NS::StringEncoding::UTF8StringEncoding;

    const char* shaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f {
            float4 position [[position]];
            float3 normal;
            half3 color;
            float2 texcoord;
        };

        struct VertexData {
            float3 position;
            float3 normal;
            float2 texcoord;
        };

        struct InstanceData {
            float4x4 instanceTransform;
            float3x3 instanceNormalTransform;
            float4 instanceColor;
        };

        struct CameraData {
            float4x4 perspectiveTransform;
            float4x4 worldTransform;
            float3x3 worldNormalTransform;
        };

        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                               device const InstanceData* instanceData [[buffer(1)]],
                               device const CameraData& cameraData [[buffer(2)]],
                               uint vertexId [[vertex_id]],
                               uint instanceId [[instance_id]] ) {
            v2f o;

            const device VertexData& vd = vertexData[ vertexId ];
            float4 pos = float4( vd.position, 1.0 );
            pos = instanceData[ instanceId ].instanceTransform * pos;
            pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
            o.position = pos;

            float3 normal = instanceData[ instanceId ].instanceNormalTransform * vd.normal;
            normal = cameraData.worldNormalTransform * normal;
            o.normal = normal;

            o.texcoord = vd.texcoord.xy;

            o.color = half3( instanceData[ instanceId ].instanceColor.rgb );
            return o;
        }

        half4 fragment fragmentMain( v2f in [[stage_in]], texture2d< half, access::sample > tex [[texture(0)]] ) {
            constexpr sampler s( address::repeat, filter::linear );
            half3 texel = tex.sample( s, in.texcoord ).rgb;

            // assume light coming from (front-top-right)
            float3 l = normalize(float3( 1.0, 1.0, 1.0 ));
            float3 n = normalize( in.normal );

            half ndotl = half( saturate( dot( n, l ) ) );

            half3 illum = (in.color * texel * 0.1) + (in.color * texel * ndotl);
            return half4( illum, 1.0 );
        }
    )";

    NS::Error* pError = nullptr;
    MTL::Library* pLibrary = device->newLibrary( NS::String::string(shaderSrc, UTF8StringEncoding), nullptr, &pError );
    if ( !pLibrary ) {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }

    MTL::Function* pVertexFn = pLibrary->newFunction( NS::String::string("vertexMain", UTF8StringEncoding) );
    MTL::Function* pFragFn = pLibrary->newFunction( NS::String::string("fragmentMain", UTF8StringEncoding) );
        
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

void buildTextures()
{
    int size_x = 0;
    int size_y = 0;
    int num_channels = 0;
    stbi_uc* data = stbi_load("./src/Textures/Grass_block.png", &size_x, &size_y, &num_channels, 4);
    if(data == nullptr) {
        throw std::runtime_error("Textures couldn't be found");
    }

    MTL::TextureDescriptor* pTextureDesc = MTL::TextureDescriptor::alloc()->init();
    pTextureDesc->setWidth( static_cast<NS::UInteger>(size_x) );
    pTextureDesc->setHeight( static_cast<NS::UInteger>(size_y) );
    pTextureDesc->setPixelFormat( MTL::PixelFormatRGBA8Unorm_sRGB );
    pTextureDesc->setTextureType( MTL::TextureType2D );
    pTextureDesc->setStorageMode( MTL::StorageModeManaged );
    pTextureDesc->setUsage( MTL::ResourceUsageSample | MTL::ResourceUsageRead );

    MTL::Texture *pTexture = device->newTexture( pTextureDesc );
    _pTexture = pTexture;

    _pTexture->replaceRegion( MTL::Region( 0, 0, 0, size_x, size_y, 1 ), 0, data, size_x * 4 );
    stbi_image_free(data);
    pTextureDesc->release();

    std::cout << "Textures built" << std::endl;
}

void buildBuffers() {
    using simd::float2;
    using simd::float3;

    const float s = 0.5f;

    shader_types::VertexData verts[] = {
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

    const size_t instanceDataSize = kMaxFramesInFlight * kNumInstances * sizeof( shader_types::InstanceData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i ) {
        _pInstanceDataBuffer[ i ] = device->newBuffer( instanceDataSize, MTL::ResourceStorageModeManaged );
    }

    const size_t cameraDataSize = kMaxFramesInFlight * sizeof( shader_types::CameraData );
    for ( size_t i = 0; i < kMaxFramesInFlight; ++i ) {
        _pCameraDataBuffer[ i ] = device->newBuffer( cameraDataSize, MTL::ResourceStorageModeManaged );
    }

    std::cout << "Buffers built" << std::endl;
}

struct FrameData {
    float angle;
};

void buildFrameData() {
    for ( int i = 0; i < kMaxFramesInFlight; ++i ) {
        _pFrameData[ i ]= device->newBuffer( sizeof( FrameData ), MTL::ResourceStorageModeManaged );
    }

    std::cout << "Frame built" <<std::endl;
}

void buildDepthStencilStates() {
    MTL::DepthStencilDescriptor* pDsDesc = MTL::DepthStencilDescriptor::alloc()->init();
    pDsDesc->setDepthCompareFunction( MTL::CompareFunction::CompareFunctionLess );
    pDsDesc->setDepthWriteEnabled( true );

    _pDepthStencilState = device->newDepthStencilState( pDsDesc );

    pDsDesc->release();

    std::cout << "Depth built" << std::endl;
}

void cursor_setup(bool visible) {
    glfwSetCursorPos(glfwWindow, static_cast<float>(window_size_x / 2), static_cast<float>(window_size_y / 2));
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, visible ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, visible);
}

int main() {

    using simd::float3;
    using simd::float4;
    using simd::float4x4;
    //glfw stuff
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindow = glfwCreateWindow(window_size_x, window_size_y, "Heavy", nullptr, nullptr);

    glfwSetCursorPosCallback(glfwWindow, on_mouse_move);

    //Metal Device
    device = MTL::CreateSystemDefaultDevice();

    //Metal Layer
    CA::MetalLayer* metalLayer = CA::MetalLayer::layer()->retain();
    metalLayer->setDevice(device);
    metalLayer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);

    //Adapt glfw window to NS Window, and give it a layer to draw to
    NS::Window* window = get_ns_window(glfwWindow, metalLayer)->retain();

    //Drawable Area
    CA::MetalDrawable* metalDrawable;

    //Command Queue
    MTL::CommandQueue* commandQueue = device->newCommandQueue()->retain();

    buildFrameData();
    buildShader();
    buildDepthStencilStates();
    buildTextures();
    buildBuffers();

    _semaphore = dispatch_semaphore_create( kMaxFramesInFlight );

    std::cout << "Main ready" << std::endl;


    Clock::time_point start = Clock::now(), prev_time = start;
    float elapsed_s = 1.0f;

    while (!glfwWindowShouldClose(glfwWindow)) {
        glfwPollEvents();

        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
        metalDrawable = metalLayer->nextDrawable();

        _frame = (_frame + 1) % kMaxFramesInFlight;
        MTL::Buffer* pInstanceDataBuffer = _pInstanceDataBuffer[ _frame ];

        MTL::CommandBuffer* pCmd = commandQueue->commandBuffer();
        dispatch_semaphore_wait( _semaphore, DISPATCH_TIME_FOREVER );
        pCmd->addCompletedHandler( ^void( MTL::CommandBuffer* pCmd ){
            dispatch_semaphore_signal( _semaphore );
        });

        const float scl = 1.f;
        shader_types::InstanceData* pInstanceData = reinterpret_cast< shader_types::InstanceData *>( pInstanceDataBuffer->contents() );

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
        shader_types::CameraData* pCameraData = reinterpret_cast< shader_types::CameraData *>( pCameraDataBuffer->contents() );
        pCameraData->perspectiveTransform = math::makePerspective( fov * M_PI / 180.f, 1.f, 0.03f, 1000.0f ) ;
        
        if (glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
            cursor_setup(true);
        }
        else if (!glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) {
            cursor_setup(false);
        }

        glm::vec3 forward_direction = glm::normalize(glm::vec3{ glm::cos(rotation.x) * glm::cos(rotation.y), glm::sin(rotation.y), -glm::sin(rotation.x) * glm::cos(rotation.y) });
        glm::vec3 up_direction = glm::vec3{ 0.0f, 1.0f, 0.0f };
        glm::vec3 right_direction = glm::normalize(glm::cross(forward_direction, up_direction));

        glm::vec3 move_direction = glm::vec3{ 0.0f };

        if(glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED)) { 
            if (glfwGetKey(glfwWindow, GLFW_KEY_W)  == GLFW_PRESS) { move_direction += forward_direction; }
            if (glfwGetKey(glfwWindow, GLFW_KEY_S)  == GLFW_PRESS) { move_direction -= forward_direction; }
            if (glfwGetKey(glfwWindow, GLFW_KEY_D)  == GLFW_PRESS) { move_direction += right_direction; }
            if (glfwGetKey(glfwWindow, GLFW_KEY_A)  == GLFW_PRESS) { move_direction -= right_direction; }
            if (glfwGetKey(glfwWindow, GLFW_KEY_SPACE)  == GLFW_PRESS) { move_direction += up_direction; }
            if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL)  == GLFW_PRESS) { move_direction -= up_direction; }
            if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS) { position += move_direction * elapsed_s * 15.f; }
            else if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT)  != GLFW_PRESS) { position += move_direction * elapsed_s * 7.5f; }
            if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE)) {glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE); }
        }

        glm::mat4 view_mat = glm::lookAt(position, position + forward_direction, up_direction);
        
        
        pCameraData->worldTransform = view_mat;
        pCameraData->worldNormalTransform = math::discardTranslation( pCameraData->worldTransform );
        pCameraDataBuffer->didModifyRange( NS::Range::Make( 0, sizeof( shader_types::CameraData ) ) );

        MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

        MTL::RenderPassColorAttachmentDescriptor* colorAttachment = renderPassDescriptor->colorAttachments()->object(0);
        colorAttachment->setTexture(metalDrawable->texture());
        colorAttachment->setLoadAction(MTL::LoadActionClear);
        colorAttachment->setClearColor(MTL::ClearColor(.2f, .2f, .3f, 1.0f));
        colorAttachment->setStoreAction(MTL::StoreActionStore);

        MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( renderPassDescriptor );

        pEnc->setRenderPipelineState( _pPSO );
        pEnc->setDepthStencilState( _pDepthStencilState );

        pEnc->setVertexBuffer( _pVertexDataBuffer, /* offset */ 0, /* index */ 0 );
        pEnc->setVertexBuffer( pInstanceDataBuffer, /* offset */ 0, /* index */ 1 );
        pEnc->setVertexBuffer( pCameraDataBuffer, /* offset */ 0, /* index */ 2 );

        pEnc->setFragmentTexture( _pTexture, /* index */ 0 );

        pEnc->setCullMode( MTL::CullModeBack );
        pEnc->setFrontFacingWinding( MTL::Winding::WindingCounterClockwise );

        pEnc->drawIndexedPrimitives( MTL::PrimitiveType::PrimitiveTypeTriangle,
                                    6 * 6, MTL::IndexType::IndexTypeUInt16,
                                    _pIndexBuffer,
                                    0,
                                    kNumInstances );

        pEnc->endEncoding();
        pCmd->presentDrawable(metalDrawable);
        pCmd->commit();
        pCmd->waitUntilCompleted();

        pool->release();
        auto now = Clock::now();
        elapsed_s = std::chrono::duration<float>(now - prev_time).count();
        prev_time = now;
        std::cout << "\r" << (1.0f / elapsed_s) << std::flush;
    }

    commandQueue->release();
    window->release();
    metalLayer->release();
    glfwTerminate();
    device->release();
    _pShaderLibrary->release();
    _pVertexColorsBuffer->release();
    for ( int i = 0; i <  kMaxFramesInFlight; ++i ) {
        _pInstanceDataBuffer[i]->release();
    }
    _pIndexBuffer->release();
    _pTexture->release();
    _pPSO->release();
    return 0;
}