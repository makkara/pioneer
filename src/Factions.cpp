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
	"settlement integer,"		     //handle to settlement
	"faction integer,"				 //handle to owning faction
	"class integer,"				 //type of precense faction has on this settlement
	"strength real"					 //weight of this presence when calculating population
	");"
	//indexes to presence table
	"create index factionpresences_settlement_class on factionpresences(settlement);"
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
	statements[stmt_create_faction]=PrepareStatement("insert into factions values(NULL,:name);");
	statements[stmt_create_faction_presence]=PrepareStatement("insert into factionpresences values(NULL,:settlement,:faction,:class,:strength);");
}

void Factions::CreateFactionPresence(long long settlement, long long faction, int type,double strength)
{
	Database::Statement stmt=statements[stmt_create_faction_presence];
	stmt.Bind(1,settlement);
	stmt.Bind(2,faction);
	stmt.Bind(3,type);
	stmt.Bind(4,strength);
	stmt.Step();
}

long long Factions::CreateFaction(std::string name)
{
	Database::Statement stmt=statements[stmt_create_faction];
	sqlite3_bind_text(stmt,1,name.c_str(),-1,SQLITE_TRANSIENT);
	stmt.Step();

	return sqlite3_last_insert_rowid(handle);
}

bool Factions::StarInDB(starhandle star)
{
	Database::Statement stmt=statements[stmt_star_in_db];
	stmt.Bind(1,star);
	bool rv=false;
	if(stmt.Step())
		rv=true;

	return rv;
}
long long Factions::CreateSettlement(starhandle star, long long body, long long population)
{
	Database::Statement stmt=statements[stmt_create_settlement];
	stmt.Bind(1,star);
	stmt.Bind(2,body);
	stmt.Bind(3,0);//TODO: orbiting
	stmt.Bind(4,population);
	stmt.Bind(5,population*2);//dummy value for max population
	stmt.Bind(6,1.00095);//growth in month
	stmt.Step();

	return sqlite3_last_insert_rowid(handle);
}

std::vector<Factions::metalbody> Factions::GetNearbyMetalBodies(starhandle star,float maxdistance)
{
	std::vector<Factions::metalbody> out;
	Database::Statement stmt=statements[stmt_get_nearby_metal_bodies];
	stmt.Bind(1,star);
	stmt.Bind(2,maxdistance*maxdistance);
	while(stmt.Step())
	{
		out.push_back(metalbody());
		metalbody & b=out.back();
		b.star=stmt.Get_int64(0);
		b.distance2=stmt.Get_double(1);
		b.body=stmt.Get_int64(2);
		b.metallicity=stmt.Get_double(3);
	}

	return out;
}

std::vector<Factions::starhandle> Factions::GetNearbyStars(Factions::starhandle star,float maxdistance)
{
	std::vector<starhandle> out;
	Database::Statement stmt=statements[stmt_get_nearby_stars];
	stmt.Bind(1,star);
	stmt.Bind(2,maxdistance*maxdistance);
	out.resize(0);
	while(stmt.Step())
	{
		out.push_back(stmt.Get_int64(0));
	}

	return out;
}

void Factions::AddBodies(starhandle star,starhandle parent,const SystemBody*body)
{
	Database::Statement stmt=statements[stmt_insert_starbody];
	stmt.Bind(1,star);
	if(parent)
		stmt.Bind(2,parent);
	else
		stmt.BindNull(2);
	stmt.Bind(3,body->path.bodyIndex);
	SystemBody::BodySuperType type=body->GetSuperType();
	stmt.Bind(4,type);
	if(type==SystemBody::SUPERTYPE_ROCKY_PLANET)
	{
		Color tc;
		double density;
		body->GetAtmosphereFlavor(&tc, &density);
		stmt.Bind(5,body->m_metallicity.ToDouble());
		stmt.Bind(6,density);
	}else{
		stmt.BindNull(5);
		stmt.BindNull(6);
	}

	stmt.Step();

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

	Database::Statement stmt=statements[stmt_insert_starlane];
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
						stmt.Bind(1,source);
						stmt.Bind(2,SystemPathToStarHandle(sx+x,sy+y,sz+z,i));
						stmt.Bind(3,distance*distance);
						stmt.Step();
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

	long long faction=CreateFaction("The Federation");
	//Couple of examples of a company
	long long burger=CreateFaction("Monolith Burger");

	long long company=CreateFaction("Corellian Engineering Corporation");


	//create some settlements in metal planets in 8ly radius
	std::vector<metalbody> bodies=GetNearbyMetalBodies(source,8);
	std::vector<metalbody>::iterator it;
	for(it=bodies.begin();it!=bodies.end();it++)
	{
		if(it->metallicity>0.4)
		{//found candidate for settlement
			long long s=CreateSettlement(it->star, it->body, 1000000);
			if(s==14)
				CreateFactionPresence(s,company,TYPE_SHIPYARD,1.0);
			if(it->distance2==0)
			{
				CreateFactionPresence(s,burger,TYPE_INDUSTRY_FOOD,0.1);
			}
			if(!StarInDB(it->star))
			{
				AddStar(StarHandleToSystemPath(it->star));
			}
			CreateFactionPresence(s,faction,TYPE_TAX,1.0);
		}
	}
}

Factions::Factions(std::string filename)
{
	try{
		Load(filename);
	
		PrepareStatements();
	}catch(const Database::Exception &)
	{
		throw CouldNotOpenFileException();
	}
	
}

void Factions::Save(std::string filename)
{
	try{
		Database::Save(filename);
	}catch (const Database::Exception &){
		throw CouldNotWriteToFileException();
	}
}

Factions::~Factions(void)
{
	std::vector<Database::Statement>::iterator it;
	
	for(it=statements.begin();it!=statements.end();it++)
	{
		sqlite3_finalize(*it);
		it->statement=0;
	}
}
