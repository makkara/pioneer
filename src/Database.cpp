#include "Database.h"


Database::Database(){
	handle=NULL;	
}

bool Database::Open(std::string filename)
{
	sqlite3_close(handle);

	if(sqlite3_open(filename.c_str(),&handle)!=SQLITE_OK)
	{
		sqlite3_close(handle);
		return false;
	}
	return true;
}

bool Database::Create()
{
	return Open(":memory:");
}

bool Database::Load(std::string filename)
{
	Database f;
	if(!f.Open(filename))return false;	
	
	//clear existing data
	Create();

	if(!Backup(f,*this))return false;

	return true;
}

bool Database::Save(std::string filename)
{
	Database f;
	if(!f.Open(filename))return false;
	if(!Backup(*this,f))return false;

	return true;	
}

bool Database::Backup(Database&src,Database&dst)
{
	bool rv=true;

	sqlite3_backup* bu=sqlite3_backup_init(dst.handle,"main",src.handle,"main");
	if(bu==NULL)return false;

	if(sqlite3_backup_step(bu,-1)!=SQLITE_DONE)
		rv=false;

	sqlite3_backup_finish(bu);

	return rv;
}

Database::~Database()
{
	sqlite3_close(handle);
}

sqlite3_stmt * Database::PrepareStatement(const char * statement)
{
	sqlite3_stmt *stmt;
	if(sqlite3_prepare_v2(handle,statement,-1,&stmt,NULL)!=SQLITE_OK)
	{
		return NULL;
	}
	return stmt;
}

bool Database::Execute(const char * str)
{
	do{
		sqlite3_stmt *stmt;
		if(sqlite3_prepare_v2(handle,str,-1,&stmt,&str)!=SQLITE_OK)
		{
			return false;
		}

		int rv;
		while((rv=sqlite3_step(stmt))!=SQLITE_DONE)
		{
			if(rv!=SQLITE_ROW)//if something else than row returned happened -> fail
			{
				sqlite3_finalize(stmt);
				return false;
			}
		}

		sqlite3_finalize(stmt);

	}while(*str);//loop until str is set to end of string

	return true;
}