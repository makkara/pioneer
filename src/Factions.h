#ifndef _FACTIONS_H
#define _FACTIONS_H

#include <string>
#include "Database.h"

class Factions
{
protected:
	Database data;
public:	
	Factions(std::string filename);
	Factions(void);
	virtual ~Factions(void);
	void Save(std::string filename);
};

#endif