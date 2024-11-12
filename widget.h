#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QApplication>
#include <QMessageBox>
#include <QRegularExpression>
#include <QThread>
#include <QTimer>
#include <QDateTime>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <QMutex>
#include <QByteArray>

using namespace std;

#define  RESPONSETIMEOUTTIMESET  200      // 响应超时时间设置

// 协议帧结构体
class ProtocolFrame {
public:
    uint16_t frameHeader;              // 帧头
    uint8_t version;                   // 版本号
    uint8_t command;                   // 指令
    uint16_t dataLength;               // 数据长度
    std::vector<uint8_t> data;         // 数据内容
    uint8_t checksum;                  // 校验和

    // 构造函数
    ProtocolFrame(uint8_t cmd, const std::vector<uint8_t>& dataPayload);

    // 计算校验和
    uint8_t calculateChecksum(const std::vector<uint8_t>& data) const;

    // 将帧序列化为字节流
    std::vector<uint8_t> serialize(bool withChecksum = true) const;


    // 检查校验和
    bool validateChecksum(const std::vector<uint8_t>& frame) const;

    // 反序列化字节流，解析响应
    static ProtocolFrame deserialize(const std::vector<uint8_t>& rawData);


};


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

    // 添加新的成员函数用于发送串口数据
    void sendSerialData(const QByteArray &data); 
    void sendFrame(const ProtocolFrame& frame);

    // 添加新的成员函数用于读取串口数据
    void readSerialData();  
    void receiveFrames(std::vector<uint8_t>& buffer);

    void scan_serial();
    void setEnabledMy(bool flag);

    // 心跳线程
    void startHeartbeatThread();
    void sendHeartbeat();

private slots:
    void on_openSerialBt_clicked();
    void on_btnSerialCheck_clicked();
    void on_upBt_pressed();
    void on_downBt_pressed();
    void on_stopBt_clicked();
    void on_mode01Bt_clicked();
    void on_mode02Bt_clicked();
    void on_mode03Bt_clicked();
    void on_mode04Bt_clicked();
    void on_mode05Bt_clicked();

    void on_openBt_clicked();

    void on_closeBt_clicked();

    void on_sendCb_clicked();

    void onResponseTimeout();  // 响应超时槽函数

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;//定义串口指针

    QThread *receiverThread;
    bool isReceiving; // 标记接收状态

    QMutex serialMutex;  // 定义串口通信的互斥锁

    QThread *heartbeatThread;  // 新增心跳检测线程
    QTimer *heartbeatTimer;    // 心跳定时器

    bool waitingForResponse;  // 等待响应标志
    QByteArray lastSentData;  // 记录上次发送的数据
    QTimer *responseTimeoutTimer;  // 响应超时定时器
};



// 协议字段定义
const uint16_t FRAME_HEADER = 0x55AA;  // 帧头
const uint8_t VERSION = 0x00;          // 默认版本

// 消息类型定义
enum CommandType {
    HEARTBEAT = 0x00,
    QUERY_STATUS = 0x08,
    DEVICE_CONTROL = 0x06
};





// 心跳检测构造
ProtocolFrame createHeartbeatFrame(bool initial);

// 查询状态构造
ProtocolFrame createQueryStatusFrame();

// 设备控制构造
ProtocolFrame createDeviceControlFrame(uint8_t deviceId, uint8_t commandValue);




#endif // WIDGET_H
