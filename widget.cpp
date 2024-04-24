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
void Widget::on_upBt_released()
{
    serialPort->write("CMD=STOP\r\n");
    appendLog("持续上升停止");
}


void Widget::on_downBt_pressed()
{
    serialPort->write("CMD=DOWN\r\n");
    appendLog("持续下降中......");
}
void Widget::on_downBt_released()
{
    serialPort->write("CMD=STOP\r\n");
    appendLog("持续下降停止");
}



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

