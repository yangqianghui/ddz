#include <algorithm>
#include <vector>
#include "XtRobotClient.h"


#include<sys/socket.h>
#include <unistd.h>
#include <string.h> 
#include<stdio.h>
#include<sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "proto.h"








XtRobotClient::XtRobotClient(struct ev_loop* evloop)
{
	m_evFoldTimer.data=this;
	m_evFollowTimer.data=this;
	m_evSeeTimer.data=this;
	m_evWrite.data=this;
	m_evRead.data=this;
	m_evCompareTimer.data=this;
	m_evAllInTimer.data=this;

	m_evChoiceCardTimer.data=this;

	m_evGrabBankerTimer.data=this;

	m_evDoubleBetTimer.data = this;

	m_evloop=evloop;

	m_header=(struct Header*)m_headerBuf;

	m_serverfd=-1;

	m_uid=0;
	m_hasSee=false;
	m_isBetting=false;
	m_isAllIn=false;
	m_seatid=-1;
	m_state=XT_PARSE_HEADER;
	m_curRound=0;

	mPlayerMap.clear();          //记录玩家信息  2016-4-21
	mBankerUid = 0;              //庄家UID

}


XtRobotClient::~XtRobotClient()
{
	if(m_serverfd!=-1)
	{
		ev_io_stop(m_evloop,&m_evWrite);
		ev_io_stop(m_evloop,&m_evRead);
		ev_timer_stop(m_evloop,&m_evFoldTimer);
		ev_timer_stop(m_evloop,&m_evFollowTimer);
		ev_timer_stop(m_evloop,&m_evSeeTimer);
		
		ev_timer_stop(m_evloop,&m_evChoiceCardTimer);
		ev_timer_stop(m_evloop,&m_evGrabBankerTimer);
		ev_timer_stop(m_evloop,&m_evDoubleBetTimer);
		
		
		close(m_serverfd);
	}

	while(!m_writeQueue.empty())
	{
		delete m_writeQueue.front();
		m_writeQueue.pop_front();
	}
}





void XtRobotClient::onReadData( struct ev_loop* loop, struct ev_io* w, int revents)
{

	int ret;
	static char recv_buf[XT_DEF_BUF_LEN];

	XtRobotClient* self = (XtRobotClient*) w->data;

	if (self->m_state == XT_PARSE_HEADER) 
	{
		ret = read(self->m_serverfd, &self->m_headerBuf[self->m_curHeaderLen],
				sizeof(struct Header) - self->m_curHeaderLen);

		if (ret < 0) 
		{
			if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) 
			{
				printf("read cb read header failed[%s]\n", strerror(errno));
				return;
			}

			self->closeConnect();

			return;
		}


		if (ret == 0) 
		{
			printf("connection close in read header[%d]\n", self->m_serverfd);
			self->closeConnect();
			return;
		}

		self->m_curHeaderLen+= ret;

		if (self->m_curHeaderLen== sizeof(struct Header)) 
		{
			if (self->m_header->length > XT_MAX_BUF_LEN || self->m_header->length == 0) 
			{
				self->closeConnect();
				return;
			}

			self->m_state = XT_PARSE_BODY;
			self->m_curHeaderLen= 0;
			self->m_body.clear();
		}
	} 
	else if (self->m_state == XT_PARSE_BODY) 
	{
		ret = read(self->m_serverfd, recv_buf, self->m_header->length - self->m_body.length());


		if (ret < 0) 
		{
			if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) 
			{
				printf("read body failed[%s]\n", strerror(errno));
				return;
			}
			printf("read body failed[%s]\n", strerror(errno));
			self->closeConnect();
			return;
		}

		if (ret == 0) 
		{
			printf("connection close in read body[%d]\n", self->m_serverfd);
			self->closeConnect();
			return;
		}


		recv_buf[ret] = '\0';
		self->m_body.append(recv_buf, ret);

		if (ret/*self->m_body.length()*/ == self->m_header->length) 
		{
			self->m_state = XT_PARSE_HEADER;

			if (self->m_packet.parse(self->m_body, ret) < 0) 
			{
				printf("parse err!!\n");
				self->closeConnect();
				return;
			}

			self->onReciveCmd(self->m_packet);

		}
    } 

}

