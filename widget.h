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

    void on_upBt_released();

    void on_downBt_released();

    void on_sendCb_clicked();

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;//定义串口指针
};

void scan_serial(Ui::Widget *ui, Widget *wgt);
void setEnabledMy(Ui::Widget *ui, bool flag);

#endif // WIDGET_H
