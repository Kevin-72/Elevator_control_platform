#include "widget.h"
#include "ui_widget.h"

// 响应等待超时处理槽函数
void Widget::onResponseTimeout() {
    if (waitingForResponse) {
        appendLog("Error: Response timeout.", Qt::red);
    }
    if (waitingForHeartbeat)
    {
        appendLog("Error: 等待心跳超时，禁止操作面板，直至心跳恢复！请检查模组连接是否出现异常。", Qt::red);
        setEnabledMy(false);    
    }
    responseTimeoutTimer->stop();  // 停止超时定时器
    isReceiving = false;  // 重置接收标志
    waitingForHeartbeat = false;
    waitingForResponse = false;  // 重置等待标志
}

// 发送数据时加锁，确保在接收操作时禁止发送
void Widget::sendSerialData(const QByteArray &data) {
    if (waitingForResponse) {
        appendLog("Error: Previous operation still waiting for response.", Qt::red);
        appendLog("发送数据失败！！", Qt::red);
        return;
    }
    // 设置标志，表明正在等待响应
    waitingForResponse = true;

    QMutexLocker locker(&serialMutex); // 加锁

    if (serialPort->isOpen() && serialPort->isWritable()) {
        serialPort->write(data);
        serialPort->waitForBytesWritten();
        appendLog("发送数据完成");
    } else {
        appendLog("Error: Serial port not open or writable.", Qt::red);
        waitingForResponse = false;  // 重置等待标志
        waitingForHeartbeat = false; // 重置心跳等待
        return;
    }

    // 启动响应超时定时器
    responseTimeoutTimer->start(RESPONSETIMEOUTTIMESET);  // 设置超时为 200 毫秒
}

// 模拟发送协议帧
void Widget::sendFrame(const ProtocolFrame& frame) {
    std::vector<uint8_t> serialized = frame.serialize();

    // 将序列化后的数据转换为 QByteArray
    QByteArray byteArrayData(reinterpret_cast<const char*>(serialized.data()), serialized.size());

    // 显示发送的帧内容
    QString logMessage = "Sending Frame: ";
    for (auto byte : serialized) {
        logMessage += QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper();  // 将字节格式化为两位十六进制
    }
    appendLog(logMessage);

    // 调用 sendSerialData 发送数据
    sendSerialData(byteArrayData);


    // // 模拟接收下位机响应
    // std::vector<uint8_t> responseData = {
    //     0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, 0x00,  // 第一帧
    //     0x55, 0xAA, 0x00, 0x03, 0x00, 0x02, 0x01,  // 第二帧
    //     0x55, 0xAA, 0x00, 0x06, 0x00, 0x01, 0x01, 0x01  // 第三帧
    // };
    // receiveFrames(responseData);
//    std::vector<uint8_t> responseData = {
//        0x55, 0xAA, 0x03, 0x07, 0x00, 0x05, 0x14, 0x01, 0x00, 0x01, 0x00, 0x24,
//        0x55, 0xAA, 0x03, 0x07, 0x00, 0x05, 0x67, 0x04, 0x00, 0x01, 0x02, 0x7C
//    };
//    receiveFrames(responseData);
}

void Widget::on_upBt_pressed()
{
    // serialPort->write("CMD=UP\r\n");
    std::vector<uint8_t> data = {static_cast<uint8_t>(DevCtrlValue::DevCtrl_UP)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::POSITION_CONTROL, data);
    appendLog("发送上升指令");
    sendFrame(dataFrame); 
}

void Widget::on_downBt_pressed()
{
    // serialPort->write("CMD=DOWN\r\n");
    std::vector<uint8_t> data = {static_cast<uint8_t>(DevCtrlValue::DevCtrl_DOWN)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::POSITION_CONTROL, data);
    appendLog("发送下降指令");
    sendFrame(dataFrame); 
}

void Widget::on_stopBt_clicked()
{
    // serialPort->write("CMD=STOP\r\n");
    std::vector<uint8_t> data = {static_cast<uint8_t>(DevCtrlValue::DevCtrl_STOP)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::POSITION_CONTROL, data);
    appendLog("发送停止指令");
    sendFrame(dataFrame); 
}


