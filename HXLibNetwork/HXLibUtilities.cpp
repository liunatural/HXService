#include "HXLibUtilities.h"

#ifdef _WINDOWS
#include <Sensapi.h>
#endif

HXLibTimerLocal::HXLibTimerLocal()
{
	RestartTimer();
	m_nBeginTime.QuadPart = 0;
}
//重新开始计时
void				HXLibTimerLocal::RestartTimer()
{
	QueryPerformanceFrequency(&m_nFreq); // 获取时钟周期 
	QueryPerformanceCounter(&m_nBeginTime); // 获取时钟计数 
}
//返回当前时间
double				HXLibTimerLocal::GetTickTimer()
{
	LARGE_INTEGER nEndTime;
	QueryPerformanceCounter(&nEndTime);
	return (double)(nEndTime.QuadPart - m_nBeginTime.QuadPart) / (double)m_nFreq.QuadPart;
}
HXFPSTimer::HXFPSTimer()
{
	m_dLastTime = GetTimer()->GetTickTimer();
	m_dAvgDuration = 0;
	m_dAlpha = 1.0 / 100.0;
	m_dFPS = 0;
	m_dFrameAdd = 0;
	m_dFPSMax = 0;
	m_dFPSAvg = 0;
}
HXFPSTimer::~HXFPSTimer()
{

}
void			HXFPSTimer::Add(double dAdd)
{
	double dTime = GetTimer()->GetTickTimer();
	double deltaTime = dTime - m_dLastTime;
	m_dFrameAdd += dAdd;
	if (deltaTime > 1.0f) {//取固定时间间隔为1秒
		m_dFPS = m_dFrameAdd;
		if (m_dFPS > m_dFPSMax)
			m_dFPSMax = m_dFPS;
		m_dFPSAvg += m_dFPS;
		m_dFPSAvg *= 0.5;
		m_dFrameAdd = 0;
		m_dLastTime = dTime;
	}
}

HXLibTimerLocal g_timer;
HXLIBNETWORK_API HXLibTimer*	GetTimer(void)
{
	return &g_timer;
}
HXLIBNETWORK_API HXLibTimer*			CreateTimer(void)
{
	return new HXLibTimerLocal();
}
//临界锁定类
HXLibCritical::HXLibCritical()
{
	sect = new CRITICAL_SECTION();
	::InitializeCriticalSection((CRITICAL_SECTION*)sect);
}
HXLibCritical::~HXLibCritical()
{
	::LeaveCriticalSection((CRITICAL_SECTION*)sect);
	delete (CRITICAL_SECTION*)sect;
}
void			HXLibCritical::Enter()
{
	::EnterCriticalSection((CRITICAL_SECTION*)sect);
}
void			HXLibCritical::Leave()
{
	::LeaveCriticalSection((CRITICAL_SECTION*)sect);
}

unsigned int __stdcall	HXLibThread::HXThreadFunction(void* lpThreadParameter)
{
	HXLibThread* thread = (HXLibThread*)lpThreadParameter;
	thread->OnThread(thread->m_bAlive, thread->m_arg);
	thread->m_bClosedInThread = true;
	return 0;
}

HXLibThread::HXLibThread()
{
	m_arg = 0;
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAlive = false;
	m_bClosedInThread = false;
}
HXLibThread::~HXLibThread()
{
	CloseThread(-1);
}
bool			HXLibThread::OpenThread(const void* arg)
{
	if (m_hThread != INVALID_HANDLE_VALUE)
		return false;
	DWORD dwThreadId;
	m_bAlive = true;
	m_bClosedInThread = false;
	m_arg = arg;
	m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HXThreadFunction, this, 0, &dwThreadId);
	return (m_hThread != INVALID_HANDLE_VALUE);
}
bool			HXLibThread::CloseThread(int timeOut)
{
	if (m_hThread == INVALID_HANDLE_VALUE || !m_bAlive)
		return false;
	m_bAlive = false;
	unsigned int t = GetTickCount();
	while (!m_bClosedInThread) {
		if (timeOut > 0) {
			if ((GetTickCount() - t) >= timeOut)
			{
				TerminateThread(m_hThread, 0);
				break;
			}
		}
		Sleep(0);
	}
	CloseHandle(m_hThread);
	m_hThread = INVALID_HANDLE_VALUE;
	return true;
}
