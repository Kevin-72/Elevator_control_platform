#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget),
    serialPort(new QSerialPort(this)),
    receiverThread(new QThread(this)),
    isReceiving(false),
    heartbeatThread(new QThread(this)),
    heartbeatTimer(new QTimer(this)),
    responseTimeoutTimer(new QTimer(this))
{
    ui->setupUi(this);
    this->setWindowTitle("升降器控制平台");

    QStringList serialNamePort;

    ui->serialCb->clear();
    // 获取并遍历所有可用的串口
    scan_serial();

    connect(serialPort, &QSerialPort::readyRead, this, &Widget::readSerialData);

    setEnabledMy(false);

    // 连接响应超时定时器的 timeout 信号到 onResponseTimeout 槽函数
    connect(responseTimeoutTimer, &QTimer::timeout, this, &Widget::onResponseTimeout);
    // 设置响应超时定时器间隔
    responseTimeoutTimer->setSingleShot(true);  // 设置为单次定时器


    ui->maxChannelSetCb->setValidator(new QIntValidator(ui->maxChannelSetCb));
    ui->ChannelSetCb->setValidator(new QIntValidator(ui->ChannelSetCb));

}

Widget::~Widget()
{
    if (receiverThread->isRunning()) {
        receiverThread->quit();
        receiverThread->wait();
    }
    if (heartbeatThread->isRunning()) {
        heartbeatThread->quit();
        heartbeatThread->wait();
    }
    // 停止并删除定时器
    heartbeatTimer->stop();
    responseTimeoutTimer->stop();
    delete ui;
}

void Widget::appendLog(const QString &text) {
    // 获取当前时间
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    // 格式化消息，加上时间戳
    QString formattedMessage = QString("[%1] %2").arg(currentTime, text);
    // 将格式化后的消息追加到日志视图
    ui->logViewer->appendPlainText(formattedMessage);
}

