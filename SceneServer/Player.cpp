#include "stdafx.h"
#include "Player.h"
//#include "PlayerManager.h"

Player::Player(LinkID linkID)
{
	mUserType		= VIP;		//�½����ʵ��Ĭ��ָ��ΪVIP�ͻ����������
	bBoundUser		= false;
	mLinkID			= linkID;
	mPlyID				= linkID.sid;

	mSeatNumber = mProfileInfo.mSeatNumber = 0;
	SetUserState(state_initial);

}


Player::~Player()
{
}

