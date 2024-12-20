#include "dataloader.h"

#include <sstream>
#include <fstream>
#include <cstring>

#include "parameters.h"

using namespace MyNest;

DataLoader *DataLoader::dataLoader = nullptr;

DataLoader::DataLoader()
{
}

DataLoader *DataLoader::getInstance()
{
	if (dataLoader == nullptr)
	{
		dataLoader = new DataLoader;
	}
	return dataLoader;
}

bool DataLoader::loadPieces()
{
	std::string line;
	std::vector<double> v1, v2;			 // 存储第一行数据、第二行数据
	std::vector<std::vector<double>> v3; // 第三行到最后一行
	std::ifstream fin(parameters.piecePath);
	if (!fin)
	{
		std::cerr << "Error: Pieces failed to open file." << std::endl;
		return false;
	}

	std::getline(fin, line); // 获取第一行数据
	std::stringstream ss;
	ss << line;
	if (!ss.eof())
	{
		double temp;
		while (ss >> temp)
			v1.push_back(temp);
	}

	std::getline(fin, line); // 读取第二行数据
	ss.clear();
	ss << line;
	if (!ss.eof())
	{
		double temp;
		while (ss >> temp)
			v2.push_back(temp);
	}

	while (getline(fin, line))
	{ // 第三行至最后一行
		ss.clear();
		std::vector<double> tmp;
		ss << line;
		if (!ss.eof())
		{
			double temp;
			while (ss >> temp)
				tmp.push_back(temp);
		}
		v3.push_back(tmp);
	}
	fin.close();

	int numPolys = v1[0]; // 多边形的数量

	bg::set<bg::max_corner, 0>(bin, v2[0] * parameters.polygonScaleRate);
	bg::set<bg::max_corner, 1>(bin, v2[1] * parameters.polygonScaleRate);
	int typeId = 0;
	static Geometry *geo = Geometry::getInstance();
	for (int i = 0; i < v3.size(); ++i)
	{
		Piece piece;
		polygon_t polygon;
		for (int j = 0; j < v3[i].size(); j += 2)
		{
			point_t p(v3[i][j] * parameters.polygonScaleRate, v3[i][j + 1] * parameters.polygonScaleRate);
			polygon.outer().push_back(p);
		}
		bg::correct(polygon); // 形成闭合多边形, 多边形逆时针化

		piece.id = i;
		// std::cout << "简化前后多边形定点数：" << polygon.outer().size() << ",";
		polygon_t simlipfiedPolygon = geo->simplifyPolygon(polygon, 0.01, 0.1);
		// std::cout << simlipfiedPolygon.outer().size() << std::endl;
		piece.polygon = simlipfiedPolygon;
		piece.area = bg::area(simlipfiedPolygon);
		piece.getEnvelope();
		if (i == 0)
		{
			piece.typeId = typeId;
			typeId++;
			pieces.push_back(piece);
			continue;
		}
		for (int j = 0; j < pieces.size(); ++j)
		{
			if (bg::equals(piece.polygon, pieces[j].polygon))
			{
				piece.typeId = pieces[j].typeId;
				break;
			}
		}
		if (piece.typeId == -1)
		{
			piece.typeId = typeId;
			typeId++;
		}
		pieces.push_back(piece);
	}
	return numPolys == pieces.size();
}

bool DataLoader::loadNfps()
{
	std::ifstream fin(parameters.nfpsPath, std::ios::in);
	if (!fin)
	{
		std::cerr << "Error: Nfps failed to open file." << std::endl;
		return false;
	}

	std::string line;

	std::getline(fin, line); // 跳过文件第一行
	while (std::getline(fin, line))
	{
		std::string str;
		std::stringstream ss(line);
		std::vector<std::string> lineArray;

		while (std::getline(ss, str, ','))
		{
			lineArray.push_back(str);
		}

		double t;
		std::istringstream iss;
		std::vector<double> values;
		polygon_t nfp;

		iss.str(lineArray[1]);
		while (iss >> t)
		{
			values.push_back(t);
		}

		for (int j = 0; j < values.size(); j += 2)
		{
			nfp.outer().push_back(point_t(values[j], values[j + 1])); // �⻷
		}
		nfpsCache.insert(std::pair<std::string, polygon_t>(lineArray[0], nfp));
	}
	return nfpsCache.size() > 0;
}

