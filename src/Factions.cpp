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
	"create table settlements("
	"settlement integer primary key,"//unique handle to settlement
	"star integer,"                  //star where this settlement exists
	"body integer,"                  //body handle to starbody table, NOT bodyindex inside star
	"orbiting integer,"              //0 - on surface of body, 1-orbiting body
	"population integer,"			 //population count
	"population_max integer,"        //maximum population
	"population_growth real"         //population growth multiplier
	");"
	//indexes to settlements table
	//index to query settlements by star and body
	"create index settlements_star_body on settlements (star,body);"

	//faction presences inside population
	"create table factionpresences("
	"factionpresence integer primary key,"
	"settlement integer,"		     //handle to population
	"faction integer,"				 //handle to owning faction
	"weight real"					 //weight of this presence when calculating population
	");"
	//indexes to presence table
	"create index factionpresences_settlement on factionpresences(settlement);"
	"create index factionpresences_faction_settlement on factionpresences(faction,settlement);"

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
	"bodyindex integer,"             //Pioneer index to this body
	"type integer,"				     //Body type
	"metallicity real,"			     //Body metallicity
	"atmosphere_pressure real"       //Body atmosphere pressure	
	");"
	//index to query all bodies inside a star
	"create index starbodies_star on starbodies (star);"
	;

SystemPath Factions::StarHandleToSystemPath(starhandle handle)
{
	short x=(handle>>48)&0xffff;
	short y=(handle>>32)&0xffff;
	short z=(handle>>16)&0xffff;
	
	SystemPath rv(
		x,
		y,
		z,
		(handle)&0xffff
		);
	return rv;
}

Factions::starhandle Factions::SystemPathToStarHandle(int sectorX,int sectorY,int sectorZ,int systemIndex)
{
	long long rv;
	rv=((starhandle)sectorX&0xffff)<<48;
	rv|=((starhandle)sectorY&0xffff)<<32;
	rv|=((starhandle)sectorZ&0xffff)<<16;
	rv|=((starhandle)systemIndex&0xffff);
	return rv;
}
Factions::starhandle Factions::SystemPathToStarHandle(const SystemPath&p)
{
	return SystemPathToStarHandle(p.sectorX,p.sectorY,p.sectorZ,p.systemIndex);
}
void Factions::PrepareStatements()
{
	statements.resize(stmt_count);

	statements[stmt_insert_starbody]=PrepareStatement("insert into starbodies values(NULL,:star,:parent,:bodyindex,:type,:metallicity,:atmosphere_pressure);");
	statements[stmt_insert_starlane]=PrepareStatement("insert into starlanes values(NULL,:source,:destination,:distance2);");
	statements[stmt_get_nearby_stars]=PrepareStatement("select destination from starlanes where source=:source and distance2 <= :distance2;");
	statements[stmt_get_nearby_metal_bodies]=PrepareStatement(
		"select starlanes.destination,starlanes.distance2,starbodies.body,starbodies.metallicity "
		"from starlanes left join starbodies on starlanes.destination=starbodies.star "
		"where starlanes.source=:star and starbodies.metallicity is not null and distance2<:distance2 "
		"union all select :star,0,starbodies.body,starbodies.metallicity "
		"from starbodies where star=:star and metallicity is not null;");
	statements[stmt_create_settlement]=PrepareStatement("insert into settlements values(NULL,:star,:body,:orbiting,:population,:population_max,:population_growth);");
	statements[stmt_star_in_db]=PrepareStatement("select star from starbodies where star=2 limit :star;");
}
bool Factions::StarInDB(starhandle star)
{
	sqlite3_stmt*stmt=statements[stmt_star_in_db];
	sqlite3_bind_int64(stmt,1,star);
	bool rv=false;
	if(sqlite3_step(stmt)==SQLITE_ROW)
		rv=true;
	sqlite3_reset(stmt);
	return rv;
}
long long Factions::CreateSettlement(starhandle star, long long body, long long population)
{
	sqlite3_stmt*stmt=statements[stmt_create_settlement];
	sqlite3_bind_int64(stmt,1,star);
	sqlite3_bind_int64(stmt,2,body);
	sqlite3_bind_int(stmt,3,0);//TODO: orbiting
	sqlite3_bind_int64(stmt,4,population);
	sqlite3_bind_int64(stmt,5,population*2);//dummy value for max population
	sqlite3_bind_double(stmt,6,1.00095);//growth in month

	if(sqlite3_step(stmt)!=SQLITE_DONE)
		{}//TODO: handle error

	sqlite3_reset(stmt);
	return sqlite3_last_insert_rowid(handle);
}

