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
#include <QMutex>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QColor>

#include "protocol.h"

using namespace std;



QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void appendLog(const QString &text, const QColor &color = Qt::black);

    // 添加新的成员函数用于发送串口数据
    void sendSerialData(const QByteArray &data); 
    void sendFrame(const ProtocolFrame& frame);

    // 添加新的成员函数用于读取串口数据
    void readSerialData();  
    void receiveFrames(std::vector<uint8_t>& buffer);
    void heartbeatHandle(std::vector<uint8_t>& data);
    void receiveHandle(std::vector<uint8_t>& data);

    // 处理各种状态
    void handle_OFF_ON(uint8_t func_val);
    void handle_ACCESS_SELECT(uint8_t func_val);
    void handle_MAXCHANNEL(uint16_t func_val);
    void handle_CHANNEL(uint16_t func_val);
    void handle_POSITION_CONTROL(uint8_t func_val);
    void handle_A_F_SELECT(uint8_t func_val);

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
    bool waitingForHeartbeat;  // 等待心跳响应标志

    bool waitingForResponse;  // 等待响应标志
    QByteArray lastSentData;  // 记录上次发送的数据
    QTimer *responseTimeoutTimer;  // 响应超时定时器

    uint8_t A_F_Flag = 0x11;
};






#endif // WIDGET_H
