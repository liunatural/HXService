#include "HXDBClient.h"
#include "protocol.h"


HXDBClient::HXDBClient()
{
	mHostName =	"127.0.0.1";
	mDatabaseName =	"test";
	mUser =	"root";
	mPassword =	"root";
	mHostPort =	3306;
}


HXDBClient::~HXDBClient()
{
	if (nullptr != hxdb)
	{
		delete hxdb;
		hxdb = nullptr;
	}
}



int HXDBClient::CreateDBConn()
{
	int ret = FAIL;
	try
	{
		hxdb = CreateDBService();
		ret =   hxdb->InitDBConn((char*)mHostName.c_str(), mHostPort, (char*)mDatabaseName.c_str(), (char*)mUser.c_str(), (char*)mPassword.c_str());
	}
	catch (HXError* e)
	{
		WriteLog(e);
	}
	return ret;
}

//one test 
int HXDBClient::GetUserInfo(int userID, TestRec* testRec)
{

	int ret = SUCCESS;
	try
	{
		if (!hxdb->IsDBConnValid())
		{
			return ERR_DBCONN_SANITY_CHECK;
		}

		hxdb->PreparedExcute("call testproc1(?)");
		hxdb->setInt(1, userID);
		hxdb->ExecuteCommand(true);


		while (hxdb->NextFieldExist())
		{
			hxdb->GetFieldValue("id", testRec->id);
			hxdb->GetFieldValue("name", testRec->name);
		}

		hxdb->CloseRecordset();
		return ret;
	}
	catch (HXError* e)
	{
		WriteLog(e);
	}

}

void HXDBClient::WriteLog(HXError* e)
{
	memset(mErrMsg, 0, sizeof(mErrMsg));
	e->GetErrorDestribe(mErrMsg, sizeof(mErrMsg));
	LOG(LogError, "error code:%d; error message: %s\n", e->GetErrorCode(), mErrMsg);
}