void XtRobotClient::onWriteData(struct ev_loop *loop, struct ev_io *w, int revents)
{

	XtRobotClient* self = (XtRobotClient*) w->data;

	if (self->m_writeQueue.empty()) 
	{
		ev_io_stop(EV_A_ w);
		return;
	}
	printf("WriteData To Server\n");

	XtBuffer* buffer = self->m_writeQueue.front();

	ssize_t written = write(self->m_serverfd, buffer->m_data, buffer->m_len);

	if (written < 0) {
		if (errno == EAGAIN || errno == EINPROGRESS || errno == EINTR) {
			printf("write failed[%s]\n", strerror(errno));
			return;
		}
		/* todo close this client */
		printf("unknow err in written [%d]\n", self->m_serverfd);
		self->closeConnect();
		return;
	}

	self->m_writeQueue .pop_front();
	delete buffer;
}


void XtRobotClient::onDoFold(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;

	self->sendFoldPackage();
}

void XtRobotClient::onDoFollow(struct ev_loop* loop,struct ev_timer* w,int events)
{

	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendFollowPackage();
}

void XtRobotClient::onDoSee(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendSeePackage();
}

void XtRobotClient::onDoCompare(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendComparePacket();
}

void XtRobotClient::onDoAllIn(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendAllInPacket();
}


void XtRobotClient::onChoiceCard(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendChoiceCardPacket();
}


//抢庄加倍
void XtRobotClient::onGrabBanker(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendGrabBankerPacket();
}

//下注加倍
void XtRobotClient::onDoubleBet(struct ev_loop* loop,struct ev_timer* w,int events)
{
	ev_timer_stop(loop,w);
	XtRobotClient* self = (XtRobotClient*) w->data;
	self->sendDoubleBetPacket();
}


int XtRobotClient::closeConnect()
{
	if(m_serverfd!=-1)
	{
		ev_io_stop(m_evloop,&m_evWrite);
		ev_io_stop(m_evloop,&m_evRead);
		ev_timer_stop(m_evloop,&m_evFoldTimer);
		ev_timer_stop(m_evloop,&m_evFollowTimer);
		ev_timer_stop(m_evloop,&m_evSeeTimer);

		ev_timer_stop(m_evloop,&m_evChoiceCardTimer);
		ev_timer_stop(m_evloop,&m_evGrabBankerTimer);
		
		close(m_serverfd);
	}
	m_serverfd=-1;
	while(!m_writeQueue.empty())
	{
		delete m_writeQueue.front();
		m_writeQueue.pop_front();
	}
	return 0;
}

///---机器人收到服务器发来的数据
int XtRobotClient::onReciveCmd(Jpacket& data)
{
    Json::Value &val = data.tojson();
    int cmd = val["cmd"].asInt();

	switch(cmd)
	{
	case GRAB::SERVER_LOGIN_ERR_UC:
		printf("error:登录失败应答\n");
		break;

	case GRAB::SERVER_LOGIN_SUCC_UC:
		printf("登录成功应答\n");
		break;

	case GRAB::SERVER_TABLE_INFO_UC:
		printf("发送房间信息\n");
		handleTableInfo(val);
		break;

	case GRAB::SERVER_LOGIN_SUCC_BC:
		printf("广播登录成功\n");
		break;

	case GRAB::SERVER_GAME_WAIT_BC://等待
		printf("广播游戏等待(清理房间)\n");
		break;

	case GRAB::SERVER_GAME_READY_BC://准备
		printf("广播游戏准备\n");
		break;

	case GRAB::SERVER_FIRST_CARD_UC:  //首次发牌
		printf("先给玩家发4张牌\n");
		handleSendCard(val);
		break;

	case GRAB::SERVER_MAKE_BANKER_BC://定庄
		printf("广播随机庄家,游戏开始\n");
		handleMakeBanker(val);
		break;

	case GRAB::SERVER_SECOND_CARD_UC:   //二次发牌
		printf("给玩家发最后1张牌\n");
		handleSecondCard(val);
		break;

	case GRAB::SERVER_GROUP_CARD_UC:
		printf("组牌应答\n");			
		break;

	case GRAB::SERVER_GROUP_CARD_BC:
		printf("广播玩家组牌\n");
		break;

	case GRAB::SERVER_COMPARE_CARD_BC:
		printf("广播比牌，游戏结束\n");
		break;
	}
	printf("onReciveCmd(%s)\n",val.toStyledString().c_str());
	return 0;
}

