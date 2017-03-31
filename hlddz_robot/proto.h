#ifndef __PROTO_H__
#define __PROTO_H__

enum CLIENT_COMMAND
{
    CLIENT_LOGIN                = 1001,         //登录   
    CLIENT_PREPARE              = 1002,         //准备 
    CLIENT_CALL                 = 1003,         //叫分 score = 0,1,2,3
    CLIENT_DOUBLE               = 1004,         //农民加倍 double: true, false
    CLIENT_OUT                  = 1005,         //出牌 不出: keep: true, false, 牌: card
    CLIENT_LOGOUT               = 1006,         //退出
    CLIENT_CHANGE               = 1007,         //换桌
};

enum SERVER_COMMAND
{
    SERVER_RESPOND              = 2000,         //其他回复
    SERVER_LOGIN                = 2001,         //其他玩家登录  
    SERVER_CARD_1               = 2002,         //第一次发牌17张,开始叫分:当前操作者id:cur_id, 叫分倒计时:time, 发牌时间:show_time, 17张牌:card
    SERVER_AGAIN_CALL           = 2003,         //通知下一个叫分: 上次叫分:score, 当前操作者id:cur_id,上一个操作者id:pre_id, 叫分倒计时:time
    SERVER_RESULT_CALL          = 2004,         //叫分结果,开始加倍： 最终分数:score, 地主id:lord, 加倍倒计时:time, 3底牌:card, 
    SERVER_DOUBLE               = 2005,         //通知加倍情况: 总加倍情况:count, 操作者id:pre_id, 是否加倍: double
    SERVER_RESULT_DOUBLE        = 2006,         //加倍结果,发底牌,通知地主出牌: 总倍数:count, 当前操作者(地主)id:cur_id,出牌倒计时:time
    SERVER_AGAIN_OUT            = 2007,         //通知下一个出牌, 上轮不出: keep = true, false, 上轮牌: card, 当前操作者id:cur_id, 上一轮操作者id:pre_id, 上轮牌出牌人out_id 出牌倒计时:time
    SERVER_END                  = 2008,         //牌局结束, info{uid, name, 是否地主isLord, 底分score, 倍数double, 炸弹数bomb}
    SERVER_REPREPARE            = 2009,         //通知机器人重新准备
    SERVER_KICK                 = 2010,         //踢人离场 uid
    SERVER_CHANGE_END           = 2011,         //因有人换桌牌局结束, info{uid, name, 是否地主isLord, 底分score, 倍数double, 炸弹数bomb}
};

enum ERROR_CODE
{
    CODE_SUCCESS                = 0,            //成功 
    CODE_SKEY                   = 1,            //skey错误
    CODE_RELOGIN                = 1,            //重连错误，牌桌没有这个玩家
    CODE_MONEY                  = 2,            //金币不足
};  

//游戏阶段
enum STATE
{
    STATE_NULL                  = 0,
    STATE_PREPARE               = 1,            //准备
    STATE_CALL                  = 2,            //叫分
    STATE_DOUBLE                = 3,            //加倍
    STATE_OUT                   = 4,            //出牌
    STATE_END                   = 5,            //结算
    STATE_MAX                   = 6,            //
};

static const char* DESC_STATE[STATE_MAX] = 
{
    "STATE_NULL",
    "STATE_PREPARE",
    "STATE_CALL",
    "STATE_DOUBLE",
    "STATE_OUT",
    "STATE_END"
};

//当前座位状态
enum OP_STATE
{
    OP_NULL                     = 0,            //NULL
    OP_PREPARE_WAIT                = 1,            //等待准备
    OP_PREPARE_REDAY               = 2,            //已准备
    OP_CALL_WAIT                   = 3,            //等待叫分通知
    OP_CALL_NOTIFY                 = 4,            //已通知
    OP_CALL_RECEIVE                = 5,            //已经叫分
    OP_DOUBLE_NOTIFY               = 6,            //已经通知
    OP_DOUBLE_RECEIVE              = 7,            //已经响应
    OP_OUT_WAIT                    = 8,            //等待出牌
    OP_GAME_END                    = 9,            //结算中
    OP_MAX                      = 10,           //MAX
};

static const char* DESC_OP[OP_MAX] = 
{
    "OP_NULL",
    "OP_PREPARE_WAIT",
    "OP_PREPARE_REDAY",
    "OP_CALL_WAIT",
    "OP_CALL_NOTIFY",
    "OP_CALL_RECEIVE",
    "OP_DOUBLE_NOTIFY",
    "OP_DOUBLE_RECEIVE",
    "OP_OUT_WAIT",
    "OP_GAME_END"
};
#endif
