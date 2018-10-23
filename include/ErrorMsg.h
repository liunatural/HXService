#pragma once

enum 
{
	SUCCESS						= 0,
	FAIL								=1,

	//------数据库服务器相关错误------------//
	ERR_DBCONNECTION		= 2001,
	ERR_DB_NOT_INIT,
	ERR_DB_NOT_VALID,
	ERR_DBCONN_SANITY_CHECK,

	//------中心服务器相关错误------------//
	ERR_CONNECT_CENTER_SERVER = 2100,  //连接中心服务器失败

	//------OBS服务器相关错误-----------//
	ERR_CONNECT_OBS_SERVER = 2200,

	ERR_MAXIMUM,
};

