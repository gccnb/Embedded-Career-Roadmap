#include "mainwindow.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIODevice>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

#include "modbus_crc.h"
#include "modbus_frame.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();

    frameTimer.setSingleShot(true);
    frameTimer.setInterval(50);

    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(openButton, &QPushButton::clicked, this, &MainWindow::openOrClosePort);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendRequest);
    connect(simulateRxButton, &QPushButton::clicked, this, &MainWindow::simulateRxFrame);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportLogs);
    connect(&serialPort, &QSerialPort::readyRead, this, &MainWindow::handleReadyRead);
    connect(&frameTimer, &QTimer::timeout, this, &MainWindow::handleFrameTimeout);

    refreshPorts();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    auto *formLayout = new QFormLayout();
    auto *buttonLayout = new QHBoxLayout();
    auto *simulateLayout = new QHBoxLayout();

    portBox = new QComboBox(this);
    baudBox = new QComboBox(this);
    baudBox->addItems(QStringList() << "9600" << "19200" << "38400" << "115200");

    functionBox = new QComboBox(this);
    functionBox->addItem("03 Read Holding Registers", 0x03);
    functionBox->addItem("06 Write Single Register", 0x06);

    slaveSpin = new QSpinBox(this);
    slaveSpin->setRange(1, 247);
    slaveSpin->setValue(1);

    addressSpin = new QSpinBox(this);
    addressSpin->setRange(0, 65535);
    addressSpin->setDisplayIntegerBase(16);
    addressSpin->setPrefix("0x");

    valueSpin = new QSpinBox(this);
    valueSpin->setRange(1, 65535);
    valueSpin->setValue(2);

    refreshButton = new QPushButton("刷新串口", this);
    openButton = new QPushButton("打开串口", this);
    sendButton = new QPushButton("发送请求", this);
    simulateRxButton = new QPushButton("模拟 RX", this);
    exportButton = new QPushButton("导出日志", this);
    statusLabel = new QLabel("未打开", this);
    simulateRxEdit = new QLineEdit(this);
    simulateRxEdit->setPlaceholderText("输入响应帧，例如：01 03 04 00 19 00 64 2A 1F");
    logView = new QTextEdit(this);
    logView->setReadOnly(true);
    parseTable = new QTableWidget(this);
    parseTable->setColumnCount(4);
    parseTable->setHorizontalHeaderLabels(QStringList() << "时间" << "类型" << "摘要" << "详情");
    parseTable->horizontalHeader()->setStretchLastSection(true);
    parseTable->verticalHeader()->setVisible(false);
    parseTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    formLayout->addRow("串口号", portBox);
    formLayout->addRow("波特率", baudBox);
    formLayout->addRow("从机地址", slaveSpin);
    formLayout->addRow("功能码", functionBox);
    formLayout->addRow("寄存器地址", addressSpin);
    formLayout->addRow("数量/数值", valueSpin);

    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(sendButton);
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(statusLabel);
    buttonLayout->addStretch();

    simulateLayout->addWidget(simulateRxEdit);
    simulateLayout->addWidget(simulateRxButton);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(simulateLayout);
    mainLayout->addWidget(parseTable);
    mainLayout->addWidget(logView);

    setCentralWidget(central);
    setWindowTitle("Qt Modbus Debugger Demo");
    resize(860, 640);
}

void MainWindow::refreshPorts()
{
    portBox->clear();

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        portBox->addItem(info.portName());
    }

    appendLog("INFO", QString("发现 %1 个串口").arg(ports.size()));
}