std::vector<Factions::metalbody> Factions::GetNearbyMetalBodies(starhandle star,float maxdistance)
{
	std::vector<Factions::metalbody> out;
	sqlite3_stmt*stmt=statements[stmt_get_nearby_metal_bodies];
	sqlite3_bind_int64(stmt,1,star);
	sqlite3_bind_double(stmt,2,maxdistance*maxdistance);
	while(sqlite3_step(stmt)==SQLITE_ROW)
	{
		out.push_back(metalbody());
		metalbody & b=out.back();
		b.star=sqlite3_column_int64(stmt,0);
		b.distance2=sqlite3_column_double(stmt,1);
		b.body=sqlite3_column_int64(stmt,2);
		b.metallicity=sqlite3_column_double(stmt,3);
	}
	sqlite3_reset(stmt);
	return out;
}

std::vector<Factions::starhandle> Factions::GetNearbyStars(Factions::starhandle star,float maxdistance)
{
	std::vector<starhandle> out;
	sqlite3_stmt*stmt=statements[stmt_get_nearby_stars];
	sqlite3_bind_int64(stmt,1,star);
	sqlite3_bind_double(stmt,2,maxdistance*maxdistance);
	out.resize(0);
	while(sqlite3_step(stmt)==SQLITE_ROW)
	{
		out.push_back(sqlite3_column_int64(stmt, 0));
	}
	sqlite3_reset(stmt);
	return out;
}

void Factions::AddBodies(starhandle star,starhandle parent,const SystemBody*body)
{
	sqlite3_stmt*stmt=statements[stmt_insert_starbody];
	sqlite3_bind_int64(stmt,1,star);
	if(parent)
		sqlite3_bind_int64(stmt,2,parent);
	else
		sqlite3_bind_null(stmt,2);
	sqlite3_bind_int(stmt,3,body->path.bodyIndex);
	SystemBody::BodySuperType type=body->GetSuperType();
	sqlite3_bind_int(stmt,4,type);
	if(type==SystemBody::SUPERTYPE_ROCKY_PLANET)
	{
		Color tc;
		double density;
		body->GetAtmosphereFlavor(&tc, &density);
		sqlite3_bind_double(stmt,5,body->m_metallicity.ToDouble());
		sqlite3_bind_double(stmt,6,density);
	}else{
		sqlite3_bind_null(stmt,5);
		sqlite3_bind_null(stmt,6);
	}

	if(sqlite3_step(stmt)!=SQLITE_DONE)
		{}//TODO: handle error

	sqlite3_reset(stmt);
	starhandle id=sqlite3_last_insert_rowid(handle);
	//add all children also
	std::vector<SystemBody*>::const_iterator i;
	for(i=body->children.begin();i!=body->children.end();i++)
	{
		AddBodies(star,id,*i);
	}
}

void Factions::AddStar(const SystemPath&path)
{
	starhandle source=SystemPathToStarHandle(path);
	Sector b(path.sectorX,path.sectorY,path.sectorZ);
	//add all bodies in starsystem to db
	RefCountedPtr<StarSystem> s=StarSystem::GetCached(path);
	AddBodies(source, 0, s->rootBody);

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
						sqlite3_bind_int64(stmt,1,source);
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

	//create some initial data for testing
	
	SystemPath path=Pi::game->GetSpace()->GetStarSystem()->GetPath();
	AddStar(path);
	//get nearby stars
	starhandle source=SystemPathToStarHandle(path);
	std::vector<starhandle> stars=GetNearbyStars(source,10);
	//add all bodies from nearby stars to db
	std::vector<starhandle>::iterator i;
	for(i=stars.begin();i!=stars.end();i++)
	{
		AddStar(StarHandleToSystemPath(*i));
	}
	//create some settlements in metal planets in 8ly radius
	std::vector<metalbody> bodies=GetNearbyMetalBodies(source,8);
	std::vector<metalbody>::iterator it;
	for(it=bodies.begin();it!=bodies.end();it++)
	{
		if(it->metallicity>0.4)
		{//found candidate for settlement
			long long s=CreateSettlement(it->star, it->body, 1000000);
			if(!StarInDB(it->star))
			{
				AddStar(StarHandleToSystemPath(it->star));
			}
		}
	}
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
