#pragma once
#include <chrono>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "boost/asio.hpp"
#include "HXLibNetwork.h"
#include "PlayerManager.h"

using namespace boost::asio;
using boost::asio::io_service;

//毫秒级定时器， 每隔30毫秒触发一次， 更新玩家的位置信息
class SvcTimer
{
public:
	SvcTimer(io_service& ios, PlayerManager*& playerMgr);
	virtual ~SvcTimer();

	void handler();

private:
	deadline_timer mTimer;
	PlayerManager* mPlayMgr;
	char buffer[HXMessagePackage::max_body_length] = { 0 };
};

