#include "dataloader.h"
#include "ilsqn.h"
#include "parameters.h"

#include <thread>
#include <atomic>
using namespace MyNest;

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::cout << argv[1] << std::endl;
    std::string fileName = argv[1];
    std::string filePath = "../parameters/" + fileName + ".txt";
    // std::cout << filePath<<std::endl;

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