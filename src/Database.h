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
			int current_bind;

			Statement();
			Statement(sqlite3_stmt*stmt);
			Statement(const Statement &s);
			Statement& operator=(const Statement&s);
			~Statement(){};
			void RaiseException(){throw Exception(sqlite3_errmsg(sqlite3_db_handle(statement)));};

			//Bind data to statement wrappers
			//use position
			Statement& Bind(int position, int32_t value){if(sqlite3_bind_int(statement,current_bind=position,value)!=SQLITE_OK)RaiseException();return *this;}
			Statement& Bind(int position, int64_t value){if(sqlite3_bind_int64(statement,current_bind=position,value)!=SQLITE_OK)RaiseException();return *this;}
			Statement& Bind(int position, uint32_t value){if(sqlite3_bind_int64(statement,current_bind=position,value)!=SQLITE_OK)RaiseException();return *this;}//unsigned 32bit is converted to 64bit for sorting to work correctly
			Statement& Bind(int position, double value){if(sqlite3_bind_double(statement,current_bind=position,value)!=SQLITE_OK)RaiseException();return *this;}
			Statement& Bind(int position, const void*value,int len){if(sqlite3_bind_blob(statement,current_bind=position,value,len,SQLITE_STATIC)!=SQLITE_OK)RaiseException();return *this;}
			Statement& BindNull(int position){if(sqlite3_bind_null(statement,current_bind=position)!=SQLITE_OK)RaiseException();return *this;}
			//increase position automatically
			Statement& Bind(int32_t value){if(sqlite3_bind_int(statement,++current_bind,value)!=SQLITE_OK)RaiseException();return *this;}
			Statement& Bind(int64_t value){if(sqlite3_bind_int64(statement,++current_bind,value)!=SQLITE_OK)RaiseException();return *this;}
			Statement& Bind(uint32_t value){if(sqlite3_bind_int64(statement,++current_bind,value)!=SQLITE_OK)RaiseException();return *this;}//unsigned 32bit is converted to 64bit for sorting to work correctly
			Statement& Bind(double value){if(sqlite3_bind_double(statement,++current_bind,value)!=SQLITE_OK)RaiseException();return *this;}
			Statement& Bind(const void*value,int len){if(sqlite3_bind_blob(statement,++current_bind,value,len,SQLITE_STATIC)!=SQLITE_OK)RaiseException();return *this;}
			Statement& BindNull(){if(sqlite3_bind_null(statement,++current_bind)!=SQLITE_OK)RaiseException();return *this;}

			//get row data
			int64_t Get_int64(int column){return sqlite3_column_int64(statement, column);}
			int32_t Get_int32(int column){return sqlite3_column_int(statement, column);}
			double Get_double(int column){return sqlite3_column_double(statement, column);}
			const void* Get_blob(int column){return sqlite3_column_blob(statement, column);}
			int Get_blob_len(int column){return sqlite3_column_bytes(statement,column);}

			//Step prepared statement, return true if row is returned
			bool Step(){int rv=sqlite3_step(statement);if(rv!=SQLITE_DONE && rv!=SQLITE_ROW)RaiseException();return (rv==SQLITE_ROW);};

			//Reset statement, also resets next binding index
			void Reset();

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

	//Create and load database
	Database(std::string filename);

	//Create empty database object, with no db handle
	Database();
	virtual ~Database();
};

class KeyValueStore
{
protected:
	Database & db;
	std::string name;
	Database::Statement stmt_get;
	Database::Statement stmt_put;
public:
	KeyValueStore(Database &_db,std::string table_name);
	void Put(const std::string & key, const std::string & value);	
	void Put(const std::string & key, int32_t value);	
	std::string GetBlob(const std::string & key);	
	int32_t GetI32(const std::string & key);

	//Clear table, delete all items
	void Clear(void);
};

#endif