bool DataLoader::loadIfrs()
{
	std::ifstream fin(parameters.ifpsPath, std::ios::in);
	if (!fin)
	{
		std::cerr << "Error: Ifrs failed to open file." << std::endl;
		return false;
	}

	std::string line;

	std::getline(fin, line); // 跳过文件第一行内容
	while (std::getline(fin, line))
	{
		std::string str;
		std::stringstream ss(line);
		std::vector<std::string> lineArray;

		while (std::getline(ss, str, ','))
		{
			lineArray.push_back(str);
		}

		double t;
		std::istringstream iss;
		std::vector<double> values;
		polygon_t ifp;

		iss.str(lineArray[1]);
		while (iss >> t)
		{
			values.push_back(t);
		}

		for (int j = 0; j < values.size(); j += 2)
		{
			ifp.outer().push_back(point_t(values[j], values[j + 1])); // �⻷
		}
		ifpsCache.insert(std::pair<std::string, polygon_t>(lineArray[0], ifp));
		box_t ifr;
		bg::envelope(ifp, ifr);
		ifrsCache.insert(std::pair<std::string, box_t>(lineArray[0], ifr));
	}
	return ifpsCache.size() > 0 && ifrsCache.size() > 0;
}

inline std::string trim(const std::string &str)
{ // 修整字符串前后的空白字符，包括空格、制表符和换行符
	std::size_t first = str.find_first_not_of(" \t\n\r");
	std::size_t last = str.find_last_not_of(" \t\n\r");
	if (first == std::string::npos || last == std::string::npos)
	{
		return "";
	}
	return str.substr(first, (last - first + 1));
}

bool DataLoader::loadParameters(const std::string &parametersPath)
{
	std::ifstream fin(parametersPath, std::ios::in);
	if (!fin)
	{
		std::cerr << "Error: Parameters failed to open file." << std::endl;
		return false;
	}

	static std::string minGap = "minGap";
	static std::string polygonScaleRate = "polygonScaleRate";
	static std::string piecePath = "piecePath";
	static std::string nfpsPath = "nfpsPath";
	static std::string ifpsPath = "ifpsPath";
	static std::string resultPath = "resultPath";
	static std::string maxRunTime = "maxRunTime";
	static std::string maxIteration = "maxIteration";
	static std::string orientations = "orientations";
	static std::string inc = "inc";
	static std::string dec = "dec";

	std::string line;
	while (std::getline(fin, line))
	{
		std::istringstream iss(line);
		std::string key;
		if (std::getline(iss, key, '='))
		{
			std::string value;
			if (std::getline(iss, value))
			{
				// 修整空白字符
				key = trim(key);
				value = trim(value);

				// 根据键名分配值给结构体成员
				if (key.compare(minGap) == 0)
				{
					parameters.minGap = std::stod(value);
				}
				else if (key.compare(polygonScaleRate) == 0)
				{
					parameters.polygonScaleRate = std::stod(value);
				}
				else if (key.compare(piecePath) == 0)
				{
					parameters.piecePath = value;
				}
				else if (key.compare(nfpsPath) == 0)
				{
					parameters.nfpsPath = value;
				}
				else if (key.compare(ifpsPath) == 0)
				{
					parameters.ifpsPath = value;
				}
				else if (key.compare(resultPath) == 0)
				{
					parameters.resultPath = value;
				}
				else if (key.compare(maxRunTime) == 0)
				{
					parameters.maxRunTime = std::stod(value);
				}
				else if (key.compare(maxIteration) == 0)
				{
					parameters.maxIteration = std::stoi(value);
				}
				else if (key.compare(orientations) == 0)
				{
					parameters.orientations = std::stoi(value);
				}
				else if (key.compare(inc) == 0)
				{
					parameters.inc = std::stod(value);
				}
				else if (key.compare(dec) == 0)
				{
					parameters.dec = std::stod(value);
				}
			}
		}
	}

	fin.close();

	return true;
}