#include "widget.h"
#include "ui_widget.h"
#include "protocol.h"

// 串口数据读取函数
void Widget::readSerialData()
{
    if (isReceiving) {
        appendLog("当前正在接收数据....禁止重复接收", Qt::red);
        return;  // 如果当前正在接收数据，跳过
    }

    QMutexLocker locker(&serialMutex);  // 上锁，防止同时发送

    isReceiving = true;  // 设置接收标志为 true，表示正在接收

    QByteArray receivedData = serialPort->readAll();  // 读取数据

//    responseTimeoutTimer->stop();  // 停止超时定时器
    isReceiving = false;  // 接收完成，重置接收标志
    waitingForHeartbeat = false;
    waitingForResponse = false;  // 重置等待标志

    // 处理接收到的帧数据
    std::vector<uint8_t> buffer(receivedData.begin(), receivedData.end());
    receiveFrames(buffer);
}


// 接收并解析多个下位机响应
void Widget::receiveFrames(std::vector<uint8_t>& buffer) {
    while (buffer.size() >= 7) {  // 至少需要7个字节（帧头 + 版本 + 命令 + 数据长度）
        // 查找帧头
        size_t headerPos = 0;
        while (headerPos + 1 < buffer.size() && (buffer[headerPos] != 0x55 || buffer[headerPos + 1] != 0xAA)) {
            headerPos++;
        }
        
        if (headerPos + 1 >= buffer.size()) {
            appendLog("Error: Incomplete or invalid frame, unable to find frame header.", Qt::red);
            break;  // 没有找到有效的帧头，退出处理
        }

        // 获取数据长度
        uint16_t dataLength = (buffer[headerPos + 4] << 8) | buffer[headerPos + 5];
        size_t totalFrameSize = 6 + dataLength + 1;  // 帧头 + 数据 + 校验和

        // 检查缓冲区是否包含完整的一帧
        if (buffer.size() < headerPos + totalFrameSize) {
            appendLog("Error: Waiting for more data to complete the frame.", Qt::red);
            break;  // 数据不完整，等待更多数据
        }

        // 提取完整帧并解析
        std::vector<uint8_t> frameData(buffer.begin() + headerPos, buffer.begin() + headerPos + totalFrameSize);
        try {
            ProtocolFrame responseFrame = ProtocolFrame::deserialize(frameData);

            // 输出接收到的帧内容
            QString logMessage = "Received Frame: ";
            for (auto byte : frameData) {
                logMessage += QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper();  // 将字节格式化为两位十六进制
            }
            appendLog(logMessage, Qt::blue);

            // 接收帧校验和
            std::vector<uint8_t> frameData_tmp(frameData.begin(), frameData.end() - 1);
            uint8_t checksum = 0x00;
            for (auto byte : frameData_tmp) {
                checksum += byte;
            }
            uint8_t calculatedChecksum = checksum & 0xff;
            uint8_t receivedChecksum = frameData.back();
            if (calculatedChecksum != receivedChecksum) {
                appendLog("校验和错误！！", Qt::red);
                appendLog("计算值：");
                appendLog(QString::number(calculatedChecksum));
                appendLog("原有值：");
                appendLog(QString::number(receivedChecksum));
                break; //校验和失败
            }

            // 根据响应的命令字解析并处理
            switch (responseFrame.command) {
                case HEARTBEAT:
                    appendLog("Heartbeat successfully.", Qt::green);
                    heartbeatHandle(responseFrame.data);
                    break;
                case MCU_RESPONSE:
                    appendLog("Response successfully.", Qt::green);
                    accessRev = false;      // 接收数据处理过程，禁止发送通道数据
                    receiveHandle(responseFrame.data);
                    accessRev = true;
                    break;
                default:
                    appendLog("Unknown command in response.", Qt::red);
                    break;
            }

            // 删除已处理的帧
            buffer.erase(buffer.begin(), buffer.begin() + headerPos + totalFrameSize);
        } catch (const std::exception& e) {
            appendLog("Error parsing received frame: ", Qt::red);
            break;
        }

    }
}

void Widget::heartbeatHandle(std::vector<uint8_t>& data)
{
    if (data.size() < 1) {
        appendLog("heartbeat data size is too small to process.", Qt::red);
        return;
    }

//    if (data[0] == 0x00) {
//        appendLog("接收到首次心跳响应");
//    } else if (data[0] == 0x01) {
//        appendLog("接收到正常心跳响应");
//    } else {
//        appendLog("接收的心跳数据错误");
//    }

    // 心跳响应正常时恢复操作
    setEnabledMy(true); 
}


