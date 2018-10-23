#include "PlayerManager.h"
#include <assert.h>

PlayerManager::PlayerManager()
{
	bigDataPkt = CreateBigPackage();

	SetUserVisibilityExternal(false);
}


PlayerManager::~PlayerManager()
{
}


void PlayerManager::AddPlayer(Player* ply)
{
	boost::mutex::scoped_lock lock(mMutex);
	push_back(ply);
}

void PlayerManager::SendClientList(LinkID& linkID)
{
	//为了防止在发送数据包时有玩家退出引起程序崩溃， 从这个地方加锁
	boost::mutex::scoped_lock lock(mMutex);

	memset(user_list_buffer, 0, sizeof(user_list_buffer));
	char*p = user_list_buffer;
	int count = 0;

	Player* ply = NULL;
	for (iterator i = begin(); i != end(); i++)
	{
		if ((sizeof(user_list_buffer) - count * sizeof(ProfileInfo)) < sizeof(ProfileInfo))
		{
			break;
		}

		ply = (Player*)(*i);
		if (NULL != ply && ply->GetUserType() == VIP && ply->GetSeatNumber() > 0)
		{
			memcpy(p, (const void*)&ply->mProfileInfo, sizeof(ProfileInfo));

			p += sizeof(ProfileInfo);
			count++;
		}
	}

	if (count > 0)
	{
		SendCmd(linkID, ID_User_Notify, s2c_client_list, user_list_buffer, sizeof(ProfileInfo)* count);
	}


}

void PlayerManager::SendClientListToCenterServer()
{
	//为了防止在发送数据包时有玩家退出引起程序崩溃， 从这个地方加锁
	boost::mutex::scoped_lock lock(mMutex);

	memset(user_list_buffer, 0, sizeof(user_list_buffer));
	char*p = user_list_buffer;
	int count = 0;

	Player* ply = NULL;
	for (iterator i = begin(); i != end(); i++)
	{
		if ((sizeof(user_list_buffer) - count * sizeof(ProfileInfo)) < sizeof(ProfileInfo))
		{
			break;
		}

		ply = (Player*)(*i);
		if (NULL != ply && ply->GetUserType() == VIP && ply->GetSeatNumber() > 0)
		{
			memcpy(p, (const void*)&ply->mProfileInfo, sizeof(ProfileInfo));

			p += sizeof(ProfileInfo);
			count++;
		}
	}

	if (count > 0)
	{
		mpConnToCenterSvr->Send(ID_Global_Notify, s2c_client_list_external, (const char*)user_list_buffer, sizeof(ProfileInfo)* count);
	}

}



bool PlayerManager::SendPlayerLeaveMsg(int& plyId)
{
	LinkID linkID;
	Player* ply = NULL;
	ply = FindPlayer(plyId);
	if (NULL == ply)
	{
		LOG(LogError, "[SendPlayerLeaveMsg] error：玩家ID在列表中不存在！");
		return false;
	}

	int seatNum = ply->GetSeatNumber();

	for (iterator i = begin(); i != end(); i++)
	{
		ply = (Player*)(*i);
		if (NULL != ply)
		{
			linkID = ply->GetLinkID();
			SendCmd(linkID, ID_User_Notify, s2c_ply_leave, &seatNum, sizeof(int));
		}
	}

	if (GetUserVisibilityExternal())
	{
		LeaveUserInfo leaveUserInfo;
		memcpy(leaveUserInfo.SceneServerID, ply->GetSceneServerID(), SCENE_SERVER_ID_LENGTH);
		leaveUserInfo.mSeatNumber = seatNum;

		mpConnToCenterSvr->Send(ID_Global_Notify, s2c_user_leave_external, (const char*)&leaveUserInfo, sizeof(LeaveUserInfo));
	}


	DeletePlayer(plyId);

	return true;

}

void PlayerManager::UpdatePlayerTransform(int plyId, TransformInfo &transInfo)
{

	boost::mutex::scoped_lock lock(mMutex);
	Player* ply = FindPlayer(plyId);
	if (NULL == ply)
	{
		LOG(LogError, "[UpdatePlayerTransform] 存储玩家位置变换数据出错：玩家ID在列表中不存在！");
		return;
	}

	ply->transInfo = transInfo;
	ply->transInfo.update = true;

}

bool PlayerManager::UpdatePlayerSeatNumber(int plyId, int seatNumber)
{

	boost::mutex::scoped_lock lock(mMutex);
	Player* ply = FindPlayer(plyId);
	if (NULL == ply)
	{
		LOG(LogError, "[UpdatePlayerSeatNumber] 更新玩家座位号出错：玩家ID在列表中不存在！");
		return false;
	}

	ply->SetSeatNumber(seatNumber);

	return true;
}


