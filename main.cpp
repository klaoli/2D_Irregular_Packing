#include "dataloader.h"
#include "ilsqn.h"
#include "parameters.h"

#include <fstream>
#include <numeric>
#include <cmath>
#include <algorithm>

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
    fo << "Dataset,";
    for (int i = 0; i < 10; ++i)
    {
        fo << "Run_" << i + 1 << ",";
    }
    fo << "Average,Variance,Max,Min" << std::endl;
    fo.close();

    for (const auto &fileName : fileNames)
    {
        std::cout << "========== Testing Dataset: " << fileName << " ==========" << std::endl;
        initialParameters();
        std::string filePath = "../parameters/" + fileName + ".txt";
        
        // 加载参数、零件、NoFitPolygon
        dataloader->loadParameters(filePath);
        dataloader->loadPieces();
        dataloader->loadNfps();
        
        // 运行主算法
        std::vector<double> ratios;
        for (int i = 0; i < 10; ++i)
        {
            std::cout << "  -> Run " << i + 1 << "/10..." << std::endl;
            ILSQN *ilsqn = ILSQN::getInstance();
            ratios.push_back(ilsqn->run());
        }
        
        if (ILSQN::ilsqn != nullptr)
        {
            delete ILSQN::ilsqn;
            ILSQN::ilsqn = nullptr;
        }

        // 统计计算
        double sum = std::accumulate(ratios.begin(), ratios.end(), 0.0);
        double average = sum / ratios.size();
        
        double variance_sum = 0.0;
        for (double v : ratios) {
            variance_sum += (v - average) * (v - average);
        }
        double variance = variance_sum / ratios.size();
        
        double max_val = *std::max_element(ratios.begin(), ratios.end());
        double min_val = *std::min_element(ratios.begin(), ratios.end());

        // 追加写入CSV文件
        std::ofstream fo_app("../result.csv", std::ios::app);
        fo_app << fileName << ",";
        for (int i = 0; i < ratios.size(); ++i)
        {
            fo_app << ratios[i] << ",";
        }
        fo_app << average << "," << variance << "," << max_val << "," << min_val << std::endl;
        fo_app.close();
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>  (or use 'all' to run all datasets 10 times)" << std::endl;
        return 1;
    }
    
    std::string fileName = argv[1];
    if (fileName == "all") 
    {
        test();
        return 0;
    }

    std::string filePath = "../parameters/" + fileName + ".txt";
    DataLoader *dataloader = DataLoader::getInstance();
    dataloader->loadParameters(filePath);
    dataloader->loadPieces();
    dataloader->loadNfps();
    
    ILSQN *ilsqn = ILSQN::getInstance();
    std::vector<double> ratios;
    for (int i = 0; i < 1; ++i) // 单次运行展示
    {
        ratios.push_back(ilsqn->run());
    }
    
    return 0;
}