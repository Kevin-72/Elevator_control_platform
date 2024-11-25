#include "widget.h"
#include "ui_widget.h"

#include <fstream>
#include <sstream>
#include <string>

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
    // 打开文件，如果文件不存在则创建
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "无法保存文件：" + filePath + "\n错误信息: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    // 保存表格数据
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        QStringList rowData;
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            QTableWidgetItem *item = tableWidget->item(row, col);
            QString cellData = item ? item->text().trimmed() : ""; // 去除空格
            rowData.append(item ? item->text() : "");
        }
        out << rowData.join(",") << "\n";
    }

    // 保存循环次数
    loop_count = loopEdit->text().toInt();
    out << "LoopCount=" << loop_count << "\n";

    file.close();
}

bool TableEditor::validateTableData(Widget *logWidget) {
    bool hasError = false;

    // 遍历每一行
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
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

void Widget::execOrCreateTable(const QString &fileName) {
    // 获取用户文档目录路径
    QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    // 拼接文件路径
    QString filePath = documentsDir + "/Elevator/" + fileName;

    // 打开或创建表格编辑窗口
    TableEditor editor(filePath, this);
    if (editor.validateTableData(this)) {
        on_mode01Bt_rightClicked();
    }
    else {
        editor.printTableDataToLog(this);
    }

}





bool Widget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::RightButton) {
            if (watched == ui->mode01Bt) {
                on_mode01Bt_rightClicked();
            } else if (watched == ui->mode02Bt) {
                on_mode02Bt_rightClicked();
            } else if (watched == ui->mode03Bt) {
                on_mode03Bt_rightClicked();
            } else if (watched == ui->mode04Bt) {
                on_mode04Bt_rightClicked();
            } else if (watched == ui->mode05Bt) {
                on_mode05Bt_rightClicked();
            } else if (watched == ui->mode06Bt) {
                on_mode06Bt_rightClicked();
            }
            return true;  // 事件已处理
        }
    }
    return QWidget::eventFilter(watched, event);  // 保留默认处理
}

void Widget::on_mode01Bt_clicked()
{
//     serialPort->write("MODE=01\r\n");
    appendLog("启动模式1");
    // 获取用户文档目录路径
    execOrCreateTable("mode01.data");
}
void Widget::on_mode01Bt_rightClicked()
{
    appendLog("右键点击模式1按钮", Qt::blue);
    openOrCreateTable("mode01.data");
}


void Widget::on_mode02Bt_clicked()
{
//     serialPort->write("MODE=02\r\n");
    appendLog("启动模式2");
}
void Widget::on_mode02Bt_rightClicked()
{
    appendLog("右键点击模式2按钮", Qt::blue);
    openOrCreateTable("mode2.data");
}

void Widget::on_mode03Bt_clicked()
{
//     serialPort->write("MODE=03\r\n");
    appendLog("启动模式3");
}
void Widget::on_mode03Bt_rightClicked()
{
    appendLog("右键点击模式3按钮", Qt::blue);
    openOrCreateTable("mode3.data");
}

void Widget::on_mode04Bt_clicked()
{
//     serialPort->write("MODE=04\r\n");
    appendLog("启动模式4");
}
void Widget::on_mode04Bt_rightClicked()
{
    appendLog("右键点击模式4按钮", Qt::blue);
    openOrCreateTable("mode4.data");
}

void Widget::on_mode05Bt_clicked()
{
//     serialPort->write("MODE=05\r\n");
    appendLog("启动模式5");
}
void Widget::on_mode05Bt_rightClicked()
{
    appendLog("右键点击模式5按钮", Qt::blue);
    openOrCreateTable("mode5.data");
}

void Widget::on_mode06Bt_clicked()
{
    appendLog("启动模式6");
}
void Widget::on_mode06Bt_rightClicked()
{
    appendLog("右键点击模式6按钮", Qt::blue);
    openOrCreateTable("mode6.data");
}


