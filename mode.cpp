#include "widget.h"
#include "ui_widget.h"

#include <fstream>
#include <sstream>
#include <string>
#include <functional>

#include <QTableWidget>
#include <QDir>


TableEditor::TableEditor(const QString &filePath, QWidget *parent)
    : QDialog(parent), filePath(filePath), loop_count(0) {
    setWindowTitle(filePath);
    resize(600, 400);

    // 表格初始化
    tableWidget = new QTableWidget(1, 4, this); // 默认 1 行 4 列
    tableWidget->setHorizontalHeaderLabels({"通道", "频道", "控制", "延时"});
    tableWidget->horizontalHeader()->setStretchLastSection(true);

    // 按钮初始化
    QPushButton *addRowButton = new QPushButton("增加一行", this);
    connect(addRowButton, &QPushButton::clicked, this, &TableEditor::addRow);

    // 循环次数输入框和标签
    QLabel *loopLabel = new QLabel("循环次数:", this);
    loopEdit = new QLineEdit(this);
    loopEdit->setValidator(new QIntValidator(0, 100000, this)); // 只允许输入正整数
    loopEdit->setPlaceholderText("请输入正整数");

    // 编辑框和标签布局
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(loopLabel);
    inputLayout->addWidget(loopEdit);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(addRowButton);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tableWidget);
    mainLayout->addLayout(inputLayout);
    mainLayout->addLayout(buttonLayout);

    // 加载数据
    loadTableData();
}

TableEditor::~TableEditor() {
    saveTableData();
}

void TableEditor::printTableDataToLog(Widget *logWidget) {
    // 获取总行数和总列数
    int rowCount = tableWidget->rowCount();
    int columnCount = tableWidget->columnCount();

    logWidget->appendLog(QString("LoopCount: %1").arg(loop_count));

    // 打印总行数和总列数
    QString logMessage = QString("Total Rows: %1, Total Columns: %2").arg(rowCount).arg(columnCount);
    logWidget->appendLog(logMessage);

    // 遍历每一行并打印每个单元格的内容
    for (int row = 0; row < rowCount; ++row) {
        QStringList rowData;
        for (int col = 0; col < columnCount; ++col) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            rowData.append(item ? item->text() : "");  // 如果该单元格没有内容，添加空字符串
        }
        // 打印该行的数据到日志控件
        logWidget->appendLog(QString("Row %1: %2").arg(row + 1).arg(rowData.join(", ")));
    }
}






void TableEditor::addRow() {
    tableWidget->insertRow(tableWidget->rowCount());
}

void TableEditor::loadTableData() {
    QFile file(filePath);
    if (!file.exists()) {
        QMessageBox::warning(this, "Error", "文件不存在：" + filePath);
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "无法读取文件：" + filePath);
        return;
    }

    QTextStream in(&file);
    // 清空表格中的所有行
    tableWidget->setRowCount(0);  // 重置行数为 0，清空所有数据
    bool isFirstRow = true;  // 标记是否是第一次加载数据

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(",");

        // 检查是否是 LoopCount
        if (values.size() == 1 && values[0].startsWith("LoopCount=")) {
            loop_count = values[0].midRef(QString("LoopCount=").length()).toInt();
            loopEdit->setText(QString::number(loop_count));
            continue;
        }

        // 如果是第一行数据，移除表格中默认的行
        if (isFirstRow) {
            tableWidget->removeRow(0);  // 删除初始化时的第一行
            isFirstRow = false;         // 设置标志位，确保只删除一次
        }

        // 添加新行
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);

        for (int col = 0; col < values.size() && col < tableWidget->columnCount(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(values[col].trimmed());
            tableWidget->setItem(row, col, item);
        }
    }

    file.close();
}



void TableEditor::saveTableData() {
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();

    // 检查目录是否存在，不存在则创建
    if (!dir.exists()) {
        if (!dir.mkpath(fileInfo.absolutePath())) { // 创建目录，如果失败，发出警告
            QMessageBox::warning(this, "Error", "无法创建目录：" + dir.absolutePath());
            return;
        }
    }

    QFile file(filePath);
    // 打开文件，覆盖原内容
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, "Error", "无法保存文件：" + filePath + "\n错误信息: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    // 保存表格数据，删除空行
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        QStringList rowData;
        bool isEmptyRow = true;
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            QString cellData = item ? item->text().trimmed() : ""; // 去除空格
            if (!cellData.isEmpty()) {
                isEmptyRow = false; // 只要有一个单元格不为空，认为该行不是空行
            }
            rowData.append(cellData);
        }
        if (!isEmptyRow) {
            out << rowData.join(",") << "\n"; // 保存非空行
        }
    }

    // 保存循环次数
    loop_count = loopEdit->text().toInt();
    out << "LoopCount=" << loop_count << "\n";

    file.flush(); // 确保内容被刷新到磁盘
    file.close();
}


