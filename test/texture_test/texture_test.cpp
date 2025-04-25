// test/texture_test/texture_test.cpp
#include <iostream>
#include <filesystem>
#include <cmath>
#include "lib/texture/texture.h"

// 测试R32_FLOAT深度值读写是否正确的函数
bool testDepthTextureReadWrite() {
    std::cout << "\n=== 测试R32_FLOAT深度值读写 ===" << std::endl;
    
    // 创建R32_FLOAT格式的深度纹理
    const int width = 64;
    const int height = 64;
    auto depthTexture = createTexture(width, height, TextureFormat::R32_FLOAT, TextureAccess::READ_WRITE);
    
    if (!depthTexture) {
        std::cerr << "创建R32_FLOAT纹理失败!" << std::endl;
        return false;
    }
    
    std::cout << "成功创建R32_FLOAT纹理: " << width << "x" << height << std::endl;
    
    // 写入一些精确的浮点深度值
    const float testValues[] = {
        0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 
        0.6f, 0.7f, 0.8f, 0.9f, 1.0f,
        0.123456f, 0.333333f, 0.666667f, 0.999999f,
        0.001f, 0.01f, 0.001f, 0.0001f
    };
    
    const int numTestValues = sizeof(testValues) / sizeof(float);
    int x = 0, y = 0;
    
    // 在纹理的不同位置写入测试值
    for (int i = 0; i < numTestValues; i++) {
        x = i % width;
        y = i / width;
        depthTexture->write(x, y, float4(testValues[i], 0.0f, 0.0f, 1.0f));
    }
    
    std::cout << "已写入" << numTestValues << "个测试深度值" << std::endl;
    
    // 直接读取并验证值是否正确
    bool allCorrect = true;
    float maxError = 0.0f;
    float errorThreshold = 0.0001f; // 允许的误差阈值
    
    for (int i = 0; i < numTestValues; i++) {
        x = i % width;
        y = i / width;
        float4 readValue = depthTexture->read(x, y);
        float error = std::abs(readValue.x - testValues[i]);
        maxError = std::max(maxError, error);
        
        if (error > errorThreshold) {
            std::cerr << "位置(" << x << "," << y << ")的深度值不匹配: "
                      << "期望=" << testValues[i] << ", 实际=" << readValue.x
                      << ", 误差=" << error << std::endl;
            allCorrect = false;
        }
    }
    
    if (allCorrect) {
        std::cout << "所有深度值读写测试通过! 最大误差: " << maxError << std::endl;
    } else {
        std::cerr << "深度值读写测试失败! 最大误差: " << maxError << std::endl;
    }
    
    // 测试保存和加载
    std::string rawDepthPath = "test_raw_depth.data";
    std::ofstream outFile(rawDepthPath, std::ios::binary);
    
    if (!outFile) {
        std::cerr << "无法创建原始深度数据文件" << std::endl;
        return false;
    }
    
    // 保存原始浮点数据
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float4 pixelValue = depthTexture->read(x, y);
            float depth = pixelValue.x;
            outFile.write(reinterpret_cast<char*>(&depth), sizeof(float));
        }
    }
    outFile.close();
    
    std::cout << "已保存原始深度数据到: " << rawDepthPath << std::endl;
    
    // 从原始数据重新加载
    std::ifstream inFile(rawDepthPath, std::ios::binary);
    if (!inFile) {
        std::cerr << "无法打开原始深度数据文件" << std::endl;
        return false;
    }
    
    auto loadedDepthTexture = createTexture(width, height, TextureFormat::R32_FLOAT, TextureAccess::READ_WRITE);
    if (!loadedDepthTexture) {
        std::cerr << "创建加载用的深度纹理失败" << std::endl;
        return false;
    }
    
    // 读取原始浮点数据
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float depth;
            inFile.read(reinterpret_cast<char*>(&depth), sizeof(float));
            loadedDepthTexture->write(x, y, float4(depth, 0.0f, 0.0f, 1.0f));
        }
    }
    inFile.close();
    
    std::cout << "已从原始数据加载深度纹理" << std::endl;
    
    // 验证加载的数据
    allCorrect = true;
    maxError = 0.0f;
    
    for (int i = 0; i < numTestValues; i++) {
        x = i % width;
        y = i / width;
        float4 readValue = loadedDepthTexture->read(x, y);
        float error = std::abs(readValue.x - testValues[i]);
        maxError = std::max(maxError, error);
        
        if (error > errorThreshold) {
            std::cerr << "加载后位置(" << x << "," << y << ")的深度值不匹配: "
                      << "期望=" << testValues[i] << ", 实际=" << readValue.x
                      << ", 误差=" << error << std::endl;
            allCorrect = false;
        }
    }
    
    if (allCorrect) {
        std::cout << "所有加载后的深度值测试通过! 最大误差: " << maxError << std::endl;
        return true;
    } else {
        std::cerr << "加载后的深度值测试失败! 最大误差: " << maxError << std::endl;
        return false;
    }
}

// 测试saveDepthToFile函数
bool testSaveDepthToFile() {
    std::cout << "\n=== 测试saveDepthToFile函数 ===" << std::endl;
    
    // 创建R32_FLOAT格式的深度纹理
    const int width = 64;
    const int height = 64;
    auto depthTexture = createTexture(width, height, TextureFormat::R32_FLOAT, TextureAccess::READ_WRITE);
    
    if (!depthTexture) {
        std::cerr << "创建R32_FLOAT纹理失败!" << std::endl;
        return false;
    }
    
    std::cout << "成功创建R32_FLOAT纹理: " << width << "x" << height << std::endl;
    
    // 写入一个深度渐变图案
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // 创建一个从左上角(0.0)到右下角(1.0)的渐变
            float depth = (float)(x + y) / (width + height - 2);
            depthTexture->write(x, y, float4(depth, 0.0f, 0.0f, 1.0f));
        }
    }
    
    std::cout << "已写入深度渐变图案" << std::endl;
    
    // 测试不同格式的保存
    std::vector<std::pair<std::string, TextureFileFormat>> testFormats = {
        {"test_depth_tga.tga", TextureFileFormat::TGA}
    };
    
    bool allSaved = true;
    
    // 测试普通保存
    for (const auto& [filename, format] : testFormats) {
        bool success = depthTexture->saveDepthToFile(filename, format);
        if (success) {
            std::cout << "成功保存深度图到: " << filename << std::endl;
        } else {
            std::cerr << "保存深度图到" << filename << "失败!" << std::endl;
            allSaved = false;
        }
    }
    
   
    
    return allSaved;
}

int main() {
    // 测试R32_FLOAT深度值读写
    if (testDepthTextureReadWrite()) {
        std::cout << "R32_FLOAT深度值读写测试成功!" << std::endl;
    } else {
        std::cerr << "R32_FLOAT深度值读写测试失败!" << std::endl;
        return 1;
    }
    
    // 测试saveDepthToFile函数
    if (testSaveDepthToFile()) {
        std::cout << "saveDepthToFile函数测试成功!" << std::endl;
    } else {
        std::cerr << "saveDepthToFile函数测试失败!" << std::endl;
        return 1;
    }
    
    std::cout << "\n所有测试完成!" << std::endl;
    return 0;
}