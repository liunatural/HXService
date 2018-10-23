#include "stdafx.h"
#include "MySqlConnector.h"
#include <sstream>



CMySqlError::CMySqlError()
{
}

CMySqlError::~CMySqlError()
{
}

void CMySqlError::SetErrorInfo(sql::SQLException &e)
{
	m_ErrorCode = e.getErrorCode();
	m_strErrorDescribe = e.what();

	throw this;
}

CMySqlConnector::CMySqlConnector() :m_DBConnect(NULL), m_DBPrepareState(NULL), m_DBRecordSet((sql::ResultSet*)NULL), m_dwTryConnectTimes(1)
{
}

CMySqlConnector::~CMySqlConnector()
{
	try
	{
		CloseConnect();

		m_DBRecordSet.reset((sql::ResultSet*)NULL);
		m_DBPrepareState.reset(NULL);
		m_DBConnect.reset(NULL);
	}
	catch (sql::SQLException &e)
	{
		SetErrorInfo(e);
	}
}


int CMySqlConnector::InitDBConn(char* hostName, unsigned short hostPort, char* databaseName, char* userName, char* password)
{

	int ret = ERR_DBCONNECTION;
	SetConnectionInfo(hostName, hostPort, databaseName, userName, password);

	bool bConn =OpenConnect();

	if (bConn)
	{
		ret = SUCCESS;
	}

	return ret;

}

void CMySqlConnector::SetErrorInfo(sql::SQLException &e)
{
	m_MySqlError.SetErrorInfo(e);
}


bool CMySqlConnector::OpenConnect()
{
	bool ret = false;
	try
	{
		sql::mysql::MySQL_Driver *driver = sql::mysql::get_mysql_driver_instance();
		sql::Connection* conn = driver->connect(m_ConnectProperties["hostName"], m_ConnectProperties["userName"], m_ConnectProperties["password"]);
		m_DBConnect.reset(conn);
		m_DBConnect->setSchema(m_ConnectProperties["schema"]);

		ret = m_DBConnect->isValid();
		if(!ret)
		{ 
			sql::SQLException ex("Connect to MySQL Server unsuccessfully.");
			throw ex;
		}
	}
	catch (sql::SQLException &e) 
	{ 
		SetErrorInfo(e);
	}

	return ret;
}


bool CMySqlConnector::IsDBConnValid()
{
	if (NULL == m_DBConnect)
	{
		return false;
	}

	return m_DBConnect->isValid();
}


bool CMySqlConnector::CloseRecordset()
{
	try
	{
		if (m_DBPrepareState != NULL)
		{
			while (m_DBPrepareState->getMoreResults())
			{
				m_DBRecordSet.reset(m_DBPrepareState->getResultSet());
			}

			m_DBPrepareState.reset(NULL);
		}

		if (m_DBRecordSet != NULL)
		{ 
			m_DBRecordSet.reset((sql::ResultSet*)NULL);
		}

		return true;
	}
	catch (sql::SQLException &e) 
	{ 
		SetErrorInfo(e); 
	}

	return false;
}

bool CMySqlConnector::CloseConnect()
{
	try
	{
		CloseRecordset();

		if (m_DBConnect != NULL)
		{
			m_DBConnect.reset(NULL);
		}

		return true;
	}
	catch (sql::SQLException &e)
	{
		SetErrorInfo(e); 
	}

	return false;
}



void CMySqlConnector::SetConnectionInfo(const string &hostIp, unsigned short hostPort, const string &dataBaseName,	const string &userName, const string &password)
{
	std::stringstream host;
	host << "tcp://" << hostIp << ":" << hostPort;

	m_ConnectProperties["hostName"] = host.str();
	m_ConnectProperties["userName"] = userName;
	m_ConnectProperties["password"] = password;
	m_ConnectProperties["schema"] = dataBaseName;

}

bool CMySqlConnector::IsRecordsetOpened()
{
	if (m_DBRecordSet == NULL)
		return false;

	if (m_DBRecordSet->isClosed())
		return false;

	return true;
}


bool CMySqlConnector::PreparedExcute(const std::string &szCommand)
{
	if (szCommand.empty())
	{
		return false;
	}

	CloseRecordset();

	try
	{
		m_DBPrepareState.reset(m_DBConnect->prepareStatement(szCommand));
		m_DBPrepareState->clearParameters();

		return true;
	}
	catch (sql::SQLException &e) 
	{
		SetErrorInfo(e);
	}

	return false;
}

bool CMySqlConnector::setBigInt(unsigned int parameterIndex, const std::string& value)
{
	try
	{
		m_DBPrepareState->setBigInt(parameterIndex, value);
		return true;
	}
	catch (sql::SQLException &e) 
	{
		SetErrorInfo(e); 
	}

	return false;
}

