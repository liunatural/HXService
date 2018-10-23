//数据库对象模块。恒心东方 刘志. 2017-11-30
#pragma once
#ifdef HXLIBDBCONNECTOR_EXPORTS
#define HXLIBDBCONNECTOR_API __declspec(dllexport)
#else
#define HXLIBDBCONNECTOR_API __declspec(dllimport)
#endif

#include <string>
using namespace std;


class HXError
{
public:
	HXError() {};
	virtual ~HXError() {};
	virtual int GetErrorCode() { return 0; };
	virtual void GetErrorDestribe(char* errMsg, int len) { return; };

protected:
	int					m_ErrorCode;
	string			m_strErrorDescribe;

};



class HXDBService
{
public:
	HXDBService() {};
	virtual ~HXDBService() {};
	//创建数据库连接
	virtual int InitDBConn(char* hostName, unsigned short hostPort, char* databaseName, char* userName, char* password) = 0;
	//关闭连接
	virtual bool CloseConnect() = 0;
	//数据库是否打开
	virtual bool IsDBConnValid() = 0;

	virtual bool PreparedExcute(const std::string &szCommand) = 0;

	//执行查询(Select)
	virtual bool Query(const std::string &szCommand) = 0;
	//执行语句(Insert,Update,Delete)
	virtual bool Execute(const std::string &szCommand) = 0;
	//执行命令(存储过程）
	virtual bool ExecuteCommand(bool bRecordset) = 0;

	//获取当前记录集
	//virtual const ResultSet& GetRecordSet() = 0;
	//获取下一记录集
	virtual bool GetNextResultSet() = 0;
	//下一记录是否存在
	virtual bool NextFieldExist() = 0;
	//关闭记录集
	virtual bool CloseRecordset() = 0;
	//记录集是否打开
	virtual bool IsRecordsetOpened() = 0;



	//设置字段的值
	virtual bool setBigInt(unsigned int parameterIndex, const std::string& value) = 0;
	virtual bool setBlob(unsigned int parameterIndex, std::istream * blob) = 0;            //长文本字符串
	virtual bool setBoolean(unsigned int parameterIndex, bool value) = 0;
	virtual bool setDateTime(unsigned int parameterIndex, const std::string& value) = 0;
	virtual bool setDouble(unsigned int parameterIndex, double value) = 0;
	virtual bool setInt(unsigned int parameterIndex, int32_t value) = 0;
	virtual bool setUInt(unsigned int parameterIndex, uint32_t value) = 0;
	virtual bool setInt64(unsigned int parameterIndex, int64_t value) = 0;
	virtual bool setUInt64(unsigned int parameterIndex, uint64_t value) = 0;
	virtual bool setString(unsigned int parameterIndex, const std::string& value) = 0;
	virtual bool setNull(unsigned int parameterIndex, int sqlType) = 0;

	//获取字段的值
	virtual bool GetFieldValue(const std::string& columnLabel, bool &bValue) = 0;
	virtual bool GetFieldValue(const std::string& columnLabel, long double &dbValue) = 0;
	virtual bool GetFieldValue(const std::string& columnLabel, int32_t &nValue) = 0;
	virtual bool GetFieldValue(const std::string& columnLabel, uint32_t &uValue) = 0;
	virtual bool GetFieldValue(const std::string& columnLabel, int64_t &llValue) = 0;
	virtual bool GetFieldValue(const std::string& columnLabel, uint64_t &lluValue) = 0;
	virtual bool GetFieldValue(const std::string& columnLabel, char szBuffer[], uint32_t uSize) = 0;
	virtual bool GetFieldValue(const std::string& columnLabel, std::string &szValue) = 0;



};

HXLIBDBCONNECTOR_API HXDBService*	CreateDBService(void);

