#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QByteArray>
#include <QMainWindow>
#include <QSerialPort>
#include <QStringList>
#include <QTimer>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
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
    void simulateRxFrame();
    void exportLogs();
    void handleReadyRead();
    void handleFrameTimeout();

private:
    void buildUi();
    void addParseRow(const QString &type, const QString &summary, const QString &detail);
    void appendLog(const QString &direction, const QString &message);
    QString csvEscape(const QString &value) const;
    QByteArray hexTextToFrame(const QString &text, bool *ok) const;
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
    QPushButton *simulateRxButton = nullptr;
    QPushButton *exportButton = nullptr;
    QLabel *statusLabel = nullptr;
    QLineEdit *simulateRxEdit = nullptr;
    QTableWidget *parseTable = nullptr;
    QTextEdit *logView = nullptr;

    QSerialPort serialPort;
    QByteArray receiveBuffer;
    QTimer frameTimer;
    QStringList csvRows;
};

#endif
