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
    // responseTimeoutTimer->stop();  // 停止超时定时器
    isReceiving = false;  // 重置接收标志
    waitingForHeartbeat = false;
    waitingForResponse = false;  // 重置等待标志
    if (selectSerial) {
        emit ui->openSerialBt->clicked();
        QMessageBox::critical(this, "错误提示", "串口选择错误！\r\n请选择正确的串口");
    }
}

// 发送数据时加锁，确保在接收操作时禁止发送
void Widget::sendSerialData(const QByteArray &data) {
    // 确保发送线程已经初始化
    if (!sendThread || !sendThread->isRunning()) {
        appendLog("Error: Send thread is not running!", Qt::red);
        return;  // 如果线程未初始化或未运行，则退出
    }

    // 将发送逻辑异步调用到发送线程中

    QMetaObject::invokeMethod(this, [this, data]() {
        if (waitingForResponse) {
            appendLog("Warning: 上一条指令正在等待响应.正在排队等待发送......", Qt::red);
            QMutexLocker locker(&sendMutex); // 加锁等待
            appendLog("Warning: 上一条指令响应已经结束.正在发送......", Qt::green);
        }
        // 创建局部事件循环，用于等待响应
        QEventLoop loop;
        // 启动定时器监控超时
        QTimer timeoutTimer;
        {
        QMutexLocker locker(&serialMutex);  // 确保线程安全

        // 显示发送的帧内容
        QString logMessage = "Sending Frame: ";
        for (auto byte : data) {
            logMessage += QString("%1 ").arg(static_cast<uint8_t>(byte), 2, 16, QChar('0')).toUpper();
        }
        appendLog(logMessage);

        // 检查串口状态
        if (!serialPort->isOpen() || !serialPort->isWritable()) {
            appendLog("Error: Serial port not open or writable.", Qt::red);
            return;
        }

        // 写入数据到串口
        serialPort->write(data);
        serialPort->waitForBytesWritten();
        appendLog("发送数据完成");



        // 连接信号，当接收到响应时退出事件循环
        connect(this, &Widget::responseReceivedSignal, &loop, &QEventLoop::quit);


        timeoutTimer.setSingleShot(true);
        timeoutTimer.setInterval(RESPONSETIMEOUTTIMESET);  // 超时时间
        connect(&timeoutTimer, &QTimer::timeout, &loop, [&]() {


            loop.quit();  // 超时时退出事件循环

        });

        timeoutTimer.start();  // 启动定时器
        }

        appendLog("等待响应中...", Qt::blue);

        // 阻塞当前线程，直到事件循环退出
        loop.exec();


        // 检查是否超时
        if (!timeoutTimer.isActive()) {
            appendLog("Warning: 响应超时，没有收到数据。", Qt::red);
            if (waitingForResponse) {
                appendLog("Error: Response timeout.", Qt::red);
            }
            if (waitingForHeartbeat)
            {
                appendLog("Error: 等待心跳超时，禁止操作面板，直至心跳恢复！请检查模组连接是否出现异常。", Qt::red);
                setEnabledMy(false);
            }
            // responseTimeoutTimer->stop();  // 停止超时定时器
            isReceiving = false;  // 重置接收标志
            waitingForHeartbeat = false;
            waitingForResponse = false;  // 重置等待标志
            if (selectSerial) {
                emit ui->openSerialBt->clicked();
                QMessageBox::critical(this, "错误提示", "串口选择错误！\r\n请选择正确的串口");
            }
        } else {
            appendLog("响应接收完成，继续发送下一条指令。", Qt::green);
        }

    }, Qt::QueuedConnection); // 使用队列连接，将方法调用放入线程事件循环
}




