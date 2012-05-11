#include "Factions.h"
#include "Serializer.h"

static const char * init_db =
	//Defines factions
	"create table factions ("
	"faction integer primary key,"
	"name text"
	");"
	//Faction presences in world, eg. Starports and Starbases
	"create table factionpresence("
	"precense integer primary key,"//unique handle to presence
	"star integer,"                //star where this presence exists
	"body integer,"                //body handle to starbody table, NOT bodyindex inside star
	"parent integer,"              //handle to parent if embedded inside other presence 
	"faction integer,"             //faction owning this presence
	"orbiting integer,"            //0 - on surface of body, 1-orbiting body
	"population integer,"          //population inside presence
	"population_max integer,"      //maximum population
	"population_growth real"       //population growth multiplier
	");"
	//indexes to presence table
	//index to query presences by star and faction
	"create index factionpresence_star_faction on factionpresence (star,faction);"
	//index to query all presences by a faction
	"create index factionpresence_faction on factionpresence (faction);"

	//represents path from one star to another, helps searching nearby stars
	"create table starlane("
	"starlane integer primary key,"//starlane handle
	"source integer,"              //source star handle for this starlane
	"destination integer,"         //destination star
	"distance2 real"               //squared distance to destination star
	");"
	//index to query starlanes by star
	"create index starlane_star on starlane (source);"

	//cache of starbodies inside star
	"create table starbody("	
	"body integer primary key,"    //unique handle to body
	"star integer,"                //star where this body exists
	"parent integer,"              //handle to parent of this body
	"bodyindex integer"            //Pioneer index to this body
	");"
	//index to query all bodies inside a star
	"create index starbody_star on starbody (star);"
	;


Factions::Factions(void)
{
	data.Create();//TODO: handle error

	//Create tables
	data.Execute(init_db);//TODO: handle error

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