// 发送数据时加锁，确保在接收操作时禁止发送
void Widget::sendSerialData(const QByteArray &data) {
    if (waitingForResponse) {
        appendLog("Error: Previous operation still waiting for response.");
        return;
    }
    // 设置标志，表明正在等待响应
    waitingForResponse = true;
    // 记录发送的数据
    lastSentData = data;

    QMutexLocker locker(&serialMutex); // 加锁

    if (serialPort->isOpen() && serialPort->isWritable()) {
        serialPort->write(data);
        serialPort->waitForBytesWritten();
        appendLog("发送数据成功");
    } else {
        appendLog("Error: Serial port not open or writable.");
        waitingForResponse = false;  // 重置等待标志
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

    // 调用 sendSerialData 发送数据
    sendSerialData(byteArrayData);

    // 显示发送的帧内容
    QString logMessage = "Sending Frame: ";
    for (auto byte : serialized) {
        logMessage += QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper();  // 将字节格式化为两位十六进制
    }
    appendLog(logMessage);

    // // 模拟接收下位机响应
    // std::vector<uint8_t> responseData = {
    //     0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, 0x00,  // 第一帧
    //     0x55, 0xAA, 0x00, 0x03, 0x00, 0x02, 0x01,  // 第二帧
    //     0x55, 0xAA, 0x00, 0x06, 0x00, 0x01, 0x01, 0x01  // 第三帧
    // };
    // receiveFrames(responseData);
    std::vector<uint8_t> responseData = {
        0x55, 0xAA, 0x03, 0x07, 0x00, 0x05, 0x14, 0x01, 0x00, 0x01, 0x00, 0x24,
        0x55, 0xAA, 0x03, 0x07, 0x00, 0x05, 0x67, 0x04, 0x00, 0x01, 0x02, 0x7C
    };
    receiveFrames(responseData);
}

// 串口数据读取函数
void Widget::readSerialData()
{
    if (isReceiving) {
        return;  // 如果当前正在接收数据，跳过
    }

    QMutexLocker locker(&serialMutex);  // 上锁，防止同时发送

    isReceiving = true;  // 设置接收标志为 true，表示正在接收

    QByteArray receivedData = serialPort->readAll();  // 读取数据
    appendLog("Received: " + receivedData);

    // 检查是否是期望的响应
    if (waitingForResponse) {
        if (receivedData == lastSentData) {
            appendLog("Response received successfully.");
            responseTimeoutTimer->stop();  // 停止超时定时器
        } else {
            appendLog("Error: Unexpected response.");
        }

        // 完成接收，重置等待标志
        waitingForResponse = false;
    }

    // 处理接收到的帧数据
    std::vector<uint8_t> buffer(receivedData.begin(), receivedData.end());
    receiveFrames(buffer);

    isReceiving = false;  // 接收完成，重置接收标志
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
            std::cout << "Error: Incomplete or invalid frame, unable to find frame header." << std::endl;
            break;  // 没有找到有效的帧头，退出处理
        }

        // 获取数据长度
        uint16_t dataLength = (buffer[headerPos + 4] << 8) | buffer[headerPos + 5];
        size_t totalFrameSize = 6 + dataLength + 1;  // 帧头 + 数据 + 校验和

        // 检查缓冲区是否包含完整的一帧
        if (buffer.size() < headerPos + totalFrameSize) {
            std::cout << "Waiting for more data to complete the frame." << std::endl;
            break;  // 数据不完整，等待更多数据
        }

        // 提取完整帧并解析
        std::vector<uint8_t> frameData(buffer.begin() + headerPos, buffer.begin() + headerPos + totalFrameSize);
        try {
            ProtocolFrame responseFrame = ProtocolFrame::deserialize(frameData);

            // 输出接收到的帧内容
            std::cout << "Received Frame: ";
            for (auto byte : frameData) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
            }
            std::cout << std::endl;
            // 接收帧校验和
            std::vector<uint8_t> frameData_tmp(frameData.begin(), frameData.end() - 1);
            uint8_t checksum = 0x00;
            for (auto byte : frameData_tmp) {
                checksum += byte;
            }
            uint8_t calculatedChecksum = checksum & 0xff;
            uint8_t receivedChecksum = frameData.back();
            if (calculatedChecksum != receivedChecksum) {
                appendLog("计算值：");
                appendLog(QString::number(calculatedChecksum));
                appendLog("原有值：");
                appendLog(QString::number(receivedChecksum));
                break; //校验和失败
            }
            // 根据响应的命令字解析并处理
            switch (responseFrame.command) {
                case HEARTBEAT:
                    std::cout << "Received Heartbeat response. Data: ";
                    for (auto byte : responseFrame.data) {
                        std::cout << std::hex << (int)byte << " ";
                    }
                    std::cout << std::endl;
                    break;
                case MCU_RESPONSE:
                    std::cout << "Received Query Status response." << std::endl;
                    receiveHandle(responseFrame.data);
                    break;
                default:
                    std::cout << "Unknown command in response." << std::endl;
                    break;
            }

            // 删除已处理的帧
            buffer.erase(buffer.begin(), buffer.begin() + headerPos + totalFrameSize);
        } catch (const std::exception& e) {
            std::cout << "Error parsing received frame: " << e.what() << std::endl;
            break;
        }
    }
}

void Widget::receiveHandle(std::vector<uint8_t>& data)
{
    uint8_t DP_ID = data[0];
//    uint16_t func_len = (data[2] << 8) | data[3];

    if (DP_ID == static_cast<uint8_t>(DPType::MAXCHANNEL) | DP_ID == static_cast<uint8_t>(DPType::CHANNEL)) {
        uint32_t channel_val = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
        if (DP_ID == static_cast<uint8_t>(DPType::MAXCHANNEL)) {
            ui->label_max_channel_value->setText(QString::number(channel_val));
        }
        else {
            ui->label_channel_value->setText(QString::number(channel_val));
        }
    }
    else {
        uint8_t func_val = data[4];
        if (DP_ID == static_cast<uint8_t>(DPType::OFF_ON))
        {
            ui->label_switch_value->setText(SwitchStatusValueMap.find(func_val)->second);
        }
        else if (DP_ID == static_cast<uint8_t>(DPType::ACCESS_SELECT))
        {
            if (A_F_Flag != 0x11)
            {
                ui->label_access_value->setText(SwitchStatusValueMap.find(func_val)->second);
            }
        }
        else if (DP_ID == static_cast<uint8_t>(DPType::POSITION_CONTROL))
        {
            ui->label_device_value->setText(DevCtrlStatusValueMap.find(func_val)->second);
        }
        else if (DP_ID == static_cast<uint8_t>(DPType::A_F_SELECT))
        {
            A_F_Flag = func_val;
        }
    }
}

