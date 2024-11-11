#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QApplication>
#include <QMessageBox>
#include <QRegularExpression>
#include <QThread>
#include <QDateTime>

#include <cstdint>
#include <iomanip>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void appendLog(const QString &text);

private slots:


    void on_openBt_clicked();

    void on_btnSerialCheck_clicked();


    void on_upBt_pressed();

    void on_downBt_pressed();

    void on_mode01Bt_clicked();

    void on_mode02Bt_clicked();

    void on_mode03Bt_clicked();

    void on_mode04Bt_clicked();

    void on_mode05Bt_clicked();

    void on_stopBt_clicked();

    // void on_upBt_released();

    // void on_downBt_released();

    void on_sendCb_clicked();

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;//定义串口指针
};

void scan_serial(Ui::Widget *ui, Widget *wgt);
void setEnabledMy(Ui::Widget *ui, bool flag);



// 协议字段定义
const uint16_t FRAME_HEADER = 0x55AA;  // 帧头
const uint8_t VERSION = 0x00;          // 默认版本

// 消息类型定义
enum CommandType {
    HEARTBEAT = 0x00,
    QUERY_STATUS = 0x08,
    DEVICE_CONTROL = 0x06
};


// 协议帧结构体
class ProtocolFrame {
private:
    uint16_t frameHeader;              // 帧头
    uint8_t version;                   // 版本号
    uint8_t command;                   // 指令
    uint16_t dataLength;               // 数据长度
    std::vector<uint8_t> data;         // 数据内容
    uint8_t checksum;                  // 校验和

    // 计算校验和（私有函数）
    uint8_t calculateChecksum(const std::vector<uint8_t>& data) const;

public:
    // 构造函数
    ProtocolFrame(uint8_t cmd, const std::vector<uint8_t>& dataPayload);

    // 将帧序列化为字节流
    std::vector<uint8_t> serialize(bool withChecksum = true) const;

    // 反序列化字节流，解析响应
    static ProtocolFrame deserialize(const std::vector<uint8_t>& rawData);
};


// 心跳检测构造
ProtocolFrame createHeartbeatFrame(bool initial = true);

// 查询状态构造
ProtocolFrame createQueryStatusFrame();

// 设备控制构造
ProtocolFrame createDeviceControlFrame(uint8_t deviceId, uint8_t commandValue);

// 模拟发送协议帧
void sendFrame(const ProtocolFrame& frame);

// 接收并解析多个下位机响应
void receiveFrames(const std::vector<uint8_t>& rawData);




#endif // WIDGET_H
