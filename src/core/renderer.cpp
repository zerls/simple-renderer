#include "maths.h"
#include "renderer.h"
#include "mesh.h"
#include "texture_io.h"
#include <omp.h>

// Global constants
constexpr int MSAA_SAMPLES = 4;
constexpr float EPSILON = 1e-6f;

// MSAA sample offsets
const Vec2f MSAA_OFFSETS[MSAA_SAMPLES] = {
    {0.25f, 0.25f}, {0.75f, 0.25f}, {0.25f, 0.75f}, {0.75f, 0.75f}};

// Utility function - Calculate barycentric coordinates (optimized)
inline Vec3f computeBarycentric2D(float x, float y, const std::array<Vec3f, 3> &v)
{
    float denominator1 = (v[0].x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * v[0].y + v[1].x * v[2].y - v[2].x * v[1].y);
    float denominator2 = (v[1].x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * v[1].y + v[2].x * v[0].y - v[0].x * v[2].y);

    if (std::abs(denominator1) < EPSILON)
        denominator1 = EPSILON;
    if (std::abs(denominator2) < EPSILON)
        denominator2 = EPSILON;

    float c1 = (x * (v[1].y - v[2].y) + (v[2].x - v[1].x) * y + v[1].x * v[2].y - v[2].x * v[1].y) / denominator1;
    float c2 = (x * (v[2].y - v[0].y) + (v[0].x - v[2].x) * y + v[2].x * v[0].y - v[0].x * v[2].y) / denominator2;

    return Vec3f(c1, c2, 1.0f - c1 - c2);
}

// Utility function - Check if point is inside triangle
inline bool isInsideTriangle(const Vec3f &barycentric)
{
    return barycentric.x > -EPSILON && barycentric.y > -EPSILON && barycentric.z > -EPSILON;
}

// Utility function - Calculate perspective weights
inline Vec4f calculatePerspectiveWeights(
    const Vec3f &barycentric,
    const std::array<ProcessedVertex, 3> &vertices)
{
    const Vec3f invW(
        1.0f / vertices[0].clipPosition.w,
        1.0f / vertices[1].clipPosition.w,
        1.0f / vertices[2].clipPosition.w);

    const float interpolated_invW = dot(barycentric, invW);

    return Vec4f(invW.x, invW.y, invW.z, 1.0f / interpolated_invW);
}

// Utility function - Backface culling check
// Note: reverse_sign is 1.0f for front face culling, -1.0f for back face culling
inline bool faceCull(
    const std::array<ProcessedVertex, 3> &verts,
    const float reverse_sign)
{
    // Direct access to x,y components instead of creating temporary Vec2f objects
    const float edge1x = verts[1].screenPosition.x - verts[0].screenPosition.x;
    const float edge1y = verts[1].screenPosition.y - verts[0].screenPosition.y;
    const float edge2x = verts[2].screenPosition.x - verts[0].screenPosition.x;
    const float edge2y = verts[2].screenPosition.y - verts[0].screenPosition.y;

    // Calculate cross product directly: edge1.x * edge2.y - edge1.y * edge2.x
    const float area2 = edge1x * edge2y - edge1y * edge2x;

    return (area2 * reverse_sign) <= EPSILON;
}

// Utility function - Calculate bounding box
inline std::tuple<int, int, int, int> calculateBoundingBox(
    const std::array<Vec3f, 3> &screenPositions,
    const int screenWidth,
    const int screenHeight)
{
    float minX = std::min(std::min(screenPositions[0].x, screenPositions[1].x), screenPositions[2].x);
    float minY = std::min(std::min(screenPositions[0].y, screenPositions[1].y), screenPositions[2].y);
    float maxX = std::max(std::max(screenPositions[0].x, screenPositions[1].x), screenPositions[2].x);
    float maxY = std::max(std::max(screenPositions[0].y, screenPositions[1].y), screenPositions[2].y);

    const int boundMinX = std::max(0, static_cast<int>(std::floor(minX)));
    const int boundMinY = std::max(0, static_cast<int>(std::floor(minY)));
    const int boundMaxX = std::min(screenWidth - 1, static_cast<int>(std::ceil(maxX)));
    const int boundMaxY = std::min(screenHeight - 1, static_cast<int>(std::ceil(maxY)));

    return {boundMinX, boundMinY, boundMaxX, boundMaxY};
}

// Utility function - Interpolate varyings
inline void interpolateVaryings(
    Varyings &output,
    const std::array<Varyings, 3> &v,
    const Vec3f &barycentric,
    const Vec4f &weights,
    float fragmentDepth)
{
    const Vec3f w = weights.xyz();
    const float correction = weights.w;

    auto interpolate = [&](const auto &v0, const auto &v1, const auto &v2)
    {
        return (v0 * w.x * barycentric.x +
                v1 * w.y * barycentric.y +
                v2 * w.z * barycentric.z) *
               correction;
    };

    output.position = interpolate(v[0].position, v[1].position, v[2].position);
    output.texCoord = interpolate(v[0].texCoord, v[1].texCoord, v[2].texCoord);
    output.color = interpolate(v[0].color, v[1].color, v[2].color);
    output.normal = normalize(interpolate(v[0].normal, v[1].normal, v[2].normal));
    output.tangent = interpolate(v[0].tangent, v[1].tangent, v[2].tangent);
    output.tangent.w = v[0].tangent.w;
    output.depth = fragmentDepth;

    if (v[0].positionLightSpace.w != 0)
    {
        output.positionLightSpace = interpolate(
            v[0].positionLightSpace,
            v[1].positionLightSpace,
            v[2].positionLightSpace);
    }
}

