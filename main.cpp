#include "dataloader.h"
#include "ilsqn.h"
#include "parameters.h"

#include <fstream>

using namespace MyNest;

std::vector<std::string> fileNames = {
    "Dighe2", "Dighe1", "Fu", "Jakobs1", "Jakobs2", "Blaz", "Marques", "Shirts",
    "Swim", "Trousers", "Mao", "Albano", "Dagli", "Shapes0", "Shapes1", "ntx"};

static void initialParameters()
{
    pieces.clear();
    piecesCache.clear();
    nfpsCache.clear();
    ifpsCache.clear();
    ifrsCache.clear();
}

void test()
{
    DataLoader *dataloader = DataLoader::getInstance();
    std::ofstream fo("../result.csv"); // 创建并打开CSV文件
    fo << "Dataset" << ",";
    for (int i = 0; i < 10; ++i)
    {
        fo << i + 1 << ",";
    }
    fo << "Average" << std::endl;
    fo.close();
    for (auto &fileName : fileNames)
    {
        initialParameters();
        std::string filePath = "../parameters/" + fileName + ".txt";
        // 加载参数
        dataloader->loadParameters(filePath);
        // 加载零件
        dataloader->loadPieces();
        // 加载NoFitPolygon
        dataloader->loadNfps();
        // 运行主算法
        std::vector<double> ratios;
        for (int i = 0; i < 10; ++i)
        {
            ILSQN *ilsqn = ILSQN::getInstance();
            ratios.push_back(ilsqn->run());
        }
        if (ILSQN::ilsqn != nullptr)
        {
            delete ILSQN::ilsqn;
            ILSQN::ilsqn = nullptr;
        }
        double sum = 0.0;
        for (auto v : ratios)
        {
            sum += v;
        }
        std::ofstream fo("../result.csv", std::ios::app); // 打开CSV文件, 追加写入
        fo << fileName << ",";
        for (int i = 0; i < ratios.size(); ++i)
        {
            fo << ratios[i] << ",";
        }
        fo << sum / ratios.size() << std::endl;
        fo.close();
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::string fileName = argv[1];
    std::string filePath = "../parameters/" + fileName + ".txt";
    DataLoader *dataloader = DataLoader::getInstance();
    // 加载参数
    dataloader->loadParameters(filePath);
    // 加载零件
    dataloader->loadPieces();
    // 加载NoFitPolygon
    dataloader->loadNfps();
    // 运行主算法
    ILSQN *ilsqn = ILSQN::getInstance();
    std::vector<double> ratios;
    for (int i = 0; i < 5; ++i)
    {
        ratios.push_back(ilsqn->run());
    }
    double sum = 0.0;
    for (auto v : ratios)
    {
        sum += v;
        std::cout << v << std::endl;
    }
    std::cout << "平均利用率=" << sum / ratios.size() << std::endl;

    return 0;
}