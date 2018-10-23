#pragma once
#include "Player.h"
#include "HXLibNetwork.h"
#include <vector>
#include <boost/thread/mutex.hpp>

using namespace std;

//玩家管理类
class PlayerManager : public std::vector<Player*>
{
public:
	PlayerManager();
	virtual ~PlayerManager();

	//设置或者获取用户对外的可见性属性
	bool GetUserVisibilityExternal() { return user_visible_flag; }
	void SetUserVisibilityExternal(bool flag) {	user_visible_flag = flag; 	}

	void SetSceneServerID(char* pSceneSvrID) { memcpy(mSceneServerID, pSceneSvrID, SCENE_SERVER_ID_LENGTH); }

	//设置或者获取与中心服务器的网络连接服务
	HXLibClient* GetCenterSvrConnection() { return mpConnToCenterSvr; };
	void SetCenterSvrConnection(HXLibClient* pConn) { mpConnToCenterSvr = pConn; };

	//设置与本地场景服务器的连接服务
	void SetNetworkService(HXLibService* pService) { mpService = pService; };

	void AddPlayer(Player* ply);

	//向本地用户发送当前连线并且已经具有座席号的VIP客户端列表消息
	void SendClientList(LinkID& linkID);

	//向外部场景服务器发送本地用户列表
	void SendClientListToCenterServer();

	//广播发送某个玩家已准备好消息
	void SendUserReadyMsg(int plyId);

	//广播玩家离开消息
	bool SendPlayerLeaveMsg(int& plyId);

	//更新玩家的座位号
	bool UpdatePlayerSeatNumber(int plyId, int seatNumber);

	//更新玩家类型
	bool UpdateUserType(int plyId, UserType userType);

	//绑定用户脸模数据到座席号
	void BindFaceModeWithSeatNumber(LinkID& linkID, FaceModel* faceModel, int& plyId);

	//广播用户状态变化消息（空-虚影-就坐等状态）
	void BroadcastUserState(int plyId, int msgID, UserState userState);

	void BroadcastExternalUserState(const HXMessagePackage* pack);

	//广播用户控制命令
	void BroadcastControlCmd(int msgID, int cmdID);

	//更新玩家位置
	void UpdatePlayerTransform(int plyId, TransformInfo& transInfo);

	//发送音视频的FLV格式的封装数据
	int SendFlvStream(char* data, int len);

	//发送音视频的Sequence_Header数据(SPS/PPS等)
	bool SendFlvSeqHeaderData(Player* user);

	//缓存音视频的Sequence_Header数据
	void CacheFlvSeqHeaderData(unsigned char* data, int len);


	bool SendMsg(const HXMessagePackage& msgPackage);

	bool SendCmd(LinkID& linkID, int msgID, int cmdID, void* data, int len);

private:
	//查找一个玩家
	Player* FindPlayer(int plyId);

	//根据座椅号查找玩家ID
	Player* FindPlayerBySeatNumber(int seatNumber);

	//从玩家列表中删除一个玩家
	bool DeletePlayer(int plyId);

	//获取一个未绑定玩家的VIP客户端
	Player*  GetFreePlayer();

public:
	boost::mutex			mMutex;

private:
	typedef map<uint8_t, FlvSeqHeader> FlvSeqHeaderMap;

	HXLibService *mpService;
	HXBigMessagePackage* bigDataPkt;
	FlvSeqHeaderMap flvSeqHeaderMap;	//FLV Sequence Header 数组
	HXLibClient* mpConnToCenterSvr;
	bool flvSeqHeaderChange;
	bool user_visible_flag;						 //用户是否能够跨场景服务器可见的标志

	char user_list_buffer[HXMessagePackage::max_body_length] = { 0 };

	char mSceneServerID[SCENE_SERVER_ID_LENGTH + 1] = { 0 };
};

