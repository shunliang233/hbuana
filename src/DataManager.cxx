#include <vector>
#include "DataManager.hpp"

DataManager::DataManager(const std::string &file_in, const std::string &dir_out, const bool auto_gain, const bool cherenkov)
    : m_fin(file_in, std::ios::binary)
{
    if (!m_fin.is_open())
    {
        throw std::runtime_error("Failed to open file: " + file_in);
    }
    m_buffer.resize(WINDOW_SIZE);
    m_tail.resize(EVENT_FOOT_SIZE); // Initialize tail with the size of EVENT_FOOT
}

DataManager::~DataManager()
{
    if (m_fin.is_open())
        m_fin.close();
}

bool DataManager::next()
{
    if (m_file_end)
        return false;
    // 读取一块新数据
    std::vector<uint8_t> chunk(CHUNK_SIZE);
    m_fin.read(reinterpret_cast<char *>(chunk.data()), CHUNK_SIZE);
    std::streamsize bytesRead = m_fin.gcount();
    if (bytesRead <= 0)
    {
        m_file_end = true;
        m_buffer.clear();
        return false;
    }
    chunk.resize(bytesRead);

    // 拼接上一次的tail和新chunk形成当前window
    m_buffer.clear();
    m_buffer.insert(m_buffer.end(), m_tail.begin(), m_tail.end());
    m_buffer.insert(m_buffer.end(), chunk.begin(), chunk.end());

    // 更新下一次的tail
    if (chunk.size() >= WINDOW_SIZE)
    {
        m_tail.assign(chunk.end() - WINDOW_SIZE, chunk.end());
    }
    else
    {
        // chunk不足windowSize，则用上一次tail拼
        if (m_tail.size() + chunk.size() > WINDOW_SIZE)
        {
            m_tail.erase(m_tail.begin(), m_tail.begin() + (m_tail.size() + chunk.size() - WINDOW_SIZE));
        }
        m_tail.insert(m_tail.end(), chunk.begin(), chunk.end());
    }
    m_offset += bytesRead;
    // 如果文件已结束，下次next会返回false
    if (m_fin.eof())
        m_file_end = true;
    return true;
}

std::vector<DataManager::EventInfo> DataManager::scan_events()
{
    std::vector<EventInfo> events;
    m_fin.clear();
    m_fin.seekg(0, std::ios::beg);
    std::vector<uint8_t> file_data((std::istreambuf_iterator<char>(m_fin)), std::istreambuf_iterator<char>());
    std::size_t pos = 0;
    while (true)
    {
        // 找到 HEAD
        auto head_it = std::search(file_data.begin() + pos, file_data.end(), EVENT_HEAD.begin(), EVENT_HEAD.end());
        if (head_it == file_data.end())
            break;
        std::size_t head_pos = std::distance(file_data.begin(), head_it);

        // 找到下一个 FOOT
        auto foot_it = std::search(head_it + EVENT_HEAD_SIZE, file_data.end(), EVENT_FOOT.begin(), EVENT_FOOT.end());
        std::size_t foot_pos = (foot_it == file_data.end()) ? std::string::npos : std::distance(file_data.begin(), foot_it) + EVENT_FOOT_SIZE;

        // 检查异常：如果下一个 HEAD 在 FOOT 之前，说明 FOOT 丢失
        auto next_head_it = std::search(head_it + EVENT_HEAD_SIZE, file_data.end(), EVENT_HEAD.begin(), EVENT_HEAD.end());
        bool is_abnormal = false;
        if (foot_it == file_data.end() || (next_head_it != file_data.end() && std::distance(file_data.begin(), next_head_it) < std::distance(file_data.begin(), foot_it)))
        {
            is_abnormal = true;
            // 如果异常，事件结束位置设为下一个 HEAD 或文件末尾
            foot_pos = (next_head_it == file_data.end()) ? file_data.size() : std::distance(file_data.begin(), next_head_it);
        }

        events.push_back({head_pos, foot_pos, is_abnormal});
        pos = foot_pos;
        if (pos >= file_data.size())
            break;
    }
    return events;
}

std::vector<uint8_t> DataManager::get_event(const EventInfo &info)
{
    if (info.foot_pos == std::string::npos || info.head_pos >= info.foot_pos)
        return {};
    std::vector<uint8_t> result(m_buffer.begin() + info.head_pos, m_buffer.begin() + info.foot_pos);
    return result;
}