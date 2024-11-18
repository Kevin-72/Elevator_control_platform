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
    this->setWindowTitle("升降器控制平台(测试版 V2.0)");

    QStringList serialNamePort;

    connect(serialPort, &QSerialPort::readyRead, this, &Widget::readSerialData);

    receiverThread->start();

    scan_serial();

    setEnabledMy(false);

    // 连接响应超时定时器的 timeout 信号到 onResponseTimeout 槽函数
    connect(responseTimeoutTimer, &QTimer::timeout, this, &Widget::onResponseTimeout);
    // 设置响应超时定时器间隔
    responseTimeoutTimer->setSingleShot(true);  // 设置为单次定时器

    // 启动心跳检测线程
    startHeartbeatThread();



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


void Widget::appendLog(const QString &text, const QColor &color) {
    // 获取当前时间
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    // 格式化消息，加上时间戳
    QString formattedMessage = QString("[%1] %2").arg(currentTime, text);

    // 设置颜色并追加消息到日志视图
    QTextCharFormat fmt;
    fmt.setForeground(color);

    // 使用 QTextCursor 设置格式
    QTextCursor cursor = ui->logViewer->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(formattedMessage + '\n', fmt);

    // 确保滚动到最后
    ui->logViewer->moveCursor(QTextCursor::End);
}



void Widget::scan_serial()
{
    scanSerialStatus = false;
    ui->serialCb->clear();
    QStringList foundPorts;          // 保存所有可用串口的名称
    QString preferredSerialPort;     // 保存第一个带有 "serial" 的串口名
    QString ch340SerialPort;         // 保存第一个带有 "ch340" 的串口名
    // 扫描串口并更新下拉框
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QString portName = info.portName();
        QString portDescription = info.description();
        QString portDetail = QString("%1 %2").arg(portName, portDescription);

        foundPorts.append(portName);
        // 记录优先选择的串口
        if (portDescription.contains("ch340", Qt::CaseInsensitive) && ch340SerialPort.isEmpty()) {
            ch340SerialPort = portName;
        } else if (portDescription.contains("serial", Qt::CaseInsensitive) && preferredSerialPort.isEmpty()) {
            preferredSerialPort = portName;
        }
        if (portDescription.contains("ch340", Qt::CaseInsensitive) || portDescription.contains("serial", Qt::CaseInsensitive)) {
            ui->serialCb->addItem(portDetail, portName);
        } else {
            ui->serialCb->addItem(portName);
        }
    }

    
    // 设置优先选项, 尝试打开串口
    if (foundPorts.isEmpty()) {
        appendLog("未找到任何串口", Qt::red);
        QMessageBox::critical(this, "错误提示", "未找到任何串口！");
    }
    else {
        appendLog(QString("找到 %1 个串口").arg(foundPorts.size()), Qt::green);
        // 优先选择 "ch340" 或 "serial"
        if (!ch340SerialPort.isEmpty()) {
            // 优先设置带有 "ch340" 的串口
            int index = ui->serialCb->findData(ch340SerialPort);
            if (index != -1) {
                ui->serialCb->setCurrentIndex(index);
            }
        } else if (!preferredSerialPort.isEmpty()) {
            // 其次设置带有 "serial" 的串口
            int index = ui->serialCb->findData(preferredSerialPort);
            if (index != -1) {
                ui->serialCb->setCurrentIndex(index);
            }
        } else {
            // 没有找到匹配的串口，显示错误提示
            scanSerialStatus = true;
//            QMessageBox::critical(
//                this,
//                "错误提示",
//                QString("未找到带有 'serial' 字段的串口。\n可用串口: %1")
//                    .arg(foundPorts.isEmpty() ? "无" : foundPorts.join(", "))
//            );
            return;
        }

        if (foundPorts.size() == 1) {
            scanSerialStatus = true;
            emit ui->serialCb->currentTextChanged(ui->serialCb->currentText()); // 主动触发信号
        }
    }
}



void Widget::setEnabledMy(bool flag)
{
//    ui->baundrateCb->setEnabled(!flag);
//    ui->btnSerialCheck->setEnabled(!flag);
//    ui->serialCb->setEnabled(!flag);

//    ui->label_1->setEnabled(!flag);
//    ui->label_2->setEnabled(!flag);
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
//    ui->sendCb->setEnabled(flag);
    ui->queryCb->setEnabled(flag);
}

