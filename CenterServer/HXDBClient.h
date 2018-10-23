#pragma once
#include <string>
#include "ErrorMsg.h"
#include "HXLibDBConnector.h"
#include "HXLibNetwork.h"
#include "HXLibMessage.h"
#include <boost/thread.hpp>
#include "MsgQueue.h"
#include "TestRec.h"

using namespace std;

typedef std::deque<HXMessagePackage> HXDBMessageQueue;

class HXDBClient
{
public:
	HXDBClient();
	
	HXDBClient(const char* hostName, int hostPort, const char* dbName, const char* user, const char* password)
	{
		mHostName = hostName;
		mHostPort = hostPort;
		mDatabaseName = dbName;
		mUser = user; 
		mPassword = password;
	}
	
	virtual ~HXDBClient();
	int CreateDBConn();
	int GetUserInfo(int userID, TestRec*  testRec);

private:
	void WriteLog(HXError* e);

public:
	boost::mutex			mMutex;

private:
	string mHostName;
	int mHostPort;
	string mDatabaseName;
	string mUser;
	string mPassword;

	char mErrMsg[2048] = {0};
	HXDBService* hxdb;
};

