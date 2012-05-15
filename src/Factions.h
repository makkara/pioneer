#ifndef _FACTIONS_H
#define _FACTIONS_H

#include <string>
#include <map>
#include <vector>
#include "galaxy/SystemPath.h"
#include "Database.h"

class SystemBody;

class Factions:protected Database
{
protected:
	//some example presence types
	enum FactionPresenceType
	{
		TYPE_TAX,           //owner of settlement
		TYPE_SHIPYARD,      //settlement contains shipyard
		TYPE_INDUSTRY_FOOD, //industry type food 
	};

	typedef long long starhandle;
	enum{
		stmt_insert_starbody,
		stmt_insert_starlane,
		stmt_get_nearby_stars,
		stmt_get_nearby_metal_bodies,
		stmt_create_settlement,
		stmt_star_in_db,
		stmt_create_faction,
		stmt_create_faction_presence,
		stmt_count
	}stmt_handles;

	std::vector<sqlite3_stmt *> statements;

	//Add systembody and all children in database
	void AddBodies(starhandle star,starhandle parent, const SystemBody*);
	//Add all bodies in star and starlanes to nearby stars to database
	void AddStar(const SystemPath&path);

	bool StarInDB(starhandle star);

	long long CreateSettlement(starhandle star, long long body, long long population);

	long long CreateFaction(std::string name);

	void CreateFactionPresence(long long settlement, long long faction, int type,double strength);

	std::vector<starhandle> GetNearbyStars(starhandle star,float maxdistance);

	struct metalbody{
		starhandle star;
		long long body;
		double distance2;
		double metallicity;		
	};
	std::vector<metalbody> GetNearbyMetalBodies(starhandle star,float maxdistance);

	//prepare all statements this class will use when database is open
	void PrepareStatements();
	starhandle SystemPathToStarHandle(const SystemPath&);
	starhandle SystemPathToStarHandle(int x,int y,int z,int s);
	SystemPath StarHandleToSystemPath(starhandle);
	//maximum distance factions can see stars
	static const float MAX_FACTION_DISTANCE;

public:	
	
	Factions(std::string filename);
	Factions(void);
	virtual ~Factions(void);
	void Save(std::string filename);
};

#endif