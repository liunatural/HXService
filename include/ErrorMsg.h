#pragma once

enum 
{
	SUCCESS						= 0,
	FAIL								=1,

	//------���ݿ��������ش���------------//
	ERR_DBCONNECTION		= 2001,
	ERR_DB_NOT_INIT,
	ERR_DB_NOT_VALID,
	ERR_DBCONN_SANITY_CHECK,

	//------���ķ�������ش���------------//
	ERR_CONNECT_CENTER_SERVER = 2100,  //�������ķ�����ʧ��

	//------OBS��������ش���-----------//
	ERR_CONNECT_OBS_SERVER = 2200,

	ERR_MAXIMUM,
};

