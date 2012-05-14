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
		stmt_count
	}stmt_handles;
	std::map<int,Faction*> factions;
	std::vector<sqlite3_stmt *> statements;

	void AddBodies(long long star,long long parent, const SystemBody*);
	void AddStar(const SystemPath&path);
	void PrepareStatements();
	starhandle SystemPathToStarHandle(const SystemPath&);
	starhandle SystemPathToStarHandle(int x,int y,int z,int s);
	static const float MAX_FACTION_DISTANCE;

public:	
	
	Factions(std::string filename);
	Factions(void);
	virtual ~Factions(void);
	void Save(std::string filename);
};

#endif