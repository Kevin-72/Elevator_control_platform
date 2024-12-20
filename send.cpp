#include "widget.h"
#include "ui_widget.h"

// 响应等待超时处理槽函数
void Widget::onResponseTimeout() {
    if (waitingForResponse) {
        waitingForResponse = false;  // 重置等待标志
        isReceiving = false;  // 重置接收标志
        appendLog("Error: Response timeout.", Qt::red);
    }
    if (waitingForHeartbeat)
    {
        appendLog("Error: 等待心跳超时，禁止操作面板，直至心跳恢复！请检查模组连接是否出现异常。", Qt::red);
        setEnabledMy(false);
        waitingForHeartbeat = false;
    }
    if (selectSerial) {
        emit ui->openSerialBt->clicked();
        QMessageBox::critical(this, "错误提示", "串口选择错误！\r\n请选择正确的串口");
    }
}

void Widget::sendNextCommand() {
    // 如果队列不为空且没有在发送，继续发送下一条指令
    if (!commandQueue.isEmpty() && !isSending) {
        auto command = commandQueue.dequeue();  // 获取队列中的指令（data 和 log）
        QByteArray data = command.first;
        QString str_log = command.second;
        isSending = true;  // 标记为正在发送

        // 将发送逻辑异步调用到发送线程中
        QMetaObject::invokeMethod(this, [this, data, str_log]() {
            // 显示发送的帧内容
            if (stopRequested) {
                appendLog(QString("%1 开始......").arg(str_log), Qt::blue);
            }
            else {
                appendLog(QString("%1 开始......").arg(str_log));
            }
            QString logMessage = "Sending Frame: ";
            for (auto byte : data) {
                logMessage += QString("%1 ").arg(static_cast<uint8_t>(byte), 2, 16, QChar('0')).toUpper();
            }
            appendLog(logMessage);

            // 函数内的发送逻辑
            auto sendData = [&]() {
                if (serialPort->isOpen() && serialPort->isWritable()) {
                    QMutexLocker locker(&serialMutex); // 加锁
                    serialPort->write(data);
                    serialPort->waitForBytesWritten();
                    appendLog("发送数据完成");
                    waitingForResponse = true; // 设置标志，表示正在等待响应
                } else {
                    appendLog("Error: Serial port not open or writable.", Qt::red);
                    waitingForResponse = false;
                }
            };

            // 创建一个标志来跟踪响应是否收到
            bool responseReceived = false;

            // 尝试最多三次发送数据
            for (int attempt = 1; attempt <= 3; ++attempt) {
                if (!waitingForResponse) {
                    sendData();  // 发送数据
                    responseTimeoutTimer->start(RESPONSETIMEOUTTIMESET);  // 启动响应超时定时器

                    // 通过一个局部变量来判断响应是否已收到
                    bool timeoutOccurred = false;
                    while (waitingForResponse && !timeoutOccurred) {
                        QCoreApplication::processEvents();  // 处理事件，避免阻塞

                        if (!waitingForResponse && responseTimeoutTimer->isActive()) {  // 如果收到响应
                            responseReceived = true;
                            responseTimeoutTimer->stop();  // 停止超时定时器
                            break;
                        }
                        if (!responseTimeoutTimer->isActive()) {
                            timeoutOccurred = true;
                            break;
                        }
                    }

                    if (responseReceived) {
                        appendLog(QString("%1 成功！").arg(str_log), Qt::green);
                        break;  // 如果收到响应，跳出循环
                    }
                }

                // 如果已经尝试了三次且仍未收到响应，则退出
                if (attempt == 3) {
                    appendLog("Error: 三次发送均未收到响应，跳过此指令.", Qt::red);
                    appendLog(QString("%1 超时！").arg(str_log), Qt::red);
                    waitingForResponse = false;
                    isSending = false;
                    return;
                }

                appendLog(QString("Warning: 第%1次发送未收到响应，重试...").arg(attempt), Qt::red);
            }

            appendLog("---------------", Qt::lightGray);

            // 发送完当前指令后，继续处理队列中的下一条指令
            isSending = false;
            sendNextCommand();  // 继续发送下一条指令

        }, Qt::QueuedConnection); // 使用队列连接，将方法调用放入线程事件循环
    }
}