// 发送协议帧
void Widget::sendFrame(const ProtocolFrame& frame) {
    // 将序列化后的数据转换为 QByteArray
    QByteArray byteArrayData(reinterpret_cast<const char*>(frame.serialize().data()), frame.serialize().size());

    // 调用 sendSerialData 发送数据
    sendSerialData(byteArrayData);
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




void Widget::on_openBt_clicked()
{
    if (switchStatus) {
        std::vector<uint8_t> data = {static_cast<uint8_t>(SwitchValue::SWITCH_ON)};
        ProtocolFrame dataFrame = createDeviceControlFrame(DPType::OFF_ON, data);
        appendLog("发送open");
        sendFrame(dataFrame);
        ui->openBt->setText("关闭");
    }
    else {
        std::vector<uint8_t> data = {static_cast<uint8_t>(SwitchValue::SWITCH_OFF)};
        ProtocolFrame dataFrame = createDeviceControlFrame(DPType::OFF_ON, data);
        appendLog("发送close");
        sendFrame(dataFrame);
        ui->openBt->setText("开启");
    }

}

void Widget::on_queryCb_clicked()
{
    ProtocolFrame queryStatusFrame = createQueryStatusFrame();
    appendLog("查询状态");
    sendFrame(queryStatusFrame);
}


void Widget::on_channelsCb_currentIndexChanged(const QString &arg1)
{
    // accessRev为false时正在处理接收数据，此时禁止发送
    if (accessRev) {
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
    }

}



void Widget::on_maxChannelSetCb_returnPressed()
{
    std::vector<uint8_t> maxChannelData(2);
    if(!ui->maxChannelSetCb->text().isEmpty())
    {
        maxChannelNumber = ui->maxChannelSetCb->text().toInt();
        maxChannelData[0] = (maxChannelNumber >> 8) & 0xFF;  // 高字节
        maxChannelData[1] = maxChannelNumber & 0xFF;         // 低字节
    }
    else
    {
        QMessageBox::critical(this, "错误提示", "最大频道设置与频道不能为空\r\n请重新设置！！！");
        appendLog("发送失败，请重新设置！", Qt::red);
    }

     // 发送最大频道值
    appendLog("发送最大频道值");
    ui->maxChannelSetCb->clear();
    ProtocolFrame maxChannelDataFrame = createDeviceControlFrame(DPType::MAXCHANNEL, maxChannelData);
    sendFrame(maxChannelDataFrame);
}


void Widget::on_ChannelSetCb_returnPressed()
{
    std::vector<uint8_t> channelData(2);
    if (!ui->ChannelSetCb->text().isEmpty()) {
        channelNumber = ui->ChannelSetCb->text().toInt();
        channelData[0] = (channelNumber >> 8) & 0xFF;       // 高字节
        channelData[1] = channelNumber & 0xFF;              // 低字节
    }

    if (maxChannelNumber < channelNumber)
    {
        QMessageBox::critical(this, "错误提示", QString("频道设置不能超过最大频道值%1\r\n请重新设置！！！").arg(maxChannelNumber));
        appendLog("发送失败，请重新设置！", Qt::red);
        return;
    }

    // 发送频道值
    appendLog("发送频道值");
    ui->ChannelSetCb->clear();
    ProtocolFrame channelDataFrame = createDeviceControlFrame(DPType::CHANNEL, channelData);
    sendFrame(channelDataFrame);
}

void Widget::sendReset()
{
    std::vector<uint8_t> allStatusData(8);

    // 1、开关状态
    allStatusData[offset_OFF_ON] = switchStatus
                                        ? static_cast<uint8_t>(SwitchValue::SWITCH_OFF)
                                        : static_cast<uint8_t>(SwitchValue::SWITCH_ON);

    // 2、通道: A/F0
    allStatusData[offset_ACCESS_SELECT] = 0x00;

    // 3、最大频道值
    allStatusData[offset_MAXCHANNEL] = (maxChannelNumber >> 8) & 0xFF;  // 高字节
    allStatusData[offset_MAXCHANNEL + 1] = maxChannelNumber & 0xFF;    // 低字节

    // 4、频道值: 99
    uint16_t valueChannel = 0x63;
    allStatusData[offset_CHANNEL] = (valueChannel >> 8) & 0xFF;  // 高字节
    allStatusData[offset_CHANNEL + 1] = valueChannel & 0xFF;     // 低字节

    // 5、设备控制: UP
    allStatusData[offset_POSITION_CONTROL] = static_cast<uint8_t>(DevCtrlValue::DevCtrl_UP);

    // 6、A/F状态
    allStatusData[offset_A_F_SELECT] = A_F_Flag;

    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::ALL_STATUS, allStatusData);
    appendLog("发送所有设备复位指令", Qt::blue);
    sendFrame(dataFrame);
}
