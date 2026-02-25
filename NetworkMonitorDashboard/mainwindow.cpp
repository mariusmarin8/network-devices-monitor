#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //initializez pe pagina 0 (login page)
    if(ui->mainStack) {
        ui->mainStack->setCurrentIndex(0);
    }

    pollStep = 0;
    filterUdp = false;
    filterAgent = false;

    //setupuri tabeluri si diagrama
    setupTables();
    setupChart();

    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, [this](){
        qDebug() << "Conectat la server!";
        ui->statusbar->showMessage("Status: CONNECTED");
    });

    connect(socket, &QTcpSocket::disconnected, this, [this](){
        qDebug() << "Disconnected!";
        ui->statusbar->showMessage("Status: DISCONNECTED");
    });

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::handleSocketData);

    socket->connectToHost("18.193.119.127", 8080);

    pollTimer = new QTimer(this);
    connect(pollTimer, &QTimer::timeout, this, &MainWindow::onPollTimerTimeout);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onPollTimerTimeout()
{
    //verificare conexiune
    if(socket->state() != QAbstractSocket::ConnectedState)
        return;

    QString cmd = "";

    if (pollStep == 0) {
        if (!filterUdp)
            cmd = "{\"command\":\"GET_STATS\"}";
    }
    else if (pollStep == 1) {
        if(filterUdp) {
            cmd = "{\"command\":\"FILTER_LOGS\",\"ip\":\"" + ui->udp_ipCombo->currentText() +
                  "\",\"severity\":\"" + ui->udp_sevCombo->currentText() +
                  "\",\"search\":\"" + ui->udp_keyEdit->text() +
                  "\",\"limit\":" + QString::number(ui->udp_limitSpin->value()) + "}";
        } else {
            cmd = "{\"command\":\"GET_LOGS\"}";
        }
    }
    else if (pollStep == 2) {
        if (!filterAgent)
            cmd = "{\"command\":\"GET_METRICS\"}";
    }
    else if (pollStep == 3) {
        if(filterAgent) {
            cmd = "{\"command\":\"FILTER_AGENTS\",\"agent\":\"" + ui->agent_idCombo->currentText() +
                  "\",\"severity\":\"" + ui->agent_sevCombo->currentText() +
                  "\",\"search\":\"" + ui->agent_keyEdit->text() +
                  "\",\"limit\":" + QString::number(ui->agent_limitSpin->value()) + "}";
        } else {
            cmd = "{\"command\":\"GET_AGENT_ALERTS\"}";
        }
    }
    else if (pollStep == 4) {
        if(!filterUdp)
            cmd = "{\"command\":\"GET_SYSLOG_IPS\"}";
    }
    else if (pollStep == 5) {
        if(!filterAgent)
            cmd = "{\"command\":\"GET_AGENT_LIST\"}";
    }

    if(!cmd.isEmpty()) {
        socket->write(cmd.toUtf8());
    }


    pollStep++;
    if(pollStep > 5)
        pollStep = 0;
}

//butoane
void MainWindow::on_loginButton_clicked()
{
    QString user = ui->userEdit->text();
    QString pass = ui->passEdit->text();

    qDebug() << "Logare pentru: " << user;

    QString payload = "{\"command\":\"LOGIN\",\"user\":\"" + user + "\",\"pass\":\"" + pass + "\"}";
    socket->write(payload.toUtf8());
}

void MainWindow::on_udp_applyBtn_clicked()
{
    if (!filterUdp) {
        filterUdp = true;
        ui->udp_applyBtn->setText("Clear Filters");
    } else {
        filterUdp = false;
        ui->udp_applyBtn->setText("Apply UDP Filters");
    }

    pollStep = 1;


    onPollTimerTimeout();
}

void MainWindow::on_agent_applyBtn_clicked()
{
    if (!filterAgent) {
        filterAgent = true;
        ui->agent_applyBtn->setText("Clear Filters");
    } else {
        filterAgent = false;
        ui->agent_applyBtn->setText("Apply Agent Filters");
    }

    pollStep = 3;


    onPollTimerTimeout();
}