void MainWindow::openOrClosePort()
{
    if (serialPort.isOpen()) {
        serialPort.close();
        openButton->setText("打开串口");
        statusLabel->setText("未打开");
        appendLog("INFO", "串口已关闭");
        return;
    }

    if (portBox->currentText().isEmpty()) {
        appendLog("ERROR", "没有可用串口");
        return;
    }

    serialPort.setPortName(portBox->currentText());
    serialPort.setBaudRate(baudBox->currentText().toInt());
    serialPort.setDataBits(QSerialPort::Data8);
    serialPort.setParity(QSerialPort::NoParity);
    serialPort.setStopBits(QSerialPort::OneStop);
    serialPort.setFlowControl(QSerialPort::NoFlowControl);

    if (!serialPort.open(QIODevice::ReadWrite)) {
        appendLog("ERROR", "串口打开失败：" + serialPort.errorString());
        return;
    }

    openButton->setText("关闭串口");
    statusLabel->setText("已打开");
    appendLog("INFO", "串口已打开：" + serialPort.portName());
}

void MainWindow::sendRequest()
{
    if (!serialPort.isOpen()) {
        appendLog("ERROR", "请先打开串口");
        return;
    }

    const quint8 slave = static_cast<quint8>(slaveSpin->value());
    const quint16 address = static_cast<quint16>(addressSpin->value());
    const quint16 value = static_cast<quint16>(valueSpin->value());
    const int functionCode = functionBox->currentData().toInt();

    QByteArray frame;
    if (functionCode == 0x03) {
        frame = buildReadHoldingRegistersFrame(slave, address, value);
    } else {
        frame = buildWriteSingleRegisterFrame(slave, address, value);
    }

    serialPort.write(frame);
    appendLog("TX", frameToHexText(frame));
}

void MainWindow::simulateRxFrame()
{
    bool ok = false;
    const QByteArray frame = hexTextToFrame(simulateRxEdit->text(), &ok);

    if (!ok) {
        appendLog("ERROR", "模拟 RX 输入无效，请输入偶数字节十六进制，例如：01 03 04 00 19 00 64 2A 1F");
        addParseRow("ERROR", "模拟 RX 输入无效", simulateRxEdit->text());
        return;
    }

    appendLog("RX-SIM", frameToHexText(frame));
    parseReceivedFrame(frame);
}

void MainWindow::exportLogs()
{
    if (csvRows.isEmpty()) {
        appendLog("INFO", "当前没有可导出的日志");
        return;
    }

    const QString fileName = QFileDialog::getSaveFileName(this,
        "导出日志",
        "modbus_debug_log.csv",
        "CSV Files (*.csv)");

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        appendLog("ERROR", "日志文件打开失败：" + file.errorString());
        return;
    }

    QTextStream stream(&file);
    stream << "time,direction,message\n";
    for (const QString &row : csvRows) {
        stream << row << "\n";
    }

    appendLog("INFO", "日志已导出：" + fileName);
}

void MainWindow::handleReadyRead()
{
    receiveBuffer.append(serialPort.readAll());
    frameTimer.start();
}

void MainWindow::handleFrameTimeout()
{
    if (receiveBuffer.isEmpty()) {
        return;
    }

    const QByteArray frame = receiveBuffer;
    receiveBuffer.clear();

    appendLog("RX", frameToHexText(frame));
    parseReceivedFrame(frame);
}

