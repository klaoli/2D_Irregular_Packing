# 2D_Irregular_Packing
2d Irregular Strip Packing(An iterated local search algorithm based on nonlinear programming for the irregular strip )

依赖：
  Boost: https://www.boost.org
  Eigen3: https://eigen.tuxfamily.org/index.php?title=Main_Page

参考：
  lbfgs.hpp: https://github.com/ZJU-FAST-Lab/LBFGS-Lite
  ClipperLib: https://github.com/AngusJohnson/Clipper2/tree/main/CPP
  ClothCutting: https://github.com/zjl9959/ClothCutting
  
  论文：
    (1)<<An iterated local search algorithm based on nonlinear programming for the irregular strip>>
    (2)<<Extended local search algorithm based on nonlinear programming for two-dimensional irregular>>

实验结果（这里使用ESICUP提供的若干数据集来进行仿真）：
  后续更新实验结果......

执行代码(Linux平台)：
  (1) 安装boost：sudo apt-get install libboost-all-dev
  (2) 安装Eigen3: sudo apt-get install libeigen3-dev
  (3) mkdir build && cd build
  (4) cmake ..
  (5) make
  (6) ./ILSQN Jakobs1
