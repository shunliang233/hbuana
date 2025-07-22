#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include "DataManager.hpp"
#include "Event.hpp"

using std::string;
using std::uint8_t;
using std::vector;
using std::size_t;

DataManager::DataManager(const string &file_in, const string &dir_out, const bool auto_gain, const bool cherenkov)
    : m_fin(file_in, std::ios::binary)
{
    if (!m_fin.is_open())
    {
        throw std::runtime_error("Failed to open file: " + file_in);
    }
}

DataManager::~DataManager()
{
    if (m_fin.is_open())
        m_fin.close();
}

bool DataManager::read()
{
    // 1. If the file has ended, return false
    if (m_file_end)
        return false;

    // 2. Read a chunk of data
    vector<uint8_t> chunk(CHUNK_SIZE);
    m_fin.read(reinterpret_cast<char *>(chunk.data()), CHUNK_SIZE);
    std::streamsize bytesRead = m_fin.gcount();
    if (bytesRead <= 0) // No data read
    {
        m_file_end = true;
        return false;
    }
    chunk.resize(bytesRead);

    // 3. Add chunk to buffer
    m_buffer.insert(m_buffer.end(), chunk.begin(), chunk.end());
    m_offset += bytesRead;

    // NOTE: Never happen when bytesRead > 0, but just in case
    if (m_fin.eof())
        m_file_end = true;
    
    return true;
}

void DataManager::reset()
{
    // 重置文件流状态
    m_fin.clear();
    m_fin.seekg(0, std::ios::beg);
    
    // 重置标记
    m_offset = 0;
    m_file_end = false;
    
    // 清空缓冲区
    m_buffer.clear();
}

vector<EventInfo> DataManager::scan_events()
{
    vector<EventInfo> events;

    // 记录当前 buffer 在文件中的起始偏移
    // m_offset 是已读取的总字节数，m_buffer.size() 是缓冲区中的数据量
    size_t buffer_start_offset = m_offset - m_buffer.size();
    
    if (m_buffer.empty())
    {
        // 如果当前没有缓冲数据，从当前文件位置读取
        if (!read())
        {
            return events;  // 文件为空或无法读取
        }
        // 重新计算 buffer 的起始位置
        buffer_start_offset = m_offset - m_buffer.size();
    }

    while (!m_file_end)
    {
        // 如果当前 buffer 为空，读取数据
        if (m_buffer.empty())
        {
            if (!read())
            {
                break; // 无法读取更多数据
            }
            buffer_start_offset = m_offset - m_buffer.size();
        }

        // 在当前 buffer 中查找 EVENT_HEAD
        auto head_it = std::search(m_buffer.begin(), m_buffer.end(),
                                   EVENT_HEAD.begin(), EVENT_HEAD.end());

        if (head_it == m_buffer.end())
        {
            // 当前 buffer 中没有找到 HEAD，需要继续读取
            if (!read())
            {
                break; // 无法读取更多数据
            }
            // buffer 增大了，但起始位置不变
            continue;
        }

        // 找到了 HEAD，计算其在文件中的绝对位置
        std::size_t head_pos = buffer_start_offset + std::distance(m_buffer.begin(), head_it);

        // 从 HEAD 之后查找对应的 FOOT
        auto search_start = head_it + EVENT_HEAD_SIZE;
        auto foot_it = std::search(search_start, m_buffer.end(),
                                   EVENT_FOOT.begin(), EVENT_FOOT.end());

        // 检查是否在当前 buffer 中找到了完整的事件
        if (foot_it != m_buffer.end())
        {
            // 找到了完整的事件
            std::size_t foot_pos = buffer_start_offset + std::distance(m_buffer.begin(), foot_it) + EVENT_FOOT_SIZE;
            events.push_back({head_pos, foot_pos, false});

            // 从 FOOT 之后继续查找下一个事件
            std::size_t processed_bytes = std::distance(m_buffer.begin(), foot_it) + EVENT_FOOT_SIZE;
            buffer_start_offset += processed_bytes;  // 更新起始偏移
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + processed_bytes);  // 移除已处理的数据
        }
        else
        {
            // 在当前 buffer 中没有找到对应的 FOOT，需要读取更多数据
            // 但首先检查是否有下一个 HEAD（可能当前事件的 FOOT 丢失）
            auto next_head_it = std::search(search_start, m_buffer.end(),
                                            EVENT_HEAD.begin(), EVENT_HEAD.end());

            if (next_head_it != m_buffer.end())
            {
                // 找到了下一个 HEAD，说明当前事件的 FOOT 丢失
                std::size_t abnormal_foot_pos = buffer_start_offset + std::distance(m_buffer.begin(), next_head_it);
                events.push_back({head_pos, abnormal_foot_pos, true}); // 标记为异常事件

                // 从下一个 HEAD 开始继续处理
                std::size_t processed_bytes = std::distance(m_buffer.begin(), next_head_it);
                buffer_start_offset += processed_bytes;  // 更新起始偏移
                m_buffer.erase(m_buffer.begin(), m_buffer.begin() + processed_bytes);  // 移除已处理的数据
            }
            else
            {
                // 没有找到下一个 HEAD，需要读取更多数据
                if (!read())
                {
                    // 无法读取更多数据，当前事件不完整
                    events.push_back({head_pos, std::string::npos, true});
                    break;
                }
                // 继续循环，在扩展的 buffer 中继续查找
                // buffer_start_offset 保持不变，因为我们只是扩展了 buffer
            }
        }
    }

    return events;
}