// Initialize vertex attributes (moved from inline function to reduce redundancy)
inline void initVertexAttributes(VertexAttributes &attributes, const Vertex &vertex)
{
    attributes.position = vertex.position;
    attributes.normal = vertex.normal;
    attributes.tangent = vertex.tangent;
    attributes.texCoord = vertex.texCoord;
    attributes.color = vertex.color;
}

// Calculate fragment depth
inline float calculateFragmentDepth(
    const Vec3f &barycentric,
    const Vec4f &weights,
    const std::array<ProcessedVertex, 3> &vertices)
{
    return (barycentric.x * vertices[0].screenPosition.z * weights.x +
            barycentric.y * vertices[1].screenPosition.z * weights.y +
            barycentric.z * vertices[2].screenPosition.z * weights.z) *
           weights.w;
}

// Common fragment processing logic (new helper function to reduce redundancy)
inline FragmentOutput processFragment(
    const Varyings &interpolatedVaryings,
    std::shared_ptr<IShader> shader)
{
    return shader->fragmentShader(interpolatedVaryings);
}

// Constructor
Renderer::Renderer(int width, int height)
    : frameBuffer(std::make_unique<FrameBuffer>(width, height)),
      modelMatrix(Matrix4x4f::identity()),
      viewMatrix(Matrix4x4f::identity()),
      projMatrix(Matrix4x4f::identity()),
      shader(nullptr),
      msaaEnabled(false)
{
    light = Light(Vec3f(0.0f, 0.0f, -1.0f), Vec3f(1.0f), 1.0f, 0.2f);
    omp_set_num_threads(4);
}

void Renderer::enableMSAA(bool enable)
{
    msaaEnabled = enable;
    frameBuffer->enableMSAA(enable);
}

void Renderer::clear(const Vec4f &color)
{
    frameBuffer->clear(color);
}

Vec3f Renderer::screenMapping(const Vec3f &ndcPos)
{
    return Vec3f(
        (ndcPos.x + 1.0f) * 0.5f * frameBuffer->getWidth(),
        (1.0f - ndcPos.y) * 0.5f * frameBuffer->getHeight(),
        ndcPos.z);
}

void Renderer::processTriangleVertices(
    const Triangle &triangle,
    std::shared_ptr<IShader> shader,
    std::array<ProcessedVertex, 3> &vertices)
{
    VertexAttributes attributes;

    for (int i = 0; i < 3; ++i)
    {
        initVertexAttributes(attributes, triangle.vertices[i]);
        const Vec4f &clipPos = shader->vertexShader(attributes, vertices[i].varying);
        vertices[i].clipPosition = clipPos;

        const float invW = 1.0f / clipPos.w;
        vertices[i].screenPosition = screenMapping(Vec3f(
            clipPos.x * invW,
            clipPos.y * invW,
            clipPos.z * invW));
    }
}

// Process pixel in both standard and MSAA modes (new function to reduce redundancy)
inline bool processPixel(
    float x, float y,
    const std::array<ProcessedVertex, 3> &verts,
    std::shared_ptr<IShader> shader,
    FrameBuffer *frameBuffer,
    bool isMSAA,
    int sampleIndex = -1)
{
    // Calculate barycentric coordinates
    const Vec3f barycentric = computeBarycentric2D(x, y,{verts[0].screenPosition, verts[1].screenPosition, verts[2].screenPosition});

    if (!isInsideTriangle(barycentric))return false;

    // Calculate perspective weights
    const auto weights = calculatePerspectiveWeights(barycentric, verts);
    
    // Calculate depth value
    const float depth = calculateFragmentDepth(barycentric, weights, verts);
    
    // Depth test
    bool depthTestPassed;
    if (isMSAA)
        depthTestPassed = frameBuffer->msaaDepthTest(x, y, sampleIndex, depth);
    else
        depthTestPassed = frameBuffer->depthTest(x, y, depth);
        
    if (!depthTestPassed) return false;
    
    // Calculate interpolated vertex attributes
    Varyings interpolatedVaryings;
    interpolateVaryings(
        interpolatedVaryings,
        {verts[0].varying, verts[1].varying, verts[2].varying},
        barycentric, weights, depth);

    // Execute fragment shader
    const FragmentOutput output = processFragment(interpolatedVaryings, shader);
    
    // Skip discarded fragments
    if (output.discard) return false;
        
    // Write output
    if (isMSAA)
        frameBuffer->accumulateMSAAColor(x, y, sampleIndex, depth, output.color);
    else
        frameBuffer->setPixel(x, y, depth, output.color);
        
    return true;
}