bool TableEditor::validateTableData(Widget *logWidget) {
    bool hasError = false;

    // 遍历每一行
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        int emptyCount = 0;

        // 检查该行的每一列是否为空
        for (int col = 0; col < 4; ++col) {
            if (tableWidget->item(row, col) == nullptr || tableWidget->item(row, col)->text().trimmed().isEmpty()) {
                emptyCount++;
            }
        }
        // 如果某行的空单元格数量大于 1，则记录日志并返回
        if (emptyCount > 1) {
            logWidget->appendLog("错误：第" + QString::number(row + 1) + "行中有超过一个单元格为空!");
            return true;  // 表示验证失败
        }

        // 检查第一列和第三列是否非空
        if (tableWidget->item(row, 0) == nullptr || tableWidget->item(row, 2) == nullptr) {
            logWidget->appendLog("错误：第" + QString::number(row + 1) + "行的第一列或第三列为空!");
            hasError = true;
            continue;  // 跳过当前行，继续检查下一行
        }

        // 获取第一列的数据，并检查是否为 StringAccessValueMap 中的键
        QString firstColValue = tableWidget->item(row, 0)->text().trimmed();
        if (StringAccessValueMap.find(firstColValue.toStdString()) == StringAccessValueMap.end()) {
            logWidget->appendLog("错误：第" + QString::number(row + 1) + "行的第一列数据 \"" + firstColValue +
                                 "\" 不是有效的 StringAccessValueMap 键!");
            hasError = true;
        }
        if (logWidget->A_F_Flag == static_cast<uint8_t>(AFSelectValue::AFSelect_A)) {
            // 检查 firstColValue 是否在 ADAccessValueMap 的值中
            bool isValidADAccessValue = false;
            for (const auto& pair : ADAccessValueMap) {
                if (pair.second == firstColValue) {  // 比较 QString 值
                    isValidADAccessValue = true;
                    break;
                }
            }

            if (!isValidADAccessValue) {
                logWidget->appendLog("错误：第一列数据 \"" + firstColValue + "\" 不是此上位机的值!");
                return true; // 验证失败
            }
        }
        else if (logWidget->A_F_Flag == static_cast<uint8_t>(AFSelectValue::AFSelect_F)) {
            // 检查 firstColValue 是否在 FAccessValueMap 的值中
            bool isValidFAccessValue = false;
            for (const auto& pair : FAccessValueMap) {
                if (pair.second == firstColValue) {  // 比较 QString 值
                    isValidFAccessValue = true;
                    break;  // 找到匹配值后直接退出循环
                }
            }

            if (!isValidFAccessValue) {
                logWidget->appendLog("错误：第一列数据 \"" + firstColValue + "\" 不是此上位机的值!");
                return true; // 验证失败
            }
        }


        // 获取第三列的数据，并检查是否为 DevCtrlValueMap 中的键
        QString thirdColValue = tableWidget->item(row, 2)->text().trimmed();
        bool isValidDevCtrlValue = false;

        // 检查 thirdColValue 是否在 DevCtrlValueMap 中
        for (const auto& pair : DevCtrlValueMap) {
            if (pair.second == thirdColValue) {
                isValidDevCtrlValue = true;
                break;
            }
        }

        if (!isValidDevCtrlValue) {
            logWidget->appendLog("错误：第" + QString::number(row + 1) + "行的第三列数据 \"" + thirdColValue +
                                 "\" 不是有效的 DevCtrlValueMap 键!");
            hasError = true;
        }

        // 检查第二列和第四列是否是小于 65536 的自然数
        bool ok;
        uint16_t secondColValue = tableWidget->item(row, 1)->text().trimmed().toUInt(&ok);
        if (!ok || secondColValue >= 0xffff) {
            logWidget->appendLog("错误：第" + QString::number(row + 1) + "行的第二列数据 \"" +
                                 tableWidget->item(row, 1)->text() + "\" 不是有效的小于 65536 的自然数!");
            hasError = true;
        }

        uint16_t fourthColValue = tableWidget->item(row, 3)->text().trimmed().toUInt(&ok);
        if (!ok || fourthColValue >= 0xffff) {
            logWidget->appendLog("错误：第" + QString::number(row + 1) + "行的第四列数据 \"" +
                                 tableWidget->item(row, 3)->text() + "\" 不是有效的小于 65536 的自然数!");
            hasError = true;
        }
    }

    // 如果有错误，输出一个提示
    if (hasError) {
        logWidget->appendLog("表格中存在错误数据，请检查日志!");
    } else {
        logWidget->appendLog("表格数据验证成功，没有发现错误!");
    }

    return hasError;
}