bool PlayerManager::UpdateUserType(int plyId, UserType userType)
{

	boost::mutex::scoped_lock lock(mMutex);
	Player* ply = FindPlayer(plyId);
	if (NULL == ply)
	{
		LOG(LogError, "[UpdatePlayerSeatNumber] 更新玩家类型出错：玩家ID在列表中不存在！");
		return false;
	}

	ply->SetUserType(userType);

	return true;
}


void PlayerManager::BindFaceModeWithSeatNumber(LinkID& linkID, FaceModel* faceModel, int& plyId)
{
	boost::mutex::scoped_lock lock(mMutex);
	Player* ply = GetFreePlayer();
	if (NULL == ply)
	{
		SendCmd(linkID, ID_ERROR, NO_AVAILABLE_SEAT_NUMBER, NULL, 0);
		LOG(LogError, "[SendSeatNumberMsg]分配座席号出错：没有可用的座席供分配！");
		return;
	}

	int seatNum = ply->GetSeatNumber();
	//向胶囊体客户端返回刚申请的当前座椅号
	SendCmd(linkID, ID_User_Login, s2c_rsp_seat_num, &seatNum, sizeof(int));
	
	ply->mProfileInfo.mFaceModel = *faceModel;
	
	//设置为绑定用户状态
	ply->bBoundUser = true;

	//返回用户ID
	plyId = ply->mPlyID;

}

void PlayerManager::SendUserReadyMsg(int seatNumber)
{
	LinkID linkID;
	Player* ply = NULL;
	Player* ply1 = NULL;

	ply = FindPlayerBySeatNumber(seatNumber);
	if (NULL == ply)
	{
		LOG(LogError, "[SendProfileUpdateMsg] 广播玩家基本信息出错：玩家ID在列表中不存在！");
		return;
	}

	if (ply->GetUserType() != VIP)//只广播VIP类型用户的状态
	{
		return;
	}

	for (iterator i = begin(); i != end(); i++)
	{
		ply1 = (Player*)(*i);
		if (NULL != ply1)
		{
			linkID = ply1->GetLinkID();
			SendCmd(linkID, ID_User_Login, s2c_ply_ready, &ply->mProfileInfo, sizeof(ProfileInfo));
		}
	}
}


void PlayerManager::BroadcastUserState(int plyId, int msgID, UserState userState)
{
	LinkID linkID;
	Player* ply = NULL;
	Player* ply1 = NULL;

	ply = FindPlayer(plyId);
	if (NULL == ply)
	{
		LOG(LogError, "[BroadcastUserState] error：玩家ID在列表中不存在！");
		return;
	}

	if (ply->GetUserType() != VIP) //只广播VIP类型用户的状态
	{
		return;
	}

	UserStateInfo usrStatInfo;
	memcpy(usrStatInfo.SceneServerID, mSceneServerID, SCENE_SERVER_ID_LENGTH);
	usrStatInfo.seatNumber = ply->GetSeatNumber();
	usrStatInfo.userState = userState;

	for (iterator i = begin(); i != end(); i++)
	{
		ply1 = (Player*)(*i);
		if (NULL != ply1 )
		{
			linkID = ply1->GetLinkID();
			SendCmd(linkID, msgID, s2c_upd_user_state, &usrStatInfo, sizeof(UserStateInfo));
		}
	}


	if (GetUserVisibilityExternal())
	{
		mpConnToCenterSvr->Send(msgID, s2c_upd_user_state, (const char*)&usrStatInfo, sizeof(UserStateInfo));
	}

}


void PlayerManager::BroadcastExternalUserState(const HXMessagePackage* pack)
{
	SendMsg(*pack);
}




void PlayerManager::BroadcastControlCmd(int msgID, int cmdID)
{
	LinkID linkID;
	Player* ply = NULL;
	for (iterator i = begin(); i != end(); i++)
	{
		ply = (Player*)(*i);
		if (NULL != ply && (ply->GetUserType() != Capsule))
		{
			linkID = ply->GetLinkID();
			SendCmd(linkID, msgID, cmdID, NULL, 0);
		}
	}
}


bool PlayerManager::SendMsg(const HXMessagePackage& msgPackage)
{

	LinkID linkID;
	Player* user = NULL;
	boost::mutex::scoped_lock lock(mMutex);
	for (iterator i = begin(); i != end(); i++)
	{
		user = (Player*)(*i);
		if (NULL != user)
		{
			linkID = user->GetLinkID();
			mpService->Send(linkID, msgPackage);
		}
	}

	return true;
}


