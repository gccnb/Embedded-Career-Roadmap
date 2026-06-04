#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QByteArray>
#include <QMainWindow>
#include <QSerialPort>
#include <QTimer>

class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refreshPorts();
    void openOrClosePort();
    void sendRequest();
    void handleReadyRead();
    void handleFrameTimeout();

private:
    void buildUi();
    void appendLog(const QString &direction, const QString &message);
    void parseReceivedFrame(const QByteArray &frame);

    QComboBox *portBox = nullptr;
    QComboBox *baudBox = nullptr;
    QComboBox *functionBox = nullptr;
    QSpinBox *slaveSpin = nullptr;
    QSpinBox *addressSpin = nullptr;
    QSpinBox *valueSpin = nullptr;
    QPushButton *refreshButton = nullptr;
    QPushButton *openButton = nullptr;
    QPushButton *sendButton = nullptr;
    QLabel *statusLabel = nullptr;
    QTextEdit *logView = nullptr;

    QSerialPort serialPort;
    QByteArray receiveBuffer;
    QTimer frameTimer;
};

#endif
