#include "HXLibConfigReader.h"
#include "HXLibNetwork.h"
#include <boost/program_options/detail/utf8_codecvt_facet.hpp>

using namespace std;

HXLibConfigReader::HXLibConfigReader()
{
}

HXLibConfigReader::~HXLibConfigReader()
{
}

bool HXLibConfigReader::OpenFile(const char* file)
{
	bool ret = false;

	try
	{
		std::ifstream fs(file);
		//fs.imbue(utf8Locale);

		if (fs)
		{
			boost::property_tree::read_xml(fs, mXmlRoot);
			ret = true;
		}
	}
	catch (...)
	{
		ret =  false;
	}

	return ret;
}

bool  HXLibConfigReader::GetStr(const char* path, char* rel)
{

	string val =   mXmlRoot.get<string>(path);
	strcpy(rel, val.c_str());

	return true;

}

int  HXLibConfigReader::GetInt(const char* path)
{
	return mXmlRoot.get(path, 0);
}

double  HXLibConfigReader::GetDouble(const char* path)
{
	return mXmlRoot.get(path, 0.0);
}


HXLIBNETWORK_API HXLibConfigService* CreateConfigReader()
{
	return new HXLibConfigReader();
}