void MainWindow::parseReceivedFrame(const QByteArray &frame)
{
    if (!hasValidModbusCrc(frame)) {
        appendLog("ERROR", "CRC 校验失败，先检查字节顺序、漏字节和串口参数");
        addParseRow("ERROR", "CRC 校验失败", frameToHexText(frame));
        return;
    }

    if (frame.size() < 5) {
        appendLog("ERROR", "帧长度过短");
        addParseRow("ERROR", "帧长度过短", frameToHexText(frame));
        return;
    }

    const quint8 slave = static_cast<quint8>(frame.at(0));
    const quint8 functionCode = static_cast<quint8>(frame.at(1));

    if ((functionCode & 0x80) != 0) {
        const quint8 exceptionCode = static_cast<quint8>(frame.at(2));
        appendLog("PARSE", QString("从机 %1 返回异常响应，功能码 0x%2，异常码 0x%3")
                  .arg(static_cast<int>(slave))
                  .arg(static_cast<int>(functionCode), 2, 16, QLatin1Char('0'))
                  .arg(static_cast<int>(exceptionCode), 2, 16, QLatin1Char('0')));
        addParseRow("EXCEPTION",
                    QString("从机 %1 异常响应").arg(static_cast<int>(slave)),
                    QString("function=0x%1, exception=0x%2")
                        .arg(static_cast<int>(functionCode), 2, 16, QLatin1Char('0'))
                        .arg(static_cast<int>(exceptionCode), 2, 16, QLatin1Char('0')));
        return;
    }

    if (functionCode == 0x03 && frame.size() >= 6) {
        const quint8 byteCount = static_cast<quint8>(frame.at(2));
        appendLog("PARSE", QString("从机 %1 返回 03 响应，数据字节数：%2")
                  .arg(static_cast<int>(slave))
                  .arg(static_cast<int>(byteCount)));
        addParseRow("03",
                    QString("从机 %1 返回保持寄存器").arg(static_cast<int>(slave)),
                    QString("byteCount=%1, raw=%2")
                        .arg(static_cast<int>(byteCount))
                        .arg(frameToHexText(frame)));
        return;
    }

    if (functionCode == 0x06 && frame.size() >= 8) {
        const quint16 address = static_cast<quint16>((static_cast<quint8>(frame.at(2)) << 8)
            | static_cast<quint8>(frame.at(3)));
        const quint16 value = static_cast<quint16>((static_cast<quint8>(frame.at(4)) << 8)
            | static_cast<quint8>(frame.at(5)));

        appendLog("PARSE", QString("从机 %1 返回 06 响应，地址 0x%2，数值 0x%3")
                  .arg(static_cast<int>(slave))
                  .arg(address, 4, 16, QLatin1Char('0'))
                  .arg(value, 4, 16, QLatin1Char('0')));
        addParseRow("06",
                    QString("从机 %1 写单寄存器回显").arg(static_cast<int>(slave)),
                    QString("address=0x%1, value=0x%2")
                        .arg(address, 4, 16, QLatin1Char('0'))
                        .arg(value, 4, 16, QLatin1Char('0')));
        return;
    }

    appendLog("PARSE", "CRC 正确，但该功能码暂未做详细解析");
    addParseRow("PARSE", "CRC 正确，功能码暂未解析", frameToHexText(frame));
}

void MainWindow::addParseRow(const QString &type, const QString &summary, const QString &detail)
{
    const int row = parseTable->rowCount();
    const QString timeText = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    parseTable->insertRow(row);
    parseTable->setItem(row, 0, new QTableWidgetItem(timeText));
    parseTable->setItem(row, 1, new QTableWidgetItem(type));
    parseTable->setItem(row, 2, new QTableWidgetItem(summary));
    parseTable->setItem(row, 3, new QTableWidgetItem(detail));
    parseTable->scrollToBottom();
}

void MainWindow::appendLog(const QString &direction, const QString &message)
{
    const QString timeText = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    const QString line = QString("[%1] [%2] %3").arg(timeText, direction, message);
    logView->append(line);
    csvRows.append(QString("%1,%2,%3")
                   .arg(csvEscape(timeText), csvEscape(direction), csvEscape(message)));
}

QString MainWindow::csvEscape(const QString &value) const
{
    QString escaped = value;
    escaped.replace("\"", "\"\"");
    return "\"" + escaped + "\"";
}

QByteArray MainWindow::hexTextToFrame(const QString &text, bool *ok) const
{
    QString compact;

    for (const QChar ch : text) {
        const ushort code = ch.unicode();

        if (ch.isSpace()) {
            continue;
        }

        const bool isHex = (code >= '0' && code <= '9')
            || (code >= 'a' && code <= 'f')
            || (code >= 'A' && code <= 'F');

        if (!isHex) {
            *ok = false;
            return QByteArray();
        }

        compact.append(ch);
    }

    if (compact.isEmpty() || (compact.size() % 2) != 0) {
        *ok = false;
        return QByteArray();
    }

    *ok = true;
    return QByteArray::fromHex(compact.toLatin1());
}
