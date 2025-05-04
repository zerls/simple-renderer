#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <mutex>

/**
 * 性能分析器 - 用于测量和记录各个渲染步骤的执行时间
 */
class Profiler {
public:
    // 单例模式获取实例
    static Profiler& getInstance() {
        static Profiler instance;
        return instance;
    }

    // 开始测量一个指定名称的任务
    void startProfile(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex);
        auto& record = records[name];
        record.starts.push_back(std::chrono::high_resolution_clock::now());
    }

    // 结束对指定名称任务的测量
    void endProfile(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex);
        auto now = std::chrono::high_resolution_clock::now();
        auto& record = records[name];
        
        if (!record.starts.empty()) {
            auto start = record.starts.back();
            record.starts.pop_back();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
            record.totalTime += duration;
            record.callCount++;
            
            if (duration < record.minTime || record.minTime == 0)
                record.minTime = duration;
                
            if (duration > record.maxTime)
                record.maxTime = duration;
        }
    }

    // 重置所有性能数据
    void reset() {
        std::lock_guard<std::mutex> lock(mutex);
        records.clear();
    }

    // 输出性能报告
    void printReport() const {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "\n===== 性能分析报告 =====\n";
        std::cout << std::left << std::setw(30) << "任务名称" 
                  << std::setw(12) << "调用次数" 
                  << std::setw(12) << "总时间(ms)" 
                  << std::setw(12) << "平均时间(ms)"
                  << std::setw(12) << "最小时间(ms)" 
                  << std::setw(12) << "最大时间(ms)" << std::endl;
        
        std::cout << std::string(90, '-') << std::endl;
        
        for (const auto& pair : records) {
            const auto& name = pair.first;
            const auto& record = pair.second;
            
            if (record.callCount > 0) {
                double avgTime = static_cast<double>(record.totalTime) / record.callCount / 1000.0;
                double totalTimeMs = record.totalTime / 1000.0;
                double minTimeMs = record.minTime / 1000.0;
                double maxTimeMs = record.maxTime / 1000.0;
                
                std::cout << std::left << std::setw(30) << name 
                          << std::setw(12) << record.callCount
                          << std::fixed << std::setprecision(3)
                          << std::setw(12) << totalTimeMs
                          << std::setw(12) << avgTime 
                          << std::setw(12) << minTimeMs
                          << std::setw(12) << maxTimeMs << std::endl;
            }
        }
        
        std::cout << "========================\n";
    }

private:
    Profiler() = default;
    ~Profiler() = default;
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    // 性能记录结构
    struct ProfileRecord {
        std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> starts;
        uint64_t totalTime = 0;  // 微秒
        uint64_t callCount = 0;
        uint64_t minTime = 0;    // 微秒
        uint64_t maxTime = 0;    // 微秒
    };

    std::unordered_map<std::string, ProfileRecord> records;
    mutable std::mutex mutex;
};

// 便于使用的辅助宏
#define PROFILE_BEGIN(name) Profiler::getInstance().startProfile(name)
#define PROFILE_END(name) Profiler::getInstance().endProfile(name)
#define PROFILE_RESET() Profiler::getInstance().reset()
#define PROFILE_REPORT() Profiler::getInstance().printReport()

// 自动测量作用域内代码执行时间的RAII类
class ScopedProfiler {
public:
    explicit ScopedProfiler(const std::string& name) : name(name) {
        PROFILE_BEGIN(name);
    }
    
    ~ScopedProfiler() {
        PROFILE_END(name);
    }
    
private:
    std::string name;
};

// 便于使用的作用域分析宏
#define PROFILE_SCOPE(name) ScopedProfiler scopedProfiler##__LINE__(name)
