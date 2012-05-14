#ifndef _DB_H
#define _DB_H
#include <string>
#include <sqlite3/sqlite3.h>

class Database {
protected:
	sqlite3 *handle;
	static bool Backup(Database&src,Database&dst);

	/* Compile Statement and return handle
	 */
	sqlite3_stmt * PrepareStatement(const char * statement);
public:	
	bool Create();//create in memory database, if database already exists close previous
	/*
	 * Open database on disk
	 */
	bool Open(std::string filename);
	/**
	 * Creates new in memory database and backups file to memory
	 */
	bool Load(std::string filename);
	/**
	 * Deletes existing file and backups in-memory database to it
	 */
	bool Save(std::string filename);
	/**
	 * Execute SQL statement(s), returns true if succesful
	 * Slow operation, statement(s) is compiled everytime this is called
	 */
	bool Execute(std::string statement);

	Database();
	virtual ~Database();
};

#endif