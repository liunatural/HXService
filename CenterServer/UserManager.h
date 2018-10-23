#pragma once
#include "User.h"
#include "HXLibNetwork.h"
#include <vector>
#include <map>
#include <boost/thread/mutex.hpp>

using namespace std;

//用户管理类
class UserManager : public std::vector<User*>
{
public:
	UserManager();
	virtual ~UserManager();

	void AddUser(User* ply);
	bool DeleteUser(int plyId);
	void SetNetworkService(HXLibService** pService);

	//发送音视频的FLV格式的封装数据
	int SendFlvStream(char* data, int len);

	//发送音视频的Sequence_Header数据(SPS/PPS等)
	bool SendFlvSeqHeaderData(User* user);
	
	//发送简单的消息
	void SendCmd(LinkID& linkID, int msgID, int cmdID, void* data, int len);

	//缓存音视频的Sequence_Header数据
	void CacheFlvSeqHeaderData(unsigned char* data, int len);

	int ForwardCommand(const HXMessagePackage *pkt);
	int ForwardMsg(LinkID& sourceLinkID, const HXMessagePackage *pkt);
	//回调函数
	void	OnStreamReback(int plyId, unsigned char* data, int len);
	//设置成OBS发送客户端
	void	SetOBSStreamClient(int plyId);
private:
	User* FindUser(int plyId);

public:
	boost::mutex			mMutex;

private:
	typedef map<uint8_t, FlvSeqHeader> FlvSeqHeaderMap;
	FlvSeqHeaderMap flvSeqHeaderMap;  //FLV Sequence Header 数组
	HXLibService *mpService;
	HXBigMessagePackage* bigDataPkt;
	bool flvSeqHeaderChange;
};

