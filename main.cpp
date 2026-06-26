#include "dataloader.h"
#include "ilsqn.h"
#include "parameters.h"

#include <fstream>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace MyNest;

std::vector<std::string> fileNames = {
    "Dighe2", "Dighe1", "Fu", "Jakobs1", "Jakobs2", "Blaz", "Marques", "Shirts",
    "Swim", "Trousers", "Mao", "Albano", "Dagli", "Shapes0", "Shapes1", "ntx"};

static void initialParameters()
{
    if (ILSQN::ilsqn != nullptr)
    {
        delete ILSQN::ilsqn;
        ILSQN::ilsqn = nullptr;
    }
    pieces.clear();
    piecesCache.clear();
    nfpsCache.clear();
    ifpsCache.clear();
    ifrsCache.clear();
}

struct CliOptions
{
    std::string dataset;
    bool hasSeed = false;
    uint32_t seed = 0;
};

static void printUsage(const char *program)
{
    std::cerr << "Usage: " << program << " <filename|all> [--seed <uint>]" << std::endl;
}

static bool parseArgs(int argc, char *argv[], CliOptions &options)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--seed")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Error: --seed requires a value." << std::endl;
                return false;
            }
            try
            {
                unsigned long parsedSeed = std::stoul(argv[++i]);
                options.seed = static_cast<uint32_t>(parsedSeed);
                options.hasSeed = true;
            }
            catch (const std::exception &)
            {
                std::cerr << "Error: --seed must be an unsigned integer." << std::endl;
                return false;
            }
        }
        else if (options.dataset.empty())
        {
            options.dataset = arg;
        }
        else
        {
            std::cerr << "Error: unexpected argument: " << arg << std::endl;
            return false;
        }
    }
    return !options.dataset.empty();
}

static void applySeed(const CliOptions &options, int datasetIndex = 0, int runIndex = 0)
{
    parameters.hasRandomSeed = options.hasSeed;
    if (options.hasSeed)
    {
        parameters.randomSeed = options.seed + static_cast<uint32_t>(datasetIndex * 1009 + runIndex);
    }
}

static bool loadDataset(const std::string &fileName)
{
    DataLoader *dataloader = DataLoader::getInstance();
    std::string filePath = "../parameters/" + fileName + ".txt";
    if (!dataloader->loadParameters(filePath))
    {
        return false;
    }
    if (!dataloader->loadPieces())
    {
        return false;
    }
    if (!dataloader->loadNfps())
    {
        std::cout << "NFP cache unavailable or invalid; it will be regenerated." << std::endl;
    }
    return true;
}

static bool runDataset(const std::string &fileName, double &ratio)
{
    initialParameters();
    if (!loadDataset(fileName))
    {
        std::cerr << "Error: failed to load dataset: " << fileName << std::endl;
        return false;
    }
    ILSQN *ilsqn = ILSQN::getInstance();
    ratio = ilsqn->run();
    if (ILSQN::ilsqn != nullptr)
    {
        delete ILSQN::ilsqn;
        ILSQN::ilsqn = nullptr;
    }
    return true;
}

void test(const CliOptions &options)
{
    std::ofstream fo("../result.csv"); // 创建并打开CSV文件
    fo << "Dataset,";
    for (int i = 0; i < 10; ++i)
    {
        fo << "Run_" << i + 1 << ",";
    }
    fo << "Average,Variance,Max,Min" << std::endl;
    fo.close();

    for (int datasetIndex = 0; datasetIndex < fileNames.size(); ++datasetIndex)
    {
        const auto &fileName = fileNames[datasetIndex];
        std::cout << "========== Testing Dataset: " << fileName << " ==========" << std::endl;
        std::vector<double> ratios;
        for (int i = 0; i < 10; ++i)
        {
            std::cout << "  -> Run " << i + 1 << "/10..." << std::endl;
            applySeed(options, datasetIndex, i);
            double ratio = 0.0;
            if (!runDataset(fileName, ratio))
            {
                break;
            }
            ratios.push_back(ratio);
        }

        std::ofstream fo_app("../result.csv", std::ios::app);
        fo_app << fileName << ",";
        if (ratios.empty())
        {
            for (int i = 0; i < 14; ++i)
            {
                if (i > 0)
                {
                    fo_app << ",";
                }
            }
            fo_app << std::endl;
            fo_app.close();
            continue;
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
        for (int i = 0; i < 10; ++i)
        {
            if (i < ratios.size())
            {
                fo_app << ratios[i];
            }
            fo_app << ",";
        }
        fo_app << average << "," << variance << "," << max_val << "," << min_val << std::endl;
        fo_app.close();
    }
}

int main(int argc, char *argv[])
{
    CliOptions options;
    if (!parseArgs(argc, argv, options))
    {
        printUsage(argv[0]);
        return 1;
    }

    std::string fileName = options.dataset;
    if (fileName == "all")
    {
        test(options);
        return 0;
    }

    applySeed(options);
    double ratio = 0.0;
    if (!runDataset(fileName, ratio))
    {
        return 1;
    }

    return 0;
}
