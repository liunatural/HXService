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
	double	mDelayTimer;//延迟时间保存
	bool	mIsOBSClient;//是否OBS发送客户端
private:
	LinkID mLinkID;
	bool		mFlvSeqHeaderFlag;
};