void Widget::openOrCreateTable(const QString &fileName) {
    // 获取用户文档目录路径
    QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    // 拼接文件路径
    QString filePath = documentsDir + "/Elevator/" + fileName;

    // 打开或创建表格编辑窗口
    TableEditor editor(filePath, this);
    editor.exec(); // 模态显示窗口
}

void Widget::execOrCreateTable(const QString &fileName, std::function<void()> pFun_rightClicked) {
    // 获取用户文档目录路径
    QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    // 拼接文件路径
    QString filePath = documentsDir + "/Elevator/" + fileName;

    // 打开或创建表格编辑窗口
    TableEditor editor(filePath, this);
    if (editor.validateTableData(this)) {
        QMessageBox::critical(this, "错误提示", "表格数据错误！\r\n请修改后重新运行");
        pFun_rightClicked();
    }
    else {
        editor.printTableDataToLog(this);
        editor.sendTableData(this);
    }

}

void TableEditor::sendTableData(Widget *logWidget) {
    logWidget->stopRequested = false; // 每次开始执行时重置
    std::vector<uint8_t> allStatusData(8);
    for (int loop = 0; loop < loop_count; ++loop) {
        if (logWidget->stopRequested) {
            logWidget->appendLog("发送操作模式已被停止。", Qt::gray);
            logWidget->sendReset();
            logWidget->setColor();
            return; // 提前退出函数
        }
        // 遍历每一行
        for (int row = 0; row < tableWidget->rowCount(); ++row) {
            try {
                if (logWidget->stopRequested) {
                    logWidget->appendLog("发送操作模式已被停止。", Qt::gray);
                    logWidget->sendReset();
                    logWidget->setColor();
                    return; // 提前退出函数
                }
                // 1、开关状态
                allStatusData[offset_OFF_ON] = logWidget->switchStatus
                                                   ? static_cast<uint8_t>(SwitchValue::SWITCH_OFF)
                                                   : static_cast<uint8_t>(SwitchValue::SWITCH_ON);

                // 2、通道
                QString valueAccess = tableWidget->item(row, 0)->text();
                if (valueAccess.isEmpty()) throw std::runtime_error("通道字段为空！");
                auto it = StringAccessValueMap.find(valueAccess.toStdString());
                if (it != StringAccessValueMap.end()) {
                    allStatusData[offset_ACCESS_SELECT] = it->second;
                } else {
                    throw std::runtime_error("表格内出现非通道字字段！");
                }

                // 3、最大频道值
                allStatusData[offset_MAXCHANNEL] = (logWidget->maxChannelNumber >> 8) & 0xFF;  // 高字节
                allStatusData[offset_MAXCHANNEL + 1] = logWidget->maxChannelNumber & 0xFF;    // 低字节

                // 4、频道值
                QString channelText = tableWidget->item(row, 1)->text();
                if (channelText.isEmpty()) throw std::runtime_error("频道值为空！");
                uint32_t valueChannel = channelText.toInt();
                allStatusData[offset_CHANNEL] = (valueChannel >> 8) & 0xFF;  // 高字节
                allStatusData[offset_CHANNEL + 1] = valueChannel & 0xFF;     // 低字节

                // 5、设备控制
                QString valueCtrl = tableWidget->item(row, 2)->text();
                if (valueCtrl.isEmpty()) throw std::runtime_error("设备控制字段为空！");
                auto its = StringDevCtrlValueMap.find(valueCtrl.toStdString());
                if (its != StringDevCtrlValueMap.end()) {
                    allStatusData[offset_POSITION_CONTROL] = static_cast<uint8_t>(its->second);
                } else {
                    throw std::runtime_error("表格内出现非设备控制字段！");
                }

                // 6、A/F状态
                allStatusData[offset_A_F_SELECT] = logWidget->A_F_Flag;

                // 发送 allStatus
                logWidget->appendLog(QString("发送 allStatus: Row %1").arg(row));
                ProtocolFrame allStatusDataFrame = createDeviceControlFrame(DPType::ALL_STATUS, allStatusData);
                logWidget->sendFrame(allStatusDataFrame);

                // 延时（非阻塞）
                QString delayText = tableWidget->item(row, 3)->text();
                if (delayText.isEmpty()) throw std::runtime_error("延时时间为空！");
                uint32_t delayTime = delayText.toUInt();
                QEventLoop loop;
                QTimer::singleShot(delayTime * 1000, &loop, &QEventLoop::quit);
                loop.exec();

                if (logWidget->stopRequested) {
                    logWidget->appendLog("发送操作模式已被停止。", Qt::gray);
                    logWidget->sendReset();
                    logWidget->setColor();
                    return; // 提前退出函数
                }
            } catch (const std::runtime_error &e) {
                QMessageBox::critical(this, "错误提示", QString("行 %1: %2").arg(row).arg(e.what()));
                logWidget->appendLog(QString("Error (Row %1): %2").arg(row).arg(e.what()), Qt::red);
                logWidget->setColor();
                return; // 停止发送
            }
        }
    }
    logWidget->appendLog("发送模式结束。", Qt::gray);
    logWidget->setColor();
    logWidget->stopRequested = true; // 每次模式执行结束时重置
}


bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            if (watched == ui->mode01Bt) {
                mode01Bt_rightClicked();
            } else if (watched == ui->mode02Bt) {
                mode02Bt_rightClicked();
            } else if (watched == ui->mode03Bt) {
                mode03Bt_rightClicked();
            } else if (watched == ui->mode04Bt) {
                mode04Bt_rightClicked();
            } else if (watched == ui->mode05Bt) {
                mode05Bt_rightClicked();
            } else if (watched == ui->mode06Bt) {
                mode06Bt_rightClicked();
            }
            return true;  // 事件已处理
        }
    }
    return QWidget::eventFilter(watched, event);  // 保留默认处理
}

void Widget::setColor(QPushButton *modeBt) {
    if (nullptr == modeBt) {
        ui->mode01Bt->setStyleSheet("background-color: lightgray;");
        ui->mode02Bt->setStyleSheet("background-color: lightgray;");
        ui->mode03Bt->setStyleSheet("background-color: lightgray;");
        ui->mode04Bt->setStyleSheet("background-color: lightgray;");
        ui->mode05Bt->setStyleSheet("background-color: lightgray;");
        ui->mode06Bt->setStyleSheet("background-color: lightgray;");
    }
    else {
        modeBt->setStyleSheet("background-color: lightgreen;");
    }
}

void Widget::on_mode01Bt_clicked()
{
    if (ui->mode01Bt->styleSheet().contains("lightgreen")) {
        ui->mode01Bt->setStyleSheet("background-color: lightgray;");
        stopRequested = true;
        appendLog("模式1已被主动停止！");
    }
    else {
        if (!stopRequested) {
            appendLog("当前正在执行其它模式.......请停止当前模式后重试", Qt::red);
            QMessageBox::critical(this, "错误提示", "当前正在执行其它模式.......请停止当前模式后重试");
            return; // 提前退出函数
        }
        setColor(ui->mode01Bt);
        appendLog("启动模式1");
        execOrCreateTable("mode01.data", [this]() { mode01Bt_rightClicked(); });
    }
}
void Widget::mode01Bt_rightClicked()
{
    ui->mode01Bt->setStyleSheet("background-color: lightgray;");
    appendLog("编辑模式1", Qt::blue);
    openOrCreateTable("mode01.data");
}

