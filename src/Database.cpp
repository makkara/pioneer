#include "Database.h"


Database::Database(){
	handle=NULL;	
}

void Database::Open(std::string filename)
{
	sqlite3_close(handle);

	if(sqlite3_open(filename.c_str(),&handle)!=SQLITE_OK)
	{
		Exception e(sqlite3_errmsg(handle));
		sqlite3_close(handle);
		throw e;
	}
}

void Database::Create()
{
	Open(":memory:");
	
	sqlite3_extended_result_codes(handle, 1);
}

void Database::Load(std::string filename)
{
	Database f;
	f.Open(filename);	
	
	//clear existing data
	Create();

	Backup(f,*this);
}

void Database::Save(std::string filename)
{
	Database f;
	f.Open(filename);
	Backup(*this,f);
}

void Database::Backup(Database&src,Database&dst)
{
	sqlite3_backup* bu=sqlite3_backup_init(dst.handle,"main",src.handle,"main");
	if(bu==NULL){throw Exception("sqlite3_backup_init_fail");};

	if(sqlite3_backup_step(bu,-1)!=SQLITE_DONE)
	{
		sqlite3_backup_finish(bu);
		throw Exception("sqlite3_backup_step_fail");
	};

	sqlite3_backup_finish(bu);

}

Database::~Database()
{
	sqlite3_close(handle);
}

Database::Statement Database::PrepareStatement(const char * statement)
{
	sqlite3_stmt *stmt;
	if(sqlite3_prepare_v2(handle,statement,-1,&stmt,NULL)!=SQLITE_OK)
	{
		RaiseException();
	}
	return stmt;
}

void Database::Execute(const char * str)
{
	do{
		sqlite3_stmt *stmt;
		if(sqlite3_prepare_v2(handle,str,-1,&stmt,&str)!=SQLITE_OK)
		{
			RaiseException();
		}

		int rv;
		while((rv=sqlite3_step(stmt))!=SQLITE_DONE)
		{
			if(rv!=SQLITE_ROW)//if something else than row returned happened -> fail
			{
				sqlite3_finalize(stmt);
				RaiseException();
			}
		}

		sqlite3_finalize(stmt);

	}while(*str);//loop until str is set to end of string
}

Database::Statement& Database::Statement::operator=(const Statement&s)
{
	statement=s.statement;
	return *this;
}

Database::Statement::Statement()
{
	statement=NULL;
}

Database::Statement::Statement(sqlite3_stmt*stmt)
{
	statement=stmt;
	if(statement)
	{
		if(sqlite3_reset(statement)!=SQLITE_OK)
			RaiseException();
	}
}

Database::Statement::Statement(const Statement &s)
{
	statement=s.statement;
	if(statement)
	{
		if(sqlite3_reset(statement)!=SQLITE_OK)
			RaiseException();
	}
}
