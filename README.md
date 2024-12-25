# 2D_Irregular_Packing  
2d Irregular Strip Packing(An iterated local search algorithm based on nonlinear programming for the irregular strip )  

依赖：代码中使用了Boost库、Eigen库、Clipper库（include目录下是Clipper库的源码）  
  Boost: https://www.boost.org  
  Eigen3: https://eigen.tuxfamily.org/index.php?title=Main_Page

参考代码：  
  lbfgs.hpp: https://github.com/ZJU-FAST-Lab/LBFGS-Lite  
  libnfporb: https://github.com/kallaballa/libnfporb  
  ClothCutting: https://github.com/zjl9959/ClothCutting
  
参考论文：  
  (1) An iterated local search algorithm based on nonlinear programming for the irregular strip  
  (2) Extended local search algorithm based on nonlinear programming for two-dimensional irregular

实验结果（这里使用ESICUP提供的若干数据集来进行仿真）：  
  parameters目录下是每个数据集的执行参数  
  nfpsCache目录下是NoFitPolygon缓存数据  
  result.csv保存了计算结果，每个数据集计算十次

执行代码(Linux平台)：  
  (1) 安装boost：sudo apt-get install libboost-all-dev  
  (2) 安装Eigen3: sudo apt-get install libeigen3-dev  
  (3) mkdir build && cd build  
  (4) cmake ..  
  (5) make  
  (6) ./ILSQN Jakobs1  

  Email：lilinfeng3983@163.com  
