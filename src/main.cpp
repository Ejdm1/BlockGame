#include <cstdio>
#define GLFW_INCLUDE_NONE
#include <glfw/include/GLFW/glfw3.h>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Metal/Metal.hpp>

#include "backend/glfw_adapter.h"
#include "namespaces.hpp"

#include <iostream>


MTL::Device* device;
MTL::RenderPipelineState* _pPSO;
MTL::Buffer* _pVertexColorsBuffer;
MTL::Library* _pShaderLibrary;
MTL::Buffer* _pFrameData[3];
static constexpr size_t kNumInstances = 32;
static constexpr size_t kMaxFramesInFlight = 3;
float _angle = 0.0f;
int _frame = 0;
dispatch_semaphore_t _semaphore;
MTL::DepthStencilState* _pDepthStencilState;
MTL::Buffer* _pInstanceDataBuffer[kMaxFramesInFlight];
MTL::Buffer* _pCameraDataBuffer[kMaxFramesInFlight];
MTL::Buffer* _pIndexBuffer;
MTL::Buffer* _pVertexDataBuffer;

namespace math {
    constexpr simd::float3 add( const simd::float3& a, const simd::float3& b ) {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    constexpr simd_float4x4 makeIdentity() {
        using simd::float4;
        return (simd_float4x4){ (float4){ 1.f, 0.f, 0.f, 0.f },
                                (float4){ 0.f, 1.f, 0.f, 0.f },
                                (float4){ 0.f, 0.f, 1.f, 0.f },
                                (float4){ 0.f, 0.f, 0.f, 1.f } };
    }

    simd::float4x4 makePerspective( float fovRadians, float aspect, float znear, float zfar ) {
        using simd::float4;
        float ys = 1.f / tanf(fovRadians * 0.5f);
        float xs = ys / aspect;
        float zs = zfar / ( znear - zfar );
        return simd_matrix_from_rows((float4){ xs, 0.0f, 0.0f, 0.0f },
                                     (float4){ 0.0f, ys, 0.0f, 0.0f },
                                     (float4){ 0.0f, 0.0f, zs, znear * zs },
                                     (float4){ 0, 0, -1, 0 });
    }

    simd::float4x4 makeXRotate( float angleRadians ) {
        using simd::float4;
        const float a = angleRadians;
        return simd_matrix_from_rows((float4){ 1.0f, 0.0f, 0.0f, 0.0f },
                                     (float4){ 0.0f, cosf( a ), sinf( a ), 0.0f },
                                     (float4){ 0.0f, -sinf( a ), cosf( a ), 0.0f },
                                     (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    simd::float4x4 makeYRotate( float angleRadians ) {
        using simd::float4;
        const float a = angleRadians;
        return simd_matrix_from_rows((float4){ cosf( a ), 0.0f, sinf( a ), 0.0f },
                                     (float4){ 0.0f, 1.0f, 0.0f, 0.0f },
                                     (float4){ -sinf( a ), 0.0f, cosf( a ), 0.0f },
                                     (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    simd::float4x4 makeZRotate( float angleRadians ) {
        using simd::float4;
        const float a = angleRadians;
        return simd_matrix_from_rows((float4){ cosf( a ), sinf( a ), 0.0f, 0.0f },
                                     (float4){ -sinf( a ), cosf( a ), 0.0f, 0.0f },
                                     (float4){ 0.0f, 0.0f, 1.0f, 0.0f },
                                     (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    simd::float4x4 makeTranslate( const simd::float3& v ) {
        using simd::float4;
        const float4 col0 = { 1.0f, 0.0f, 0.0f, 0.0f };
        const float4 col1 = { 0.0f, 1.0f, 0.0f, 0.0f };
        const float4 col2 = { 0.0f, 0.0f, 1.0f, 0.0f };
        const float4 col3 = { v.x, v.y, v.z, 1.0f };
        return simd_matrix( col0, col1, col2, col3 );
    }

    simd::float4x4 makeScale( const simd::float3& v ) {
        using simd::float4;
        return simd_matrix((float4){ v.x, 0, 0, 0 },
                           (float4){ 0, v.y, 0, 0 },
                           (float4){ 0, 0, v.z, 0 },
                           (float4){ 0, 0, 0, 1.0 });
    }

}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
    }
}

void buildShader() {
    using NS::StringEncoding::UTF8StringEncoding;

    const char* shaderSrc = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f {
            float4 position [[position]];
            half3 color;
        };

        struct VertexData {
            float3 position;
        };

        struct InstanceData {
            float4x4 instanceTransform;
            float4 instanceColor;
        };

        struct CameraData {
            float4x4 perspectiveTransform;
            float4x4 worldTransform;
        };

        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                               device const InstanceData* instanceData [[buffer(1)]],
                               device const CameraData& cameraData [[buffer(2)]],
                               uint vertexId [[vertex_id]],
                               uint instanceId [[instance_id]] )
        {
            v2f o;
            float4 pos = float4( vertexData[ vertexId ].position, 1.0 );
            pos = instanceData[ instanceId ].instanceTransform * pos;
            pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
            o.position = pos;
            o.color = half3( instanceData[ instanceId ].instanceColor.rgb );
            return o;
        }

        half4 fragment fragmentMain( v2f in [[stage_in]] ) {
            return half4( in.color, 1.0 );
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

void buildBuffers() {
    using simd::float3;
    const float s = 0.5f;

    float3 verts[] = {
        { -s, -s, +s },
        { +s, -s, +s },
        { +s, +s, +s },
        { -s, +s, +s },

        { -s, -s, -s },
        { -s, +s, -s },
        { +s, +s, -s },
        { +s, -s, -s }
    };

    uint16_t indices[] = {
        0, 1, 2, /* front */
        2, 3, 0,

        1, 7, 6, /* right */
        6, 2, 1,

        7, 4, 5, /* back */
        5, 6, 7,

        4, 0, 3, /* left */
        3, 5, 4,

        3, 2, 6, /* top */
        6, 5, 3,

        4, 7, 1, /* bottom */
        1, 0, 4
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
    for ( int i = 0; i < kMaxFramesInFlight; ++i )
    {
        _pFrameData[ i ]= device->newBuffer( sizeof( FrameData ), MTL::ResourceStorageModeManaged );
    }
}

struct CameraData {
    simd::float4x4 perspectiveTransform;
    simd::float4x4 worldTransform;
};

void buildDepthStencilStates() {
    MTL::DepthStencilDescriptor* pDsDesc = MTL::DepthStencilDescriptor::alloc()->init();
    pDsDesc->setDepthCompareFunction( MTL::CompareFunction::CompareFunctionLess );
    pDsDesc->setDepthWriteEnabled( true );

    _pDepthStencilState = device->newDepthStencilState( pDsDesc );

    pDsDesc->release();

    std::cout << "Depth built" << std::endl;
}

int main() {

    using simd::float3;
    using simd::float4;
    using simd::float4x4;
    //glfw stuff
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* glfwWindow = glfwCreateWindow(512, 512, "Heavy", NULL, NULL);

    //Metal Device
    device = MTL::CreateSystemDefaultDevice();

    //Metal Layer
    CA::MetalLayer* metalLayer = CA::MetalLayer::layer()->retain();
    metalLayer->setDevice(device);
    // metalLayer->setPixelFormat(MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);

    //Adapt glfw window to NS Window, and give it a layer to draw to
    NS::Window* window = get_ns_window(glfwWindow, metalLayer)->retain();

    //Drawable Area
    CA::MetalDrawable* metalDrawable;

    //Command Queue
    MTL::CommandQueue* commandQueue = device->newCommandQueue()->retain();

    buildShader();
    buildBuffers();
    buildDepthStencilStates();
    buildFrameData();

    _semaphore = dispatch_semaphore_create( kMaxFramesInFlight );

    std::cout << "Main ready" << std::endl;

    while (!glfwWindowShouldClose(glfwWindow)) {
        glfwPollEvents();

        glfwSetKeyCallback(glfwWindow, key_callback);

        NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
        metalDrawable = metalLayer->nextDrawable();

        _frame = (_frame + 1) % kMaxFramesInFlight;
        MTL::Buffer* pInstanceDataBuffer = _pInstanceDataBuffer[ _frame ];

        MTL::CommandBuffer* pCmd = commandQueue->commandBuffer();
        dispatch_semaphore_wait( _semaphore, DISPATCH_TIME_FOREVER );
        pCmd->addCompletedHandler( ^void( MTL::CommandBuffer* pCmd ){
            dispatch_semaphore_signal( _semaphore );
        });

        _angle += 0.01f;

        const float scl = 0.1f;
        shader_types::InstanceData* pInstanceData = reinterpret_cast< shader_types::InstanceData *>( pInstanceDataBuffer->contents() );

        float3 objectPosition = { 0.f, 0.f, -5.f };

        // Update instance positions:

        float4x4 rt = math::makeTranslate( objectPosition );
        float4x4 rr = math::makeYRotate( -_angle );
        float4x4 rtInv = math::makeTranslate( { -objectPosition.x, -objectPosition.y, -objectPosition.z } );
        float4x4 fullObjectRot = rt * rr * rtInv;

        for ( size_t i = 0; i < kNumInstances; ++i ) {
            float iDivNumInstances = i / (float)kNumInstances;
            float xoff = (iDivNumInstances * 2.0f - 1.0f) + (1.f/kNumInstances);
            float yoff = sin( ( iDivNumInstances + _angle ) * 2.0f * M_PI);

            // Use the tiny math library to apply a 3D transformation to the instance.
            float4x4 scale = math::makeScale( (float3){ scl, scl, scl } );
            float4x4 zrot = math::makeZRotate( _angle );
            float4x4 yrot = math::makeYRotate( _angle );
            float4x4 translate = math::makeTranslate( math::add( objectPosition, { xoff, yoff, 0.f } ) );

            pInstanceData[ i ].instanceTransform = fullObjectRot * translate * yrot * zrot * scale;

            float r = iDivNumInstances;
            float g = 1.0f - r;
            float b = sinf( M_PI * 2.0f * iDivNumInstances );
            pInstanceData[ i ].instanceColor = (float4){ r, g, b, 1.0f };
        }
        pInstanceDataBuffer->didModifyRange( NS::Range::Make( 0, pInstanceDataBuffer->length() ) );

        // Update camera state:

        MTL::Buffer* pCameraDataBuffer = _pCameraDataBuffer[ _frame ];
        shader_types::CameraData* pCameraData = reinterpret_cast< shader_types::CameraData *>( pCameraDataBuffer->contents() );
        pCameraData->perspectiveTransform = math::makePerspective( 45.f * M_PI / 180.f, 1.f, 0.03f, 500.0f ) ;
        pCameraData->worldTransform = math::makeIdentity();
        pCameraDataBuffer->didModifyRange( NS::Range::Make( 0, sizeof( shader_types::CameraData ) ) );


        MTL::RenderPassDescriptor* renderPassDescriptor = MTL::RenderPassDescriptor::alloc()->init();

        MTL::RenderPassColorAttachmentDescriptor* colorAttachment = renderPassDescriptor->colorAttachments()->object(0);
        colorAttachment->setTexture(metalDrawable->texture());
        colorAttachment->setLoadAction(MTL::LoadActionClear);
        colorAttachment->setClearColor(MTL::ClearColor(1.0f, 0.0f, 0.0f, 1.0f));
        colorAttachment->setStoreAction(MTL::StoreActionStore);

        MTL::RenderCommandEncoder* pEnc = pCmd->renderCommandEncoder( renderPassDescriptor );
        pEnc->setDepthStencilState( _pDepthStencilState );


        pEnc->setRenderPipelineState( _pPSO );
        pEnc->setVertexBuffer( _pVertexDataBuffer, /* offset */ 0, /* index */ 0 );
        pEnc->setVertexBuffer( pInstanceDataBuffer, /* offset */ 0, /* index */ 1 );
        pEnc->setVertexBuffer( pCameraDataBuffer, /* offset */ 0, /* index */ 2 );

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
    _pPSO->release();
    return 0;
}