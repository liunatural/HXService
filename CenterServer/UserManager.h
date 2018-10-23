#pragma once
#include "User.h"
#include "HXLibNetwork.h"
#include <vector>
#include <map>
#include <boost/thread/mutex.hpp>

using namespace std;

//�û�������
class UserManager : public std::vector<User*>
{
public:
	UserManager();
	virtual ~UserManager();

	void AddUser(User* ply);
	bool DeleteUser(int plyId);
	void SetNetworkService(HXLibService** pService);

	//��������Ƶ��FLV��ʽ�ķ�װ����
	int SendFlvStream(char* data, int len);

	//��������Ƶ��Sequence_Header����(SPS/PPS��)
	bool SendFlvSeqHeaderData(User* user);
	
	//���ͼ򵥵���Ϣ
	void SendCmd(LinkID& linkID, int msgID, int cmdID, void* data, int len);

	//��������Ƶ��Sequence_Header����
	void CacheFlvSeqHeaderData(unsigned char* data, int len);

	int ForwardCommand(const HXMessagePackage *pkt);
	int ForwardMsg(LinkID& sourceLinkID, const HXMessagePackage *pkt);
	//�ص�����
	void	OnStreamReback(int plyId, unsigned char* data, int len);
	//���ó�OBS���Ϳͻ���
	void	SetOBSStreamClient(int plyId);
private:
	User* FindUser(int plyId);

public:
	boost::mutex			mMutex;

private:
	typedef map<uint8_t, FlvSeqHeader> FlvSeqHeaderMap;
	FlvSeqHeaderMap flvSeqHeaderMap;  //FLV Sequence Header ����
	HXLibService *mpService;
	HXBigMessagePackage* bigDataPkt;
	bool flvSeqHeaderChange;
};

