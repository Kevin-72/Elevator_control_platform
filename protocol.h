#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

#include "widget.h"

using namespace std;

#define HEARTBEATTIMESET 10000     // 心跳间隔时间
#define RESPONSETIMEOUTTIMESET  200      // 响应超时时间设置
#define offset_BASE             4
#define offset_OFF_ON           0
#define offset_ACCESS_SELECT    1
#define offset_MAXCHANNEL       2
#define offset_CHANNEL          4
#define offset_POSITION_CONTROL 6
#define offset_A_F_SELECT       7

// 协议字段定义
const uint16_t FRAME_HEADER = 0x55AA;  // 帧头
const uint8_t VERSION = 0x00;          // 默认版本

// 消息类型定义
enum CommandType {
    HEARTBEAT = 0x00,
    QUERY_STATUS = 0x08,
    DEVICE_CONTROL = 0x06,
    MCU_RESPONSE = 0x07
};

// 协议帧结构体
class ProtocolFrame {
public:
    uint16_t frameHeader;              // 帧头
    uint8_t version;                   // 版本号
    uint8_t command;                   // 指令
    uint16_t dataLength;               // 数据长度
    std::vector<uint8_t> data;         // 数据内容
    uint8_t checksum;                  // 校验和

    // 构造函数
    ProtocolFrame(uint8_t cmd, const std::vector<uint8_t>& dataPayload);

    // 计算校验和
    uint8_t calculateChecksum(const std::vector<uint8_t>& data) const;

    // 将帧序列化为字节流
    std::vector<uint8_t> serialize(bool withChecksum = true) const;

    // 反序列化字节流，解析响应
    static ProtocolFrame deserialize(const std::vector<uint8_t>& rawData);


};


enum class DPType : uint8_t {
    OFF_ON = 0x14,
    ACCESS_SELECT = 0x15,
    MAXCHANNEL = 0x65,
    CHANNEL = 0x66,
    POSITION_CONTROL = 0x67,
    A_F_SELECT = 0x68,
    ALL_STATUS = 0x69
};


// 定义 数据类型 枚举
enum class DataType : uint8_t {
    TYPE_01 = 0x01,
    TYPE_02 = 0x02,
    TYPE_04 = 0x04
};

// 创建 DPType 到 DataType 的映射表
static const std::unordered_map<DPType, DataType> DPTypeToDataTypeMap = {
    {DPType::OFF_ON, DataType::TYPE_01},
    {DPType::ACCESS_SELECT, DataType::TYPE_04},
    {DPType::MAXCHANNEL, DataType::TYPE_02},
    {DPType::CHANNEL, DataType::TYPE_02},
    {DPType::POSITION_CONTROL, DataType::TYPE_04},
    {DPType::A_F_SELECT, DataType::TYPE_01},
    {DPType::ALL_STATUS, DataType::TYPE_02}
};


// 定义开关值
enum class SwitchValue : uint8_t {
    SWITCH_OFF = 0x00,
    SWITCH_ON  = 0x01,
};
static const std::unordered_map<SwitchValue, QString> SwitchValueMap = {
    {SwitchValue::SWITCH_OFF, "OFF"},
    {SwitchValue::SWITCH_ON, "ON"}
};


// 定义AD类型通道值
enum class ADAccessValue : uint8_t {
    ADACCESS_A = 0x00,
    ADACCESS_B = 0x01,
    ADACCESS_C = 0x02,
    ADACCESS_D = 0x03
};
static const std::unordered_map<ADAccessValue, QString> ADAccessValueMap = {
    {ADAccessValue::ADACCESS_A, "A"},
    {ADAccessValue::ADACCESS_B, "B"},
    {ADAccessValue::ADACCESS_C, "C"},
    {ADAccessValue::ADACCESS_D, "D"}
};

// 定义F类型通道值
enum class FAccessValue : uint8_t {
    FACCESS_F0 = 0x00,
    FACCESS_F1 = 0x01,
    FACCESS_F2 = 0x02,
    FACCESS_F3 = 0x03,
    FACCESS_F4 = 0x04,
    FACCESS_F5 = 0x05,
    FACCESS_F6 = 0x06,
    FACCESS_F7 = 0x07,
    FACCESS_F8 = 0x08,
    FACCESS_F9 = 0x09
};
static const std::unordered_map<FAccessValue, QString> FAccessValueMap = {
    {FAccessValue::FACCESS_F0, "F0"},
    {FAccessValue::FACCESS_F1, "F1"},
    {FAccessValue::FACCESS_F2, "F2"},
    {FAccessValue::FACCESS_F3, "F3"},
    {FAccessValue::FACCESS_F4, "F4"},
    {FAccessValue::FACCESS_F5, "F5"},
    {FAccessValue::FACCESS_F6, "F6"},
    {FAccessValue::FACCESS_F7, "F7"},
    {FAccessValue::FACCESS_F8, "F8"},
    {FAccessValue::FACCESS_F9, "F9"}
};

// 定义通道字符对应的值
static const std::unordered_map<std::string, uint8_t> StringAccessValueMap = {
    {"A", static_cast<uint8_t>(ADAccessValue::ADACCESS_A)},
    {"B", static_cast<uint8_t>(ADAccessValue::ADACCESS_B)},
    {"C", static_cast<uint8_t>(ADAccessValue::ADACCESS_C)},
    {"D", static_cast<uint8_t>(ADAccessValue::ADACCESS_D)},
    {"F0", static_cast<uint8_t>(FAccessValue::FACCESS_F0)},
    {"F1", static_cast<uint8_t>(FAccessValue::FACCESS_F1)},
    {"F2", static_cast<uint8_t>(FAccessValue::FACCESS_F2)},
    {"F3", static_cast<uint8_t>(FAccessValue::FACCESS_F3)},
    {"F4", static_cast<uint8_t>(FAccessValue::FACCESS_F4)},
    {"F5", static_cast<uint8_t>(FAccessValue::FACCESS_F5)},
    {"F6", static_cast<uint8_t>(FAccessValue::FACCESS_F6)},
    {"F7", static_cast<uint8_t>(FAccessValue::FACCESS_F7)},
    {"F8", static_cast<uint8_t>(FAccessValue::FACCESS_F8)},
    {"F9", static_cast<uint8_t>(FAccessValue::FACCESS_F9)}
};

// 定义设备控制功能值
enum class DevCtrlValue : uint8_t {
    DevCtrl_UP = 0x00,
    DevCtrl_STOP = 0x01,
    DevCtrl_DOWN = 0x02
};
static const std::unordered_map<DevCtrlValue, QString> DevCtrlValueMap = {
    {DevCtrlValue::DevCtrl_UP, "UP"},
    {DevCtrlValue::DevCtrl_STOP, "STOP"},
    {DevCtrlValue::DevCtrl_DOWN, "DOWN"}
};

// 定义A/F通道类型值
enum class AFSelectValue : uint8_t {
    AFSelect_A = 0x00,
    AFSelect_F = 0x01
};
static const std::unordered_map<AFSelectValue, std::string> AFSelectValueMap = {
    {AFSelectValue::AFSelect_A, "A"},
    {AFSelectValue::AFSelect_F, "F"}
};






// 心跳检测构造
ProtocolFrame createHeartbeatFrame();

// 查询状态构造
ProtocolFrame createQueryStatusFrame();

// 设备控制构造
ProtocolFrame createDeviceControlFrame(DPType dpId, const std::vector<uint8_t>& commandValue);



#endif
