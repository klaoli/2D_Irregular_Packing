#include "dataloader.h"

#include <sstream>
#include <fstream>
#include <cstring>


#include "parameters.h"

using namespace MyNest;

DataLoader* DataLoader::dataLoader = nullptr;

DataLoader::DataLoader() {

}

DataLoader* DataLoader::getInstance() {
	if (dataLoader == nullptr) {
		dataLoader = new DataLoader;
	}
	return dataLoader;
}

bool DataLoader::loadPieces() {

	std::string line;
	std::vector<double> v1, v2;   // �洢��һ�����ݡ��ڶ�������
	std::vector< std::vector<double> > v3;  // �����е����һ��
	std::ifstream fin(parameters.piecePath);
	if (!fin) {
		std::cerr << "Error: Pieces failed to open file." << std::endl;
		return false;
	}

	std::getline(fin, line);    // ��ȡ��һ������
	std::stringstream ss;
	ss << line;
	if (!ss.eof()) {
		double temp;
		while (ss >> temp)
			v1.push_back(temp);
	}

	std::getline(fin, line);    // ��ȡ�ڶ�������
	ss.clear();
	ss << line;
	if (!ss.eof()) {
		double temp;
		while (ss >> temp)
			v2.push_back(temp);
	}

	while (getline(fin, line)) {    // �����������һ��
		ss.clear();
		std::vector<double> tmp;
		ss << line;
		if (!ss.eof()) {
			double temp;
			while (ss >> temp)
				tmp.push_back(temp);
		}
		v3.push_back(tmp);
	}
	fin.close();

	int numPolys = v1[0];    // ����ε�����

	bg::set<bg::max_corner, 0>(bin, v2[0] * parameters.polygonScaleRate);
	bg::set<bg::max_corner, 1>(bin, v2[1] * parameters.polygonScaleRate);
	int typeId = 0;
	for (int i = 0; i < v3.size(); ++i) {
		Piece piece;
		polygon_t polygon;
		for (int j = 0; j < v3[i].size(); j += 2) {
			point_t p(v3[i][j] * parameters.polygonScaleRate, v3[i][j + 1] * parameters.polygonScaleRate);
			polygon.outer().push_back(p);
		}
		bg::correct(polygon); // �γɱպ϶����, �������ʱ�뻯
		piece.id = i;
		piece.polygon = polygon;
		piece.area = bg::area(polygon);
		piece.getEnvelope();
		if (i == 0) {
			piece.typeId = typeId;
			typeId++;
			pieces.push_back(piece);
			continue;
		}
		for (int j = 0; j < pieces.size(); ++j) {
			if (bg::equals(piece.polygon, pieces[j].polygon)) {
				piece.typeId = pieces[j].typeId;
				break;
			}
		}
		if (piece.typeId == -1) {
			piece.typeId = typeId;
			typeId++;
		}
		pieces.push_back(piece);
	}
	return numPolys == pieces.size();
}

bool DataLoader::loadNfps() {
	std::ifstream fin(parameters.nfpsPath, std::ios::in);
	if (!fin) {
		std::cerr << "Error: Nfps failed to open file." << std::endl;
		return false;
	}

	std::string line;

	std::getline(fin, line);   // �����ļ���һ��
	while (std::getline(fin, line)) {
		std::string str;
		std::stringstream ss(line);
		std::vector<std::string> lineArray;

		while (std::getline(ss, str, ',')) {
			lineArray.push_back(str);
		}

		double t;
		std::istringstream iss;
		std::vector<double> values;
		polygon_t nfp;

		iss.str(lineArray[1]);
		while (iss >> t) {
			values.push_back(t);
		}

		for (int j = 0; j < values.size(); j += 2) {
			nfp.outer().push_back(point_t(values[j], values[j + 1]));    //�⻷
		}
		nfpsCache.insert(std::pair<std::string, polygon_t>(lineArray[0], nfp));
	}
	return nfpsCache.size() > 0;
}

bool DataLoader::loadIfrs() {
	std::ifstream fin(parameters.ifpsPath, std::ios::in);
	if (!fin) {
		std::cerr << "Error: Ifrs failed to open file." << std::endl;
		return false;
	}

	std::string line;

	std::getline(fin, line);   // �����ļ���һ������
	while (std::getline(fin, line)) {
		std::string str;
		std::stringstream ss(line);
		std::vector<std::string> lineArray;

		while (std::getline(ss, str, ',')) {
			lineArray.push_back(str);
		}

		double t;
		std::istringstream iss;
		std::vector<double> values;
		polygon_t ifp;

		iss.str(lineArray[1]);
		while (iss >> t) {
			values.push_back(t);
		}

		for (int j = 0; j < values.size(); j += 2) {
			ifp.outer().push_back(point_t(values[j], values[j + 1]));    //�⻷
		}
		ifpsCache.insert(std::pair<std::string, polygon_t>(lineArray[0], ifp));
		box_t ifr;
		bg::envelope(ifp, ifr);
		ifrsCache.insert(std::pair<std::string, box_t>(lineArray[0], ifr));
	}
	return ifpsCache.size() > 0 && ifrsCache.size() > 0;
}
 
inline std::string trim(const std::string& str) {   // �����ַ���ǰ��Ŀհ��ַ��������ո��Ʊ���ͻ��з�
	std::size_t first = str.find_first_not_of(" \t\n\r");
	std::size_t last = str.find_last_not_of(" \t\n\r");
	if (first == std::string::npos || last == std::string::npos) {
		return "";
	}
	return str.substr(first, (last - first + 1));
}

bool DataLoader::loadParameters(const std::string& parametersPath) {
	std::ifstream fin(parametersPath, std::ios::in);
	if (!fin) {
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
	while (std::getline(fin, line)) {
		std::istringstream iss(line);
		std::string key;
		if (std::getline(iss, key, '=')) {
			std::string value;
			if (std::getline(iss, value)) {
				// �����հ��ַ�
				key = trim(key);
				value = trim(value);

				// ���ݼ�������ֵ���ṹ���Ա
				if (key.compare(minGap) == 0) {
					parameters.minGap = std::stod(value);
				}
				else if (key.compare(polygonScaleRate) == 0) {
					parameters.polygonScaleRate = std::stod(value);
				}
				else if (key.compare(piecePath) == 0) {
					parameters.piecePath = value;
				}
				else if (key.compare(nfpsPath) == 0) {
					parameters.nfpsPath = value;
				}
				else if (key.compare(ifpsPath) == 0) {
					parameters.ifpsPath = value;
				}
				else if (key.compare(resultPath) == 0) {
					parameters.resultPath = value;
				}
				else if (key.compare(maxRunTime) == 0) {
					parameters.maxRunTime = std::stod(value);
				}
				else if (key.compare(maxIteration) == 0) {
					parameters.maxIteration = std::stoi(value);
				}
				else if (key.compare(orientations) == 0) {
					parameters.orientations = std::stoi(value);
				}
				else if (key.compare(inc) == 0) {
					parameters.inc = std::stod(value);
				}
				else if (key.compare(dec) == 0) {
					parameters.dec = std::stod(value);
				}
			}
		}
	}

	fin.close();

	return true;

}