void Widget::startHeartbeatThread()
{
    // 如果心跳线程已经在运行，直接返回，不重复启动
    if (heartbeatThread->isRunning()) {
        return;
    }

    // 如果定时器已经启动，直接返回，不重复启动
    if (heartbeatTimer->isActive()) {
        return;
    }

    // 将定时器设置为定期触发
    heartbeatTimer->setInterval(HEARTBEATTIMESET); // 每隔几秒发送一次心跳
    connect(heartbeatTimer, &QTimer::timeout, this, &Widget::sendHeartbeat);

    // 如果线程没有在运行，则启动线程
    if (!heartbeatThread->isRunning()) {
        // 启动心跳线程
        heartbeatThread->start();
    }

    // 启动心跳定时器
//    heartbeatTimer->start();
}


void Widget::sendHeartbeat()
{
    ProtocolFrame frame = createHeartbeatFrame();  // 创建心跳帧
    waitingForHeartbeat = true;
    appendLog("发送心跳帧");
    sendFrame(frame);  // 发送心跳帧
}


//检测通讯端口槽函数
void Widget::on_btnSerialCheck_clicked()
{
    //通过QSerialPortInfo查找可用串口
    scan_serial();
}


void Widget::on_serialCb_currentTextChanged(const QString &arg1)
{
    // 如果旧串口已打开，关闭它
    if (serialPort && serialPort->isOpen()) {
        isReceiving = false;

        // 停止接收线程
        if (receiverThread->isRunning()) {
            receiverThread->quit();
            receiverThread->wait();
        }

        // 停止心跳检测线程
        if (heartbeatThread->isRunning()) {
            heartbeatThread->quit();
            heartbeatThread->wait();
        }

        // 停止定时器
        heartbeatTimer->stop();
        responseTimeoutTimer->stop();

        waitingForHeartbeat = false;
        waitingForResponse = false;

        serialPort->close(); // 关闭串口
        QThread::msleep(150); // 延迟200毫秒检查关闭状态
        serialPort->close(); // 关闭串口

        if (serialPort->isOpen()) {
            QMessageBox::critical(this, "错误提示", "串口关闭失败！请重试......");
            return;
        } else {
            setEnabledMy(false);
            appendLog("串口关闭成功", Qt::green);
        }
    }


    if (scanSerialStatus) {
        // 初始化串口属性，设置 端口号、波特率、数据位、停止位、奇偶校验位数
        QRegularExpression re("COM\\d+");
        QString portName = re.match(ui->serialCb->currentText()).captured(0);
        if (portName.isEmpty()) {
            QMessageBox::critical(this, "错误提示", "无效的端口号，请选择正确的串口！");
            appendLog("无效的端口号", Qt::red);
            return;
        }
        serialPort->setPortName(portName);
        serialPort->setBaudRate(QSerialPort::Baud9600);
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setStopBits(QSerialPort::OneStop);
        serialPort->setParity(QSerialPort::NoParity);


        // 尝试打开串口
        if (serialPort->open(QIODevice::ReadWrite)) {
            setEnabledMy(true);
            appendLog(QString("串口 %1 打开成功，波特率：9600").arg(portName), Qt::green);

//            // 启动接收线程
//            receiverThread->start();

            ProtocolFrame queryStatusFrame = createQueryStatusFrame();
            appendLog("尝试连接设备：");
            waitingForResponse = false;
            deviceStatus = true;
            sendFrame(queryStatusFrame);    // 这里之后的接收数据会改变deviceStatus的值
        } else {
            // 打开串口失败
            if (serialOpenCount > 0) {
                QMessageBox::critical(this, "错误提示", QString("串口 %1 打开失败!!!\r\n该串口可能被占用或无效!!!!!").arg(portName));
            }
            
            appendLog(QString("串口 %1 打开失败!!!该串口可能被占用或无效!!!!!").arg(portName), Qt::red);
        }

        serialOpenCount += 1;
    }

    scanSerialStatus = true;
    
}

