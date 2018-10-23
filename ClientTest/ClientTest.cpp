// ClientTest.cpp : 定义控制台应用程序的入口点。
//
#include <Windows.h>
#include "HXLibNetwork.h"
#include "protocol.h"
#include <string>
#include "direct.h"
#include<vector>
#include <thread>

#include <conio.h>

#include<stdlib.h>
#include<time.h>
#define random(x) (rand()%x)


using namespace std;

//循环发送位置变换信息
void sendMsgFun(HXLibClient* client)
{


	//for (int count = 0; count < 1000; count++)
	//{
	//	int len = sizeof(int);
	//	int id = count;
	//	HXMessagePackage msgPackage;
	//	msgPackage.header()->id = ID_User_Verify;
	//	msgPackage.header()->id2 = c2s_req_id_verify;
	//	msgPackage.header()->length = len;
	//	memcpy(msgPackage.body(), (void*)&id, len);
	//	client->Send(msgPackage);

	//	Sleep(30);
	//}


	//for (int i = 0; i < 1000; i++)
	//{	
	//	
	//	srand((int)time(0));
	//	//发送座椅号
	//	int id = random(30) + 1;
	//	int len = sizeof(int);
	//	HXMessagePackage msgPackage;
	//	msgPackage.header()->id = ID_User_Login;
	//	msgPackage.header()->id2 = c2s_tell_seat_num;
	//	msgPackage.header()->length = len;
	//	memcpy(msgPackage.body(), (void*) &id, len);
	//	client->Send(msgPackage);
	//}


	//
	//Sleep(1000);

	////发送准备就绪消息
	//msgPackage.header()->id = ID_User_Login;
	//msgPackage.header()->id2 = c2s_tell_ready;
	//msgPackage.header()->length = 0;
	//client->Send(msgPackage);


	//Sleep(1000);

	////发送起飞命令
	//msgPackage.header()->id = ID_Global_Notify;
	//msgPackage.header()->id2 = c2s_begin_flying;
	//msgPackage.header()->length = 0;
	//client->Send(msgPackage);

	////发送用户外部可见命令
	//msgPackage.header()->id = ID_Global_Notify;
	//msgPackage.header()->id2 = c2s_seen_external;
	//msgPackage.header()->length = 0;
	//client->Send(msgPackage);

	//发送位置变换信息
	LOG(LogDebug, "发送位置变换信息\n");
	int x = 1;
	int y = 100;
	for (int count = 0; count < 10000; count++)
	{
		vec3 pos = { ++x, ++x, ++x };
		vec3 dir = { ++y, ++y, ++y };
		TransformInfo TransformInfo;
		TransformInfo.pos = pos;
		TransformInfo.dir = dir;
		int len = sizeof(TransformInfo) ;
		HXMessagePackage msgPackage;
		msgPackage.header()->id = ID_User_Transform; //发送位置变换信息
		msgPackage.header()->id2 = 0;
		msgPackage.header()->length = len;
		memcpy(msgPackage.body(), (void*)&TransformInfo, len);
		client->Send(msgPackage);

		Sleep(30);
	}


}


//#define CAPSULE_USERTYPE 
#define VIP_USERTYPE

