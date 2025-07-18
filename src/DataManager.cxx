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