//给玩家发牌
void XtRobotClient::handleSendCard(Json::Value& data)
{
	int size= data["holes"].size(); 
	
	for(int i=0; i<size; ++i)
	{
		if (i < 5)
		{
			m_mycard[i]=data["holes"][i].asInt();
		}
	}

	//用户抢庄
	doGrabBanker();

	//更新下用户信息
	addPlayer(data);
}


///二次发牌（第5张牌）
void XtRobotClient::handleSecondCard(Json::Value& data)
{
	int size= data["holes"].size(); 
	int i = 0;

	if (size > 0)
	{
		m_mycard[4]=data["holes"][i].asInt();
	}	

	//选牌组牌
	doChoiceCard();
}


//处理定庄
void XtRobotClient::handleMakeBanker(Json::Value& data)
{

	//非庄下注
	doDoubleBet();
}

void XtRobotClient::addPlayer(Json::Value& val)
{	
	mPlayerMap.clear();    ///登录进来清理下

	int playCount= val["players"].size(); 

	for(int i=0; i<playCount; ++i)
	{
		/* set player info (新进来的玩家)*/
		SimplyPlayer *p = new (std::nothrow) SimplyPlayer();
		if (NULL == p) 
		{
			printf("new player err\n");
			return ;
		}


		p->uid = val["players"][i]["uid"].asInt();        //玩家id
		p->seatid =  val["players"][i]["seatid"].asInt(); //位置id
		int role = val["players"][i]["role"].asInt();     //是否是庄家

		if (role > 0)
		{
			mBankerUid = p->uid;
		}	

		p->name = val["players"][i]["name"].asString();
		p->exp = val["players"][i]["exp"].asInt();
		p->rmb = val["players"][i]["rmb"].asInt(); 
		p->money = val["players"][i]["money"].asInt(); 

		mPlayerMap[p->uid] = p;
	}
}

//处理房间信息(记录玩家信息)
void XtRobotClient::handleTableInfo(Json::Value& val)
{
	base_money = val["base_money"].asInt();   //房间底注
	//保存玩家信息
	addPlayer(val);
}


///---游戏结束后，机器人随机换桌，别一直在这里打
void XtRobotClient::handleGameEnd(Json::Value& data)
{
	if(rand()%12<6)    
	{
		doChangeTable();
	}
}



//void XtRobotClient::handleTableInfo(Json::Value& data)
//{
//	m_seatid=data["seatid"].asInt();
//	m_isBetting=false;
//}

///---游戏开始
void XtRobotClient::handleGameStart(Json::Value& data)
{
	m_isBetting=true;
	m_hasSee=false;
	m_isAllIn=false;
	m_cardType=0;
	m_maxRound=0;
	m_curRound=0;

	for(int i=0;i<5;i++)
	{
		m_seatBettingInfo[i]=0;
	}

	int size= data["seatids"].size();  ///---填充桌子Betting状态
	for(int i=0;i<size;i++)
	{
		m_seatBettingInfo[data["seatids"][i].asInt()]=1;
	}
}
 ///---处理Bet广播消息
void XtRobotClient::handleBetBc(Json::Value& data)
{
/*	int action=data["action"].asInt();

	switch(action)
	{
		case PLAYER_CALL:
			break;

		case PLAYER_RAISE:
			break;

		case PLAYER_COMPARE:
			{

				int seat_id=data["seatid"].asInt();
				int status=data["status"].asInt();

				int target_seatid = data["target_seatid"].asInt();
				int target_status = data["target_status"].asInt();

				if(target_status==2)
				{
					m_seatBettingInfo[target_seatid]=0;
					if(target_seatid==m_seatid)
					{
						m_isBetting=false;
					}
				}
				if(status==2)
				{
					m_seatBettingInfo[seat_id]=0;
					if(seat_id==m_seatid)    ///---比牌失败，Betting停止
					{
						m_isBetting=false;
					}
				}
			}
			break;

		case PLAYER_SEE:
			{
				if(data["uid"].asInt()==m_uid)
				{
					m_cardType=data["card_type"].asInt();
					switch(m_cardType)
					{
						case CARD_TYPE_BAOZI:
						case CARD_TYPE_SHUNJIN:
						case CARD_TYPE_JINHUA:
							m_maxRound=rand()%9+10;
							break;

						case CARD_TYPE_SHUNZI:
							m_maxRound=rand()%4+7;
							break;

						case CARD_TYPE_DUIZI:
							m_maxRound=rand()%4+4;
							break;

						case CARD_TYPE_DANPAI:
						case CARD_TYPE_TESHU:
							m_maxRound=0;
							break;
					}
				}
			}

			break;

		case PLAYER_FOLD:
			{
				int seat_id=data["seatid"].asInt();
				m_seatBettingInfo[seat_id]=0;
			}
			break;

		case PLAYER_ALLIN :
			m_isAllIn=true;

			break;

		case PLAYER_ALLIN_COMPARE :
			break;
	}*/
}


