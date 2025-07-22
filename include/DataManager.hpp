#pragma once
#include <array>
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include "Event.hpp"

class DataManager
{
private:
    // 1. Hardware constants
    static const int LAYER_NO = 40;
    static const int CHIP_NO = 9;
    static const int CHANNEL_NO = 36;

    // 1. File header constants
    static constexpr std::array<std::uint8_t, 10> FILE_HEAD = {0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xbb, 0xaa, 0xaa};
    static constexpr std::size_t FILE_HEAD_SIZE = FILE_HEAD.size();

    // 2. Event constants - using constexpr for compile-time optimization
    static constexpr std::array<std::uint8_t, 4> EVENT_HEAD = {0xfb, 0xee, 0xfb, 0xee};
    static constexpr std::array<std::uint8_t, 4> EVENT_FOOT = {0xfe, 0xdd, 0xfe, 0xdd};
    static constexpr std::size_t EVENT_HEAD_SIZE = 4;
    static constexpr std::size_t EVENT_FOOT_SIZE = 4;
    static constexpr std::size_t EVENT_HEAD_OVERLAP = 3;
    static constexpr std::size_t EVENT_FOOT_OVERLAP = 3;

    // 3. Layer constants
    static constexpr std::array<std::uint8_t, 4> LAYER_HEAD = {0xfa, 0x5a, 0xfa, 0x5a};
    static constexpr std::array<std::uint8_t, 4> LAYER_FOOT = {0xfe, 0xdd, 0xfe, 0xdd};
    static constexpr std::size_t LAYER_HEAD_SIZE = 4;
    static constexpr std::size_t LAYER_FOOT_SIZE = 4;

    // 4. Size
    static constexpr std::size_t CHIP_SIZE = 2 * CHANNEL_NO + 2 * CHANNEL_NO + 2 + 2;
    static constexpr std::size_t LAYER_SIZE = LAYER_HEAD_SIZE +
                                              4 + 2 +
                                              CHIP_SIZE * CHIP_NO +
                                              LAYER_FOOT_SIZE + 2;
    static constexpr std::size_t EVENT_SIZE = EVENT_HEAD_SIZE +
                                              LAYER_SIZE * LAYER_NO + 4 +
                                              EVENT_FOOT_SIZE;

    // 5. Read file
    static constexpr std::size_t CHUNK_SIZE = 4096; // Size of each read chunk
    std::ifstream m_fin;                            // Input file stream

    // 6. Buffer and state
    std::vector<std::uint8_t> m_buffer; // 持续的缓冲区，存储未处理的数据
    std::size_t m_offset = 0;           // 文件中已读取的总字节数
    bool m_file_end = false;            // 标记文件是否已结束
    std::vector<std::uint8_t> m_event;  // 当前事件数据

private:
    /**
     * @brief 读取下一个数据块
     * @return 如果成功读取到数据块，返回 true; 如果文件结束或读取失败，返回 false
     */
    bool read();

    /**
     * @brief 顺序读取下一个完整事例
     * @return 如果成功读取到事例，返回 true; 如果读取失败，返回 false
     */
    bool get_next_event();

public:
    /**
     * @brief 构造函数，打开 file_in 文件
     * @param file_in 输入文件路径
     * @param dir_out 输出目录路径
     * @param auto_gain 是否自动增益调整（默认为 false）
     * @param cherenkov 是否启用切伦科夫探测器（默认为 false）
     */
    explicit DataManager(const std::string &file_in, const std::string &dir_out = ".", const bool auto_gain = false, const bool cherenkov = false);

    /**
     * @brief 析构函数，关闭文件
     */
    ~DataManager();

    /**
     * @brief 重置数据管理器到文件开头，清除所有缓冲区
     */
    void reset();

    /**
     * @brief 扫描整个文件，分割所有事件
     * @return 所有事件的信息列表
     */
    std::vector<EventInfo> scan_events();

    /**
     * @brief 从文件开头扫描所有事件
     * @return 所有事件的信息列表
     */
    std::vector<EventInfo> scan_all_events();

    /**
     * @brief 读取指定事件内容
     * @param info 事件信息
     * @return 事件的字节内容
     */
    std::vector<std::uint8_t> get_event(const EventInfo &info);

    /**
     * @brief 检查是否还有更多事件可读取
     * @return true 如果还有事件，false 如果已到文件末尾
     */
    bool has_more_events() const;
};