#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// Chart Includes
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

    QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_loginButton_clicked();
    void on_udp_applyBtn_clicked();
    void on_agent_applyBtn_clicked();


    void handleSocketData();

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;
    QTimer *pollTimer;


    int pollStep;
    bool filterUdp;
    bool filterAgent;

    QChart *chartStats;
    QBarSeries *seriesStats;
    QBarSet *setErrors;
    QBarSet *setWarnings;
    QBarSet *setInfo;
    QValueAxis *axisY;
    void onPollTimerTimeout();
    void setupTables();
    void setupChart();

    void updateChart(int err, int warn, int info, int total);
    void updateMetrics(const QJsonObject &obj);
    void updateSyslogTable(const QJsonArray &arr);
    void updateAgentTable(const QJsonArray &arr);
    void updateFilterList(const QJsonArray &arr, const QString &type);
};
#endif
