#include "stdafx.h"
#include "SvcTimer.h"
#include "protocol.h"

//毫秒级定时器， 每隔30毫秒触发一次， 更新玩家的位置信息
SvcTimer::SvcTimer(io_service& ios, PlayerManager*& playerMgr)
	: mTimer(ios, boost::posix_time::milliseconds(30)), mPlayMgr(playerMgr)
{
	mTimer.async_wait(boost::bind(&SvcTimer::handler, this));
}

SvcTimer::~SvcTimer()
{
}

void SvcTimer::handler()
{
	//为了防止在发送数据包时有玩家退出引起程序崩溃， 从这个地方加锁
	boost::mutex::scoped_lock lock(mPlayMgr->mMutex);		

	memset(buffer, 0, sizeof(buffer));
	char*p = buffer;
	int count = 0;

	for (PlayerManager::iterator i = mPlayMgr->begin(); i != mPlayMgr->end(); i++)
	{
		if ((sizeof(buffer) - count * sizeof(TransformInfo)) < sizeof(TransformInfo))
		{
			break;
		}
		Player* ply = (Player*)(*i);
		if (NULL != ply && ply->transInfo.update)
		{
			memcpy(p, (const void*)&ply->transInfo, sizeof(TransformInfo));
			ply->transInfo.update = false;

			p += sizeof(TransformInfo);
			count++;
		}
	}

	if(count > 0)
	{ 
		for (PlayerManager::iterator j = mPlayMgr->begin(); j != mPlayMgr->end(); j++)
		{
			Player* ply1 = (Player*)(*j);
			if (NULL != ply1)
			{
				mPlayMgr->SendCmd(ply1->GetLinkID(), ID_User_Transform, 0, &buffer, sizeof(TransformInfo)* count);
			}
		}

		if (mPlayMgr->GetUserVisibilityExternal())
		{
			mPlayMgr->GetCenterSvrConnection()->Send(ID_Global_Transform, 0, (char*)&buffer, sizeof(TransformInfo)* count);
		}

	}

	mTimer.expires_at(mTimer.expires_at() + boost::posix_time::milliseconds(30));
	mTimer.async_wait(boost::bind(&SvcTimer::handler, this));
}

