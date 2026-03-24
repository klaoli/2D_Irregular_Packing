#include "PackingWorker.h"
#include "../dataloader.h"
#include "../ilsqn.h"
#include "../parameters.h"

using namespace MyNest;

PackingWorker::PackingWorker(QObject *parent)
    : QObject(parent), m_stopFlag(false)
{
}

void PackingWorker::setDataset(const QString &name)
{
    m_datasetName = name;
}

void PackingWorker::stop()
{
    m_stopFlag = true;
    // 通知 ILSQN 停止
    if (ILSQN::ilsqn) {
        ILSQN::ilsqn->stopRequested = true;
    }
}

void PackingWorker::run()
{
    m_stopFlag = false;

    // 清理全局状态
    pieces.clear();
    piecesCache.clear();
    nfpsCache.clear();
    ifpsCache.clear();
    ifrsCache.clear();

    std::string filePath = "../parameters/" + m_datasetName.toStdString() + ".txt";
    DataLoader *dataloader = DataLoader::getInstance();

    emit statusMessage("正在加载参数...");
    dataloader->loadParameters(filePath);
    emit parametersLoaded();

    emit statusMessage("正在加载零件数据...");
    dataloader->loadPieces();

    emit statusMessage("正在加载 NFP 缓存...");
    dataloader->loadNfps();

    emit statusMessage("排样算法运行中...");

    ILSQN *ilsqn = ILSQN::getInstance();
    ilsqn->stopRequested = false;

    // 设置回调：当布局更新时通过信号通知 GUI
    ilsqn->onLayoutUpdated = [this](const box_t &bin,
                                     const std::vector<Piece> &pieces,
                                     const std::vector<Vector> &vectors,
                                     double util) {
        emit layoutUpdated(bin, pieces, vectors, util);
    };

    double result = ilsqn->run();

    // 清理
    ilsqn->onLayoutUpdated = nullptr;
    if (ILSQN::ilsqn != nullptr) {
        delete ILSQN::ilsqn;
        ILSQN::ilsqn = nullptr;
    }

    emit statusMessage(QString("排样完成！最终利用率: %1%").arg(result * 100.0, 0, 'f', 2));
    emit finished(result);
}
