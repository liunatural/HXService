#include "User.h"
#include "UserManager.h"

User::User(LinkID linkID)
{
	mIsOBSClient = false;
	mUserType = SceneServer;	
	mDelayTimer = 0;
	mLinkID = linkID;
	mUserID = linkID.sid;
}


User::~User()
{
}

