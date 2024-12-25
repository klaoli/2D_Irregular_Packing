#ifndef DATALOADER_H
#define DATALOADER_H
#include <string>
namespace MyNest
{

	class DataLoader
	{
	public:
		static DataLoader *dataLoader;
		static DataLoader *getInstance();

		bool loadNfps();
		// bool loadIfrs();
		bool loadPieces();
		bool loadParameters(const std::string &);

	private:
		DataLoader();
		DataLoader(const DataLoader &) = delete;
		void operator=(const DataLoader &) = delete;
	};
}

#endif // DATALOADER_H
