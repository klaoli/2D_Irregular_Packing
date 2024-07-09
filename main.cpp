#include "dataloader.h"
#include "ilsqn.h"
#include "parameters.h"
using namespace MyNest;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
	std::cout << argv[1]<<std::endl;
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
    // 加载InnerFitPolygon
    dataloader->loadIfrs();
	
    // 运行主算法
	ILSQN *ilsqn = ILSQN::getInstance();
    ilsqn->run();

    return 0;
}