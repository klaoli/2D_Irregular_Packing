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
  Jakobs1	Jakobs2	Fu	Dighe1	Dighe2	Marques	Blaz	Shapes0	Shapes1	Mao	Dagli	Albano	Shirts	Swim	Trousers
0.886882	0.803784	0.918824	0.910252	0.999999	0.884664	0.820401	0.646132	0.716652		0.853041				
0.825524	0.812651	0.904637	0.859908	0.999999	0.88286	0.806703	0.65326	0.734047		0.864875				
0.886882	0.823295	0.918589	0.915374	0.999999	0.879489	0.819564	0.653927	0.731056		0.856311				
0.883269	0.80399	0.909727	0.856404	0.999999	0.880161	0.812279	0.655263	0.731803		0.860693				
0.885073	0.803784	0.899801	0.863204	0.999999	0.88715	0.822917	0.654595	0.71592		0.854565				
0.849671	0.80399	0.896135	0.999999	0.999999	0.888283	0.823758	0.659119	0.734047		0.862451				
0.880794	0.80399	0.910656	0.915609	0.999999	0.887377	0.813108	0.646132	0.731803		0.8693				
0.878101	0.832377	0.895221	0.999999	0.999999	0.883087	0.823758	0.642352	0.724557		0.856311				
0.886882	0.80399	0.920938	0.915609	0.999999	0.873227	0.815602	0.65326	0.718116		0.864214				
0.816517	0.804605	0.918824	0.999999	0.999999	0.875905	0.816435	0.665881	0.736298		0.868413				
0.8679595	0.8096456	0.9093352	0.9210525	0.999999	0.8822203	0.8174525	0.6529921	0.7274299		0.8610174				
![Uploading image.png…]()


执行代码(Linux平台)：<br>
  (1) 安装boost：sudo apt-get install libboost-all-dev<br>
  (2) 安装Eigen3: sudo apt-get install libeigen3-dev<br>
  (3) mkdir build && cd build<br>
  (4) cmake ..<br>
  (5) make<br>
  (6) ./ILSQN Jakobs1<br>