void Widget::on_mode02Bt_clicked()
{
    if (ui->mode02Bt->styleSheet().contains("lightgreen")) {
        ui->mode02Bt->setStyleSheet("background-color: lightgray;");
        stopRequested = true;
        appendLog("模式2已被主动停止！");
    }
    else {
        if (!stopRequested) {
            appendLog("当前正在执行其它模式.......请停止当前模式后重试", Qt::red);
            QMessageBox::critical(this, "错误提示", "当前正在执行其它模式.......请停止当前模式后重试");
            return;
        }
        setColor(ui->mode02Bt);
        appendLog("启动模式2");
        execOrCreateTable("mode02.data", [this]() { mode02Bt_rightClicked(); });
    }
}
void Widget::mode02Bt_rightClicked()
{
    ui->mode02Bt->setStyleSheet("background-color: lightgray;");
    appendLog("编辑模式2", Qt::blue);
    openOrCreateTable("mode02.data");
}

void Widget::on_mode03Bt_clicked()
{
    if (ui->mode03Bt->styleSheet().contains("lightgreen")) {
        ui->mode03Bt->setStyleSheet("background-color: lightgray;");
        stopRequested = true;
        appendLog("模式3已被主动停止！");
    }
    else {
        if (!stopRequested) {
            appendLog("当前正在执行其它模式.......请停止当前模式后重试", Qt::red);
            QMessageBox::critical(this, "错误提示", "当前正在执行其它模式.......请停止当前模式后重试");
            return;
        }
        setColor(ui->mode03Bt);
        appendLog("启动模式3");
        execOrCreateTable("mode03.data", [this]() { mode03Bt_rightClicked(); });
    }
}
void Widget::mode03Bt_rightClicked()
{
    ui->mode03Bt->setStyleSheet("background-color: lightgray;");
    appendLog("编辑模式3", Qt::blue);
    openOrCreateTable("mode03.data");
}

void Widget::on_mode04Bt_clicked()
{
    if (ui->mode04Bt->styleSheet().contains("lightgreen")) {
        ui->mode04Bt->setStyleSheet("background-color: lightgray;");
        stopRequested = true;
        appendLog("模式4已被主动停止！");
    }
    else {
        if (!stopRequested) {
            appendLog("当前正在执行其它模式.......请停止当前模式后重试", Qt::red);
            QMessageBox::critical(this, "错误提示", "当前正在执行其它模式.......请停止当前模式后重试");
            return;
        }
        setColor(ui->mode04Bt);
        appendLog("启动模式4");
        execOrCreateTable("mode04.data", [this]() { mode04Bt_rightClicked(); });
    }
}
void Widget::mode04Bt_rightClicked()
{
    ui->mode04Bt->setStyleSheet("background-color: lightgray;");
    appendLog("编辑模式4", Qt::blue);
    openOrCreateTable("mode04.data");
}

void Widget::on_mode05Bt_clicked()
{
    if (ui->mode05Bt->styleSheet().contains("lightgreen")) {
        ui->mode05Bt->setStyleSheet("background-color: lightgray;");
        stopRequested = true;
        appendLog("模式5已被主动停止！");
    }
    else {
        if (!stopRequested) {
            appendLog("当前正在执行其它模式.......请停止当前模式后重试", Qt::red);
            QMessageBox::critical(this, "错误提示", "当前正在执行其它模式.......请停止当前模式后重试");
            return;
        }
        setColor(ui->mode05Bt);
        appendLog("启动模式5");
        execOrCreateTable("mode05.data", [this]() { mode05Bt_rightClicked(); });
    }
}
void Widget::mode05Bt_rightClicked()
{
    ui->mode05Bt->setStyleSheet("background-color: lightgray;");
    appendLog("编辑模式5", Qt::blue);
    openOrCreateTable("mode05.data");
}

void Widget::on_mode06Bt_clicked()
{
    if (ui->mode06Bt->styleSheet().contains("lightgreen")) {
        ui->mode06Bt->setStyleSheet("background-color: lightgray;");
        stopRequested = true;
        appendLog("模式6已被主动停止！");
    }
    else {
        if (!stopRequested) {
            appendLog("当前正在执行其它模式.......请停止当前模式后重试", Qt::red);
            QMessageBox::critical(this, "错误提示", "当前正在执行其它模式.......请停止当前模式后重试");
            return;
        }
        setColor(ui->mode06Bt);
        appendLog("启动模式6");
        execOrCreateTable("mode06.data", [this]() { mode06Bt_rightClicked(); });
    }
}
void Widget::mode06Bt_rightClicked()
{
    ui->mode06Bt->setStyleSheet("background-color: lightgray;");
    appendLog("编辑模式6", Qt::blue);
    openOrCreateTable("mode06.data");
}