void Widget::scan_serial()
{
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        // 格式化串口名称和额外信息
        if (info.description().contains("serial", Qt::CaseInsensitive)) {
            QString portDetail = QString("%1 %2").arg(info.portName(), info.description());
            ui->serialCb->addItem(portDetail, info.portName());
        }
        else {
            ui->serialCb->addItem(info.portName());
        }
    }

    appendLog("查找串口成功");

    ui->channelsCb->addItem("A");
    ui->channelsCb->addItem("B");
    ui->channelsCb->addItem("C");
    ui->channelsCb->addItem("D");
}

void Widget::setEnabledMy(bool flag)
{
    ui->baundrateCb->setEnabled(!flag);
    ui->btnSerialCheck->setEnabled(!flag);
    ui->serialCb->setEnabled(!flag);

    ui->label_1->setEnabled(!flag);
    ui->label_2->setEnabled(!flag);
    ui->label_3->setEnabled(flag);
    ui->label_4->setEnabled(flag);
    ui->label_5->setEnabled(flag);

    ui->openBt->setEnabled(flag);
    ui->closeBt->setEnabled(flag);
    ui->upBt->setEnabled(flag);
    ui->downBt->setEnabled(flag);
    ui->stopBt->setEnabled(flag);
    ui->mode01Bt->setEnabled(flag);
    ui->mode02Bt->setEnabled(flag);
    ui->mode03Bt->setEnabled(flag);
    ui->mode04Bt->setEnabled(flag);
    ui->mode05Bt->setEnabled(flag);
    ui->channelsCb->setEnabled(flag);
    ui->maxChannelSetCb->setEnabled(flag);
    ui->ChannelSetCb->setEnabled(flag);
    ui->sendCb->setEnabled(flag);
    ui->queryCb->setEnabled(flag);
}

void Widget::startHeartbeatThread()
{
    // 将定时器设置为定期触发
    heartbeatTimer->setInterval(8000); // 每隔1秒发送一次心跳
    connect(heartbeatTimer, &QTimer::timeout, this, &Widget::sendHeartbeat);

    // 将定时器移到独立线程中
    heartbeatTimer->moveToThread(heartbeatThread);

    // 启动心跳线程
    heartbeatThread->start();

    // 启动定时器
    heartbeatTimer->start();
}

void Widget::sendHeartbeat()
{
    ProtocolFrame frame = createHeartbeatFrame();  // 创建心跳帧
    sendFrame(frame);  // 调用原有的 sendFrame 函数发送心跳帧
    appendLog("发送心跳帧");
}




/*打开串口*/
void Widget::on_openSerialBt_clicked()
{
    // 初始化串口属性，设置 端口号、波特率、数据位、停止位、奇偶校验位数
    QRegularExpression re("COM\\d+");
    serialPort->setPortName(re.match(ui->serialCb->currentText()).captured(0));
    serialPort->setBaudRate(ui->baundrateCb->currentText().toInt());
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setParity(QSerialPort::NoParity);

    // 根据初始化好的串口属性，打开串口
    // 如果打开成功，反转打开按钮显示和功能。打开失败，无变化，并且弹出错误对话框。
    if(ui->openSerialBt->text() == "打开串口"){
        if(serialPort->open(QIODevice::ReadWrite) == true){
            ui->openSerialBt->setText("关闭串口");
            // 让端口号下拉框不可选，避免误操作（选择功能不可用，控件背景为灰色）
            //  ui->serialCb->setEnabled(false);
            setEnabledMy(true);
            appendLog("串口打开成功");

            // 启动接收线程
            // isReceiving = true;
            receiverThread->start();

            // 启动心跳检测线程
            startHeartbeatThread();
        }else{
            QMessageBox::critical(this, "错误提示", "串口打开失败！！！\r\n该串口可能被占用\r\n请选择正确的串口");
            appendLog("串口打开失败");
        }
    }else{
        isReceiving = false;
        // 停止接收线程
        if (receiverThread->isRunning()) {
            receiverThread->quit();
            receiverThread->wait();
        }
        if (heartbeatThread->isRunning()) {
            heartbeatThread->quit();
            heartbeatThread->wait();
        }
        // 停止并删除定时器
        heartbeatTimer->stop();
        responseTimeoutTimer->stop();

        serialPort->close();
        ui->openSerialBt->setText("打开串口");
        // 端口号下拉框恢复可选，避免误操作
        // ui->serialCb->setEnabled(true);
        setEnabledMy(false);
        appendLog("串口关闭成功");
    }
}