void XtRobotClient::handleGameNextBet(Json::Value&  val)
{
	/*if(!m_isBetting)
	{
		return;
	}

	printf("uid=(%d,%d)",m_uid,val["uid"].asInt());
	if(m_uid==val["uid"].asInt())
	{
		m_curRound++;
		int cur_round=val["cur_round"].asInt();
		if(m_curRound>30)
		{
			doFold();
			return;
		}

		if(!m_hasSee)  ///---还没看
		{
			if(rand()%10<7||m_isAllIn)
			{
				doSee();
				m_hasSee=true;
			}
			else 
			{
				doFollow();
			}
			return ;
		}

		if(m_cardType==CARD_TYPE_DANPAI||m_cardType==CARD_TYPE_TESHU)
		{
			doFold();
			m_isBetting=false;
			return;
		}

		if(m_isAllIn)
		{
			if(m_cardType==CARD_TYPE_SHUNZI)
			{
				if(rand()%10<6)
				{
					doAllIn();
				}
				else 
				{
					doFold();
				}
			}
			else  if(m_cardType==CARD_TYPE_DUIZI)
			{
				if(rand()%14<2)
				{
					doAllIn();
				}
				else 
				{
					doFold();
				}

			}
			else 
			{
				doAllIn();
			}

			return;
		}

		if(cur_round<m_maxRound)
		{
			doFollow();
			return;
		}
		else 
		{
			doCompare();
			return;
		}
	}*/
}





void XtRobotClient::doLogin()
{
	sendLoginPackage();
}

void XtRobotClient::doFold()
{
	ev_timer_stop(m_evloop,&m_evFoldTimer);
	ev_timer_set(&m_evFoldTimer,rand()%5+2,0);   ///---弃牌时间
	ev_timer_start(m_evloop,&m_evFoldTimer);

}

//选牌
void XtRobotClient::doChoiceCard()  
{
	ev_timer_stop(m_evloop,&m_evChoiceCardTimer);
	ev_timer_set(&m_evChoiceCardTimer,rand()%5+2,0);   ///随机组牌时间
	ev_timer_start(m_evloop,&m_evChoiceCardTimer);
}


//抢庄
void XtRobotClient::doGrabBanker()  
{
	ev_timer_stop(m_evloop,&m_evGrabBankerTimer);
	ev_timer_set(&m_evGrabBankerTimer,rand()%2+2,0);   ///随机组牌时间
	ev_timer_start(m_evloop,&m_evGrabBankerTimer);
}


//下注
void XtRobotClient::doDoubleBet()  
{
	ev_timer_stop(m_evloop,&m_evDoubleBetTimer);
	ev_timer_set(&m_evDoubleBetTimer,rand()%2+2,0);   ///随机组牌时间
	ev_timer_start(m_evloop,&m_evDoubleBetTimer);
}


void XtRobotClient::doFollow()
{
	ev_timer_stop(m_evloop,&m_evFollowTimer);
	ev_timer_set(&m_evFollowTimer,rand()%5+1,0);  ///---跟牌时间
	ev_timer_start(m_evloop,&m_evFollowTimer);
}

void XtRobotClient::doSee()
{
	ev_timer_stop(m_evloop,&m_evSeeTimer);
	ev_timer_set(&m_evSeeTimer,rand()%4+1,0);     ///---看牌时间
	ev_timer_start(m_evloop,&m_evSeeTimer);
}


