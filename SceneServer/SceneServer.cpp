#include "HXLibNetwork.h"
#include <vector>
#include "../sharetest.h"
#include "PlayerManager.h"
#include <direct.h>
#include <thread>
#include  "SvcTimer.h"
#include "ErrorMsg.h"
#include "direct.h"
#include "io.h"


void TimeFun(PlayerManager *playerMgr)
{
	io_service io;
	SvcTimer p(io, playerMgr);
	io.run();
}

int main()
{
	char drive[512], dir[512];
	_splitpath(_pgmptr, drive, dir, 0, 0);
	strcat(drive, dir);
	strcat(drive, "log/");

	//日志目录不存在，就创建它
	if (0 != _access(drive, 0))
	{
		if (0 != _mkdir(drive))
		{
			cout << "创建日志目录失败！" << endl;
			return -1;
		}
	}

	strcat(drive, "SceneServer/");
	if (0 != _access(drive, 0))
	{
		if (0 != _mkdir(drive))
		{
			cout << "创建日志目录失败！" << endl;
			return -1;
		}
	}

	//*****首先要开启日志系统文件*****//
	SetupLogFiles(drive, true);

	//*****读取配置文件*****//
	HXLibConfigService* confReader = CreateConfigReader();
	char path[MAX_PATH] = { 0 };
	_getcwd(path, MAX_PATH);
	strcat(path, "/SceneService.xml");
	bool ret = confReader->OpenFile(path);
	if (!ret)
	{
		LOG(LogError, "配置文件读取失败\n");
		return FAIL;
	}

	//*****启动场景服务器*****//
	int port = confReader->GetInt("root.Server.port");
	int maxlinks = confReader->GetInt("root.Server.maxlinks");
	char sceneServerID[SCENE_SERVER_ID_LENGTH + 1] = { 0 };
	confReader->GetStr("root.Server.name", sceneServerID);
	
	HXLibService*	service = CreateNewService();
	if (!service->Start(port, maxlinks))
	{
		return FAIL;
	}

	//*****创建用户管理器*****//
	PlayerManager *playerMgr = new PlayerManager();
	playerMgr->SetNetworkService(service);

	playerMgr->SetSceneServerID(sceneServerID);

	//*****连接中心服务器*****//
	char centerServerIP[50] = { 0 };
	char centerServerPort[50] = { 0 };
	confReader->GetStr("root.CenterServer.ip", centerServerIP);
	confReader->GetStr("root.CenterServer.port", centerServerPort);
	HXLibClient* 	centerServerConn = CreateNewClient();
	if (centerServerConn->Start(centerServerIP, centerServerPort) != 0)
	{
		LOG(LogError, "连接中心服务器失败!\n");
		return ERR_CONNECT_CENTER_SERVER;
	}
	playerMgr->SetCenterSvrConnection(centerServerConn);



	//*****启动定时器处理玩家位置变换信息*****//
	std::thread tr(&TimeFun, playerMgr);
	tr.detach();


	//*****总消息循环处理	*****//
	while (true)
	{
		//处理与本地用户的消息交互
		HXMessageMap& mmap = service->SwapQueue();
		for (int i = 0; i < mmap.GetCount(); i++)
		{
			const HXMessagePackage* pack = mmap.GetPackage(i);

			LinkID linkID = pack->linkid();
			int plyId = (pack->linkid()).sid;

			switch (pack->header()->id)		//根据消息ID和命令ID分别进行逻辑处理
			{
			case HXMessagePackage::Header::ID_Connect:
			{
				Player*  ply = new Player(pack->linkid());
				ply->SetSceneServerID(sceneServerID);
				playerMgr->AddPlayer(ply);
				
				playerMgr->SendClientList(linkID);
				break;
			}
			case HXMessagePackage::Header::ID_Disconnect:
			{
				playerMgr->SendPlayerLeaveMsg(plyId);
				break;
			}
			case ID_User_Notify:
			{
				int cmdID = pack->header()->id2;
				break;

			}
			case ID_Global_Notify:
			{
				int cmdID = pack->header()->id2;

				if (c2s_begin_flying == cmdID)
				{
					//向中心服务器转发此消息
					centerServerConn->Send(ID_Global_Notify, s2c_begin_flying, NULL, 0);
				}
				else if (c2s_play_video == cmdID)
				{
					//向中心服务器转发播放视频消息
					centerServerConn->Send(ID_Global_Notify, s2c_play_video, NULL, 0);
				}
				else if (c2s_stand_up == cmdID)
				{
					//向中心服务器转发用户站立消息
					centerServerConn->Send(ID_Global_Notify, s2c_stand_up, NULL, 0);
				}
				else if (c2s_walk == cmdID)
				{
					//向中心服务器转发用户开始行走消息
					centerServerConn->Send(ID_Global_Notify, s2c_walk, NULL, 0);
				}
				else if (c2s_present_flowers == cmdID)
				{
					//向其他VIP客户端广播当前用户的状态为献花状态
					playerMgr->BroadcastUserState(plyId, ID_User_Login, state_present_flowers);
				}
				else if (c2s_stop_present_flowers == cmdID)
				{
					//向其他VIP客户端广播当前用户的状态为结束献花状态
					playerMgr->BroadcastUserState(plyId, ID_User_Login, state_stop_present_flowers);
				}
				else if (c2s_seen_external == cmdID)
				{
					//******此命令只收到一次***********//

					//设置用户跨场景服务器可见
					playerMgr->SetUserVisibilityExternal(true);

					//向中心服务器发送当前场景服务器上的用户列表
					playerMgr->SendClientListToCenterServer();

					//向中心服务器请求其他场景服务器上的用户列表消息
					centerServerConn->Send(ID_Global_Notify, s2s_req_usr_list, NULL, 0);
				}
				break;
			}
			case ID_User_Login:
			{
				int cmdID = pack->header()->id2;
				char* userID = (char*)pack->body();
				int len = pack->header()->length;

				if (c2s_tell_seat_num == cmdID)		//接收从VIP客户端发来的座椅号消息
				{
					int seatNum = *(int*)pack->body();

					//绑定座位号到一个玩家
					bool bRet = playerMgr->UpdatePlayerSeatNumber(plyId, seatNum);
					if (bRet)
					{
						//向其他VIP客户端广播当前用户的状态为初始状态
						playerMgr->BroadcastUserState(plyId, ID_User_Login, state_initial);
					}

					//******每增加一个用户就执行一次******//
					if (playerMgr->GetUserVisibilityExternal())
					{
						//向中心服务器发送当前场景服务器上的用户列表
						playerMgr->SendClientListToCenterServer();

						//向中心服务器请求其他场景服务器上的用户列表消息
						centerServerConn->Send(ID_Global_Notify, s2s_req_usr_list, NULL, 0);
					}

				}
				else if (c2s_tell_user_type == cmdID)		//接收从VIP客户端发来的用户类型消息
				{
					int userType = *(int*)pack->body();

					//更新用户类型
					bool bRet = playerMgr->UpdateUserType(plyId, (UserType)userType);

				}
				else if (c2s_tell_user_profile == cmdID)	//接收从胶囊体客户端发出的用户ID和脸模文件名称
				{
					int retPlyId;
					FaceModel* faceModel = (FaceModel*)pack->body();
					playerMgr->BindFaceModeWithSeatNumber(linkID, faceModel, retPlyId);

					//向其他VIP客户端广播当前用户的状态为虚影状态
					playerMgr->BroadcastUserState(retPlyId, ID_User_Login, state_entering);
				}
				else if (c2s_tell_ready == cmdID)		//接收从VIP客户端发来的准备就绪消息
				{
					//向其他VIP客户端广播当前用户的状态为就绪状态
					int seatNum = *(int*)pack->body();
					playerMgr->SendUserReadyMsg(seatNum);
				}
				break;
			}
			case ID_User_Transform:		//玩家位置变换消息
			{
				TransformInfo* transInfo = (TransformInfo*)pack->body();
				transInfo->plyId = plyId;

				playerMgr->UpdatePlayerTransform(plyId, *transInfo);

				break;
			}
			//case ID_User_Verify:
			//{
			//	int cmdID = pack->header()->id2;
			//	if (c2s_req_id_verify == cmdID)
			//	{
			//		HXMessagePackage msgPackage;
			//		msgPackage.header()->id = ID_User_Verify; //发送位置变换信息
			//		msgPackage.header()->id2 = c2s_req_id_verify;
			//		msgPackage.header()->length = pack->length() + 4;

			//		memcpy(msgPackage.body(), (void*)pack, pack->length() + 4);

			//		int userID = *(int*)(pack->body());
			//		//cout << "用户" << userID << "请求验证！" << endl;

			//		centerServerConn->Send(msgPackage); //向中心服务器发送用户身份验证请求
			//	}
			//	break;
			//}
			}//switch end

		}// for end
		
		//接收并处理中心服务器的消息（目前是接收并转发OBS的FLV音视频流，还有用户身份验证信息）
		if (centerServerConn)
		{
			HXMessageMap& mmap1 = centerServerConn->SwapQueue();
			for (int i = 0; i < mmap1.GetCount(); i++)
			{
				const HXMessagePackage* pack = mmap1.GetPackage(i);
				LinkID linkID = pack->linkid();
				switch (pack->header()->id)
				{
				case ID_Global_Notify:
				{
					int cmdID = pack->header()->id2;
					if (s2c_begin_flying == cmdID)
					{
						//向所有VIP客户端发出起飞命令
						playerMgr->BroadcastControlCmd(ID_User_control, s2c_begin_flying);
					}
					else if (s2c_play_video == cmdID)
					{
						//向所有VIP客户端发出播放视频命令
						playerMgr->BroadcastControlCmd(ID_User_control, s2c_play_video);
					}
					else if (s2c_stand_up == cmdID)
					{
						//向所有VIP客户端发出站立命令
						playerMgr->BroadcastControlCmd(ID_User_control, s2c_stand_up);
					}
					else if (s2c_walk == cmdID)
					{
						//向所有VIP客户端发出开始行走命令
						playerMgr->BroadcastControlCmd(ID_User_control, s2c_walk);
					}
					else if (s2c_client_list_external == cmdID)
					{
						//向所有VIP客户端发送中心服务器转发过来的其它场景服务器上的用户列表
						playerMgr->SendMsg(*pack);
					}
					else if (s2c_user_leave_external == cmdID)
					{
						//向所有VIP客户端发送中心服务器转发过来的其它场景服务器上的用户离开消息
						playerMgr->SendMsg(*pack);
					}
					else if (s2s_req_usr_list == cmdID)
					{
						//设置用户跨场景服务器可见
						playerMgr->SetUserVisibilityExternal(true);

						playerMgr->SendClientListToCenterServer();
					}
					break;
				}
				case ID_User_Login:
				{
					int cmdID = pack->header()->id2;
					if (s2c_upd_user_state == cmdID) 
					{
						//向所有VIP客户端发送中心服务器转发过来的其它场景服务器上的用户状态变化消息(开始献花，结束献花等等)
						playerMgr->BroadcastExternalUserState(pack);
					}
					break;
				}
				case ID_Global_Transform:
				{
					//
					playerMgr->SendMsg(*pack);
					break;
				}
				//case ID_User_Verify:
				//{
				//	int cmdID = pack->header()->id2;
				//	if (s2c_rsp_id_verify == cmdID)
				//	{

				//		HXMessagePackage* pMsgPackage = (HXMessagePackage*)pack->body();
				//		LinkID usrLnkID = pMsgPackage->linkid();

				//		int userID = *(int*)(pMsgPackage->body());
				//		cout << "用户" << userID << "验证完毕！" << endl;

				//		service->Send(usrLnkID, *pMsgPackage);		//向胶囊体客户端发送用户身份验证结果
				//	}
				//	break;
				//}
				case ID_FLV_Sequence_Header:
				{
					uint8_t* data = (uint8_t*)pack->body();
					int len = pack->body_length();
					playerMgr->CacheFlvSeqHeaderData(data, len);

					break;
				}
				case ID_FLV_Stream:
				{
					char* data = (char*)pack->body();
					int len = pack->body_length();
					{
						playerMgr->SendFlvStream(data, len);
					}
					break;
				}

				} //switch end
			} // for end
		}// if end 	
		
		


		Sleep(1);
	
	}

	return 0;
}