vector<EventInfo> DataManager::scan_all_events()
{
    // 重置到文件开头，然后扫描所有事件
    reset();
    return scan_events();
}

vector<uint8_t> DataManager::get_event(const EventInfo &info)
{
    if (info.foot_pos == std::string::npos || info.head_pos >= info.foot_pos)
        return {};

    // 重置文件指针到事件开始位置
    m_fin.clear();
    m_fin.seekg(info.head_pos, std::ios::beg);

    // 计算事件大小
    std::size_t event_size = info.foot_pos - info.head_pos;

    // 读取事件数据
    std::vector<uint8_t> result(event_size);
    m_fin.read(reinterpret_cast<char *>(result.data()), event_size);

    // 检查是否成功读取了完整的数据
    std::streamsize bytes_read = m_fin.gcount();
    if (bytes_read != static_cast<std::streamsize>(event_size))
    {
        result.resize(bytes_read); // 调整到实际读取的大小
    }

    return result;
}

bool DataManager::get_next_event()
{
    while (!m_file_end)
    {
        // 如果当前 buffer 为空，读取数据
        if (m_buffer.empty())
        {
            if (!read())
            {
                m_event.clear();
                return false; // 无法读取更多数据
            }
        }

        // 在当前 buffer 中查找 EVENT_HEAD
        auto head_it = std::search(m_buffer.begin(), m_buffer.end(),
                                   EVENT_HEAD.begin(), EVENT_HEAD.end());

        if (head_it == m_buffer.end())
        {
            // 当前 buffer 中没有找到 HEAD，需要继续读取
            if (!read())
            {
                m_event.clear();
                return false; // 无法读取更多数据
            }
            continue;
        }

        // 找到了 HEAD，现在查找对应的 FOOT
        auto search_start = head_it + EVENT_HEAD_SIZE;
        auto foot_it = std::search(search_start, m_buffer.end(),
                                   EVENT_FOOT.begin(), EVENT_FOOT.end());

        // 检查是否在当前 buffer 中找到了完整的事件
        if (foot_it != m_buffer.end())
        {
            // 找到了完整的事件，提取事件数据
            auto event_end = foot_it + EVENT_FOOT_SIZE;
            m_event.assign(head_it, event_end);

            // 从 buffer 中移除已处理的数据
            m_buffer.erase(m_buffer.begin(), event_end);
            
            return true;
        }
        else
        {
            // 在当前 buffer 中没有找到对应的 FOOT
            // 检查是否有下一个 HEAD（可能当前事件的 FOOT 丢失）
            auto next_head_it = std::search(search_start, m_buffer.end(),
                                            EVENT_HEAD.begin(), EVENT_HEAD.end());

            if (next_head_it != m_buffer.end())
            {
                // 找到了下一个 HEAD，说明当前事件的 FOOT 丢失
                // 返回截断到下一个 HEAD 的数据
                m_event.assign(head_it, next_head_it);

                // 从下一个 HEAD 开始继续处理
                m_buffer.erase(m_buffer.begin(), next_head_it);
                
                return true;
            }
            else
            {
                // 没有找到下一个 HEAD，需要读取更多数据
                if (!read())
                {
                    // 无法读取更多数据，返回剩余的不完整事件
                    m_event.assign(head_it, m_buffer.end());
                    m_buffer.clear();
                    return !m_event.empty();
                }
                // 继续循环，在扩展的 buffer 中继续查找
            }
        }
    }

    m_event.clear();
    return false; // 文件结束，没有更多事件
}

bool DataManager::has_more_events() const
{
    return !m_file_end || !m_buffer.empty();
}