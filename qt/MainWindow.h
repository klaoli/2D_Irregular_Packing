#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QThread>
#include "PackingWidget.h"
#include "PackingWorker.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartClicked();
    void onStopClicked();
    void onLayoutUpdated(const MyNest::box_t &bin,
                         const std::vector<MyNest::Piece> &pieces,
                         const std::vector<MyNest::Vector> &vectors,
                         double utilization);
    void onFinished(double finalUtil);
    void onStatusMessage(const QString &msg);
    void onParametersLoaded();

private:
    void setupUI();

    // UI 组件
    PackingWidget *m_packingWidget;
    QComboBox *m_datasetCombo;
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;
    QLabel *m_statusLabel;
    QLabel *m_utilLabel;
    QLabel *m_datasetLabel;
    QLabel *m_piecesLabel;

    // 参数面板组件
    QLabel *m_maxRunTimeLabel;
    QLabel *m_maxIterationLabel;
    QLabel *m_incLabel;
    QLabel *m_decLabel;

    // 工作线程
    QThread *m_workerThread = nullptr;
    PackingWorker *m_worker = nullptr;
};
