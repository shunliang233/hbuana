#ifndef CONFIG_HH
#define CONFIG_HH
#include <string>
#include "yaml-cpp/yaml.h"

class Config
{
public:
	YAML::Node conf;

	Config();
	~Config();

	virtual void Print();
	virtual void Parse(const std::string config_file);
	virtual int Run();
};

#endif
