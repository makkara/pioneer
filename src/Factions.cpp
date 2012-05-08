#include "Factions.h"
#include "Serializer.h"


Factions::Factions(void)
{
	data.Create();//TODO: handle error
}

Factions::Factions(std::string filename)
{
	if(!data.Load(filename))
	{
		throw CouldNotOpenFileException();
	}
}

void Factions::Save(std::string filename)
{
	if(!data.Save(filename))
	{
		throw CouldNotWriteToFileException();
	}
}

Factions::~Factions(void)
{
}
