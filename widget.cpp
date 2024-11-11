#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("升降器控制平台");

    QStringList serialNamePort;

    serialPort = new QSerialPort(this);

    ui->serialCb->clear();
    // 获取并遍历所有可用的串口
    scan_serial(ui, this);

    setEnabledMy(ui, false);
}

Widget::~Widget()
{
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


void setEnabledMy(Ui::Widget *ui, bool flag)
{
    ui->baundrateCb->setEnabled(!flag);
    ui->btnSerialCheck->setEnabled(!flag);
    ui->serialCb->setEnabled(!flag);

    ui->label_1->setEnabled(!flag);
    ui->label_2->setEnabled(!flag);
    ui->label_3->setEnabled(flag);
    ui->label_4->setEnabled(flag);

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
    ui->sendCb->setEnabled(flag);
}

void scan_serial(Ui::Widget *ui, Widget *wgt)
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

    wgt->appendLog("查找串口成功");
}

/*打开串口*/
void Widget::on_openBt_clicked()
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
    if(ui->openBt->text() == "打开串口"){
        if(serialPort->open(QIODevice::ReadWrite) == true){
            ui->openBt->setText("关闭串口");
            // 让端口号下拉框不可选，避免误操作（选择功能不可用，控件背景为灰色）
//            ui->serialCb->setEnabled(false);
            setEnabledMy(ui, true);
            appendLog("串口打开成功");
        }else{
            QMessageBox::critical(this, "错误提示", "串口打开失败！！！\r\n该串口可能被占用\r\n请选择正确的串口");
            appendLog("串口打开失败");
        }
    }else{
        serialPort->close();
        ui->openBt->setText("打开串口");
        // 端口号下拉框恢复可选，避免误操作
//        ui->serialCb->setEnabled(true);
        setEnabledMy(ui, false);
        appendLog("串口关闭成功");
    }
}

//检测通讯端口槽函数
void Widget::on_btnSerialCheck_clicked()
{
    ui->serialCb->clear();
    //通过QSerialPortInfo查找可用串口
    scan_serial(ui, this);
}





void Widget::on_upBt_pressed()
{
    serialPort->write("CMD=UP\r\n");
    appendLog("持续上升中......");
}
// void Widget::on_upBt_released()
// {
//     serialPort->write("CMD=STOP\r\n");
//     appendLog("持续上升停止");
// }


void Widget::on_downBt_pressed()
{
    serialPort->write("CMD=DOWN\r\n");
    appendLog("持续下降中......");
}
// void Widget::on_downBt_released()
// {
//     serialPort->write("CMD=STOP\r\n");
//     appendLog("持续下降停止");
// }



void Widget::on_stopBt_clicked()
{
    serialPort->write("CMD=STOP\r\n");
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



void Widget::on_sendCb_clicked()
{
    if(!ui->maxChannelSetCb->text().isEmpty())
    {
        QString channelInfo = QString("CHANNEL=%1+%2\r\n").arg(ui->channelsCb->currentText(), ui->maxChannelSetCb->text());
        serialPort->write(channelInfo.toUtf8());
        appendLog(QString("通道和最大频道设置信息已发送：%1").arg(channelInfo));
    }
    else
    {
        QMessageBox::critical(this, "错误提示", "最大频道设置不能为空\r\n请重新设置！！！");
        appendLog("发送失败，请重新设置！");
    }

}




// 构造函数实现
ProtocolFrame::ProtocolFrame(uint8_t cmd, const std::vector<uint8_t>& dataPayload)
    : frameHeader(FRAME_HEADER), version(VERSION), command(cmd), dataLength(dataPayload.size()), data(dataPayload) {
    checksum = calculateChecksum(serialize(false)); // 计算校验和
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
static ProtocolFrame deserialize(const std::vector<uint8_t>& rawData) {
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

// 计算校验和函数实现
uint8_t ProtocolFrame::calculateChecksum(const std::vector<uint8_t>& data) const {
    uint8_t checksum = 0x00;
    for (auto byte : data) {
        checksum += byte;
    }
    return ~checksum + 1;  // 计算补码
}



// 心跳检测构造
ProtocolFrame createHeartbeatFrame(bool initial = true) {
    std::vector<uint8_t> data = {initial ? 0x00 : 0x01};  // 初始发送0x00，后续0x01
    return ProtocolFrame(HEARTBEAT, data);
}

// 查询状态构造
ProtocolFrame createQueryStatusFrame() {
    return ProtocolFrame(QUERY_STATUS, {});
}

// 设备控制构造
ProtocolFrame createDeviceControlFrame(uint8_t deviceId, uint8_t commandValue) {
    std::vector<uint8_t> data = {deviceId, commandValue};
    return ProtocolFrame(DEVICE_CONTROL, data);
}

// 模拟发送协议帧
void sendFrame(const ProtocolFrame& frame) {
    std::vector<uint8_t> serialized = frame.serialize();

    // 显示发送的帧内容
    std::cout << "Sending Frame: ";
    for (auto byte : serialized) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::endl;

    // 模拟接收下位机响应
    std::vector<uint8_t> responseData = {
        0x55, 0xAA, 0x00, 0x03, 0x00, 0x01, 0x00,  // 第一帧
        0x55, 0xAA, 0x00, 0x03, 0x00, 0x02, 0x01,  // 第二帧
        0x55, 0xAA, 0x00, 0x06, 0x00, 0x01, 0x01, 0x01  // 第三帧
    };
    receiveFrames(responseData);
}

// 接收并解析多个下位机响应
void receiveFrames(std::vector<uint8_t>& buffer) {
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

            // 根据响应的命令字解析并处理
            switch (responseFrame.command) {
                case HEARTBEAT:
                    std::cout << "Received Heartbeat response. Data: ";
                    for (auto byte : responseFrame.data) {
                        std::cout << std::hex << (int)byte << " ";
                    }
                    std::cout << std::endl;
                    break;
                case QUERY_STATUS:
                    std::cout << "Received Query Status response." << std::endl;
                    break;
                case DEVICE_CONTROL:
                    std::cout << "Received Device Control response." << std::endl;
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


// // 发送心跳检测
// ProtocolFrame heartbeatFrame = createHeartbeatFrame();
// sendFrame(heartbeatFrame);

// // 发送查询状态请求
// ProtocolFrame queryStatusFrame = createQueryStatusFrame();
// sendFrame(queryStatusFrame);

// // 发送设备控制命令（示例: 控制设备开关）
// ProtocolFrame deviceControlFrame = createDeviceControlFrame(0x14, 0x01);  // 示例: 控制开关
// sendFrame(deviceControlFrame);
