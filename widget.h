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
    void on_queryCb_clicked();
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



enum class DPType : uint8_t {
    OFF_ON = 0x14,
    ACCESS_SELECT = 0x15,
    MAXCHANNEL = 0x65,
    CHANNEL = 0x66,
    POSITION_CONTROL = 0x67,
    A_F_SELECT = 0x68
};


// 定义 数据类型 枚举
enum class DataType : uint8_t {
    TYPE_01 = 0x01,
    TYPE_02 = 0x02,
    TYPE_04 = 0x04
};

// 创建 DPType 到 DataType 的映射表
static const std::unordered_map<DPType, DataType> DPTypeToDataTypeMap = {
    {DPType::OFF_ON, DataType::TYPE_01},
    {DPType::ACCESS_SELECT, DataType::TYPE_04},
    {DPType::MAXCHANNEL, DataType::TYPE_02},
    {DPType::CHANNEL, DataType::TYPE_02},
    {DPType::POSITION_CONTROL, DataType::TYPE_04},
    {DPType::A_F_SELECT, DataType::TYPE_01}
};


// 定义开关值
enum class SwitchValue : uint8_t {
    SWITCH_OFF = 0x00,
    SWITCH_ON  = 0x01,
};
static const std::unordered_map<SwitchValue, std::string> SwitchValueMap = {
    {SwitchValue::SWITCH_OFF, "OFF"},
    {SwitchValue::SWITCH_ON, "ON"}
};

// 定义AD类型通道值
enum class ADAccessValue : uint8_t {
    ADACCESS_A = 0x00,
    ADACCESS_B = 0x01,
    ADACCESS_C = 0x02,
    ADACCESS_D = 0x03
};
static const std::unordered_map<ADAccessValue, std::string> ADAccessValueMap = {
    {ADAccessValue::ADACCESS_A, "A"},
    {ADAccessValue::ADACCESS_B, "B"},
    {ADAccessValue::ADACCESS_C, "C"},
    {ADAccessValue::ADACCESS_D, "D"}
};

// 定义F类型通道值
enum class FAccessValue : uint8_t {
    FACCESS_0 = 0x00,
    FACCESS_1 = 0x01,
    FACCESS_2 = 0x02,
    FACCESS_3 = 0x03,
    FACCESS_4 = 0x04,
    FACCESS_5 = 0x05,
    FACCESS_6 = 0x06,
    FACCESS_7 = 0x07,
    FACCESS_8 = 0x08,
    FACCESS_9 = 0x09
};
static const std::unordered_map<FAccessValue, std::string> FAccessValueMap = {
    {FAccessValue::FACCESS_0, "F0"},
    {FAccessValue::FACCESS_1, "F1"},
    {FAccessValue::FACCESS_2, "F2"},
    {FAccessValue::FACCESS_3, "F3"},
    {FAccessValue::FACCESS_4, "F4"},
    {FAccessValue::FACCESS_5, "F5"},
    {FAccessValue::FACCESS_6, "F6"},
    {FAccessValue::FACCESS_7, "F7"},
    {FAccessValue::FACCESS_8, "F8"},
    {FAccessValue::FACCESS_9, "F9"}
};

// 定义通道字符对应的值
static const std::unordered_map<std::string, uint8_t> StringAccessValueMap = {
    {"A", 0x00},
    {"B", 0x01},
    {"C", 0x02},
    {"D", 0x03},
    {"F0", 0x00},
    {"F1", 0x01},
    {"F2", 0x02},
    {"F3", 0x03},
    {"F4", 0x04},
    {"F5", 0x05},
    {"F6", 0x06},
    {"F7", 0x07},
    {"F8", 0x08},
    {"F9", 0x09}
};

// 定义设备控制功能值
enum class DevCtrlValue : uint8_t {
    DevCtrl_UP = 0x00,
    DevCtrl_STOP = 0x01,
    DevCtrl_DOWN = 0x02
};
static const std::unordered_map<DevCtrlValue, std::string> DevCtrlValueMap = {
    {DevCtrlValue::DevCtrl_UP, "UP"},
    {DevCtrlValue::DevCtrl_STOP, "STOP"},
    {DevCtrlValue::DevCtrl_DOWN, "DOWN"}
};

// 定义A/F通道类型值
enum class AFSelectValue : uint8_t {
    AFSelect_A = 0x00,
    AFSelect_F = 0x01
};
static const std::unordered_map<AFSelectValue, std::string> AFSelectValueMap = {
    {AFSelectValue::AFSelect_A, "A"},
    {AFSelectValue::AFSelect_F, "F"}
};






// 心跳检测构造
ProtocolFrame createHeartbeatFrame();

// 查询状态构造
ProtocolFrame createQueryStatusFrame();

// 设备控制构造
ProtocolFrame createDeviceControlFrame(DPType dpId, const std::vector<uint8_t>& commandValue);




#endif // WIDGET_H