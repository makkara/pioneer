#ifndef _DB_H
#define _DB_H
#include <string>
#include <sqlite3/sqlite3.h>

class Database {
protected:
	sqlite3 *handle;
public:
	bool Create();//create in memory database, if database already exists close previous
	/**
	 * Execute SQL statement, returns true if succesful
	 * Slow operation, statement is compiled everytime this is called
	 */
	bool Execute(std::string & statement);
	Database();
	virtual ~Database();
};

#endif