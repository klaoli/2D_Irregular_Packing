# 2D_Irregular_Packing
2d Irregular Strip Packing(An iterated local search algorithm based on nonlinear programming for the irregular strip )

依赖：<br>
  Boost: https://www.boost.org<br>
  Eigen3: https://eigen.tuxfamily.org/index.php?title=Main_Page<br>

参考：<br>
  lbfgs.hpp: https://github.com/ZJU-FAST-Lab/LBFGS-Lite<br>
  libnfporb: https://github.com/kallaballa/libnfporb<br>
  ClipperLib: https://github.com/AngusJohnson/Clipper2/tree/main/CPP<br>
  ClothCutting: https://github.com/zjl9959/ClothCutting<br>
  
  论文：<br>
    (1) An iterated local search algorithm based on nonlinear programming for the irregular strip<br>
    (2) Extended local search algorithm based on nonlinear programming for two-dimensional irregular<br>

实验结果（这里使用ESICUP提供的若干数据集来进行仿真）：<br>
  后续更新实验结果......<br>			
![image](https://github.com/user-attachments/assets/5e12dda5-fd2e-4dcf-baf1-ccadf91d2eaf)
![image](https://github.com/user-attachments/assets/cd05aae5-3114-4398-8a79-62fbc76c2690)



执行代码(Linux平台)：<br>
  (1) 安装boost：sudo apt-get install libboost-all-dev<br>
  (2) 安装Eigen3: sudo apt-get install libeigen3-dev<br>
  (3) mkdir build && cd build<br>
  (4) cmake ..<br>
  (5) make<br>
  (6) ./ILSQN<br>

  Email：lilinfeng3983@163.com
