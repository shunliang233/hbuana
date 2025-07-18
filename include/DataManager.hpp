#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>

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

public:
    DataManager(const std::string &file_in, const std::string &dir_out, const bool auto_gain = 0, const bool cherenkov = 0);
    ~DataManager();

    /**
     * @brief 读取下一个数据块
     * @return 如果成功读取到数据块，返回 true；如果文件结束或读取失败，返回 false
     */
    bool next();

private:
    static constexpr std::size_t CHUNK_SIZE = 4096;  // Size of each read chunk
    static constexpr std::size_t WINDOW_SIZE = 8; // Size of the processing window
    std::ifstream m_fin;                             // Input file stream
    std::vector<uint8_t> m_buffer;                   // 当前窗口的内容
    std::vector<uint8_t> m_tail;                     // 上一块的尾部
    std::size_t m_offset = 0;                        // 当前窗口在文件中的起始偏移
    bool m_file_end = false;                         // 标记文件是否已结束
};