void Widget::receiveHandle(std::vector<uint8_t>& data)
{
    if (data.size() < 5) {
        appendLog("receive data size is too small to process.", Qt::red);
        return;
    }

    uint8_t DP_ID = data[0];
    switch (static_cast<DPType>(DP_ID)) {
        case DPType::OFF_ON: {
            uint8_t func_val = data[offset_BASE];
            handle_OFF_ON(func_val);
            break;
        }

        case DPType::ACCESS_SELECT: {
            uint8_t func_val = data[offset_BASE];
            handle_ACCESS_SELECT(func_val);
            break;
        }

        case DPType::MAXCHANNEL: {
            if (data.size() < 6) {
                appendLog("Data size is insufficient for MAXCHANNEL.", Qt::red);
                return;
            }
            uint16_t max_channel_value = (data[offset_BASE] << 8) | data[offset_BASE + 1];
            handle_MAXCHANNEL(max_channel_value);
            break;
        }

        case DPType::CHANNEL: {
            if (data.size() < 6) {
                appendLog("Data size is insufficient for CHANNEL.", Qt::red);
                return;
            }
            uint16_t channel_value = (data[offset_BASE] << 8) | data[offset_BASE + 1];
            handle_CHANNEL(channel_value);
            break;
        }

        case DPType::POSITION_CONTROL: {
            uint8_t func_val = data[offset_BASE];
            handle_POSITION_CONTROL(func_val);
            break;
        }

        case DPType::A_F_SELECT: {
            uint8_t func_val = data[offset_BASE];
            handle_A_F_SELECT(func_val);
            break;
        }
            
        case DPType::ALL_STATUS: {
            appendLog("处理所有状态");
            if (data.size() < 0x0c) {
                appendLog("Data size is insufficient for CHANNEL.", Qt::red);
                return;
            }
            uint16_t max_channel_value = (data[offset_BASE + offset_MAXCHANNEL] << 8) | data[offset_BASE + offset_MAXCHANNEL + 1];
            uint16_t channel_value = (data[offset_BASE + offset_CHANNEL] << 8) | data[offset_BASE + offset_CHANNEL + 1];

            handle_A_F_SELECT(data[offset_BASE + offset_A_F_SELECT]);   // 必须先确定下位机类型，然后再处理其他状态
            handle_OFF_ON(data[offset_BASE + offset_OFF_ON]);
            handle_ACCESS_SELECT(data[offset_BASE + offset_ACCESS_SELECT]);
            handle_MAXCHANNEL(max_channel_value);
            handle_CHANNEL(channel_value);
            handle_POSITION_CONTROL(data[offset_BASE + offset_POSITION_CONTROL]);
            break;
        }
            
        default:
            appendLog("Unknown command in response.", Qt::red);
            break;
    }
}


void Widget::handle_OFF_ON(uint8_t func_val)
{
    auto it = SwitchValueMap.find(static_cast<SwitchValue>(func_val));
    ui->label_switch_value->setText(it != SwitchValueMap.end() ? it->second : "Unknown");
    if (static_cast<SwitchValue>(func_val) == SwitchValue::SWITCH_OFF) {
        ui->openBt->setText("开启");
        switchStatus = true;
    }
    else {
        ui->openBt->setText("关闭");
        switchStatus = false;
    }

}

void Widget::handle_ACCESS_SELECT(uint8_t func_val)
{
    if (A_F_Flag == static_cast<uint8_t>(AFSelectValue::AFSelect_A)) {
        auto it = ADAccessValueMap.find(static_cast<ADAccessValue>(func_val));
        ui->label_access_value->setText(it != ADAccessValueMap.end() ? it->second : "Unknown");
        ui->channelsCb->setCurrentText(it != ADAccessValueMap.end() ? it->second : "Unknown");
    } 
    else if (A_F_Flag == static_cast<uint8_t>(AFSelectValue::AFSelect_F)) {
        auto it = FAccessValueMap.find(static_cast<FAccessValue>(func_val));
        ui->label_access_value->setText(it != FAccessValueMap.end() ? it->second : "Unknown");
        ui->channelsCb->setCurrentText(it != FAccessValueMap.end() ? it->second : "Unknown");
    }
}

void Widget::handle_MAXCHANNEL(uint16_t func_val)
{
    ui->label_max_channel_value->setText(QString::number(func_val));
    maxChannelNumber = func_val;
}

void Widget::handle_CHANNEL(uint16_t func_val)
{
    ui->label_channel_value->setText(QString::number(func_val));
}

void Widget::handle_POSITION_CONTROL(uint8_t func_val)
{
    auto it = DevCtrlValueMap.find(static_cast<DevCtrlValue>(func_val));
    ui->label_device_value->setText(it != DevCtrlValueMap.end() ? it->second : "Unknown");
}

void Widget::handle_A_F_SELECT(uint8_t func_val)
{
    A_F_Flag = func_val;
    ui->channelsCb->clear();
    if (A_F_Flag == static_cast<uint8_t>(AFSelectValue::AFSelect_A)) {
        for (const auto &pair : ADAccessValueMap) {
            ui->channelsCb->addItem(pair.second);
        }
    } 
    else if (A_F_Flag == static_cast<uint8_t>(AFSelectValue::AFSelect_F)) {
        for (const auto &pair : FAccessValueMap) {
            ui->channelsCb->addItem(pair.second);
        }
    }
    else
    {
        appendLog("AF通道选择数据异常，修改失败....................", Qt::red);
    }
}

