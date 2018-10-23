#pragma once
#include "protocol.h"
#include  "HXLibMessage.h"

class UserManager;
class User
{
public:
	User(LinkID linkID);
	virtual ~User();
	LinkID& GetLinkID() { return mLinkID; };

	bool GetFlvSeqHeaderFlag() { return mFlvSeqHeaderFlag == true; }
	void SetFlvSeqHeaderFlag(bool flag) { mFlvSeqHeaderFlag = flag; }
public:
	int		mUserID;
	int		mUserType;				
	double	mDelayTimer;//�ӳ�ʱ�䱣��
	bool	mIsOBSClient;//�Ƿ�OBS���Ϳͻ���
private:
	LinkID mLinkID;
	bool		mFlvSeqHeaderFlag;
};
