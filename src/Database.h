#ifndef _DB_H
#define _DB_H
#include <stdint.h>
#include <string>
#include <sqlite3/sqlite3.h>

class Database {
protected:
	sqlite3 *handle;
	static bool Backup(Database&src,Database&dst);

public:	
	/* Simple wrapper around sqlite statement
	 * Instantiate in stack, statement is automatically reset when object goes out of scope
	 */
	class Statement
	{
		public:
			sqlite3_stmt * statement;

			Statement();
			Statement(sqlite3_stmt*stmt);
			Statement(const Statement &s);
			Statement& operator=(const Statement&s);
			~Statement(){if(statement)sqlite3_reset(statement);};

			//Bind data to statement wrappers
			void Bind(int position, int32_t value){sqlite3_bind_int(statement,position,value);}
			void Bind(int position, int64_t value){sqlite3_bind_int64(statement,position,value);}
			void Bind(int position, uint32_t value){sqlite3_bind_int64(statement,position,value);}//unsigned 32bit is converted to 64bit for sorting to work correctly
			void Bind(int position, double value){sqlite3_bind_double(statement,position,value);}
			void BindNull(int position){sqlite3_bind_null(statement,position);}

			//get row data
			int64_t Get_int64(int column){return sqlite3_column_int64(statement, column);}
			int32_t Get_int32(int column){return sqlite3_column_int(statement, column);}
			double Get_double(int column){return sqlite3_column_double(statement, column);}

			//Operations on statement			
			bool NextRow(){return sqlite3_step(statement)==SQLITE_ROW;}//return true if row is received, TODO: errors should be handled with exceptions
			bool Run(){return sqlite3_step(statement)==SQLITE_DONE;}//return true if statement completes succesfully

			//convert to sqlite3 statement handle
			operator sqlite3_stmt*(){return statement;}
	};

	/* 
	 * Compile Statement and return handle
	 */
	Statement  PrepareStatement(const char * statement);

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
	bool Execute(const char * statement);

	Database();
	virtual ~Database();
};

#endif