// 发送数据时加锁，确保在接收操作时禁止发送
void Widget::sendSerialData(const QByteArray &data, const QString &str_log) {
    // 确保发送线程已经初始化
    if (!sendThread || !sendThread->isRunning()) {
        appendLog("Error: Send thread is not running!", Qt::red);
        return;  // 如果线程未初始化或未运行，则退出
    }

    // 将指令加入队列
    commandQueue.enqueue(qMakePair(data, str_log)); 

    // 如果当前没有正在发送的指令，则开始发送
    if (!isSending) {
        sendNextCommand();  // 开始发送下一条指令
    }
}

// 发送协议帧
void Widget::sendFrame(const ProtocolFrame& frame, const QString &str_log) {
    // 将序列化后的数据转换为 QByteArray
    QByteArray byteArrayData(reinterpret_cast<const char*>(frame.serialize().data()), frame.serialize().size());

    // 调用 sendSerialData 发送数据
    sendSerialData(byteArrayData, str_log);
}

void Widget::on_upBt_pressed()
{
    setBottonImage(ui->upBt, ":/icons/up.png");

    std::vector<uint8_t> data = {static_cast<uint8_t>(DevCtrlValue::DevCtrl_UP)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::POSITION_CONTROL, data);
    sendFrame(dataFrame, "发送上升指令");
}

void Widget::on_downBt_pressed()
{
    setBottonImage(ui->downBt, ":/icons/down.png");

    std::vector<uint8_t> data = {static_cast<uint8_t>(DevCtrlValue::DevCtrl_DOWN)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::POSITION_CONTROL, data);
    sendFrame(dataFrame, "发送下降指令");
}

void Widget::on_stopBt_clicked()
{
    setBottonImage(ui->stopBt, ":/icons/stop.png");

    std::vector<uint8_t> data = {static_cast<uint8_t>(DevCtrlValue::DevCtrl_STOP)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::POSITION_CONTROL, data);
    sendFrame(dataFrame, "发送停止指令");
}


void Widget::on_openBt_clicked()
{
    if (switchStatus) {
        std::vector<uint8_t> data = {static_cast<uint8_t>(SwitchValue::SWITCH_ON)};
        ProtocolFrame dataFrame = createDeviceControlFrame(DPType::OFF_ON, data);
        sendFrame(dataFrame, "发送open");
        // ui->openBt->setText("关闭");
        setBottonImage(ui->openBt, ":/icons/power_green.png");
    }
    else {
        std::vector<uint8_t> data = {static_cast<uint8_t>(SwitchValue::SWITCH_OFF)};
        ProtocolFrame dataFrame = createDeviceControlFrame(DPType::OFF_ON, data);
        appendLog("发送close");
        sendFrame(dataFrame, "发送close");
        // ui->openBt->setText("开启");
        setBottonImage(ui->openBt, ":/icons/power_red.png");
    }

}

void Widget::on_queryCb_clicked()
{
    ProtocolFrame queryStatusFrame = createQueryStatusFrame();
    sendFrame(queryStatusFrame, "查询状态");
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
        sendFrame(accessDataFrame, "发送通道值");
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
    ui->maxChannelSetCb->clear();
    ProtocolFrame maxChannelDataFrame = createDeviceControlFrame(DPType::MAXCHANNEL, maxChannelData);
    sendFrame(maxChannelDataFrame, "发送最大频道值");
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
    ui->ChannelSetCb->clear();
    ProtocolFrame channelDataFrame = createDeviceControlFrame(DPType::CHANNEL, channelData);
    sendFrame(channelDataFrame, "发送频道值");
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
    sendFrame(dataFrame, "发送所有设备复位指令");
    QEventLoop loop;
    QTimer::singleShot(RESPONSETIMEOUTTIMESET * 3, &loop, &QEventLoop::quit);
    loop.exec();
}