void MainWindow::handleSocketData()
{
    QByteArray data = socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if(doc.isNull()) return;

    QJsonObject root = doc.object();
    QString status = root["status"].toString();

    if(ui->mainStack->currentIndex() == 0) {
        if(status == "SUCCESS") {

            ui->mainStack->setCurrentIndex(1);
            pollTimer->start(500);
        } else {
            qDebug() << "Failed:" << root["message"].toString();
            ui->statusbar->showMessage("Login Error: " + root["message"].toString());
        }
        return;
    }

    if(root.contains("data")) {
        QJsonValue val = root["data"];

        if(val.isObject()) {
            QJsonObject obj = val.toObject();

            if(obj.contains("avg_cpu")) {
                updateMetrics(obj);
            }
            else if(obj.contains("total_logs")) {
                updateChart(obj["errors"].toInt(), obj["warnings"].toInt(), obj["info"].toInt(), obj["total_logs"].toInt());
            }
            else if(obj.contains("logs")) {
                updateSyslogTable(obj["logs"].toArray());
                if(obj.contains("stats")) {
                    QJsonObject s = obj["stats"].toObject();
                    updateChart(s["errors"].toInt(), s["warnings"].toInt(), s["info"].toInt(), s["total_logs"].toInt());
                }
            }
            else if(obj.contains("alerts")) {
                updateAgentTable(obj["alerts"].toArray());
                if(obj.contains("metrics")) {
                    updateMetrics(obj["metrics"].toObject());
                }
            }
        }
        else if (val.isArray()) {
            QJsonArray arr = val.toArray();
            if(arr.isEmpty()) return;

            QJsonValue firstItem = arr.at(0);

            if (firstItem.isString()) {
                QString txt = firstItem.toString();
                if (txt.contains('.')) {
                    updateFilterList(arr, "IP");
                } else {
                    updateFilterList(arr, "AGENT");
                }
            }
            else if (firstItem.isObject()) {
                QJsonObject firstObj = firstItem.toObject();
                if (firstObj.contains("agent")) {
                    updateAgentTable(arr);
                }
                else if (firstObj.contains("ip")) {
                    updateSyslogTable(arr);
                }
            }
        }
    }
}

