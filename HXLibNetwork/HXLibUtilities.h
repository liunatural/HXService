#pragma once

#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include "HXLibNetwork.h"
class HXLibTimerLocal:public HXLibTimer
{
public:
	HXLibTimerLocal();
	virtual ~HXLibTimerLocal() {}
	
	virtual	void				RestartTimer();
	virtual	double				GetTickTimer();
public:
	LARGE_INTEGER				m_nFreq;
	LARGE_INTEGER				m_nBeginTime;
};