bool PlayerManager::SendCmd(LinkID& linkID, int msgID, int cmdID, void* data, int len)
{
	HXMessagePackage msgPackage;
	msgPackage.header()->id = msgID;
	msgPackage.header()->id2 = cmdID;
	msgPackage.header()->length = 0;

	if ((NULL != data) && (0 != len))
	{
		msgPackage.header()->length = len;
		memcpy(msgPackage.body(), data, len);
	}

	return mpService->Send(linkID, msgPackage);
}


int PlayerManager::SendFlvStream(char* data, int len)
{
	LinkID linkID;
	Player* user = NULL;
	bigDataPkt->SetBigPackage(ID_FLV_Stream, 0, data, len);

	boost::mutex::scoped_lock lock(mMutex);
	for (iterator i = begin(); i != end(); i++)
	{

		user = (Player*)(*i);
		if (NULL != user)
		{
			//如果还没有发送过SequenceHeader数据，或者SequenceHeader数据变了，就发送之 
			if (!user->GetFlvSeqHeaderFlag() || true == flvSeqHeaderChange)
			{
				SendFlvSeqHeaderData(user);
				flvSeqHeaderChange = false;
			}

			linkID = user->GetLinkID();
			mpService->Send(linkID, *bigDataPkt);
		}
	}

	return 1;
}

bool PlayerManager::SendFlvSeqHeaderData(Player* user)
{
	bool ret = true;

	LinkID linkID = user->GetLinkID();
	HXBigMessagePackage* bigSeqHeaderPkt = CreateBigPackage();

	map<uint8_t, FlvSeqHeader>::iterator it;
	it = flvSeqHeaderMap.begin();

	while (it != flvSeqHeaderMap.end())
	{
		bigSeqHeaderPkt->SetBigPackage(ID_FLV_Sequence_Header, 0, it->second.data, it->second.size);
		ret = mpService->Send(linkID, *bigSeqHeaderPkt);
		if (!ret)
		{
			break;
		}

		it++;
	}

	if (ret)
	{
		user->SetFlvSeqHeaderFlag(true);
	}

	delete bigSeqHeaderPkt;
	return ret;
}


void PlayerManager::CacheFlvSeqHeaderData(unsigned char* data, int len)
{

	if (!data || len <= 0)
	{
		return;
	}

	uint8_t dt = data[0];

	boost::mutex::scoped_lock lock(mMutex);
	FlvSeqHeaderMap::iterator it = flvSeqHeaderMap.find(dt);
	if (it != flvSeqHeaderMap.end())
	{
		delete (*it).second.data;
		flvSeqHeaderMap.erase(it);

		flvSeqHeaderChange = true;
	}

	unsigned char* buf = new unsigned char[len];
	FlvSeqHeader confData;
	confData.id = data[0];
	confData.data = buf;
	confData.size = len;
	memcpy(buf, data, len);

	flvSeqHeaderMap.insert(make_pair(dt, confData));

}


////////////////////////////////////////////////////////////////////////////////////////////////////////

Player* PlayerManager::FindPlayer(int plyId)
{
	Player* ply = NULL;
	Player* plyTemp = NULL;

	for (iterator i = begin(); i != end(); i++)
	{
		plyTemp = (Player*)(*i);
		if (plyTemp->mPlyID == plyId)
		{
			ply = plyTemp;
			break;
		}
	}

	return ply;
}


Player* PlayerManager::FindPlayerBySeatNumber(int seatNumber)
{
	Player* ply = NULL;
	Player* plyTemp = NULL;

	for (iterator i = begin(); i != end(); i++)
	{
		plyTemp = (Player*)(*i);
		if (plyTemp->GetSeatNumber() == seatNumber)
		{
			ply = plyTemp;
			break;
		}
	}

	return ply;
}

Player*  PlayerManager::GetFreePlayer()
{
	Player* ply				= NULL;
	Player* plyTemp		= NULL;
	for (iterator i = begin(); i != end(); i++)
	{
		plyTemp = (Player*)(*i);
		if (NULL != plyTemp && !(plyTemp->bBoundUser) && (plyTemp->GetUserType() == VIP) )
		{
			ply = plyTemp;
			break;
		}
	}

	return ply;
}


bool PlayerManager::DeletePlayer(int plyId)
{
	bool ret = false;
	Player* ply = NULL;
	boost::mutex::scoped_lock lock(mMutex);
	for (iterator i = begin(); i != end(); i++)
	{
		ply = (Player*)(*i);
		if (NULL != ply && (ply->mPlyID == plyId))
		{
			delete ply;
			ply = NULL;
			this->erase(i);

			ret = true;
			break;
		}
	}

	return ret;
}
