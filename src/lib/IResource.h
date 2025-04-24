// IResource.h
#pragma once

#include <string>
#include <memory>
#include <unordered_map>

// 资源类型枚举
enum class ResourceType {
    MESH,
    TEXTURE,
    SHADER,
    MATERIAL
    // 可以根据需要添加更多类型
};

// 资源接口
class IResource {
public:
    virtual ~IResource() = default;
    
    // 获取资源GUID
    const std::string& getGUID() const { return guid; }
    
    // 获取资源类型
    ResourceType getType() const { return type; }
    
    // 设置资源GUID
    void setGUID(const std::string& guid) { this->guid = guid; }
    
protected:
    IResource(ResourceType type, const std::string& guid = "") : type(type), guid(guid) {}
    
    ResourceType type;
    std::string guid;
};

// 资源管理器
class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;
    
    // 添加资源
    template <typename T>
    std::shared_ptr<T> addResource(std::shared_ptr<T> resource, const std::string& guid) {
        if (!resource) return nullptr;
        
        // 设置资源GUID
        resource->setGUID(guid);
        
        // 添加到资源映射
        resources[guid] = resource;
        
        return resource;
    }
    
    // 获取资源
    template <typename T>
    std::shared_ptr<T> getResource(const std::string& guid) {
        auto it = resources.find(guid);
        if (it != resources.end()) {
            return std::dynamic_pointer_cast<T>(it->second);
        }
        return nullptr;
    }
    
    // 检查资源是否存在
    bool hasResource(const std::string& guid) {
        return resources.find(guid) != resources.end();
    }
    
    // 移除资源
    void removeResource(const std::string& guid) {
        resources.erase(guid);
    }
    
    // 清空所有资源
    void clear() {
        resources.clear();
    }
    
private:
    std::unordered_map<std::string, std::shared_ptr<IResource>> resources;
};