void XtRobotClient::doCompare()
{
	ev_timer_stop(m_evloop,&m_evCompareTimer);
	ev_timer_set(&m_evCompareTimer,rand()%3+2,0);   ///---比牌时间
	ev_timer_start(m_evloop,&m_evCompareTimer);
}

void XtRobotClient::doAllIn()
{
	ev_timer_stop(m_evloop,&m_evAllInTimer);
	ev_timer_set(&m_evAllInTimer,rand()%3+2,0);     ///全压时间
	ev_timer_start(m_evloop,&m_evAllInTimer);
}




void XtRobotClient::doChangeTable()
{
	sendChangeTablePackage();
}



///连接服务器
int XtRobotClient::connectToServer(const char* ip,int port,int uid)
{
	int socket_fd;
	struct sockaddr_in serv_addr;

	socket_fd=socket(AF_INET,SOCK_STREAM,0);
	if(socket_fd==-1)
	{
		printf("create Socket failed\n");
		return -1;
	}

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(port);
	serv_addr.sin_addr.s_addr=inet_addr(ip);
	memset(&serv_addr.sin_zero,0,8);

	if(connect(socket_fd,(struct sockaddr*)&serv_addr,sizeof(struct sockaddr))==-1)
	{
		printf("connect to server failed\n");
		return -1;
	}



	m_serverfd=socket_fd;   ///与服务器会话socket
	m_uid=uid;

	ev_io_init(&m_evRead,XtRobotClient::onReadData,m_serverfd,EV_READ);   ///设置读回调
	ev_io_start(m_evloop,&m_evRead);

	ev_io_init(&m_evWrite,XtRobotClient::onWriteData,m_serverfd,EV_WRITE);   ///设置写回调


	ev_timer_init(&m_evFoldTimer,XtRobotClient::onDoFold,4,0);        ///---弃牌timer
	ev_timer_init(&m_evFollowTimer,XtRobotClient::onDoFollow,4,0);    ///---跟牌timer
	ev_timer_init(&m_evSeeTimer,XtRobotClient::onDoSee,4,0);          ///---看牌timer
	ev_timer_init(&m_evCompareTimer,XtRobotClient::onDoCompare,4,0);  ///---比牌timer
	ev_timer_init(&m_evAllInTimer,XtRobotClient::onDoAllIn,4,0);  


	ev_timer_init(&m_evChoiceCardTimer,XtRobotClient::onChoiceCard,4,0);  

	ev_timer_init(&m_evGrabBankerTimer,XtRobotClient::onGrabBanker,4,0);  

	ev_timer_init(&m_evDoubleBetTimer,XtRobotClient::onDoubleBet,4,0);  


	


	//登录
	doLogin();   

	return 0;
}

//登录
void XtRobotClient::sendLoginPackage()
{
	Jpacket data;
	data.val["cmd"]=GRAB::CLIENT_LOGIN_REQ;
	data.val["uid"]=m_uid;
	data.val["skey"]="fsdffdf";
	data.end();

	send(data.tostring());
}

void XtRobotClient::sendChangeTablePackage()
{

	Jpacket data;
	data.val["cmd"]=GRAB::CLIENT_CHANGE_REQ;

	data.end();
	send(data.tostring());
}

void XtRobotClient::sendFoldPackage()
{
	Jpacket data;
	data.val["cmd"] = GRAB::CLIENT_BET_REQ;
	data.val["action"] = GRAB::PLAYER_FOLD;
	data.end();
	send(data.tostring());
}

void XtRobotClient::sendSeePackage()
{
	Jpacket data;
	data.val["cmd"] = GRAB::CLIENT_BET_REQ;
	data.val["action"] = GRAB::PLAYER_SEE;
	data.end();
	send(data.tostring());
}

void XtRobotClient::sendFollowPackage()
{
	Jpacket data;
	data.val["cmd"] = GRAB::CLIENT_BET_REQ;
	data.val["action"] = GRAB::PLAYER_CALL;
	data.end();
	send(data.tostring());
}



void XtRobotClient::sendAllInPacket()
{
	int target_seat_id=getTargetSeatId();
	Jpacket data;
	data.val["cmd"] = GRAB::CLIENT_BET_REQ;
	data.val["action"] = GRAB::PLAYER_ALLIN_COMPARE;
	data.val["seatid"]=m_seatid;
	data.val["target_seatid"]=target_seat_id;
	data.end();
	send(data.tostring());
}

