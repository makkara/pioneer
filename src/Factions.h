#ifndef _FACTIONS_H
#define _FACTIONS_H

#include <string>
#include <map>
#include <vector>
#include "galaxy/SystemPath.h"
#include "Database.h"

class SystemBody;

class Faction
{
public:
	Faction();
	virtual ~Faction(void);
	void update(int handle);
};

class Factions:protected Database
{
protected:
	typedef long long starhandle;
	enum{
		stmt_insert_starbody,
		stmt_insert_starlane,
		stmt_get_nearby_stars,
		stmt_count
	}stmt_handles;
	std::map<int,Faction*> factions;
	std::vector<sqlite3_stmt *> statements;

	//Add systembody and all children in database
	void AddBodies(starhandle star,starhandle parent, const SystemBody*);
	//Add all bodies in star and starlanes to nearby stars to database
	void AddStar(const SystemPath&path);

	void GetNearbyStars(starhandle star,float maxdistance,std::vector<starhandle>&);

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