//检测通讯端口槽函数
void Widget::on_btnSerialCheck_clicked()
{
    ui->serialCb->clear();
    //通过QSerialPortInfo查找可用串口
    scan_serial();
}

void Widget::on_upBt_pressed()
{
    // serialPort->write("CMD=UP\r\n");
    std::vector<uint8_t> data = {static_cast<uint8_t>(DevCtrlValue::DevCtrl_UP)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::POSITION_CONTROL, data);
    sendFrame(dataFrame); 
    appendLog("上升中......");
}

void Widget::on_downBt_pressed()
{
    // serialPort->write("CMD=DOWN\r\n");
    std::vector<uint8_t> data = {static_cast<uint8_t>(DevCtrlValue::DevCtrl_DOWN)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::POSITION_CONTROL, data);
    sendFrame(dataFrame); 
    appendLog("下降中......");
}

void Widget::on_stopBt_clicked()
{
    // serialPort->write("CMD=STOP\r\n");
    std::vector<uint8_t> data = {static_cast<uint8_t>(DevCtrlValue::DevCtrl_STOP)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::POSITION_CONTROL, data);
    sendFrame(dataFrame); 
    appendLog("停止运行");
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
    sendFrame(dataFrame); 
    appendLog("发送open");
}

void Widget::on_closeBt_clicked()
{
    std::vector<uint8_t> data = {static_cast<uint8_t>(SwitchValue::SWITCH_OFF)};
    ProtocolFrame dataFrame = createDeviceControlFrame(DPType::OFF_ON, data);  
    sendFrame(dataFrame); 
    appendLog("发送close");
}

void Widget::on_queryCb_clicked()
{
    ProtocolFrame queryStatusFrame = createQueryStatusFrame();
    sendFrame(queryStatusFrame);
    appendLog("查询状态");
}

void Widget::on_sendCb_clicked()
{
    std::vector<uint8_t> maxChannelData(4);
    std::vector<uint8_t> channelData(4);
    if(!ui->maxChannelSetCb->text().isEmpty() && !ui->ChannelSetCb->text().isEmpty())
    {
        int maxChannelNumber = ui->maxChannelSetCb->text().toInt();
        maxChannelData[0] = (maxChannelNumber >> 24) & 0xFF;  // 高字节
        maxChannelData[1] = (maxChannelNumber >> 16) & 0xFF;
        maxChannelData[2] = (maxChannelNumber >> 8) & 0xFF;
        maxChannelData[3] = maxChannelNumber & 0xFF;           // 低字节

        int channelNumber = ui->ChannelSetCb->text().toInt();
        channelData[0] = (channelNumber >> 24) & 0xFF;  // 高字节
        channelData[1] = (channelNumber >> 16) & 0xFF;
        channelData[2] = (channelNumber >> 8) & 0xFF;
        channelData[3] = channelNumber & 0xFF;           // 低字节

        if (maxChannelNumber < channelNumber)
        {
            QMessageBox::critical(this, "错误提示", "频道设置不能超过最大频道值\r\n请重新设置！！！");
            appendLog("发送失败，请重新设置！");
            return;
        }
    }
    else
    {
        QMessageBox::critical(this, "错误提示", "最大频道设置与频道不能为空\r\n请重新设置！！！");
        appendLog("发送失败，请重新设置！");
    }


    std::vector<uint8_t> accessData;
    auto it = StringAccessValueMap.find(ui->channelsCb->currentText().toStdString());
    if (it != StringAccessValueMap.end()) {
        accessData.push_back(it->second);
    } else {
        QMessageBox::critical(this, "错误提示", "Invalid access channel string.\r\n");
        appendLog("Error: Invalid access channel string.");
        return; // 或者执行其他错误处理逻辑
    }
    ProtocolFrame accessDataFrame = createDeviceControlFrame(DPType::ACCESS_SELECT, accessData);  // 发送通道值
    sendFrame(accessDataFrame); 

    ProtocolFrame maxChannelDataFrame = createDeviceControlFrame(DPType::MAXCHANNEL, maxChannelData);  // 发送最大频道值
    sendFrame(maxChannelDataFrame); 
    ProtocolFrame channelDataFrame = createDeviceControlFrame(DPType::CHANNEL, channelData);  // 发送频道值
    sendFrame(channelDataFrame); 
}

// 超时处理槽函数
void Widget::onResponseTimeout() {
    if (waitingForResponse) {
        appendLog("Error: Response timeout.");
        waitingForResponse = false;  // 重置等待标志
    }
}














