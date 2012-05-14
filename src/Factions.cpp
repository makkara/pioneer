#include "Factions.h"
#include "Serializer.h"
#include "Pi.h"
#include "galaxy/StarSystem.h"

const float Factions::MAX_FACTION_DISTANCE=16.0f;

static const char * init_db =
	//Defines factions
	"create table factions ("
	"faction integer primary key,"
	"name text"
	"class integer"					 //faction type
	");"
	//Population centers in world, eg. Starports and Starbases
	"create table populations("
	"population integer primary key,"//unique handle to population
	"star integer,"                  //star where this population exists
	"body integer,"                  //body handle to starbody table, NOT bodyindex inside star
	"orbiting integer,"              //0 - on surface of body, 1-orbiting body
	"population_count integer,"      //population count
	"population_max integer,"        //maximum population
	"population_growth real"         //population growth multiplier
	");"
	//indexes to populations table
	//index to query populations by star and body
	"create index populations_star_body on populations (star,body);"

	//faction presences inside population
	"create table factionpresences("
	"factionpresence integer primary key,"
	"population integer,"		     //handle to population
	"faction integer,"				 //handle to owning faction
	"weight real"					 //weight of this presence when calculating population
	");"
	//indexes to presence table
	"create index factionpresences_population on factionpresences(population);"
	"create index factionpresences_faction_population on factionpresences(faction,population);"

	//represents path from one star to another, helps searching nearby stars
	"create table starlanes("
	"starlane integer primary key,"  //starlane handle
	"source integer,"                //source star handle for this starlane
	"destination integer,"           //destination star
	"distance2 real"                 //squared distance to destination star
	");"
	//index to query starlanes by star
	"create index starlanes_star on starlanes (source);"

	//cache of starbodies inside star
	"create table starbodies("	
	"body integer primary key,"      //unique handle to body
	"star integer,"                  //star where this body exists
	"parent integer,"                //handle to parent of this body
	"bodyindex integer"              //Pioneer index to this body
	");"
	//index to query all bodies inside a star
	"create index starbodies_star on starbodies (star);"
	;

long long Factions::SystemPathToStarHandle(int sectorX,int sectorY,int sectorZ,int systemIndex)
{
	long long rv;
	rv=((long long)sectorX<<48) |
	   ((long long)sectorY<<32) |
	   ((long long)sectorZ<<16) |
	   ((long long)systemIndex);
	return rv;
}
long long Factions::SystemPathToStarHandle(const SystemPath&p)
{
	return SystemPathToStarHandle(p.sectorX,p.sectorY,p.sectorZ,p.systemIndex);
}
void Factions::PrepareStatements()
{
	statements.resize(stmt_count);

	statements[stmt_insert_starbody]=PrepareStatement("insert into starbodies values(NULL,:star,:parent,:bodyindex);");
	statements[stmt_insert_starlane]=PrepareStatement("insert into starlanes values(NULL,:source,:destination,:distance2);");
}

void Factions::AddBodies(long long star,long long parent,const SystemBody*body)
{
	sqlite3_stmt*stmt=statements[stmt_insert_starbody];
	sqlite3_bind_int64(stmt,1,star);
	sqlite3_bind_int64(stmt,2,parent?parent:NULL);
	sqlite3_bind_int(stmt,3,body->path.bodyIndex);

	if(sqlite3_step(stmt)!=SQLITE_DONE)
		{}//TODO: handle error

	sqlite3_reset(stmt);
	long long id=sqlite3_last_insert_rowid(handle);
	//add all children also
	std::vector<SystemBody*>::const_iterator i;
	for(i=body->children.begin();i!=body->children.end();i++)
	{
		AddBodies(star,id,*i);
	}
}

void Factions::AddStar(const SystemPath&path)
{
	long long starhandle=SystemPathToStarHandle(path);
	Sector b(path.sectorX,path.sectorY,path.sectorZ);
	//add all bodies in starsystem to db
	RefCountedPtr<StarSystem> s=StarSystem::GetCached(path);
	AddBodies(starhandle, 0, s->rootBody);

	//add starlanes from this star
	int x=path.sectorX;
	int y=path.sectorY;
	int z=path.sectorZ;

	sqlite3_stmt*stmt=statements[stmt_insert_starlane];
	//loop at 16ly radius around sector
	int r=MAX_FACTION_DISTANCE/Sector::SIZE;
	for (int sx = -r; sx <= r; sx++) {
		for (int sy = -r; sy <= r; sy++) {
			for (int sz = -r; sz <= r; sz++) {
				Sector s(sx+x,sy+y,sz+z);
				int i;

				for(i=0;i!=s.m_systems.size();i++)
				{
					float distance=Sector::DistanceBetween(&b,path.systemIndex,&s,i);
					if(distance==0.0f)//if all coordinates are same, result is 0.0f
						continue;
					if(distance<=MAX_FACTION_DISTANCE)
					{
						sqlite3_bind_int64(stmt,1,starhandle);
						sqlite3_bind_int64(stmt,2,SystemPathToStarHandle(sx+x,sy+y,sz+z,i));
						sqlite3_bind_double(stmt,3,distance*distance);
						sqlite3_step(stmt);
						sqlite3_reset(stmt);
					}
				}
			}
		}
	}
}

Factions::Factions(void)
{
	Create();//TODO: handle error

	//Create tables
	Execute(init_db);//TODO: handle error

	PrepareStatements();

	//create some initial data
	
	AddStar(Pi::game->GetSpace()->GetStarSystem()->GetPath());
	
}

Factions::Factions(std::string filename)
{
	if(!Load(filename))
	{
		throw CouldNotOpenFileException();
	}
	PrepareStatements();
}

void Factions::Save(std::string filename)
{
	if(!Database::Save(filename))
	{
		throw CouldNotWriteToFileException();
	}
}

Factions::~Factions(void)
{
	//Finalize all precompiled statements
	std::vector<sqlite3_stmt*>::iterator i;
	for(i=statements.begin();i!=statements.end();i++)
	{
		if(*i)
			sqlite3_finalize(*i);
	}
}