bool CMySqlConnector::setBlob(unsigned int parameterIndex, std::istream * value)            //长文本字符串
{
	try
	{
		m_DBPrepareState->setBlob(parameterIndex, value);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::setBoolean(unsigned int parameterIndex, bool value)
{
	try
	{
		m_DBPrepareState->setBoolean(parameterIndex, value);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::setDateTime(unsigned int parameterIndex, const std::string& value)
{
	try
	{
		m_DBPrepareState->setDateTime(parameterIndex, value);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::setDouble(unsigned int parameterIndex, double value)
{
	try
	{
		m_DBPrepareState->setDouble(parameterIndex, value);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::setInt(unsigned int parameterIndex, int32_t value)
{
	try
	{
		m_DBPrepareState->setInt(parameterIndex, value);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::setUInt(unsigned int parameterIndex, uint32_t value)
{
	try
	{
		m_DBPrepareState->setUInt(parameterIndex, value);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::setInt64(unsigned int parameterIndex, int64_t value)
{
	try
	{
		m_DBPrepareState->setInt64(parameterIndex, value);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::setUInt64(unsigned int parameterIndex, uint64_t value)
{
	try
	{
		m_DBPrepareState->setUInt64(parameterIndex, value);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::setString(unsigned int parameterIndex, const std::string& value)
{
	try
	{
		m_DBPrepareState->setString(parameterIndex, value);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::setNull(unsigned int parameterIndex, int sqlType)
{
	try
	{
		m_DBPrepareState->setNull(parameterIndex, sqlType);

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}


bool CMySqlConnector::ExecuteCommand(bool bRecordset)
{
	try
	{
		m_DBPrepareState->executeUpdate();

		if (bRecordset)
			m_DBRecordSet.reset(m_DBPrepareState->getResultSet());

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

//执行查询(Select)
bool CMySqlConnector::Query(const std::string &szCommand)
{
	if (szCommand.empty())    return false;

	//close RecordSet;
	CloseRecordset();
	try
	{
		m_DBPrepareState.reset(m_DBConnect->prepareStatement(szCommand));

		m_DBPrepareState->executeUpdate();
		m_DBRecordSet.reset(m_DBPrepareState->getResultSet());
		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

//执行语句(Insert,Update,Delete)
bool CMySqlConnector::Execute(const std::string &szCommand)
{
	if (szCommand.empty())    return false;

	//close RecordSet;
	CloseRecordset();
	try
	{
		m_DBPrepareState.reset(m_DBConnect->prepareStatement(szCommand));
		m_DBPrepareState->executeUpdate();

		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

//获取当前 Result set
const ResultSet &CMySqlConnector::GetRecordSet()
{
	return m_DBRecordSet;
}

//get Next Record set
bool CMySqlConnector::GetNextResultSet()
{
	if (m_DBPrepareState == NULL)
		return false;

	if (m_DBPrepareState->getMoreResults())
	{
		m_DBRecordSet.reset(m_DBPrepareState->getResultSet());
		return true;
	}
	return false;
}

//next
bool CMySqlConnector::NextFieldExist()
{
	if (m_DBRecordSet == NULL)
		return false;

	return m_DBRecordSet->next();
}


bool CMySqlConnector::GetFieldValue(const std::string& columnLabel, bool &bValue)
{
	bValue = false;

	if (!IsRecordsetOpened())
		return false;

	try
	{
		bValue = m_DBRecordSet->getBoolean(columnLabel);
		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}


bool CMySqlConnector::GetFieldValue(const std::string& columnLabel, long double &dbValue)
{
	dbValue = 0.00;

	if (!IsRecordsetOpened())
		return false;

	try
	{
		dbValue = m_DBRecordSet->getDouble(columnLabel);
		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}


bool CMySqlConnector::GetFieldValue(const std::string& columnLabel, int32_t &nValue)
{
	nValue = 0;

	if (!IsRecordsetOpened())
		return false;

	try
	{
		nValue = m_DBRecordSet->getInt(columnLabel);
		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}


bool CMySqlConnector::GetFieldValue(const std::string& columnLabel, uint32_t &uValue)
{
	uValue = 0;

	if (!IsRecordsetOpened())
		return false;

	try
	{
		uValue = m_DBRecordSet->getUInt(columnLabel);
		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}


bool CMySqlConnector::GetFieldValue(const std::string& columnLabel, int64_t &llValue)
{
	llValue = 0;

	if (!IsRecordsetOpened())
		return false;

	try
	{
		llValue = m_DBRecordSet->getInt64(columnLabel);
		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}


bool CMySqlConnector::GetFieldValue(const std::string& columnLabel, uint64_t &lluValue)
{
	lluValue = 0;

	if (!IsRecordsetOpened())
		return false;

	try
	{
		lluValue = m_DBRecordSet->getUInt64(columnLabel);
		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::GetFieldValue(const std::string& columnLabel, char szBuffer[], uint32_t uSize)
{
	memset(szBuffer, 0, uSize);

	if (!IsRecordsetOpened())
		return false;

	try
	{
		sql::SQLString tempstr = m_DBRecordSet->getString(columnLabel);
		strncpy_s(szBuffer, uSize, tempstr.c_str(), uSize - 1);
		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

bool CMySqlConnector::GetFieldValue(const std::string& columnLabel, std::string &szValue)
{
	if (!IsRecordsetOpened())
		return false;

	try
	{
		szValue = m_DBRecordSet->getString(columnLabel).asStdString();
		return true;
	}
	catch (sql::SQLException &e) { SetErrorInfo(e); }

	return false;
}

/*
//获取参数,SYSTEMTIME 可以通过 COleDateTime(const SYSTEMTIME& systimeSrc) 转换为 COleDateTime
bool CMySqlConnector::GetFieldValue(const std::string& columnLabel,SYSTEMTIME &systime)
{
if(!IsRecordsetOpened())
return false;
memset(&systime,0,sizeof(SYSTEMTIME));
try
{
std::string timestr =  m_DBRecordSet->getString(columnLabel).asStdString();
sscanf(timestr.c_str(),"%04d-%02d-%02d %02d:%02d:%02d",&systime.wYear,&systime.wMonth,&systime.wDay,
&systime.wHour,&systime.wMinute,&systime.wSecond);
return true;
}
catch (sql::SQLException &e) { SetErrorInfo(e);}

return false;
}
*/


HXLIBDBCONNECTOR_API HXDBService * CreateDBService(void)
{
	return new CMySqlConnector();
}