void Widget::on_mode01Bt_clicked()
{
     serialPort->write("MODE=01\r\n");
    appendLog("启动模式1");
}

void Widget::on_mode02Bt_clicked()
{
     serialPort->write("MODE=02\r\n");
    appendLog("启动模式2");
}

void Widget::on_mode03Bt_clicked()
{
     serialPort->write("MODE=03\r\n");
    appendLog("启动模式3");
}

void Widget::on_mode04Bt_clicked()
{
     serialPort->write("MODE=04\r\n");
    appendLog("启动模式4");
}

void Widget::on_mode05Bt_clicked()
{
     serialPort->write("MODE=05\r\n");
    appendLog("启动模式5");
}

void Widget::on_openBt_clicked()
{
    std::vector<uint8_t> data = {static_cast<uint8_t>(SwitchValue::SWITCH_ON)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::OFF_ON, data);  
    appendLog("发送open");
    sendFrame(dataFrame); 
}

void Widget::on_closeBt_clicked()
{
    std::vector<uint8_t> data = {static_cast<uint8_t>(SwitchValue::SWITCH_OFF)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::OFF_ON, data);  
    appendLog("发送close");
    sendFrame(dataFrame); 
}

void Widget::on_queryCb_clicked()
{
    ProtocolFrame queryStatusFrame = createQueryStatusFrame();
    appendLog("查询状态");
    sendFrame(queryStatusFrame);
}

void Widget::on_sendCb_clicked()
{
    std::vector<uint8_t> maxChannelData(2);
    std::vector<uint8_t> channelData(2);
    if(!ui->maxChannelSetCb->text().isEmpty() && !ui->ChannelSetCb->text().isEmpty())
    {
        int maxChannelNumber = ui->maxChannelSetCb->text().toInt();
        maxChannelData[0] = (maxChannelNumber >> 8) & 0xFF;  // 高字节
        maxChannelData[1] = maxChannelNumber & 0xFF;         // 低字节

        int channelNumber = ui->ChannelSetCb->text().toInt();
        channelData[0] = (channelNumber >> 8) & 0xFF;       // 高字节
        channelData[1] = channelNumber & 0xFF;              // 低字节

        if (maxChannelNumber < channelNumber)
        {
            QMessageBox::critical(this, "错误提示", "频道设置不能超过最大频道值\r\n请重新设置！！！");
            appendLog("发送失败，请重新设置！", Qt::red);
            return;
        }
    }
    else
    {
        QMessageBox::critical(this, "错误提示", "最大频道设置与频道不能为空\r\n请重新设置！！！");
        appendLog("发送失败，请重新设置！", Qt::red);
    }

    // 发送通道值
    std::vector<uint8_t> accessData;
    auto it = StringAccessValueMap.find(ui->channelsCb->currentText().toStdString());
    if (it != StringAccessValueMap.end()) {
        accessData.push_back(it->second);
    } else {
        QMessageBox::critical(this, "错误提示", "Invalid access channel string.\r\n");
        appendLog("Error: Invalid access channel string.", Qt::red);
        return; // 或者执行其他错误处理逻辑
    }
    ProtocolFrame accessDataFrame = createDeviceControlFrame(DPType::ACCESS_SELECT, accessData);  
    appendLog("发送通道值");
    sendFrame(accessDataFrame); 

//    QThread::msleep(1000);
//    appendLog("等待 1s");

//     // 发送最大频道值
    ProtocolFrame maxChannelDataFrame = createDeviceControlFrame(DPType::MAXCHANNEL, maxChannelData);
    sendFrame(maxChannelDataFrame);

//    QThread::msleep(1000);
//    appendLog("等待 1s");

//    // 发送频道值
    ProtocolFrame channelDataFrame = createDeviceControlFrame(DPType::CHANNEL, channelData);
    sendFrame(channelDataFrame);
}