void MainWindow::setupTables()
{
    ui->udpTable->setColumnCount(4);
    QStringList hSyslog;
    hSyslog << "Timestamp" << "Source IP" << "Severity" << "Message";
    ui->udpTable->setHorizontalHeaderLabels(hSyslog);
    ui->udpTable->horizontalHeader()->setStretchLastSection(true);
    ui->udpTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    ui->agentTable->setColumnCount(3);
    QStringList hAgent;
    hAgent << "Agent Name" << "Severity" << "Message";
    ui->agentTable->setHorizontalHeaderLabels(hAgent);
    ui->agentTable->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::setupChart()
{
    setErrors = new QBarSet("Errors"); setErrors->setColor(Qt::red);
    setWarnings = new QBarSet("Warnings"); setWarnings->setColor(Qt::yellow);
    setInfo = new QBarSet("Info"); setInfo->setColor(Qt::blue);

    *setErrors << 0; *setWarnings << 0; *setInfo << 0;

    seriesStats = new QBarSeries();
    seriesStats->append(setErrors);
    seriesStats->append(setWarnings);
    seriesStats->append(setInfo);
    seriesStats->setLabelsVisible(true);

    chartStats = new QChart();
    chartStats->addSeries(seriesStats);
    chartStats->setTitle("Event Statistics (%)");
    chartStats->setAnimationOptions(QChart::SeriesAnimations);
    chartStats->legend()->setAlignment(Qt::AlignBottom);

    axisY = new QValueAxis();
    chartStats->addAxis(axisY, Qt::AlignLeft);
    seriesStats->attachAxis(axisY);

    QChartView *view = new QChartView(chartStats);
    view->setRenderHint(QPainter::Antialiasing);

    if(ui->statsContainer->layout()) {
        delete ui->statsContainer->layout();
    }

    QVBoxLayout *layout = new QVBoxLayout(ui->statsContainer);
    layout->addWidget(view);
    layout->setContentsMargins(0,0,0,0);
}

void MainWindow::updateChart(int err, int warn, int info, int total)
{
    setErrors->replace(0, err);
    setWarnings->replace(0, warn);
    setInfo->replace(0, info);

    if (total > 0) {
        int pErr = (err * 100) / total;
        int pWarn = (warn * 100) / total;
        int pInfo = (info * 100) / total;

        setErrors->setLabel("Err: " + QString::number(err) + " (" + QString::number(pErr) + "%)");
        setWarnings->setLabel("Warn: " + QString::number(warn) + " (" + QString::number(pWarn) + "%)");
        setInfo->setLabel("Info: " + QString::number(info) + " (" + QString::number(pInfo) + "%)");
    } else {
        setErrors->setLabel("Err: 0");
        setWarnings->setLabel("Warn: 0");
        setInfo->setLabel("Info: 0");
    }

    int maxVal = err;
    if (warn > maxVal) maxVal = warn;
    if (info > maxVal) maxVal = info;

    if (maxVal == 0) {
        axisY->setRange(0, 10);
    } else {
        axisY->setRange(0, maxVal + (maxVal/5) + 2);
    }
}

void MainWindow::updateMetrics(const QJsonObject &obj)
{
    int cpu = obj["avg_cpu"].toInt();
    int ram = obj["avg_ram"].toInt();

    ui->cpuProgressBar->setValue(cpu);
    ui->ramLCD->display(ram);

    QString style = "QProgressBar::chunk { background-color: #2ecc71; }";
    if (cpu > 80)
        style = "QProgressBar::chunk { background-color: red; }";
    else if (cpu > 50)
        style = "QProgressBar::chunk { background-color: orange; }";

    ui->cpuProgressBar->setStyleSheet(style);
}

void MainWindow::updateSyslogTable(const QJsonArray &arr)
{
    ui->udpTable->setRowCount(0);

    for(int i = 0; i < arr.size(); i++) {
        QJsonObject row = arr.at(i).toObject();
        ui->udpTable->insertRow(i);

        QString sev = row["severity"].toString();
        QColor bg = Qt::white;

        if(sev.contains("ERR") || sev.contains("CRIT")) bg = QColor(255, 200, 200);
        else if(sev.contains("WARN")) bg = QColor(255, 255, 200);

        auto createItem = [&](QString txt) {
            QTableWidgetItem *item = new QTableWidgetItem(txt);
            item->setBackground(bg);
            return item;
        };

        ui->udpTable->setItem(i, 0, createItem(row["timestamp"].toString()));
        ui->udpTable->setItem(i, 1, createItem(row["ip"].toString()));
        ui->udpTable->setItem(i, 2, createItem(sev));
        ui->udpTable->setItem(i, 3, createItem(row["message"].toString()));
    }
}

void MainWindow::updateAgentTable(const QJsonArray &arr)
{
    ui->agentTable->setRowCount(0);

    for(int i = 0; i < arr.size(); i++) {
        QJsonObject row = arr.at(i).toObject();
        ui->agentTable->insertRow(i);

        QString sev = row["severity"].toString();
        QColor bg = Qt::white;

        if(sev == "CRITICAL" || sev == "ERROR")
            bg = QColor(255, 200, 200);
        else if(sev == "WARNING")
            bg = QColor(255, 255, 200);

        auto createItem = [&](QString txt) {
            QTableWidgetItem *item = new QTableWidgetItem(txt);
            item->setBackground(bg);
            return item;
        };

        ui->agentTable->setItem(i, 0, createItem(row["agent"].toString()));
        ui->agentTable->setItem(i, 1, createItem(sev));
        ui->agentTable->setItem(i, 2, createItem(row["message"].toString()));
    }
}

void MainWindow::updateFilterList(const QJsonArray &arr, const QString &type)
{
    QComboBox* box;

    if (type == "IP") {
        box = ui->udp_ipCombo;
    } else {
        box = ui->agent_idCombo;
    }

    QString currText = box->currentText();

    box->blockSignals(true);
    box->clear();
    box->addItem("ALL");

    for (int i = 0; i < arr.size(); i++) {
        box->addItem(arr.at(i).toString());
    }

    int idx = box->findText(currText);

    if (idx != -1) {
        box->setCurrentIndex(idx);
    } else {
        box->setCurrentIndex(0);
    }

    box->blockSignals(false);
}