//上行组牌
void XtRobotClient::sendChoiceCardPacket()
{
	printf("上行玩家组牌\n");
	Jpacket data;
	data.val["cmd"] = GRAB::CLIENT_GROUP_CARD_REQ;
	
	for(int i=0; i < 3; ++i)
	{
		//组什么牌给服务器没什么关系，只用来记录下组牌时间
		data.val["holes"].append(m_mycard[i]); 
	}
	data.end();
	send(data.tostring());

}

//抢庄(有5个选项：不抢、×1、×2、×3、×4)
void XtRobotClient::sendGrabBankerPacket()
{
	// “抢庄”允许的最大倍数
	int maxDouble = 0;  //0不抢庄
	if (mPlayerMap.find(m_uid) != mPlayerMap.end())
	{
		int doubleGrab = mPlayerMap[m_uid]->money/(base_money*25);
		if (doubleGrab >= 1)
		{
			maxDouble = doubleGrab;
			if (maxDouble >4)
			{
				maxDouble = 4;
			}
		}
	}

	int double_grab = 0;
	if (maxDouble > 0)
	{
		double_grab = rand()% maxDouble + 1;
	}

	printf("Func[%s] 上行玩家抢庄uid[%d] maxDouble[%d] double_grab[%d]\n",__FUNCTION__, m_uid, maxDouble, double_grab);

	Jpacket data;
	data.val["cmd"] = GRAB::CLIENT_GRAB_BANKER_REQ;
	data.val["double_bet"] = double_grab;	
	data.end();

	send(data.tostring());
}

/*
    小的一套倍数是 ×5  ×10  ×15  ×20  ×25
    大的一套倍数是 ×5  ×15  ×25  ×35  ×45
*/
//下注加倍
void XtRobotClient::sendDoubleBetPacket()
{
	// “下注”允许的最大倍数
	int maxDouble = 1;
	if (mPlayerMap.find(m_uid) != mPlayerMap.end()
		&& mPlayerMap.find(mBankerUid) != mPlayerMap.end())
	{
		int bankerMax = mPlayerMap[mBankerUid]->money/(base_money*16);  //庄家
		int playerMax = mPlayerMap[m_uid]->money/(base_money*4);  //闲家

		int lesser = (bankerMax < playerMax) ? bankerMax : playerMax;

		if (lesser >= 5)
		{
			maxDouble = 5;
		}
	}

	printf("Func[%s] 非庄下注 maxDouble[%d]\n",__FUNCTION__, maxDouble);

	Jpacket data;
	data.val["cmd"] = GRAB::CLIENT_DOUBLE_BET_REQ;
	data.val["double_bet"] = maxDouble;	
	data.end();

	send(data.tostring());
}




void XtRobotClient::sendComparePacket()
{
	int target_seat_id=getTargetSeatId();
	Jpacket data;
	data.val["cmd"] = GRAB::CLIENT_BET_REQ;
	data.val["action"] = GRAB::PLAYER_COMPARE;
	data.val["seatid"]=m_seatid;
	data.val["target_seatid"]=target_seat_id;
	data.end();
	send(data.tostring());
}

///---选择一个随机位置作为目标位置
int XtRobotClient::getTargetSeatId()
{
	std::vector<int> bets;

	for(int i=0;i<5;i++)
	{
		if((m_seatBettingInfo[i]==1)&&(i!=m_seatid))
		{
			bets.push_back(i);
		}
	}

	random_shuffle(bets.begin(),bets.end());  ///STL中的函数random_shuffle()用来对一个元素序列进行重新排序（随机的）

	if(bets.size()>0)
	{
		return bets[0];
	}

	return m_seatid;
}






int XtRobotClient::send(const char *buf, unsigned int len)
{
	if (m_serverfd>=0)
	{
		if (m_writeQueue.empty()) 
		{
			m_evWrite.data = this;
			ev_io_start(m_evloop, &m_evWrite);
		}
		m_writeQueue.push_back(new XtBuffer(buf, len));
		return 0;
	}

	printf("server error\n");

	return -1;
}

int XtRobotClient::send(const std::string &res)
{
	return send(res.c_str(), res.length());
	//return safe_writen(res.c_str(), res.length());
}