using namespace std;
int main()
{
	char drive[512], dir[512];
	char buffer[HXMessagePackage::max_body_length] = { 0 };
	_splitpath(_pgmptr, drive, dir, 0, 0);
	strcat(drive, dir);
	strcat(drive, "log/Client_");
	//首先要开启日志系统文件
	SetupLogFiles(drive, true);
	//
	
	HXLibClient*	client = CreateNewClient();

	HXLibConfigService* confReader = CreateConfigReader();
	char path[MAX_PATH] = { 0 };
	_getcwd(path, MAX_PATH);
	strcat(path, "/ClientConfig.xml");
	bool ret = confReader->OpenFile(path);
	if (!ret)
	{
		LOG(LogError, "打开配置文件失败！\n");
		return -1;
	}


	char port[100] = { 0 };
	char ip[100] = { 0 };
	confReader->GetStr("root.Server.port", port);
	confReader->GetStr("root.Server.ip", ip);

	if (port == 0 || ip == 0)
	{
		LOG(LogError, "从配置文件中读取IP地址或端口号出错！");
		return -1;
	}

	if (client->Start(ip, port) != 0)
		return -1;

	//FILE* fp_out = fopen("ddd1.flv", "wb");

	//if (!fp_out)
	//{
	//	LOG(LogError, "打开文件出错！");
	//	return -1;
	//}


	while (true) {

		//if (_kbhit()) {//如果有按键按下，则_kbhit()函数返回真
		//	int ch = _getch();//使用_getch()函数获取按下的键值
		//	if (ch == 27) 
		//	{
		//		fflush(fp_out);
		//		fclose(fp_out);
		//		break; 
		//	}//当按下ESC时循环，ESC键的键值时27.
		//}

		HXMessageMap& mmap = client->SwapQueue();
		for (unsigned int i = 0; i < mmap.GetCount(); i++) {
			const HXMessagePackage* pack = mmap.GetPackage(i);
			switch (pack->header()->id)
			{
			case HXMessagePackage::Header::ID_Connect:
			{
				LOG(LogDebug, "链接成功了\n");


#ifdef VIP_USERTYPE



				std::thread tr(&sendMsgFun, client);
				tr.detach();
#endif

		
#ifdef CAPSULE_USERTYPE

				LOG(LogDebug, "请求脸模文件\n");
				char* userID = "UserID_00000001";
				int len = strlen(userID);
				HXMessagePackage msgPackage;
				msgPackage.header()->id = ID_User_Login;
				msgPackage.header()->id2 = c2s_req_fmd_name;  //请求脸模文件
				msgPackage.header()->length = len;
				memcpy(msgPackage.body(), (void*)userID, len);
				client->Send(msgPackage);


				LOG(LogDebug, "请求座席号\n");
				userID = "UserID_00000001";
				len = strlen(userID);
				msgPackage.header()->id = ID_User_Login;
				msgPackage.header()->id2 = c2s_req_seat_num;  //请求座席号
				msgPackage.header()->length = len;
				memcpy(msgPackage.body(), (void*)userID, len);
				client->Send(msgPackage);
#endif
				break;

			}

			case 	HXMessagePackage::Header::ID_ConnectFailure:
			{
					LOG(LogDebug, "连接服务器失败\n");
					break;
			}


			case HXMessagePackage::Header::ID_Disconnect:
			{
				int state = pack->header()->id2;
				if (HXMessagePackage::Header::Dissconnect_ClosedByMain != state)  //不是客户端主动断开的连接
				{
					LOG(LogDebug, "断开连接了\n");
				}

				break;
			}


#ifdef VIP_USERTYPE
			case ID_User_Notify:
			{
				int cmdID = pack->header()->id2;
				char buffer[100] = { 0 };

				if (s2c_client_list == cmdID)
				{

					const char * p = pack->body();
					int s = pack->body_length();

					if (s > 0)
					{
						int cout = s / sizeof(ProfileInfo);

						int v = 0;
						printf("+++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
						while (v < cout)
						{
							ProfileInfo* profileInfo = (ProfileInfo*)p;
							char buffer[100] = { 0 };
							sprintf(buffer, "%04d座椅开机\r\n", profileInfo->mSeatNumber);
							printf(buffer);

							p += sizeof(ProfileInfo);

							v++;
						}
						printf("///////////////////////////////////////////\r\n");
					}


				}
				else if (s2c_upd_user_profile == cmdID)
				{
					ProfileInfo profile = *(ProfileInfo*)(pack->body());
					sprintf(buffer, "%04d座椅更新了脸模数据\n", profile.mSeatNumber);
				}
				else if (s2c_ply_leave == cmdID)
				{
					int seatNum = *(int *)(pack->body());
					sprintf(buffer, "%04d座椅下线\n", seatNum);
				}

				printf(buffer);
				break;
			}

			case ID_Global_Notify:
			{
			
				int cmdID = pack->header()->id2;
				if (s2c_begin_flying == cmdID)
				{
					//向所有VIP客户端发出起飞命令
					printf("s2c_begin_flying\n");
				}
				else if (s2c_client_list_external == cmdID)
				{
					const char * p = pack->body();
					int s = pack->body_length();

					if (s > 0)
					{
						int cout = s / sizeof(ProfileInfo);

						int v = 0;
						printf("+++++++++++++++++++++++++++++++++++++++++++++++++\r\n");
						while (v < cout)
						{
							ProfileInfo* profileInfo = (ProfileInfo*)p;
							char buffer[100] = { 0 };
							sprintf(buffer, "s2c_client_list_external seat number：%d\r\n", profileInfo->mSeatNumber);
							printf(buffer);

							p += sizeof(ProfileInfo);

							v++;
						}
						printf("///////////////////////////////////////////////////\r\n");
					}
				}
				break;
			}
			case ID_Global_Transform:
			{
				const char * p = pack->body();
				int s = pack->header()->length;

				if (s > 0)
				{
					int cout = s / sizeof(TransformInfo);

					int v = 0;
					while (v < cout)
					{
						TransformInfo* transInfo = (TransformInfo*)p;
						char buffer[100] = { 0 };
						sprintf(buffer, "外部用户%04d发送位置变换信息[%4.2f_%4.2f_%4.2f]\n", transInfo->plyId, transInfo->pos.x, transInfo->pos.y, transInfo->pos.z);
						printf(buffer);

						p += sizeof(TransformInfo);

						v++;
					}
				}
				break;
			}

			case ID_FLV_Sequence_Header:
			case ID_FLV_Stream:
			{
				char* data = (char*)pack->body();
				int len = pack->length();
				//if (len > 32 * 1024)
				{
					
					//fwrite(data, 1, len, fp_out);

					//fflush(fp_out);

					char msg[100] = { 0 };
					sprintf(msg, "接收字节：%d\r\n", len);
					printf(msg);
				}
				break;
			}
			/*		case ID_OnlineUserList:
					{
						int plyId = *(int*)(pack->body());
						char buffer[100] = { 0 };
						sprintf(buffer, "%04d用户在线\n", plyId);
						printf(buffer);
						break;
					}*/

			case ID_User_Verify:
			{
				int plyId = *(int*)(pack->body());
				char buffer[100] = { 0 };
				sprintf(buffer, "%04d用户验证通过\n", plyId);
				printf(buffer);
				break;
			}
			case ID_User_Transform:
			{
				const char * p =pack->body();
				int s = pack->header()->length;
				
				if (s > 0)
				{
					int cout = s / sizeof(TransformInfo);
					
					int v = 0;
					while(v< cout)
					{
						TransformInfo* transInfo = (TransformInfo*)p;
						char buffer[100] = { 0 };
						sprintf(buffer, "%04d用户发送位置变换信息[%4.2f_%4.2f_%4.2f]\n", transInfo->plyId, transInfo->pos.x, transInfo->pos.y, transInfo->pos.z);
						printf(buffer);

						p += sizeof(TransformInfo);

						v++;
					}
				}
				break;
			}

#endif
			case ID_User_Login:
			{
				int cmdID = pack->header()->id2;

#ifdef CAPSULE_USERTYPE
				if (s2c_rsp_fmd_name == cmdID)
				{
					FaceModel fmd = *(FaceModel*)(pack->body());
					sprintf(buffer, "脸模文件名称:%s\n", fmd.faceModelFile);
					printf(buffer);
				}
				if (s2c_rsp_seat_num == cmdID)
				{
	
					int seatNum = *(int*)(pack->body());
					sprintf(buffer, "座椅号：%d\n", seatNum);
					printf(buffer);
				}
#endif
	
				if (s2c_upd_user_state == cmdID)
				{
					UserStateInfo usrStatInfo = *(UserStateInfo*)(pack->body());
					sprintf(buffer, "座椅号:%d，当前状态:%d\n", usrStatInfo.seatNumber, usrStatInfo.userState);
					printf(buffer);
					
				}

				if (c2s_tell_seat_num == cmdID)
				{
					int SeatID = *(int*)(pack->body());
					sprintf(buffer, "座椅号:%d\n", SeatID);
					printf(buffer);

				}

				
				
				break;

			}

			case ID_ERROR:
			{

				int errcode = pack->header()->id2;
				sprintf(buffer, "####发现错误！错误码是：%d\n", errcode);
				printf(buffer);
				break;
			}

			}
		}

		Sleep(1);

	}

	//fclose(fp_out);
	return 0;
}

