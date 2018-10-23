#pragma once
#include <boost/property_tree/xml_parser.hpp>
#include "HXLibNetwork.h"
using namespace boost::property_tree;

class HXLibConfigReader : public HXLibConfigService
{
public:
	HXLibConfigReader();
	virtual ~HXLibConfigReader();

	bool	OpenFile(const char* file);
	bool	GetStr(const char* path, char* rel);
	int		GetInt(const char* path);
	double	GetDouble(const char* path);

private:
	ptree  mXmlRoot;
};
