#pragma once
#include <vector>
#include <cstdint>

// 事件信息结构体
struct EventInfo
{
    std::size_t head_pos = 0; // 事件头部位置，默认为0
    std::size_t foot_pos = 0; // 事件尾部位置，默认为0
    bool is_abnormal = false; // 是否异常事件，默认为正常
};

// Event 类
class Event
{
private:
    EventInfo info;                 // 事件信息
    std::vector<std::uint8_t> data; // 事件数据
};