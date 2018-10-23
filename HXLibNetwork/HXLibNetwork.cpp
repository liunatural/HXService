#include "HXLibNetwork.h"
#include "HXLibMessageQueue.h"
#include <boost/thread/mutex.hpp>
#include <boost/make_unique.hpp>

OnLogCallback	g_logCallback = 0;
HXLIBNETWORK_API void			SetLogCallback(OnLogCallback cb)
{
	g_logCallback = cb;
}
class LogFile {
public:
	LogFile() {
		m_file = 0;
		m_bConsole = true;
		m_filePath[0] = 0;
	}
	~LogFile() {
		if (m_file) {
			LOG(LogDefault, "StopLogSystem\n");
			fclose(m_file);
		}
	}
	void		write(LogChannelInfo& channel, const char* buf, int length) {
		if (!channel.bOutput)
			return;
		//多线程加锁写入
		char temp[MAX_LOG_TEXT_LENGTH + 128];//最大format一行日志8192字节
		SYSTEMTIME time;
		GetLocalTime(&time);
		int len = sprintf(temp, "[%d]\t%d-%02d-%02d\t%02d:%02d:%02d(%04d)\t%s\t%s", ::GetCurrentProcessId(), time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, channel.name, buf);
		{
			boost::mutex::scoped_lock lock(m_mutex);
			if (m_openTime.wDay != time.wDay) {
				if (m_file) {
					fclose(m_file);
					m_file = 0;
				}
			}
			if (!m_file) {
				if (!open())
					return;
			}
			fwrite(temp, len, 1, m_file);
			if (channel.bConsole) {
				char temp[MAX_LOG_TEXT_LENGTH + 128];//最大format一行日志8192字节
				len = sprintf(temp, "%02d:%02d:%02d(%04d)%s %s", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, channel.name, buf);
				if (g_logCallback)
					(*g_logCallback)(channel.index, temp, len);
				printf("%s", temp);
			}
		}
	}
	char			m_filePath[256];
protected:
	SYSTEMTIME		m_openTime;
	bool			m_bConsole;
	FILE*			m_file;
	boost::mutex	m_mutex;
	bool		open() {
		GetLocalTime(&m_openTime);
		char file[1024];
		sprintf(file, "%s%d-%02d-%02d.log", m_filePath, m_openTime.wYear, m_openTime.wMonth, m_openTime.wDay);
		bool bExist = false;
		FILE* temp = fopen(file, "rb");
		if (temp) {
			bExist = true;
			fclose(temp);
		}
		if (m_file)
			fclose(m_file);
		if (bExist)
			m_file = fopen(file, "a+b");
		else
			m_file = fopen(file, "wb");
		return (m_file != 0);
	}
};
LogChannelInfo	g_logChannel[LogChannel_Count];
LogFile			g_logFile;
//如果不想调用SetLogFile来一个一个初始化则使用以下函数来一次性生成所有的日志文件,仅创建LogDefault,LogError,LogDebug三种日志状态
HXLIBNETWORK_API void			SetupLogFiles(const char* szPath, bool bUseDebugLog)
{
	strcpy(g_logFile.m_filePath, szPath);
	strcpy(g_logChannel[LogDebug].name, "debug");
	strcpy(g_logChannel[LogDefault].name, "info");
	strcpy(g_logChannel[LogError].name, "error");
	g_logChannel[LogDebug].bOutput = bUseDebugLog;
	LOG(LogDefault, "StartLogSystem\n");
	for (int i = 0; i < LogChannel_Count; i++) {
		g_logChannel[i].index = i;
	}
}
HXLIBNETWORK_API void			SetLogInfo(int channel/*LogChannel*/, const LogChannelInfo& info)
{
	if (channel < 0 || channel >= LogChannel_Count)
		return;
	g_logChannel[channel] = info;//
}

//写入日志
HXLIBNETWORK_API void			LOG(int channel, const char* format, ...)
{
	if (channel < 0 || channel >= LogChannel_Count)
		return;
	char temp[MAX_LOG_TEXT_LENGTH];//最大format一行日志8192字节
	int len = _vsnprintf(temp, MAX_LOG_TEXT_LENGTH, format, (char*)(&format + 1));
	//日志格式化失败
	if (len < 0 || len>= MAX_LOG_TEXT_LENGTH)
		return;
	temp[len] = 0;
	g_logFile.write(g_logChannel[channel], temp, len);
}
HXLIBNETWORK_API HXBigMessagePackage*	CreateBigPackage(void)
{
	return new HXBigMessageQueue();
}
HXLIBNETWORK_API HXLibMessageQueueAB*	CreateMessageQueue(void)
{
	return new HXMessageQueueAB();
}
HXLIBNETWORK_API void			SleepInThread(int millisecond)
{
	Sleep(millisecond);
}

