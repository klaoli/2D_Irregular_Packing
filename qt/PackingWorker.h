#pragma once

#include <QObject>
#include <QThread>
#include "../piece.h"
#include "../vector.h"
#include "../geometry.h"

class PackingWorker : public QObject
{
    Q_OBJECT

public:
    explicit PackingWorker(QObject *parent = nullptr);
    void setDataset(const QString &name);

signals:
    void layoutUpdated(const MyNest::box_t &bin,
                       const std::vector<MyNest::Piece> &pieces,
                       const std::vector<MyNest::Vector> &vectors,
                       double utilization);
    void parametersLoaded();
    void finished(double finalUtil);
    void statusMessage(const QString &msg);

public slots:
    void run();
    void stop();

private:
    QString m_datasetName;
    bool m_stopFlag = false;
};
