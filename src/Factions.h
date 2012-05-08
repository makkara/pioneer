#ifndef _FACTIONS_H
#define _FACTIONS_H

#include "Database.h"

class Factions
{
protected:
	Database data;
public:	
	Factions(void);
	virtual ~Factions(void);
};

#endif