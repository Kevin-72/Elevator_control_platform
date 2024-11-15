#include "widget.h"
#include "ui_widget.h"


// 构造函数实现
ProtocolFrame::ProtocolFrame(uint8_t cmd, const std::vector<uint8_t>& dataPayload)
    : frameHeader(FRAME_HEADER), version(VERSION), command(cmd), dataLength(dataPayload.size()), data(dataPayload) {
    checksum = calculateChecksum(serialize(false)); // 计算校验和
}

// 计算校验和函数实现
uint8_t ProtocolFrame::calculateChecksum(const std::vector<uint8_t>& data) const {
    uint8_t checksum = 0x00;
    for (auto byte : data) {
        checksum += byte;
    }
    return checksum & 0xff;   // 对256取余
}

// 序列化函数实现
std::vector<uint8_t> ProtocolFrame::serialize(bool withChecksum) const {
    std::vector<uint8_t> frame;
    frame.push_back((frameHeader >> 8) & 0xFF); // 帧头的高字节
    frame.push_back(frameHeader & 0xFF);         // 帧头的低字节
    frame.push_back(version);                    // 版本号
    frame.push_back(command);                    // 命令
    frame.push_back((dataLength >> 8) & 0xFF);   // 数据长度的高字节
    frame.push_back(dataLength & 0xFF);          // 数据长度的低字节
    frame.insert(frame.end(), data.begin(), data.end()); // 插入数据
    if (withChecksum) {
        frame.push_back(checksum);               // 插入校验和
    }
    return frame;
}


// 反序列化字节流，解析响应
ProtocolFrame ProtocolFrame::deserialize(const std::vector<uint8_t>& rawData) {
    if (rawData.size() < 7) {
        throw std::invalid_argument("Invalid frame size");
    }

    uint16_t frameHeader = (rawData[0] << 8) | rawData[1];
    uint8_t version = rawData[2];
    uint8_t command = rawData[3];
    uint16_t dataLength = (rawData[4] << 8) | rawData[5];
    std::vector<uint8_t> data(rawData.begin() + 6, rawData.begin() + 6 + dataLength);
    uint8_t checksum = rawData[6 + dataLength];

    ProtocolFrame frame(command, data);
    frame.frameHeader = frameHeader;
    frame.version = version;
    frame.dataLength = dataLength;
    frame.checksum = checksum;

    return frame;
}






// 心跳检测构造
ProtocolFrame createHeartbeatFrame() {
    std::vector<uint8_t> data = {};  // 初始接收0x00，后续0x01
    return ProtocolFrame(HEARTBEAT, data);
}

// 查询状态构造
ProtocolFrame createQueryStatusFrame() {
    std::vector<uint8_t> data = {};
    return ProtocolFrame(QUERY_STATUS, data);
}

// 设备控制构造
ProtocolFrame createDeviceControlFrame(DPType dpId, const std::vector<uint8_t>& commandValue) {
    std::vector<uint8_t> data;
    // 确保 dpId 对应的 DataType 存在
    auto it = DPTypeToDataTypeMap.find(dpId);
    if (it == DPTypeToDataTypeMap.end()) {
        throw std::invalid_argument("Invalid DPType: no corresponding DataType found.");
    }

    // 构造数据
    data.push_back(static_cast<uint8_t>(dpId));           // DP ID
    data.push_back(static_cast<uint8_t>(it->second));                           // 数据类型
    uint16_t commandLength = static_cast<uint16_t>(commandValue.size());
    data.push_back((commandLength >> 8) & 0xFF);          // 功能长度的高字节
    data.push_back(commandLength & 0xFF);                 // 功能长度的低字节
    data.insert(data.end(), commandValue.begin(), commandValue.end()); // 插入功能指令数据

    return ProtocolFrame(DEVICE_CONTROL, data);
}
