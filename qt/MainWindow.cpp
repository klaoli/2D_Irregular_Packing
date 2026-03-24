#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFont>
#include <QApplication>
#include "../parameters.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("ILSQN 2D 不规则排样 - 实时可视化");
    resize(1200, 750);
    setStyleSheet(R"(
        QMainWindow { background-color: #0f0f23; }
        QLabel { color: #e0e0e0; font-size: 13px; }
        QGroupBox {
            color: #00ff88;
            font-size: 14px;
            font-weight: bold;
            border: 1px solid #333;
            border-radius: 6px;
            margin-top: 10px;
            padding-top: 16px;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }
        QPushButton {
            background-color: #00ff88;
            color: #0f0f23;
            font-size: 14px;
            font-weight: bold;
            border: none;
            border-radius: 6px;
            padding: 10px 24px;
        }
        QPushButton:hover { background-color: #33ffaa; }
        QPushButton:disabled { background-color: #444; color: #888; }
        QPushButton#stopBtn {
            background-color: #e74c3c;
            color: white;
        }
        QPushButton#stopBtn:hover { background-color: #ff6b6b; }
        QComboBox {
            background-color: #1a1a2e;
            color: #e0e0e0;
            border: 1px solid #444;
            border-radius: 4px;
            padding: 6px 12px;
            font-size: 13px;
        }
        QComboBox QAbstractItemView {
            background-color: #1a1a2e;
            color: #e0e0e0;
            selection-background-color: #00ff88;
            selection-color: #0f0f23;
        }
    )");

    setupUI();
}

MainWindow::~MainWindow()
{
    if (m_workerThread && m_workerThread->isRunning()) {
        m_worker->stop();
        m_workerThread->quit();
        m_workerThread->wait(3000);
    }
}

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // ===== 左侧面板 =====
    QVBoxLayout *leftPanel = new QVBoxLayout();
    leftPanel->setSpacing(12);

    // 数据集选择
    QGroupBox *ctrlGroup = new QGroupBox("控制面板");
    QVBoxLayout *ctrlLayout = new QVBoxLayout(ctrlGroup);

    ctrlLayout->addWidget(new QLabel("选择数据集:"));
    m_datasetCombo = new QComboBox();
    QStringList datasets = {
        "Dighe2", "Dighe1", "Fu", "Jakobs1", "Jakobs2", "Blaz",
        "Marques", "Shirts", "Swim", "Trousers", "Mao",
        "Albano", "Dagli", "Shapes0", "Shapes1", "ntx"
    };
    m_datasetCombo->addItems(datasets);
    ctrlLayout->addWidget(m_datasetCombo);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_startBtn = new QPushButton("▶ 开始排样");
    m_stopBtn = new QPushButton("■ 停止");
    m_stopBtn->setObjectName("stopBtn");
    m_stopBtn->setEnabled(false);
    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_stopBtn);
    ctrlLayout->addLayout(btnLayout);

    leftPanel->addWidget(ctrlGroup);

    // 信息面板
    QGroupBox *infoGroup = new QGroupBox("运行信息");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);

    m_datasetLabel = new QLabel("数据集: -");
    m_piecesLabel = new QLabel("零件数: -");
    m_utilLabel = new QLabel("当前利用率: -");
    m_utilLabel->setStyleSheet("color: #00ff88; font-size: 20px; font-weight: bold;");
    m_statusLabel = new QLabel("状态: 就绪");

    infoLayout->addWidget(m_datasetLabel);
    infoLayout->addWidget(m_piecesLabel);
    infoLayout->addWidget(m_utilLabel);
    infoLayout->addWidget(m_statusLabel);

    leftPanel->addWidget(infoGroup);

    // 参数面板
    QGroupBox *paramGroup = new QGroupBox("算法参数");
    QVBoxLayout *paramLayout = new QVBoxLayout(paramGroup);

    m_maxRunTimeLabel = new QLabel("最大时间: -");
    m_maxIterationLabel = new QLabel("单次迭代: -");
    m_incLabel = new QLabel("膨胀系数(inc): -");
    m_decLabel = new QLabel("收缩系数(dec): -");

    paramLayout->addWidget(m_maxRunTimeLabel);
    paramLayout->addWidget(m_maxIterationLabel);
    paramLayout->addWidget(m_incLabel);
    paramLayout->addWidget(m_decLabel);

    leftPanel->addWidget(paramGroup);
    leftPanel->addStretch();

    // ===== 右侧画布 =====
    m_packingWidget = new PackingWidget();

    mainLayout->addLayout(leftPanel, 1);
    mainLayout->addWidget(m_packingWidget, 3);

    // 信号连接
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
}

void MainWindow::onStartClicked()
{
    m_startBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
    m_datasetCombo->setEnabled(false);

    QString dataset = m_datasetCombo->currentText();
    m_datasetLabel->setText("数据集: " + dataset);

    // 清空参数面板显示
    m_maxRunTimeLabel->setText("最大时间: -");
    m_maxIterationLabel->setText("单次迭代: -");
    m_incLabel->setText("膨胀系数(inc): -");
    m_decLabel->setText("收缩系数(dec): -");

    // 创建工作线程
    m_workerThread = new QThread(this);
    m_worker = new PackingWorker();
    m_worker->setDataset(dataset);
    m_worker->moveToThread(m_workerThread);

    // 连接信号
    connect(m_workerThread, &QThread::started, m_worker, &PackingWorker::run);
    connect(m_worker, &PackingWorker::parametersLoaded, this, &MainWindow::onParametersLoaded);
    connect(m_worker, &PackingWorker::layoutUpdated, this, &MainWindow::onLayoutUpdated);
    connect(m_worker, &PackingWorker::finished, this, &MainWindow::onFinished);
    connect(m_worker, &PackingWorker::statusMessage, this, &MainWindow::onStatusMessage);
    connect(m_worker, &PackingWorker::finished, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread->start();
}

void MainWindow::onStopClicked()
{
    if (m_worker) {
        m_worker->stop();
    }
    m_statusLabel->setText("状态: 正在停止...");
}

void MainWindow::onLayoutUpdated(const MyNest::box_t &bin,
                                  const std::vector<MyNest::Piece> &pieces,
                                  const std::vector<MyNest::Vector> &vectors,
                                  double utilization)
{
    m_packingWidget->updateLayout(bin, pieces, vectors, utilization);
    m_utilLabel->setText(QString("当前利用率: %1%").arg(utilization * 100.0, 0, 'f', 2));
    m_piecesLabel->setText(QString("零件数: %1").arg(pieces.size()));
}

void MainWindow::onFinished(double finalUtil)
{
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
    m_datasetCombo->setEnabled(true);
    m_utilLabel->setText(QString("最终利用率: %1%").arg(finalUtil * 100.0, 0, 'f', 2));
}

void MainWindow::onStatusMessage(const QString &msg)
{
    m_statusLabel->setText("状态: " + msg);
}

void MainWindow::onParametersLoaded()
{
    m_maxRunTimeLabel->setText(QString("最大时间: %1 s").arg(MyNest::parameters.maxRunTime));
    m_maxIterationLabel->setText(QString("单次迭代: %1").arg(MyNest::parameters.maxIteration));
    m_incLabel->setText(QString("膨胀系数(inc): %1").arg(MyNest::parameters.inc));
    m_decLabel->setText(QString("收缩系数(dec): %1").arg(MyNest::parameters.dec));
}
