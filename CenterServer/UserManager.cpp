#include "UserManager.h"

UserManager::UserManager()
{
	bigDataPkt = CreateBigPackage();
	bigDataPkt->SetAllowLostPackage(true, 1024 * 1024 * 100);

	flvSeqHeaderChange = false;
}


UserManager::~UserManager()
{
}


void UserManager::AddUser(User* ply)
{
	boost::mutex::scoped_lock lock(mMutex);
	push_back(ply);
}


void UserManager::SetNetworkService(HXLibService** pService)
{
	mpService = *pService;
}


int UserManager::ForwardCommand(const HXMessagePackage *pkt)
{
	LinkID linkID;
	User* user = NULL;

	boost::mutex::scoped_lock lock(mMutex);
	for (iterator i = begin(); i != end(); i++)
	{
		user = (User*)(*i);
		if (NULL != user)
		{
			linkID = user->GetLinkID();
			mpService->Send(linkID, *pkt);
		}
	}

	return 1;
}


int UserManager::ForwardMsg(LinkID& sourceLinkID, const HXMessagePackage *pkt)
{
	LinkID linkID;
	User* user = NULL;

	boost::mutex::scoped_lock lock(mMutex);
	for (iterator i = begin(); i != end(); i++)
	{
		user = (User*)(*i);
		if (NULL != user)
		{
			linkID = user->GetLinkID();
			if (sourceLinkID.sid != linkID.sid)//向除消息源用户外的其它用户转发此消息
			{
				mpService->Send(linkID, *pkt);
			}
		}
	}

	return 1;
}


int UserManager::SendFlvStream(char* data, int len)
{
	LinkID linkID;
	User* user = NULL;
	FlvStreamReback reback = *(FlvStreamReback*)(data + len - sizeof(FlvStreamReback));
	reback.dTimer = GetTimer()->GetTickTimer();
	memcpy(data + len - sizeof(FlvStreamReback), &reback, sizeof(FlvStreamReback));
	bigDataPkt->SetBigPackage(ID_FLV_Stream, 0, data, len);
	boost::mutex::scoped_lock lock(mMutex);
	for (iterator i = begin(); i != end(); i++)
	{
		user = (User*)(*i);
		if (NULL != user)
		{
			//如果是OBS发送客户端则不向自己发送回视频数据
			if (user->mIsOBSClient)
				continue;
			//如果还没有发送过SequenceHeader数据，或者SequenceHeader数据变了，就发送之 
			if (!user->GetFlvSeqHeaderFlag() || true == flvSeqHeaderChange)
			{
				SendFlvSeqHeaderData(user);

				flvSeqHeaderChange = false;
			}
			reback.dCenterToClient = user->mDelayTimer*0.5;
			reback.dTimer = GetTimer()->GetTickTimer();
			linkID = user->GetLinkID();
			mpService->Send(linkID, ID_FLV_StreamReback, reback);
			mpService->Send(linkID, *bigDataPkt);
		}
	}

	return 1;
}

void UserManager::OnStreamReback(int plyId, unsigned char* data, int len)
{
	boost::mutex::scoped_lock lock(mMutex);
	for (iterator i = begin(); i != end(); i++)
	{
		User* user = (User*)(*i);
		if (user->mUserID == plyId)
		{
			FlvStreamReback* reback = (FlvStreamReback*)data;
			user->mDelayTimer = GetTimer()->GetTickTimer() - reback->dTimer;
			break;
		}
	}
}
void UserManager::SetOBSStreamClient(int plyId)
{
	boost::mutex::scoped_lock lock(mMutex);
	for (iterator i = begin(); i != end(); i++)
	{
		User* user = (User*)(*i);
		if (user->mUserID == plyId)
		{
			user->mIsOBSClient = true;
			break;
		}
	}
}

bool UserManager::SendFlvSeqHeaderData(User* user)
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

void UserManager::CacheFlvSeqHeaderData(unsigned char* data, int len)
{
	
	if (!data || len <= 0)
	{
		return;
	}

	uint8_t dt = data[0];

	FlvSeqHeaderMap::iterator it;
	it = flvSeqHeaderMap.find(dt);
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


void UserManager::SendCmd(LinkID& linkID, int msgID, int cmdID, void* data, int len)
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

	mpService->Send(linkID, msgPackage);
}


User* UserManager::FindUser(int plyId)
{
	User* ply = NULL;
	User* plyTemp = NULL;

	for (iterator i = begin(); i != end(); i++)
	{
		plyTemp = (User*)(*i);
		if (plyTemp->mUserID == plyId)
		{
			ply = plyTemp;
			break;
		}
	}

	return ply;
}
bool UserManager::DeleteUser(int plyId)
{
	bool ret = false;
	User* ply = NULL;
	boost::mutex::scoped_lock lock(mMutex);
	for (iterator i = begin(); i != end(); i++)
	{
		ply = (User*)(*i);
		if (NULL != ply && (ply->mUserID == plyId))
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
