#include "Database.h"


Database::Database(){
	handle=NULL;	
}

bool Database::Create()
{
	sqlite3_close(handle);

	if(sqlite3_open(":memory:",&handle)!=SQLITE_OK)
	{
		sqlite3_close(handle);
		return false;
	}
	return true;
}

Database::~Database()
{
	sqlite3_close(handle);
}