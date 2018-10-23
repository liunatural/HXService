#ifndef Protocol_H
#define Protocol_H

#include "consts.h"


#pragma pack(push, 1)

#define net_protocol_ver 100

//消息ID
enum
{
	ID_User_Login							=1001,			//用户登录消息ID
	ID_User_Notify,												//用户通知消息ID
	ID_User_Transform,										//用户数据变换消息ID
	ID_User_Verify,												//用户验证消息ID
	ID_User_control,											//用户控制消息
	ID_FLV_Header,												//FLV头信息
	ID_FLV_Sequence_Header,								//FLV序列头信息
	ID_FLV_Stream,												//FLV数据流
	ID_Global_Notify,											//跨场景服务器的通知消息
	ID_Global_Transform,										//跨场景服务器的运动变换消息
	ID_ERROR,													//运行错误消息ID
	ID_FLV_StreamReback,									//数据流获取反馈消息
};

//命令ID
enum
{
	c2s_tell_seat_num = 1,									//VIP客户端连接后发送座位号给场景服务器
	c2s_tell_user_type,										//客户端类型：胶囊体客户端；VIP客户端；摄像机视角客户端	
	c2s_tell_ready,												//客户端已准备好
	c2s_req_seat_num,										//胶囊客户端请求座位号
	c2s_tell_user_profile,										//胶囊客户端登录后发送用户基本描述信息给场景服务器
	c2s_begin_flying,											//起飞命令
	c2s_seen_external,
	c2s_play_video,												//播放视频命令
	c2s_stand_up,												//玩家站起命令	
	c2s_walk,														//玩家行走命令
	c2s_present_flowers,										//玩家发起献花	
	c2s_stop_present_flowers,								//玩家结束献花	


	s2c_ply_ready = 101,										//服务器返回用户已准备好命令ID
	s2c_ply_leave,												//服务器返回用农离开命令ID
	s2c_begin_flying,											//用户起飞命令
	s2c_rsp_seat_num,										//下发座位号给胶囊体
	s2c_upd_user_profile,										//通知VIP客户端更新用户基本描述信息命令ID
	s2c_upd_user_state,
	s2c_client_list,
	s2c_client_list_external,									//其它场景服务器的用户列表
	s2c_user_leave_external,								//外部用户离开
	s2c_play_video,												//播放视频命令
	s2c_stand_up,												//玩家站起命令
	s2c_walk,														//玩家行走命令
	s2c_rsp_id_verify,

	s2s_req_usr_list = 200,
	//最大协议数值254，大了会出错
};

struct vec3
{
	float x;
	float y;
	float z;
};

struct TransformInfo
{
	int plyId;
	int seatNumber;				//座椅号
	vec3 pos;						//位置坐标
	vec3 head;						//头部朝向
	vec3 lhand;					//左手数据
	vec3 rhand;					//右手数据
	vec3 dir;						//身体朝向
	bool update;					//是否更新位置标志

	TransformInfo()
	{
		update = false;
	}

	TransformInfo(const TransformInfo& other)
	{
		this->pos = other.pos;
		this->head = other.head;
		this->dir = other.dir;
		this->lhand = other.lhand;
		this->rhand = other.rhand;
		this->plyId = other.plyId;
		this->seatNumber = other.seatNumber;
	}

	TransformInfo& operator = (const TransformInfo& other)
	{
		this->pos = other.pos;
		this->head = other.head;
		this->dir = other.dir;
		this->lhand = other.lhand;
		this->rhand = other.rhand;
		this->plyId = other.plyId;
		this->seatNumber = other.seatNumber;
		return *this;
	}
};

//存储用户ID和用户头像模型文件名字
struct  FaceModel
{
	char userID[USER_ID_LENGTH + 1] = {0};
	char faceModelFile[FILE_NAME_LENGTH + 1] = {0};
};

//存储用户基本描述信息
struct  ProfileInfo
{
	char SceneServerID[SCENE_SERVER_ID_LENGTH + 1] = {0};
	int mSeatNumber;
	FaceModel mFaceModel;
};

struct UserStateInfo
{
	char SceneServerID[SCENE_SERVER_ID_LENGTH + 1] = { 0 };
	int seatNumber;
	UserState userState;
};


struct LeaveUserInfo
{
	char SceneServerID[SCENE_SERVER_ID_LENGTH + 1] = { 0 };
	int mSeatNumber;
};

struct FlvStreamReback
{
	double		dTimer;//当前时间
	double		dEncodeTimer;//压缩延迟时间
	double		dOBStoCenter;//OBS到Center的发送延迟
	double		dCenterToClient;//从中心服务器到客户端的延迟
	double		dShowTime;//OBS端显示时候的时间
};

#pragma pack(pop)
#endif //Protocol_H