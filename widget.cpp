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
    this->setWindowTitle("升降器控制平台(测试版 V1.0)");

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
    waitingForHeartbeat = true;
    sendFrame(frame);  // 发送心跳帧
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

