#ifndef _DB_H
#define _DB_H
#include <stdint.h>
#include <string>
#include <sqlite3/sqlite3.h>
#include <stdexcept>


class Database {
public:
	class Exception : public std::runtime_error
	{
	public:
		Exception(const char*msg):std::runtime_error(msg){}
	};

protected:
	sqlite3 *handle;
	static void Backup(Database&src,Database&dst);
	void RaiseException(){throw Exception(sqlite3_errmsg(handle));}

public:	
	/* Simple wrapper around sqlite statement
	 * statement is automatically reset when object is created
	 */
	class Statement
	{
		public:
			sqlite3_stmt * statement;

			Statement();
			Statement(sqlite3_stmt*stmt);
			Statement(const Statement &s);
			Statement& operator=(const Statement&s);
			~Statement(){};
			void RaiseException(){throw Exception(sqlite3_errmsg(sqlite3_db_handle(statement)));};

			//Bind data to statement wrappers
			void Bind(int position, int32_t value){if(sqlite3_bind_int(statement,position,value)!=SQLITE_OK)RaiseException();}
			void Bind(int position, int64_t value){if(sqlite3_bind_int64(statement,position,value)!=SQLITE_OK)RaiseException();}
			void Bind(int position, uint32_t value){if(sqlite3_bind_int64(statement,position,value)!=SQLITE_OK)RaiseException();}//unsigned 32bit is converted to 64bit for sorting to work correctly
			void Bind(int position, double value){if(sqlite3_bind_double(statement,position,value)!=SQLITE_OK)RaiseException();}
			void BindNull(int position){if(sqlite3_bind_null(statement,position)!=SQLITE_OK)RaiseException();}

			//get row data
			int64_t Get_int64(int column){return sqlite3_column_int64(statement, column);}
			int32_t Get_int32(int column){return sqlite3_column_int(statement, column);}
			double Get_double(int column){return sqlite3_column_double(statement, column);}

			//Step prepared statement, return true if row is returned
			bool Step(){int rv=sqlite3_step(statement);if(rv!=SQLITE_DONE && rv!=SQLITE_ROW)RaiseException();return (rv==SQLITE_ROW);};

			//convert to sqlite3 statement handle
			operator sqlite3_stmt*(){return statement;}
	};

	/* 
	 * Compile Statement and return handle
	 */
	Statement  PrepareStatement(const char * statement);

	void Create();//create in memory database, if database already exists close previous
	/*
	 * Open database on disk
	 */
	void Open(std::string filename);
	/**
	 * Creates new in memory database and backups file to memory
	 */
	void Load(std::string filename);
	/**
	 * Deletes existing file and backups in-memory database to it
	 */
	void Save(std::string filename);
	/**
	 * Execute SQL statement(s)
	 * Slow operation, statement(s) is compiled everytime this is called
	 */
	void Execute(const char * statement);

	Database();
	virtual ~Database();
};

#endif