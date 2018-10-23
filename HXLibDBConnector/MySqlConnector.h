//数据库对象模块。恒心东方 刘志. 2017-11-30
#pragma once
#include "HXLibDBConnector.h"
#include "ErrorMsg.h"
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/statement.h>
#include <map>

using namespace std;

typedef boost::scoped_ptr<sql::Connection> Connection;
typedef boost::scoped_ptr<sql::PreparedStatement> PreparedStatement;
typedef boost::scoped_ptr<sql::Statement> Statement;
typedef boost::shared_ptr<sql::ResultSet> ResultSet;
typedef sql::mysql::MySQL_Driver MySQL_Driver;

class CMySqlError : public HXError
{
public:
	CMySqlError();
	~CMySqlError();
	int GetErrorCode() { return m_ErrorCode; } 
	void SetErrorInfo(sql::SQLException &e);			//设置错误
	void  GetErrorDestribe(char* errMsg, int len)	//错误描述
	{
		//int errMsgLen = m_strErrorDescribe.length();
		//errMsgLen = errMsgLen > len ? len : errMsgLen;
		strcpy_s( errMsg, len, m_strErrorDescribe.c_str() );
		return;
	}	
};


//Mysql数据库访问接口类
class CMySqlConnector: public HXDBService
{
protected:
	Connection						m_DBConnect;
	PreparedStatement			m_DBPrepareState;
	ResultSet						m_DBRecordSet;
	map<string, string>		m_ConnectProperties;			//连接信息
	CMySqlError					m_MySqlError;					//当前错误信息
	const unsigned int			m_dwTryConnectTimes;

public:
	CMySqlConnector();
	~CMySqlConnector();

	//创建数据库连接
	int InitDBConn(char* hostName, unsigned short hostPort, char* databaseName, char* userName, char* password);
	//关闭连接
	bool CloseConnect();
	//数据库是否打开
	bool IsDBConnValid();
	//准备prepareState
	bool PreparedExcute(const std::string &szCommand);

	//执行查询(Select)
	bool Query(const std::string &szCommand);
	//执行语句(Insert,Update,Delete)
	bool Execute(const std::string &szCommand);
	//执行命令(存储过程）
	bool ExecuteCommand(bool bRecordset);

	//获取当前记录集
	const ResultSet &GetRecordSet();
	//获取下一记录集
	bool GetNextResultSet();
	//下一记录是否存在
	bool NextFieldExist();
	//关闭记录集
	bool CloseRecordset();
	//记录集是否打开
	bool IsRecordsetOpened();



	//设置字段的值
	bool setBigInt(unsigned int parameterIndex, const std::string& value);
	bool setBlob(unsigned int parameterIndex, std::istream * blob);            //长文本字符串
	bool setBoolean(unsigned int parameterIndex, bool value);
	bool setDateTime(unsigned int parameterIndex, const std::string& value);
	bool setDouble(unsigned int parameterIndex, double value);
	bool setInt(unsigned int parameterIndex, int32_t value);
	bool setUInt(unsigned int parameterIndex, uint32_t value);
	bool setInt64(unsigned int parameterIndex, int64_t value);
	bool setUInt64(unsigned int parameterIndex, uint64_t value);
	bool setString(unsigned int parameterIndex, const std::string& value);
	bool setNull(unsigned int parameterIndex, int sqlType);

	//获取字段的值
	bool GetFieldValue(const std::string& columnLabel, bool &bValue);
	bool GetFieldValue(const std::string& columnLabel, long double &dbValue);
	bool GetFieldValue(const std::string& columnLabel, int32_t &nValue);
	bool GetFieldValue(const std::string& columnLabel, uint32_t &uValue);
	bool GetFieldValue(const std::string& columnLabel, int64_t &llValue);
	bool GetFieldValue(const std::string& columnLabel, uint64_t &lluValue);
	bool GetFieldValue(const std::string& columnLabel, char szBuffer[], uint32_t uSize);
	bool GetFieldValue(const std::string& columnLabel, std::string &szValue);
	//bool GetFieldValue(const std::string& columnLabel,SYSTEMTIME &systime);

private:
	void SetErrorInfo(sql::SQLException &e);
	//设置信息
	void SetConnectionInfo(const string &hostIp, unsigned short hostPort, const string &dataBaseName, const string &userName, const string &password);
	//打开连接
	bool OpenConnect();
};
