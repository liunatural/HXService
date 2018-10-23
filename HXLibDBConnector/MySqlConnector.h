//���ݿ����ģ�顣���Ķ��� ��־. 2017-11-30
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
	void SetErrorInfo(sql::SQLException &e);			//���ô���
	void  GetErrorDestribe(char* errMsg, int len)	//��������
	{
		//int errMsgLen = m_strErrorDescribe.length();
		//errMsgLen = errMsgLen > len ? len : errMsgLen;
		strcpy_s( errMsg, len, m_strErrorDescribe.c_str() );
		return;
	}	
};


//Mysql���ݿ���ʽӿ���
class CMySqlConnector: public HXDBService
{
protected:
	Connection						m_DBConnect;
	PreparedStatement			m_DBPrepareState;
	ResultSet						m_DBRecordSet;
	map<string, string>		m_ConnectProperties;			//������Ϣ
	CMySqlError					m_MySqlError;					//��ǰ������Ϣ
	const unsigned int			m_dwTryConnectTimes;

public:
	CMySqlConnector();
	~CMySqlConnector();

	//�������ݿ�����
	int InitDBConn(char* hostName, unsigned short hostPort, char* databaseName, char* userName, char* password);
	//�ر�����
	bool CloseConnect();
	//���ݿ��Ƿ��
	bool IsDBConnValid();
	//׼��prepareState
	bool PreparedExcute(const std::string &szCommand);

	//ִ�в�ѯ(Select)
	bool Query(const std::string &szCommand);
	//ִ�����(Insert,Update,Delete)
	bool Execute(const std::string &szCommand);
	//ִ������(�洢���̣�
	bool ExecuteCommand(bool bRecordset);

	//��ȡ��ǰ��¼��
	const ResultSet &GetRecordSet();
	//��ȡ��һ��¼��
	bool GetNextResultSet();
	//��һ��¼�Ƿ����
	bool NextFieldExist();
	//�رռ�¼��
	bool CloseRecordset();
	//��¼���Ƿ��
	bool IsRecordsetOpened();



	//�����ֶε�ֵ
	bool setBigInt(unsigned int parameterIndex, const std::string& value);
	bool setBlob(unsigned int parameterIndex, std::istream * blob);            //���ı��ַ���
	bool setBoolean(unsigned int parameterIndex, bool value);
	bool setDateTime(unsigned int parameterIndex, const std::string& value);
	bool setDouble(unsigned int parameterIndex, double value);
	bool setInt(unsigned int parameterIndex, int32_t value);
	bool setUInt(unsigned int parameterIndex, uint32_t value);
	bool setInt64(unsigned int parameterIndex, int64_t value);
	bool setUInt64(unsigned int parameterIndex, uint64_t value);
	bool setString(unsigned int parameterIndex, const std::string& value);
	bool setNull(unsigned int parameterIndex, int sqlType);

	//��ȡ�ֶε�ֵ
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
	//������Ϣ
	void SetConnectionInfo(const string &hostIp, unsigned short hostPort, const string &dataBaseName, const string &userName, const string &password);
	//������
	bool OpenConnect();
};
