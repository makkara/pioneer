#include "Database.h"

//Create and load database
Database::Database(std::string filename)
{
	handle=NULL;
	Open(filename);
}

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
	current_bind=s.current_bind;
	statement=s.statement;
	return *this;
}

Database::Statement::Statement()
{
	current_bind=0;
	statement=NULL;
}
Database::Statement::Statement(sqlite3_stmt*stmt)
{
	current_bind=0;
	statement=stmt;
	if(statement)
	{
		Reset();
	}
}

Database::Statement::Statement(const Statement &s)
{
	current_bind=0;
	statement=s.statement;
	if(statement)
	{
		Reset();
	}
}

void Database::Statement::Reset()
{
	current_bind=0;
	if(sqlite3_reset(statement)!=SQLITE_OK)
		RaiseException();
}


KeyValueStore::KeyValueStore(Database &_db,std::string table_name):db(_db),name(table_name)
{
	std::string cmd;
	cmd= cmd + "create table if not exists kv_"+name+"(key blob primary key, value blob);";
	db.Execute(cmd.c_str());
	stmt_put=db.PrepareStatement((std::string()+"insert into kv_"+name+" values(:key,:value);").c_str());
	stmt_get=db.PrepareStatement((std::string()+"select value from kv_"+name+" where key = :key;").c_str());
}

void KeyValueStore::Put(const std::string & key, const std::string & value)
{
	stmt_put.Bind(key.c_str(),key.length());
	stmt_put.Bind(value.c_str(),value.length());
	stmt_put.Step();
	stmt_put.Reset();
}

std::string KeyValueStore::GetBlob(const std::string & key)
{
	stmt_get.Bind(key.c_str(),key.length());
	stmt_get.Step();
	const void * blob=stmt_get.Get_blob(0);
	int len=stmt_get.Get_blob_len(0);
	std::string rv((const char*)blob,len);
	stmt_get.Reset();
	return rv;
}

void KeyValueStore::Put(const std::string & key, int32_t value)
{
	stmt_put.Bind(key.c_str(),key.length());
	stmt_put.Bind(value);
	stmt_put.Step();
	stmt_put.Reset();
}

int32_t KeyValueStore::GetI32(const std::string & key)
{
	stmt_get.Bind(key.c_str(),key.length());
	stmt_get.Step();
	int rv=stmt_get.Get_int32(0);
	stmt_get.Reset();
	return rv;
}

void KeyValueStore::Clear(void)
{
	db.Execute((std::string()+"delete from kv_"+name).c_str());
}
