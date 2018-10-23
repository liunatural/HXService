#pragma once
#include "protocol.h"
#include  "HXLibMessage.h"

class Player
{
public:
	Player(LinkID linkID);
	virtual ~Player();
	LinkID& GetLinkID() { return mLinkID; };

	//场景服务器ID
	char* GetSceneServerID() { return mProfileInfo.SceneServerID; }
	void SetSceneServerID(char* pSceneSvrID) { memcpy(mProfileInfo.SceneServerID, pSceneSvrID, SCENE_SERVER_ID_LENGTH); }

	bool GetFlvSeqHeaderFlag() { return mFlvSeqHeaderFlag == true; }
	void SetFlvSeqHeaderFlag(bool flag) { mFlvSeqHeaderFlag = flag; }

	void SetSeatNumber(int seatNum) {mSeatNumber = mProfileInfo.mSeatNumber = seatNum;	}
	int GetSeatNumber() { return mSeatNumber; }



	void UpdateProfileInfo(ProfileInfo* profileInfo) { mProfileInfo = *profileInfo; }

	void SetUserType(int userType) {	mUserType = userType;}
	int GetUserType() { return mUserType; }


	void SetUserState(UserState userState) { mUserState = userState; }
	UserState GetUserState() { return mUserState; }




public:
	int		mPlyID;
	bool	bBoundUser;

	UserState mUserState;
	ProfileInfo mProfileInfo;
	TransformInfo transInfo;

private:
	int mSeatNumber;
	LinkID mLinkID;
	bool		mFlvSeqHeaderFlag;
	int		mUserType;			//1: VIP客户端， 2: 胶囊客户端

};