void Renderer::rasterizeStandardMode(
    int x, int y,
    const std::array<ProcessedVertex, 3> &verts,
    std::shared_ptr<IShader> shader)
{
    const float pixelCenterX = x + 0.5f;
    const float pixelCenterY = y + 0.5f;
    
    processPixel(pixelCenterX, pixelCenterY, verts, shader, frameBuffer.get(), false);
}

void Renderer::rasterizeMSAAMode(
    int x, int y,
    const std::array<ProcessedVertex, 3> &verts,
    std::shared_ptr<IShader> shader)
{
    bool shaderExecuted = false;
    FragmentOutput cachedOutput;
    Varyings cachedVaryings;

    for (int s = 0; s < MSAA_SAMPLES; ++s)
    {
        float sampleX = x + MSAA_OFFSETS[s].x;
        float sampleY = y + MSAA_OFFSETS[s].y;
        
        processPixel(sampleX, sampleY, verts, shader, frameBuffer.get(), true, s);
    }
}

void Renderer::rasterizeTriangle(const Triangle &triangle, std::shared_ptr<IShader> shader)
{
    if (!shader)
    {
        std::cerr << "Error: No valid shader provided for triangle rendering.\n";
        return;
    }

    std::array<ProcessedVertex, 3> verts;
    processTriangleVertices(triangle, shader, verts);

    // Perform backface culling
    // Note: The sign is determined by the projection matrix
    float sign =  (projMatrix.m11 < 0)? 1.0f : -1.0f;
    if (faceCull(verts, sign)) return;
    
    // Calculate bounding box
    // Note: The bounding box is calculated in screen space
    auto [minX, minY, maxX, maxY] = calculateBoundingBox(
        {verts[0].screenPosition, verts[1].screenPosition, verts[2].screenPosition},
        frameBuffer->getWidth(), frameBuffer->getHeight());

    // Rasterization loop
    if (msaaEnabled)
    {
        // #pragma omp parallel for collapse(2) if(maxX - minX > 32)
        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                rasterizeMSAAMode(x, y, verts, shader);
            }
        }
    }
    else
    {
        // #pragma omp parallel for collapse(2) if(maxX - minX > 32)
        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                rasterizeStandardMode(x, y, verts, shader);
            }
        }
    }
}

std::shared_ptr<Texture> Renderer::createShadowMap(int width, int height)
{
    shadowFrameBuffer = std::make_unique<FrameBuffer>(width, height);
    shadowFrameBuffer->clear(Vec4f(1.0f), 1.0f);

    auto shadowMap = textures::createTexture(width, height, TextureFormat::R32_FLOAT, TextureAccess::READ_WRITE);
    this->shadowMap = shadowMap;

    return shadowMap;
}

void Renderer::shadowPass(const std::vector<std::pair<std::shared_ptr<Mesh>, Matrix4x4f>> &shadowCasters)
{
    if (!shadowFrameBuffer || !shadowMap)
    {
        std::cerr << "Cannot render shadow map: shadow buffer or texture not initialized" << std::endl;
        return;
    }

    // Save current rendering state
    auto originalMsaaEnabled = msaaEnabled;
    auto originalFrameBuffer = std::move(frameBuffer);

    // Switch to shadow rendering state
    msaaEnabled = false;
    frameBuffer = std::move(shadowFrameBuffer);

    clear(Vec4f(1.0f));

    auto shadowShader = createShadowMapShader();

    ShaderUniforms uniforms;
    uniforms.viewMatrix = getViewMatrix();
    uniforms.projMatrix = getProjMatrix();
    uniforms.lightSpaceMatrix = uniforms.projMatrix * uniforms.viewMatrix;

    for (const auto &[mesh, modelMatrix] : shadowCasters)
    {
        uniforms.modelMatrix = modelMatrix;
        shadowShader->setUniforms(uniforms);

        for (const auto &triangle : mesh->getTriangles())
        {
            rasterizeTriangle(triangle, shadowShader);
        }
    }

    // Copy shadow frame buffer to shadow map texture
    const int width = frameBuffer->getWidth();
    const int height = frameBuffer->getHeight();

    // #pragma omp parallel for collapse(2)
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float depth = frameBuffer->getDepth(x, y);
            shadowMap->write(x, y, Vec4f(depth));
        }
    }

    // Restore original rendering state
    shadowFrameBuffer = std::move(frameBuffer);
    frameBuffer = std::move(originalFrameBuffer);
    msaaEnabled = originalMsaaEnabled;
}

void Renderer::drawMeshPass(const std::shared_ptr<Mesh> &mesh, std::shared_ptr<IShader> activeShader)
{
    if (!mesh || !activeShader)
    {
        if (!activeShader)
        {
            std::cerr << "Error: No shader set for material, cannot render mesh." << std::endl;
        }
        return;
    }

    // Directly render all triangles
    for (const Triangle &triangle : mesh->getTriangles())
    {
        rasterizeTriangle(triangle, activeShader);
    }
}