#pragma once

#include <stdio.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <array>

#include <TFile.h>
#include <TTree.h>
#include <TMath.h>

using namespace std;

class DatManager
{
private:
	// 1. Constants for layers, chips, channels.
	static const int Layer_No = 40;
	static const int chip_No = 9;
	static const int channel_No = 36;

	// 2. Constants for event - using constexpr for compile-time optimization
	static constexpr std::array<unsigned char, 4> s_event_head = {0xfb, 0xee, 0xfb, 0xee};
	static constexpr std::array<unsigned char, 4> s_event_foot = {0xfe, 0xdd, 0xfe, 0xdd};
	static constexpr size_t s_event_head_size = s_event_head.size();
	static constexpr size_t s_event_foot_size = s_event_foot.size();
	static constexpr size_t s_event_head_overlap = s_event_head_size - 1;
	static constexpr size_t s_event_foot_overlap = s_event_foot_size - 1;
	static constexpr size_t s_least_event_size = s_event_head_size + s_event_foot_size;

	// 3. Buffers to hold data read from file
	static constexpr size_t s_read_size = 40960;					// Size of temp_buffer to read from file
	static constexpr size_t s_buffer_size = 1000000;				// Size of m_buffer to hold data
	static constexpr size_t s_half_buffer_size = s_buffer_size / 2; // Half of the s_buffer_size
	vector<unsigned char> m_buffer;									// Buffer for read data
	size_t m_buffer_start = 0;									// Start position of valid data in m_buffer

	vector<int> m_event_v; // Vector for 1 event data
	vector<int> _buffer_v; // Buffer for 1 SPIROC data

public:
	static const int channel_FEE = 73; //(36charges+36times + BCIDs )*16column+ ChipID
	string outname = "";
	int _Run_No;
	int _cycleID;
	int _triggerID;
	unsigned int _Event_Time;
	vector<int> _chip_v[Layer_No][chip_No];
	vector<int> _cellID;
	vector<int> _bcid;
	vector<int> _hitTag;
	vector<int> _gainTag_tdc;
	vector<int> _gainTag;
	vector<int> _cherenkov;
	vector<double> _HG_Charge;
	vector<double> _LG_Charge;
	vector<double> _Hit_Time;
	int count_chipbuffer = 0;

	DatManager() {};
	virtual ~DatManager();

	/**
	 * @brief 将原始二进制数据文件解码为物理分析所需的结构化数据，并保存为 ROOT 文件
	 * @param binary_name 原始二进制数据文件名
	 * @param raw_name 输出目录名
	 * @param b_auto_gain 是否自动增益调整（默认为 false）
	 * @param b_cherenkov 是否启用切伦科夫探测器（默认为 false）
	 * @return 返回解码结果状态（0 表示成功，非 0 表示失败）
	 */
	int Decode(const string &binary_name, const string &raw_name, const bool b_auto_gain = 0, const bool b_cherenkov = 0);

	/**
	 * @brief 从输入文件中捕获一个完整的事件包
	 * @param f_in 输入文件流
	 * @param buffer_v 存储捕获的事件数据
	 * @param cherenkov_counter 切伦科夫计数器
	 * @return 返回捕获事件包的状态（0 表示失败，1 表示成功）
	 */
	int CatchEventBag(ifstream &f_in, vector<int> &buffer_v, long &cherenkov_counter);

	/**
	 * @brief 从 event_v 中捕获一个 layer 包
	 * @param event_v 存储 event 数据的 vector
	 * @param buffer_v 存储捕获的 SPIROC 数据
	 * @param layer_id 层 ID
	 * @param cycleID 周期 ID
	 * @param triggerID 触发 ID
	 * @return 返回捕获 SPIROC 事件包的状态（0 表示失败，1 表示成功）
	 */
	int CatchSPIROCBag(vector<int> &event_v, vector<int> &buffer_v, int &layer_id, int &cycleID, int &triggerID);
	int CatchSPIROCBag(ifstream &f_in, vector<int> &buffer_v, int &layer_id, int &cycleID, int &triggerID);

	/**
	 * @brief 在 TTree 中设置分支，用于存储解码后的数据
	 * @param tree TTree 对象，用于存储解码后的数据
	 */
	void SetTreeBranch(TTree *tree);

	void BranchClear();
	int DecodeAEvent(vector<int> &chip_v, int layer_ID, int Memo_ID, const bool b_auto_gain);

	/**
	 * @brief 检查所有 chip 的缓冲区是否都为空
	 * @return 返回芯片缓冲区状态（0 表示都为空，1 表示存在缓冲区不为空的 chip）
	 */
	int Chipbuffer_empty()
	{
		int b_chipbuffer = 0;
		for (int i_layer = 0; i_layer < Layer_No; ++i_layer)
		{
			for (int i_chip = 0; i_chip < chip_No; ++i_chip)
			{
				if (_chip_v[i_layer][i_chip].size())
					b_chipbuffer = 1;
			}
		}
		return b_chipbuffer;
	}

	int FillChipBuffer(vector<int> &buffer_v, int cycleID, int triggerID, int layer_id);
};
