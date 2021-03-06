#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <algorithm>
#include <assert.h>

#include "hlddz.h"
#include "game.h"
#include "log.h"
#include "table.h"
#include "client.h"
#include "player.h"
#include "proto.h"
#include "XtCard.h"

extern HLDDZ hlddz;
extern Log xt_log;

Table::Table()
{
    m_timerCall.data = this;
    ev_timer_init(&m_timerCall, Table::callCB, ev_tstamp(hlddz.game->CALLTIME), ev_tstamp(hlddz.game->CALLTIME));

    m_timerOut.data = this;
    ev_timer_init(&m_timerOut, Table::outCB, ev_tstamp(hlddz.game->OUTTIME), ev_tstamp(hlddz.game->OUTTIME));

    m_timerKick.data = this;
    ev_timer_init(&m_timerKick, Table::kickCB, ev_tstamp(hlddz.game->KICKTIME), ev_tstamp(hlddz.game->KICKTIME));

    m_timerUpdate.data = this;
    ev_timer_init(&m_timerUpdate, Table::updateCB, ev_tstamp(hlddz.game->UPDATETIME), ev_tstamp(hlddz.game->UPDATETIME));

    m_timerEntrustOut.data = this;
    ev_timer_init(&m_timerEntrustOut, Table::entrustOutCB, ev_tstamp(ENTRUST_OUT_TIME), ev_tstamp(ENTRUST_OUT_TIME));
}

Table::~Table()
{
    ev_timer_stop(hlddz.loop, &m_timerCall);
    ev_timer_stop(hlddz.loop, &m_timerOut);
    ev_timer_stop(hlddz.loop, &m_timerKick);
    ev_timer_stop(hlddz.loop, &m_timerUpdate);
    ev_timer_stop(hlddz.loop, &m_timerEntrustOut);
}

int Table::init(int tid)
{
    // xt_log.debug("begin to init table [%d]\n", table_id);
    m_tid = tid;
    reset();
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        setSeat(0, i);
    }
    return 0;
}

void Table::reset(void)
{
    //xt_log.debug("reset.\n");
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        m_seats[i] = 0;
        m_callScore[i] = 0;
        m_seatCard[i].reset();
        m_bomb[i] = 0;
        m_outNum[i] = 0;
        m_money[i] = 0;
        m_entrust[i] = false;
        m_timeout[i] = false;
        m_opState[i] = OP_PREPARE_WAIT; 
    }
    m_bottomCard.clear();
    m_lastCard.clear();
    m_players.clear();
    m_deck.fill();
    m_deck.shuffle(m_tid);
    m_curSeat = 0;
    m_preSeat = 0;
    m_lordSeat = 0;
    m_outSeat = 0;
    m_topCall = 0;
    m_win = 0;
    m_time = 0;
    m_state = STATE_PREPARE; 

    ev_timer_stop(hlddz.loop, &m_timerCall);
    ev_timer_stop(hlddz.loop, &m_timerOut);
    ev_timer_stop(hlddz.loop, &m_timerKick);
    ev_timer_stop(hlddz.loop, &m_timerUpdate);
    ev_timer_stop(hlddz.loop, &m_timerEntrustOut);
}

int Table::broadcast(Player *p, const std::string &packet)
{
    Player *player;
    std::map<int, Player*>::iterator it;
    for (it = m_players.begin(); it != m_players.end(); it++)
    {
        player = it->second;
        if (player == p || player->client == NULL)
        {
            continue;
        }
        player->client->send(packet);
    }

    return 0;
}

int Table::unicast(Player *p, const std::string &packet)
{
    if (p && p->client)
    {
        return p->client->send(packet);
    }
    return -1;
}

int Table::random(int start, int end)
{
    return start + rand() % (end - start + 1);
}

void Table::vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
    if (cards.empty()) 
    {
        return;
    }

    for (unsigned int i = 0; i < cards.size(); i++) 
    {
        packet.val[key].append(cards[i].m_value);
    }
}

void Table::map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key)
{
    std::map<int, XtCard>::iterator it;
    for (it = cards.begin(); it != cards.end(); it++)
    {
        XtCard &card = it->second;
        packet.val[key].append(card.m_value);
    }
}

void Table::json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key)
{
    Json::Value &val = packet.tojson();
    if(!val.isMember(key))
    {
        return;
    }

    for (unsigned int i = 0; i < val[key].size(); i++)
    {
        XtCard card(val[key][i].asInt());
        cards.push_back(card);
    }
}

void Table::callCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerCall);
    //xt_log.debug("stop m_timerCall for timerup.\n");
    table->onCall();
}

void Table::onCall(void)
{
    //xt_log.debug("onCall\n");
    m_callScore[m_curSeat] = 0;
    //记录状态
    m_opState[m_curSeat] = OP_CALL_RECEIVE;
    logicCall();
}

void Table::outCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerOut);
    //xt_log.debug("stop m_timerOut for timerup.\n");
    table->onOut();
}

void Table::onOut(void)
{
    //xt_log.debug("onOut. m_curSeat:%d\n", m_curSeat);
    Player* player = getSeatPlayer(m_curSeat);
    bool keep = false;
    vector<XtCard> curCard;
    vector<XtCard> &myCard = m_seatCard[m_curSeat].m_cards;

    XtCard::sortByDescending(myCard);
    XtCard::sortByDescending(m_lastCard);

    if(m_entrust[m_curSeat])
    {
        xt_log.error("%s:%d, entrust player shall not timerout. seatid:%d\n", __FILE__, __LINE__, m_curSeat); 
    }

    //前一次已经超时
    if(m_timeout[m_curSeat])
    {
        //首轮出牌
        //没人跟自己的牌
        if(m_lastCard.empty() || m_curSeat == m_outSeat)
        {
            m_deck.getFirst(myCard, curCard);
        }
        //跟别人的牌
        else
        {
            m_deck.getOut(myCard, m_lastCard, curCard);
        }
        m_entrust[m_curSeat] = true;
    }
    //第一次超时
    else
    {
        //记录超时
        m_timeout[m_curSeat] = true;
        //首轮出牌
        //没人跟自己的牌
        if(m_lastCard.empty() || m_curSeat == m_outSeat)
        {
            m_deck.getFirst(myCard, curCard);
            m_entrust[m_curSeat] = true;
        }
        //跟别人的牌
        else
        {
            curCard.clear();
        }
    }

    keep = curCard.empty() ? true : false; 

    //判断是否结束和通知下一个出牌人，本轮出牌
    logicOut(player, curCard, keep);
}

void Table::kickCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerKick);
    table->onKick();
}

void Table::onKick(void)
{
    Player* pl = NULL;
    for(vector<Player*>::iterator it = m_delPlayer.begin(); it != m_delPlayer.end(); ++it)
    {
        pl = *it;
        if(pl->isRobot())
        {
            continue;
        }
        xt_log.debug("%s:%d, del player active! m_uid:%d \n",__FILE__, __LINE__, pl->m_uid); 
        //删除后，最后流程走回这里的logout
        hlddz.game->del_player(*it);
    }
    m_delPlayer.clear();
}

void Table::updateCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    table->onUpdate();
}

void Table::onUpdate(void)
{
    if(--m_time >= 0)
    {
        sendTime();
    }
}

void Table::entrustOutCB(struct ev_loop *loop, struct ev_timer *w, int revents)
{
    Table *table = (Table*) w->data;
    ev_timer_stop(hlddz.loop, &table->m_timerEntrustOut);
    table->onEntrustOut();
}

void Table::onEntrustOut(void)
{
    entrustOut();
}

int Table::login(Player *player)
{
    xt_log.debug("player login m_uid:%d, tid:%d\n", player->m_uid, m_tid);
    if(m_players.find(player->m_uid) != m_players.end())
    {
        xt_log.error("%s:%d, login fail! player was existed! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        return 0;
    }

    //给机器人加钱
    addRobotMoney(player);

    //检查入场费
    if(player->m_money < hlddz.game->ROOMTAX)
    {
        xt_log.error("%s:%d, player was no enouth money! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        loginUC(player, CODE_MONEY);
        return 0; 
    }

    if(!sitdown(player))
    {
        return 0;
    }

    //登录回复
    loginUC(player, CODE_SUCCESS);

    //广播玩家信息
    loginBC(player);

    //准备状态
    m_opState[player->m_seatid] = OP_PREPARE_REDAY; 
    if(!allSeatFit(OP_PREPARE_REDAY))
    {
        xt_log.debug("not all is prepare.\n");
        return 0;
    }
    else if(m_players.size() != SEAT_NUM)
    {
        xt_log.debug("not enouth player, size:%d\n", m_players.size());
        return 0;
    }
    else
    {
        gameStart(); 
    }

    return 0;
}

void Table::reLogin(Player* player) 
{
    xt_log.debug("player relogin m_uid:%d\n", player->m_uid);
    if(m_players.find(player->m_uid) == m_players.end())
    {
        xt_log.error("%s:%d, player was not existed! m_uid:%d\n", __FILE__, __LINE__, player->m_uid); 
        sendError(player, CLIENT_LOGIN, CODE_RELOGIN);
        return;
    }

    if(player->m_seatid < 0 || player->m_seatid > SEAT_NUM)
    {
        xt_log.error("%s:%d, player seat error! uid:%d, seatid:%d\n", __FILE__, __LINE__, player->m_uid, player->m_seatid); 
        sendError(player, CLIENT_LOGIN, CODE_SEAT);
        return;
    }

    //给机器人加钱
    addRobotMoney(player);

    loginUC(player, CODE_SUCCESS);
}

void Table::msgCall(Player* player)
{
    //xt_log.debug("msg Call m_uid:%d\n", player->m_uid);
    //检查状态
    if(m_state != STATE_CALL)
    {
        xt_log.error("%s:%d, call fail!, game state not call_state, m_state:%s\n", __FILE__, __LINE__, DESC_STATE[m_state]); 
        sendError(player, CLIENT_CALL, CODE_STATE);
        return;
    }

    //座位玩家是否匹配
    if(player->m_seatid >= SEAT_NUM || player->m_seatid < 0 || getSeat(player->m_seatid) != player->m_uid)
    {
        xt_log.error("%s:%d, call fail!, seat info error. player seatid:%d, m_uid:%d, seatuid:%d\n", __FILE__, __LINE__, player->m_seatid, player->m_uid, getSeat(player->m_seatid)); 
        sendError(player, CLIENT_CALL, CODE_SEAT);
        return;
    }

    //座位通知过或者已经已经自动叫分
    if(m_opState[player->m_seatid] != OP_CALL_NOTIFY)
    {        
        xt_log.error("%s:%d, call fail!, player callstate error. player seatid:%d, m_uid:%d, callstate:%s\n", __FILE__, __LINE__, player->m_seatid, player->m_uid, DESC_OP[m_opState[player->m_seatid]]); 
        sendError(player, CLIENT_CALL, CODE_NOTIFY);
        return; 
    }

    //是否当前操作者
    if(m_curSeat != player->m_seatid)
    {
        xt_log.error("%s:%d, call fail!, operator error. m_curSeat:%d, playerSeat:%d\n", __FILE__, __LINE__, m_curSeat, player->m_seatid); 
        sendError(player, CLIENT_CALL, CODE_CURRENT);
        return; 
    }

    //托管中
    if(m_entrust[player->m_seatid])
    {
        xt_log.error("%s:%d, call fail!, entrusting. playerSeat:%d\n", __FILE__, __LINE__, player->m_seatid); 
        sendError(player, CLIENT_CALL, CODE_ENTRUST);
        return; 
    }

    //有效叫分
    Json::Value &msg = player->client->packet.tojson();
    int score = msg["score"].asInt();
    if(score < 0 || score > 3)
    {
        xt_log.error("%s:%d, call fail!, score error. uid:%d, score:%d\n", __FILE__, __LINE__, player->m_uid, score); 
        sendError(player, CLIENT_CALL, CODE_SCORE);
        return; 
    }

    //停止叫分定时器
    //xt_log.debug("stop m_timerCall for msg.\n");
    ev_timer_stop(hlddz.loop, &m_timerCall);

    //保存当前叫分
    m_callScore[m_curSeat] = score;
    //记录状态
    m_opState[m_curSeat] = OP_CALL_RECEIVE;
    //xt_log.debug("call score, m_uid:%d, seatid:%d, score :%d\n", player->m_uid, player->m_seatid, m_callScore[player->m_seatid]);
    logicCall();
}

void Table::msgOut(Player* player)
{
    //检查状态
    if(m_state != STATE_OUT)
    {
        xt_log.error("%s:%d, out fail! game state not out_state, m_state:%s\n", __FILE__, __LINE__, DESC_STATE[m_state]); 
        sendError(player, CLIENT_OUT, CODE_STATE);
        return;
    }

    //是否当前操作者
    if(m_curSeat != player->m_seatid)
    {
        xt_log.error("%s:%d, out fail!, operator error. m_curSeat:%d, playerSeat:%d\n", __FILE__, __LINE__, m_curSeat, player->m_seatid); 
        sendError(player, CLIENT_OUT, CODE_CURRENT);
        return; 
    }

    //托管中
    if(m_entrust[player->m_seatid])
    {
        xt_log.error("%s:%d, out fail!, entrusting. playerSeat:%d\n", __FILE__, __LINE__, player->m_seatid); 
        sendError(player, CLIENT_OUT, CODE_ENTRUST);
        return; 
    }

    Json::Value &msg = player->client->packet.tojson();

    vector<XtCard> curCard;
    json_array_to_vector(curCard, player->client->packet, "card");

    //不出校验
    bool keep = msg["keep"].asBool();

    //xt_log.debug("msgOut, m_uid:%d, seatid:%d, keep:%s\n", player->m_uid, player->m_seatid, keep ? "true" : "false");
    //xt_log.debug("curCard:\n");
    //show(curCard);
    //xt_log.debug("lastCard:\n");
    //show(m_lastCard);
    //
    if(keep && !curCard.empty())
    {
        xt_log.error("%s:%d, out fail! not allow keep && not empty card. m_uid:%d, seatid:%d, keep:%s\n", __FILE__, __LINE__, player->m_uid, player->m_seatid, keep ? "true" : "false"); 
        xt_log.debug("curCard:\n");
        show(curCard);
        sendError(player, CLIENT_OUT, CODE_KEEP);
        return;
    }

    //校验手牌存在
    if(!checkCard(player->m_seatid, curCard))
    {
        xt_log.error("%s:%d, out fail! outcard not in hand!. m_uid:%d, seatid:%d, \n", __FILE__, __LINE__, player->m_uid, player->m_seatid); 
        //show(m_bottomCard);
        show(curCard);
        show(m_seatCard[player->m_seatid].m_cards);
        sendError(player, CLIENT_OUT, CODE_CARD_EXIST);
        return;
    }

    //清除超时用户
    m_timeout[player->m_seatid] = false;

    //停止出牌定时器
    //xt_log.debug("stop m_timerOut for msg.\n");
    ev_timer_stop(hlddz.loop, &m_timerOut);

    logicOut(player, curCard, keep);
}

void Table::msgChange(Player* player)
{
    xt_log.debug("msgChange, m_uid:%d, seatid:%d\n", player->m_uid, player->m_seatid);

    if(m_state != STATE_PREPARE)
    {
        sendError(player, CLIENT_CHANGE, CODE_STATE);
        return;
    }

    //清理座位信息
    setSeat(0, player->m_seatid);

    hlddz.game->change_table(player);
}

void Table::msgView(Player* player)
{
    Json::Value &msg = player->client->packet.tojson();
    int uid = msg["uid"].asInt();
    int index = uid % hlddz.main_size;

    if (hlddz.main_rc[index]->command("hgetall hu:%d", uid) < 0) {
        xt_log.error("msg view fail! 1, get player infomation error. uid:%d\n", uid);
        sendError(player, CLIENT_VIEW, CODE_NOEXIST);
    }

    if (hlddz.main_rc[index]->is_array_return_ok() < 0) {
        xt_log.error("msg view fail! 2, get player infomation error. uid:%d\n", uid);
        sendError(player, CLIENT_VIEW, CODE_NOEXIST);
    }

    int money = hlddz.main_rc[index]->get_value_as_int("money");
    double total = hlddz.main_rc[index]->get_value_as_int("total");
    int victory = hlddz.main_rc[index]->get_value_as_int("victory");
    double victory_prob = (total > 0) ? (victory / total) : 0; 

    Jpacket packet;
    packet.val["cmd"]           = SERVER_RESPOND;
    packet.val["msgid"]         = CLIENT_VIEW;
    packet.val["uid"]           = uid;
    packet.val["code"]          = CODE_SUCCESS;
    packet.val["name"]          = hlddz.main_rc[index]->get_value_as_string("name");
    packet.val["avatar"]        = hlddz.main_rc[index]->get_value_as_string("avatar");
    packet.val["sex"]           = hlddz.main_rc[index]->get_value_as_int("sex");
    packet.val["money"]         = money;
    packet.val["level"]         = hlddz.main_rc[index]->get_value_as_int("level");
    packet.val["title"]         = getTitle(money);                                   //头衔
    packet.val["victory_num"]   = victory;                                           //胜场
    packet.val["victory_prob"]  = victory_prob;                                      //胜率
    packet.end();
    unicast(player, packet.tostring());
}

void Table::msgEntrust(Player* player)
{
    //检查状态
    if(m_state == STATE_PREPARE || m_state == STATE_END)
    {
        xt_log.error("%s:%d, entrust fail! game state not out_state, m_state:%s\n", __FILE__, __LINE__, DESC_STATE[m_state]); 
        sendError(player, CLIENT_ENTRUST, CODE_OUT_ENTRUST);
        return;
    }

    Json::Value &msg = player->client->packet.tojson();
    bool entrust = msg["active"].asBool();

    //重复
    if(m_entrust[player->m_seatid] == entrust)
    {
        xt_log.error("%s:%d, entrust fail! op repeat, entrust:%s\n", __FILE__, __LINE__, entrust ? "true" : "false"); 
        sendError(player, CLIENT_ENTRUST, CODE_REPEAT_ENTRUST);
        return;
    }

    //当前操作人正好是托管人
    m_entrust[player->m_seatid] = entrust;
    if(entrust && player->m_seatid == m_curSeat)
    {
        entrustProc(true, m_curSeat);
    }
    sendEntrust(player->m_uid, entrust);
}

void Table::msgChat(Player* player)
{
    Json::Value &msg = player->client->packet.tojson();
    Jpacket packet;
    packet.val["cmd"]         = SERVER_CHAT;
    packet.val["uid"]         = player->m_uid;
    packet.val["chatid"]      = msg["chatid"].asInt();
    packet.val["content"]     = msg["content"].asString();
    packet.end();
    broadcast(NULL, packet.tostring());
}
        
void Table::msgMotion(Player* player)
{
    if(player->m_money < hlddz.game->MOTIONMONEY || player->m_money < hlddz.game->ROOMLIMIT)
    {
        sendError(player, CLIENT_MOTION, CODE_MONEY);
        return;
    }

    player->changeMoney(-hlddz.game->MOTIONMONEY);

    Json::Value &msg = player->client->packet.tojson();
    Jpacket packet;
    packet.val["cmd"]         = SERVER_MOTION;
    packet.val["target_id"]   = msg["target_id"].asInt();
    packet.val["src_id"]      = player->m_uid;
    packet.val["type"]        = msg["type"].asInt();
    packet.val["price"]       = hlddz.game->MOTIONMONEY;
    packet.end();
    broadcast(NULL, packet.tostring());
}

bool Table::sitdown(Player* player)
{
    int seatid = -1;
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(getSeat(i) == 0) 
        {
            seatid = i; 
            break;
        }
    }
    if(seatid < 0)
    {
        xt_log.error("%s:%d, no empty seat.\n", __FILE__, __LINE__); 
        return false; 
    }

    player->m_seatid = seatid;
    player->m_tid = m_tid;
    setSeat(player->m_uid, seatid);
    m_players[player->m_uid] = player;
    //xt_log.debug("sitdown uid:%d, seatid:%d\n", player->m_uid, seatid);
    return true;
}

void Table::loginUC(Player* player, int code)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_RESPOND;
    packet.val["code"]      = code;
    packet.val["msgid"]     = CLIENT_LOGIN;
    packet.val["tid"]       = m_tid;
    packet.val["seatid"]    = player->m_seatid;

    //pack other player info
    for(map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        if(it->first == player->m_uid)  
        {
            continue; 
        }
        Json::Value jval;          
        Player* pl = it->second;
        jval["uid"]     = pl->m_uid;
        jval["seatid"]  = pl->m_seatid;
        jval["name"]    = pl->m_name;
        jval["money"]   = pl->m_money;
        jval["level"]   = pl->m_level;
        jval["sex"]     = pl->m_sex;
        jval["avatar"]  = pl->m_avatar;
        jval["state"]   = m_opState[pl->m_seatid];
        packet.val["userinfo"].append(jval);
    }

    vector_to_json_array(m_seatCard[player->m_seatid].m_cards, packet, "card");

    //重登处理
    packet.val["state"]       = m_state;

    switch(m_state)
    {
        case STATE_PREPARE:
            {
                for(unsigned int i = 0; i < SEAT_NUM; ++i)
                {
                    packet.val["prepare"].append(m_opState[i] == OP_PREPARE_REDAY); 
                }
            }
            break;
        case STATE_CALL:
            {
                //叫分
                for(unsigned int i = 0; i < SEAT_NUM; ++i)
                {
                    packet.val["callScore"].append(m_callScore[i]);
                }
                //剩余时间
                packet.val["time"] = m_time;
            }
            break;
        case STATE_OUT:
            {
                //发牌
                //自己的手牌 
                vector_to_json_array(m_seatCard[player->m_seatid].m_cards, packet, "myCard");
                //上轮出的牌
                vector_to_json_array(m_lastCard, packet, "lastCard");
                //底牌
                vector_to_json_array(m_bottomCard, packet, "bottomCard");
                //上轮出牌者座位
                packet.val["outSeat"] = m_outSeat;
                //当前操作者座位
                packet.val["currentSeat"] = m_curSeat;
                //剩余时间
                packet.val["time"] = m_time;
            }
            break;
    }

    packet.end();
    unicast(player, packet.tostring());
}

void Table::loginBC(Player* player)
{
    Jpacket packet;
    Json::Value jval;          
    Player* pl = player;
    jval["uid"]     = pl->m_uid;
    jval["seatid"]  = pl->m_seatid;
    jval["name"]    = pl->m_name;
    jval["money"]   = pl->m_money;
    jval["level"]   = pl->m_level;
    jval["sex"]     = pl->m_sex;
    jval["avatar"]  = pl->m_avatar;
    jval["state"]   = m_state;

    packet.val["userinfo"].append(jval);
    packet.val["cmd"]       = SERVER_LOGIN;

    /* //直接这样发，客户端解析有错误
    Jpacket packet;
    packet.val["uid"]       = player->m_uid;
    packet.val["seatid"]    = player->m_seatid;
    packet.val["name"]      = player->m_name;
    packet.val["money"]     = player->m_money;
    packet.val["level"]     = player->m_level;
    packet.val["sex"]       = player->m_sex;
    packet.val["avatar"]    = player->m_avatar;
    packet.val["state"]     = m_state;
    */

    packet.end();
    broadcast(player, packet.tostring());
}

bool Table::allocateCard(void)
{
    //底牌    
    if(!m_deck.getHoleCards(m_bottomCard, BOTTON_CARD_NUM))
    {
        xt_log.error("%s:%d, get bottom card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
        return false;
    }

    //xt_log.debug("allocateCard, bottonCard:\n");
    //show(m_bottomCard);

    //手牌
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(!m_deck.getHoleCards(m_seatCard[i].m_cards, HAND_CARD_NUM))
        {
            xt_log.error("%s:%d, get hand card error,  tid:%d\n",__FILE__, __LINE__, m_tid); 
            return false;
        }
        //xt_log.debug("uid:%d\n", getSeat(i));
        //show(m_seatCard[i].m_cards);
    }

    return true;
}

void Table::prepareProc(void)
{
    m_state = STATE_PREPARE; 
    setAllSeatOp(OP_PREPARE_WAIT);
}

void Table::callProc(void)
{
    m_state = STATE_CALL; 
    setAllSeatOp(OP_CALL_WAIT);
    m_opState[m_curSeat] = OP_CALL_NOTIFY;
    m_time = hlddz.game->CALLTIME;
    ev_timer_again(hlddz.loop, &m_timerCall);
    //xt_log.debug("m_timerCall first start \n");
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::outProc(void)
{
    m_state = STATE_OUT;
    setAllSeatOp(OP_OUT_WAIT);
    m_curSeat = m_lordSeat;
    m_preSeat = m_curSeat;
    m_time = hlddz.game->OUTTIME;
    ev_timer_again(hlddz.loop, &m_timerOut);
    //xt_log.debug("m_timerOut first start \n");
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
}

void Table::logout(Player* player)
{
    xt_log.debug("player logout, uid:%d\n", player->m_uid);
    //退出发生后，牌桌内只有机器人,重新进入准备状态，方便测试
    map<int, Player*>::iterator it = m_players.find(player->m_uid);
    if(it != m_players.end())
    {
        m_players.erase(it);
    }

    if(m_players.empty())
    {
        reset(); 
        //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
    }

    bool findHuman = false;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        if(!it->second->isRobot()) 
        {
            findHuman = true;
            break;
        }
    }

    //通知机器人重新准备
    if(!findHuman)
    {
        reset();
        //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
        for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
        {
            Jpacket packet;
            packet.val["cmd"]           = SERVER_REPREPARE;
            packet.val["uid"]           = it->first;
            packet.end();
            unicast(it->second, packet.tostring());
        }
    }

    //清理座位信息
    setSeat(0, player->m_seatid);
}

void Table::endProc(void)
{
    m_state = STATE_END;
    setAllSeatOp(OP_GAME_END);
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);

    //总倍数
    int doubleNum = getAllDouble();
    //计算各座位输赢
    calculate(doubleNum);
    //结算处理
    payResult();
    //统计局数和胜场
    total();
    //通知结算
    sendEnd(doubleNum);
    //增加经验
    addPlayersExp(); 
    //xt_log.debug("state: %s\n", DESC_STATE[m_state]);
    //清空seatid
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        it->second->m_seatid = 0;
    }

    //重置游戏
    reset();
}

void Table::entrustProc(bool killtimer, int entrustSeat)
{
    switch(m_state)
    {
        case STATE_CALL:
            {
                if(killtimer)
                {
                    ev_timer_stop(hlddz.loop, &m_timerCall);
                }
                onCall();
                sendEntrustCall(getSeatPlayer(entrustSeat), m_callScore[entrustSeat]); 
            }
            break;
        case STATE_OUT:
            {
                if(killtimer)
                {
                    ev_timer_stop(hlddz.loop, &m_timerOut);
                }
                ev_timer_again(hlddz.loop, &m_timerEntrustOut);
            }
            break;
    }
}

void Table::logicCall(void)
{
    //是否已经选出地主
    if(selecLord())
    {
        outProc();
        addBottom2Lord();
        sendCallResult(); 

        //任意一个农民是托管，都要进行处理
        int famer1 = (m_lordSeat + 1) % 3;
        int famer2 = (m_lordSeat + 2) % 3;
        if(m_entrust[famer1])
        {
            entrustProc(true, famer1);
        }

        if(m_entrust[famer2])
        {
            entrustProc(true, famer2);
        }
    }//设置下一个操作人
    else if(getNext())
    {
        //广播当前叫分和下一个叫分
        //xt_log.debug("m_timerCall again start.\n");
        m_time = hlddz.game->CALLTIME;
        sendCallAgain(); 
        if(m_entrust[m_curSeat])
        {
            entrustProc(false, m_curSeat);
        }
        else
        {
            ev_timer_again(hlddz.loop, &m_timerCall);
        }
    }
    else
    {//重新发牌
        xt_log.debug("nobody call, need send card again.\n");
        gameRestart();
    }
}

void Table::logicOut(Player* player, vector<XtCard>& curCard, bool keep)
{
    if(!curCard.empty())
    {
        //牌型校验
        XtCard::sortByDescending(curCard);
        int cardtype = m_deck.getCardType(curCard);
        if(cardtype == CT_ERROR)
        {
            xt_log.error("%s:%d,out fail! cardtype error. m_uid:%d, seatid:%d, keep:%s\n", __FILE__, __LINE__, player->m_uid, player->m_seatid, keep ? "true" : "false"); 
            xt_log.debug("curCard:\n");
            show(curCard);
            sendError(player, CLIENT_OUT, CODE_CARD);
            return;
        }
        //记录炸弹
        if(cardtype == CT_BOMB || cardtype == CT_ROCKET)
        {
            m_bomb[player->m_seatid]++; 
        }
    }

    //出牌次数
    m_outNum[player->m_seatid] += 1;

    if(m_lastCard.empty())
    {//首轮出牌
        //xt_log.debug("first round\n");
        m_lastCard = curCard;
        m_outSeat = player->m_seatid;
    }
    else if(keep && curCard.empty())
    {//不出
        //xt_log.debug("keep\n");
    }
    else if(m_outSeat == player->m_seatid)
    {//自己的牌
        //xt_log.debug("self card, new start\n");
    }
    else if(!curCard.empty())
    {//出牌
        //xt_log.debug("compare\n");
        if(!m_deck.compareCard(curCard, m_lastCard))
        {
            xt_log.error("%s:%d, compare fail.", __FILE__, __LINE__); 
            xt_log.error("curCard:\n");
            show(curCard);
            xt_log.error("lastCard:\n");
            show(m_lastCard);
            sendError(player, CLIENT_OUT, CODE_COMPARE);
            return;
        }
        m_lastCard = curCard;
        m_outSeat = player->m_seatid;
    }


    //扣除手牌
    if(!curCard.empty())
    {
        m_seatCard[player->m_seatid].popCard(curCard);
    }

    //判定结束
    if(m_seatCard[player->m_seatid].m_cards.empty())
    {
        //发送最后一轮出牌
        getNext(); 
        sendOutAgain(true);    
        xt_log.debug("=======================================gameover\n");
        m_win = player->m_seatid;
        endProc();
    }
    //挑选下一个操作者
    else if(getNext())
    {
        //如果没人接出牌者的牌
        if(m_curSeat == m_outSeat)
        {
            //xt_log.debug("无人接牌， 新一轮\n");
            m_lastCard.clear(); 
        }

        m_time = hlddz.game->OUTTIME;
        //发送出牌
        sendOutAgain(false);    

        //如果下一个出牌人托管
        if(m_entrust[m_curSeat])
        {
            entrustProc(false, m_curSeat);
        }
        else
        {
            if(m_timeout[m_curSeat])
            {//上次有超时过，缩短计时
                ev_timer_set(&m_timerOut, ev_tstamp(hlddz.game->SECOND_OUTTIME), ev_tstamp(hlddz.game->SECOND_OUTTIME));
            }
            else
            {
                ev_timer_set(&m_timerOut, ev_tstamp(hlddz.game->OUTTIME), ev_tstamp(hlddz.game->OUTTIME));
            }
            //开启出牌定时器
            //xt_log.debug("m_timerOut again start \n");
            ev_timer_again(hlddz.loop, &m_timerOut);
        }
    }
}

void Table::sendCard1(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_CARD_1;
        vector_to_json_array(m_seatCard[pl->m_seatid].m_cards, packet, "card");
        packet.val["time"]          = hlddz.game->CALLTIME;
        packet.val["show_time"]     = hlddz.game->SHOWTIME;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendCallAgain(void) 
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_AGAIN_CALL;
        packet.val["time"]          = hlddz.game->CALLTIME;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["pre_id"]        = getSeat(m_preSeat);
        packet.val["score"]         = m_callScore[m_preSeat];
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendCallResult(void)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_RESULT_CALL;
        packet.val["time"]          = hlddz.game->OUTTIME;
        packet.val["score"]         = m_topCall;
        packet.val["lord"]          = getSeat(m_lordSeat);
        vector_to_json_array(m_bottomCard, packet, "card");
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendOutAgain(bool last)
{
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        Jpacket packet;
        packet.val["cmd"]           = SERVER_AGAIN_OUT;
        packet.val["time"]          = hlddz.game->OUTTIME;
        packet.val["last"]          = last;
        packet.val["cur_id"]        = getSeat(m_curSeat);
        packet.val["pre_id"]        = getSeat(m_preSeat);
        packet.val["out_id"]        = getSeat(m_outSeat);
        packet.val["keep"]          = (m_preSeat != m_outSeat);
        packet.val["num"]           = static_cast<int>(m_seatCard[m_outSeat].m_cards.size());
        vector_to_json_array(m_lastCard, packet, "card");
        packet.end();
        unicast(pl, packet.tostring());
    }
}

void Table::sendEnd(int doubleNum)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_END;
    packet.val["code"]      = CODE_SUCCESS;
    packet.val["double"]    = doubleNum;
    packet.val["bomb"]      = getBombNum();
    packet.val["score"]     = hlddz.game->ROOMSCORE;

    //xt_log.debug("end info: double:%d, bomb:%d, score:%d\n", doubleNum, getBombNum(), hlddz.game->ROOMSCORE);

    for(map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it)
    {
        Json::Value jval;          
        Player* pl = it->second;
        jval["uid"]     = pl->m_uid;
        jval["name"]    = pl->m_name;
        jval["money"]   = m_money[pl->m_seatid];
        jval["isLord"]  = (pl->m_seatid == m_lordSeat);
        packet.val["info"].append(jval);
        //xt_log.debug("end info: uid:%d, name:%s, money:%d\n", pl->m_uid, pl->m_name.c_str(), m_money[pl->m_seatid]);
    }

    packet.end();
    broadcast(NULL, packet.tostring());

}

void Table::sendTime(void)
{
    //xt_log.debug("$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$sendTime\n");
    Jpacket packet;
    packet.val["cmd"]       = SERVER_TIME;
    packet.val["time"]      = m_time;
    packet.end();
    broadcast(NULL, packet.tostring());
}

void Table::sendError(Player* player, int msgid, int errcode)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_RESPOND;
    packet.val["code"]      = errcode;
    packet.val["msgid"]     = msgid;
    packet.end();
    unicast(player, packet.tostring());
    xt_log.error("error msg, msgid:%d, code:%d\n", msgid, errcode);
}
        
void Table::sendEntrustOut(Player* player, vector<XtCard>& curCard, bool keep)
{
    Jpacket packet;
    packet.val["cmd"]           = SERVER_ENTRUST_OUT;
    packet.val["keep"]          = keep;
    vector_to_json_array(curCard, packet, "card");
    packet.end();
    unicast(player, packet.tostring());
}

void Table::sendEntrustCall(Player* player, int score)
{
    Jpacket packet;
    packet.val["cmd"]           = SERVER_ENTRUST_CALL;
    packet.val["score"]         = score;
    packet.end();
    unicast(player, packet.tostring());
}

void Table::sendEntrustDouble(Player* player, bool dou)
{
    Jpacket packet;
    packet.val["cmd"]           = SERVER_ENTRUST_DOUBLE;
    packet.val["double"]        = dou;
    packet.end();
    unicast(player, packet.tostring());
}
        
void Table::sendEntrust(int uid, bool active)
{
    Jpacket packet;
    packet.val["cmd"]       = SERVER_ENTRUST;
    packet.val["uid"]       = uid;
    packet.val["active"]    = active;
    packet.end();
    broadcast(NULL, packet.tostring());
}

void Table::gameStart(void)
{
    m_curSeat = rand() % SEAT_NUM;
    xt_log.debug("=======================================start send card, cur_id:%d, next_id:%d, next_id:%d, tid:%d\n",
            getSeat(m_curSeat), getSeat((m_curSeat + 1) % SEAT_NUM), getSeat((m_curSeat + 2) % SEAT_NUM), m_tid);

    payTax();
    callProc();
    allocateCard();

    sendCard1();

    ev_timer_stop(hlddz.loop, &m_timerUpdate);
    ev_timer_again(hlddz.loop, &m_timerUpdate);

    //如果托管直接自动处理
    if(m_entrust[m_curSeat])
    {
        entrustProc(true, m_curSeat);
    }
}

void Table::gameRestart(void)
{
    //重置部分数据
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        m_callScore[i] = 0;
        m_seatCard[i].reset();
        m_bomb[i] = 0;
        m_outNum[i] = 0;
        m_money[i] = 0;
    }
    m_bottomCard.clear();
    m_lastCard.clear();
    m_deck.fill();
    m_deck.shuffle(m_tid);
    m_curSeat = 0;
    m_preSeat = 0;
    m_lordSeat = 0;
    m_outSeat = 0;
    m_topCall = 0;
    m_win = 0;

    m_curSeat = rand() % SEAT_NUM;
    xt_log.debug("=======================================restart send card, cur_id:%d, next_id:%d, next_id:%d, tid:%d\n",
            getSeat(m_curSeat), getSeat((m_curSeat + 1) % SEAT_NUM), getSeat((m_curSeat + 2) % SEAT_NUM), m_tid);

    callProc();

    allocateCard();
    sendCard1();

    ev_timer_stop(hlddz.loop, &m_timerUpdate);
    ev_timer_again(hlddz.loop, &m_timerUpdate);
    //如果托管直接自动处理
    if(m_entrust[m_curSeat])
    {
        entrustProc(true, m_curSeat);
    }
}

bool Table::getNext(void)
{
    int nextSeat = (m_curSeat + 1) % SEAT_NUM;
    switch(m_state)
    {
        case STATE_CALL:
            {
                if(m_opState[nextSeat] == OP_CALL_WAIT) 
                {
                    m_preSeat = m_curSeat;
                    m_curSeat = nextSeat;
                    m_opState[m_curSeat] = OP_CALL_NOTIFY;
                    //xt_log.debug("get next success, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
                    return true; 
                }
            }
            break;
        case STATE_OUT:
            {//校验出牌时间戳
                m_preSeat = m_curSeat;
                m_curSeat = nextSeat;
                //xt_log.debug("get next success, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
                return true;
            }
            break;
    }

    //xt_log.debug("get next finish, cur_seat:%d, pre_seat:%d\n", m_curSeat, m_preSeat);
    return false;
}

Player* Table::getSeatPlayer(unsigned int seatid)
{
    int uid = getSeat(seatid);
    map<int, Player*>::const_iterator it = m_players.find(uid);
    if(it != m_players.end())
    {
        return it->second; 
    }

    xt_log.error("%s:%d, getSeatPlayer error! seatid:%d\n",__FILE__, __LINE__, seatid); 
    return NULL;
}

void Table::setAllSeatOp(int state)
{
    for(unsigned int i = 0; i < SEAT_NUM; ++i) 
    {
        m_opState[i] = state; 
    }
}

bool Table::allSeatFit(int state)
{
    for(unsigned int i = 0; i < SEAT_NUM; ++i) 
    {
        if(m_opState[i] != state) 
        {
            //xt_log.debug("seatid:%d state is %s, check state is %s\n", i, DESC_OP[m_opState[i]], DESC_OP[state]);
            return false; 
        }
    }
    return true;
}

bool Table::selecLord(void)
{
    //如果有3分直接地主, 
    if(m_callScore[m_curSeat] == 3)
    {
        m_topCall = m_callScore[m_curSeat];
        m_lordSeat = m_curSeat;
        //xt_log.debug("selectLord success, score:%d, seatid:%d, uid:%d\n", m_topCall, m_lordSeat, getSeat(m_lordSeat));
        return true;
    }

    //如果3人都已经叫过分，选择最高分地主
    bool isAll = true;
    unsigned int seatid = 0;
    int score = 0;
    for(unsigned int i = 0; i < SEAT_NUM; ++i) 
    {
        if(m_callScore[i] > score) 
        {
            score = m_callScore[i]; 
            seatid = i;
        }

        if(m_opState[i] != OP_CALL_RECEIVE)
        {
            isAll = false;
        }
    }
    //not all respond
    if(!isAll)
    {
        //xt_log.debug("selectLord fail, no all respond.\n");
        return false;
    }
    // no one give score
    if(score == 0)
    {
        //xt_log.debug("selectLord fail, no one give score.\n");
        return false;
    }

    m_topCall = score;
    m_lordSeat = seatid;

    //xt_log.debug("selectLord success, score:%d, seatid:%d, uid:%d\n", m_topCall, m_lordSeat, getSeat(m_lordSeat));
    return true;
}

static string printStr;

void Table::show(const vector<XtCard>& card)
{
    printStr.clear();
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        //xt_log.debug("%s\n", it->getCardDescription());
        printStr.append(it->getCardDescriptionString());
        printStr.append(" ");
    }
    xt_log.debug("%s\n", printStr.c_str());
}

void Table::showGame(void)
{
    Player* tmpplayer = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        tmpplayer = it->second;
        if(tmpplayer == NULL) continue;
        xt_log.debug("uid:%d, money:%d, name:%s\n", tmpplayer->m_uid, tmpplayer->m_money, tmpplayer->m_name.c_str());
    }

    xt_log.debug("tid:%d, seat0:%d, seat1:%d, seat2:%d\n", m_tid, getSeat(0), getSeat(1), getSeat(2));
}

int Table::getAllDouble(void)
{
    int ret = 0;

    //叫分加倍
    int callDouble = 0;
    //炸弹加倍
    int bombDouble = 0;
    //底牌加倍
    int bottomDouble = getBottomDouble();
    //春天加倍
    int springDouble = isSpring() ? 2 : 0;
    //反春天加倍
    int antiSpringDouble = isAntiSpring() ? 2 : 0;
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        bombDouble += m_bomb[i];
        if(m_callScore[i] > callDouble)
        {
            callDouble = m_callScore[i];
        }
    }
    //xt_log.debug("double: callDouble:%d, bombDouble:%d, bottomDouble:%d, springDouble:%d, antiSpringDouble:%d\n", callDouble, bombDouble, bottomDouble, springDouble, antiSpringDouble);
    ret = callDouble + bombDouble + bottomDouble + springDouble + antiSpringDouble;
    return max(ret, 1);
}

int Table::getBottomDouble(void)
{
    bool littleJoke = false;
    bool bigJoke = false;
    set<int> suitlist; 
    set<int> facelist; 
    bool isContinue = m_deck.isNContinue(m_bottomCard, 1);
    for(vector<XtCard>::const_iterator it = m_bottomCard.begin(); it != m_bottomCard.end(); ++it)
    {
        if(it->m_value == 0x00) 
        {
            littleJoke = true;
        }
        else if(it->m_value == 0x10) 
        {
            bigJoke = true; 
        }
        suitlist.insert(it->m_suit);
        facelist.insert(it->m_face);
    }

    //火箭
    if(bigJoke && littleJoke)
    {        
        //printf("火箭");
        return 4;
    }

    //大王
    if(bigJoke && !littleJoke)
    {
        //printf("大王");
        return 2;
    }

    //小王
    if(!bigJoke && littleJoke)
    {
        //printf("小王");
        return 2;
    }

    //同花
    if(!isContinue && suitlist.size() == 1)
    {
        //printf("同花");
        return 3; 
    }

    //顺子
    if(isContinue && suitlist.size() != 1)
    {
        //printf("顺子");
        return 3; 
    }

    //同花顺
    if(isContinue && suitlist.size() == 1)
    {
        //printf("同花顺");
        return 4; 
    }

    //三同
    if(facelist.size() == 1)
    {
        //printf("三同");
        return 4; 
    }
    return 0;
}

bool Table::isSpring(void)
{
    //农民没出过牌，且地主出牌(因为开始时候地主也没出牌)
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(i != m_lordSeat && m_outNum[i] != 0)
        {
            return false; 
        }
    }
    if(m_outNum[m_lordSeat] == 0)
    {
        return false;
    }
    return true;
}

bool Table::isAntiSpring(void)
{
    //地主只出过1次牌，农民出过牌（初始时候农民没出牌）
    if(m_outNum[m_lordSeat] != 1)
    {
        return false;
    }
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        if(i != m_lordSeat && m_outNum[i] <= 0)
        {
            return false; 
        }
    }
    return true;
}

int Table::getBombNum(void)
{
    int ret = 0;
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        ret +=  m_bomb[i]; 
    }
    return ret;
}

int Table::getMinMoney(void)
{
    int money = 0; 
    for(unsigned int i = 0; i < SEAT_NUM; ++i)
    {
        Player* pl = getSeatPlayer(i);
        if(pl)
        {
            if(money == 0)
            {
                money = pl->m_money;
            }
            else if(pl->m_money < money)
            {
                money = pl->m_money;
            }
        }
    }
    return money;
}

void Table::payResult(void)
{
    //修改参数点
    Player* tmpplayer = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        tmpplayer = it->second;
        if(tmpplayer == NULL) continue;
        tmpplayer->changeMatch(m_money[tmpplayer->m_seatid]);
    }

    //返回报名费，奖励话费券
    Player* lord = getSeatPlayer(m_lordSeat); 
    Player* f1 = getSeatPlayer((m_lordSeat + 1) % 3); 
    Player* f2 = getSeatPlayer((m_lordSeat + 2) % 3); 
    if(m_win == m_lordSeat)
    {
        lord->changeMoney(hlddz.game->ROOMTAX);
        lord->changeBill(hlddz.game->BILL);
        f1->changeMoney(hlddz.game->ROOMTAX / 2);
        f2->changeMoney(hlddz.game->ROOMTAX / 2);
    }
    else
    {
        f1->changeMoney(hlddz.game->ROOMTAX);
        f2->changeMoney(hlddz.game->ROOMTAX);
        f1->changeBill(hlddz.game->BILL / 2);
        f2->changeBill(hlddz.game->BILL / 2);
    }
}

void Table::total(void)
{
    Player* tmpplayer = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        tmpplayer = it->second;
        if(tmpplayer == NULL) continue;
        tmpplayer->keepTotal(tmpplayer->m_seatid == m_win);
    }
}

void Table::calculate(int doubleNum)
{
    //台面额度
    int score = hlddz.game->ROOMSCORE * doubleNum;
    Player* lord = getSeatPlayer(m_lordSeat); 
    Player* big = getSeatPlayer((m_lordSeat + 1) % 3); 
    Player* small = getSeatPlayer((m_lordSeat + 2) % 3); 
    if(big->m_money < small->m_money)
    {
        std::swap(big, small);
    }

    //地主钱, 农民大，农民小, 改成参数点
    double lordmoney = static_cast<double>(lord->m_match);
    double bigmoney = static_cast<double>(big->m_match);
    double smallmoney = static_cast<double>(small->m_match);

    double lordchange = 0;
    double bigchange = 0;
    double smallchange = 0;

    if(m_win == m_lordSeat)
    {
        if(score * 2 <= lordmoney)
        {
            if(lordmoney / 2 <= smallmoney)     
            {
                lordchange = score * 2;
                smallchange = -score;
                bigchange = -score;
            }
            else
            {
                lordchange = lordmoney;
                smallchange = -smallmoney;
                bigchange = -(lordmoney-smallmoney);
            }
        }
        else
        {
            lordchange = lordmoney; 
            smallchange = -lordmoney * (smallmoney/(smallmoney + bigmoney));
            bigchange = -lordmoney * (bigmoney/(smallmoney + bigmoney));
        }
    }
    else
    {
        if(score * 2 <= lordmoney)
        {
            if(smallmoney <= score)
            {
                if(bigmoney <= score) 
                {
                    lordchange = -(smallmoney + bigmoney); 
                    smallchange = smallmoney;
                    bigchange = bigmoney;
                }
                else
                {
                    lordchange = -(smallmoney + score); 
                    smallchange = smallmoney;
                    bigchange = score;
                }
            }
            else
            {
                lordchange = -(score * 2); 
                smallchange = score;
                bigchange = score;
            }
        }
        else
        {
            lordchange = -lordmoney; 
            smallchange = lordmoney * (smallmoney/(smallmoney + bigmoney));
            bigchange = lordmoney * (bigmoney/(smallmoney + bigmoney));
        }
    }

    m_money[m_lordSeat] = lordchange;
    m_money[big->m_seatid] = bigchange;
    m_money[small->m_seatid] = smallchange;
}

void Table::setSeat(int uid, int seatid)
{
    if(seatid < 0 || seatid >= static_cast<int>(SEAT_NUM))
    {
        xt_log.error("%s:%d, setSeat error seatid:%d, uid:%d\n", __FILE__, __LINE__, seatid, uid); 
        return; 
    }
    m_seats[seatid] = uid;
}

int Table::getSeat(int seatid)
{
    if(seatid < 0 || seatid >= static_cast<int>(SEAT_NUM))
    {
        xt_log.error("%s:%d, getSeat error seatid:%d\n", __FILE__, __LINE__, seatid); 
        return 0; 
    }
    return m_seats[seatid];
}

void Table::kick(void)
{
    //xt_log.debug("check kick.\n");
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        Player* pl = it->second;
        if(pl->m_money < hlddz.game->ROOMTAX || m_entrust[pl->m_seatid])
        {
            Jpacket packet;
            packet.val["cmd"]           = SERVER_KICK;
            packet.val["uid"]           = pl->m_uid;
            packet.end();
            unicast(NULL, packet.tostring());
            xt_log.debug("%s:%d, kick player for not enough money, uid:%d, seatid:%d, money:%d, roomtax:%d\n",__FILE__, __LINE__, pl->m_uid, pl->m_seatid, pl->m_money, hlddz.game->ROOMTAX); 
            m_delPlayer.push_back(pl);
            //不能这里删除，否则logout里有对m_players的删除操作,导致容器错误, 且要保证发送消息完毕
        }
    }

    ev_timer_again(hlddz.loop, &m_timerKick);
}

void Table::addRobotMoney(Player* player)
{
    if(!player->isRobot() || player->m_money > hlddz.game->ROOMTAX)
    {
        return;
    }

    int addval = hlddz.game->ROOMTAX * (rand() % 9 + 1) + 100000;
    //xt_log.debug("%s:%d, addRobotMoney, uid:%d, money:%d \n",__FILE__, __LINE__, player->m_uid, addval); 
    player->changeMoney(addval);
}
        
void Table::addPlayersExp(void)
{
    int exp = 0;
    Player* player = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        player = it->second;
        exp = money2exp(m_money[player->m_seatid]); 
        player->addExp(exp);
    }
}
        
int Table::money2exp(int money)
{
    if(money <= 0)
    {
       return 0; 
    }
    else if(money <= 1000)
    {
        return 1;
    }
    else if(money >= 1001 && money <= 10000)
    {
        return 2;
    }
    else if(money >= 10001 && money <= 100000)
    {
        return 3;
    }
    else if(money >= 100001 && money <= 1000000)
    {
        return 4;
    }
    else if(money >= 1000001)
    {
        return 5;
    }
    else
    {
        return 0;
    }
}
        
void Table::entrustOut(void)
{
    //xt_log.debug("entrustOut. m_curSeat:%d\n", m_curSeat);
    Player* player = getSeatPlayer(m_curSeat);
    bool keep = false;
    vector<XtCard> curCard;
    vector<XtCard> &myCard = m_seatCard[m_curSeat].m_cards;

    XtCard::sortByDescending(myCard);
    XtCard::sortByDescending(m_lastCard);
    //首轮出牌
    if(m_lastCard.empty())
    {
        m_deck.getFirst(myCard, curCard);
    }
    //没人跟自己的牌
    else if(m_curSeat == m_outSeat)
    {
        m_deck.getFirst(myCard, curCard);
    }
    //跟别人的牌
    else
    {
        m_deck.getOut(myCard, m_lastCard, curCard);
        /*
        if(!curCard.empty() && CT_ERROR == m_deck.getCardType(curCard))
        {
            xt_log.debug("my card\n");
            show(myCard);
            xt_log.debug("last card\n");
            show(m_lastCard);
            xt_log.debug("select card\n");
            show(curCard);
        }
        */
    }
    keep = curCard.empty() ? true : false; 

    sendEntrustOut(player, curCard, keep); 
    //xt_log.debug("entrust out, uid:%d, keep:%s\n", player->m_uid, keep ? "true" : "false");
        //show(curCard);

    //判断是否结束和通知下一个出牌人，本轮出牌
    logicOut(player, curCard, keep);
}

void Table::payTax(void)
{
    Player* tmpplayer = NULL;
    for(std::map<int, Player*>::iterator it = m_players.begin(); it != m_players.end(); ++it) 
    {
        tmpplayer = it->second;
        if(tmpplayer == NULL) continue;
        tmpplayer->changeMoney(-hlddz.game->ROOMTAX);
        tmpplayer->changeMatch(hlddz.game->ROOMTAX);
    }
}

bool Table::checkCard(unsigned int seatid, const vector<XtCard>& outcard)  
{
    if(seatid < 0 || seatid > SEAT_NUM)
    {
        return false;
    }

    if(outcard.empty())
    {
        return true;
    }

    set<int> tmpset;
    const vector<XtCard>& holdcard = m_seatCard[seatid].m_cards;
    for(vector<XtCard>::const_iterator it = holdcard.begin(); it != holdcard.end(); ++it)
    {
        tmpset.insert(it->m_value);    
    }

    for(vector<XtCard>::const_iterator it = outcard.begin(); it != outcard.end(); ++it)
    {
        if(tmpset.find(it->m_value) == tmpset.end())
        {
            return false; 
        }
    }

    return true;
}
        
void Table::addBottom2Lord(void)
{
    for(vector<XtCard>::const_iterator it = m_bottomCard.begin(); it != m_bottomCard.end(); ++it)
    {
        m_seatCard[m_lordSeat].m_cards.push_back(*it);
    }
}

void Table::refreshConfig(void)
{
    //最低携带
    int ret = 0;
	ret = hlddz.cache_rc->command("hget %s accessStart", hlddz.game->m_venuename.c_str());
    long long accessStart = 0;
    if(ret < 0 || false == hlddz.cache_rc->getSingleInt(accessStart))
    {
		xt_log.error("get accessStart fail. venuename:%s\n", hlddz.game->m_venuename.c_str());
    }
    else
    {
        hlddz.game->ROOMLIMIT = accessStart;
    }

    //房间底分
	ret = hlddz.cache_rc->command("hget %s startPoint", hlddz.game->m_venuename.c_str());
    long long startPoint = 0;
    if(ret < 0 || false == hlddz.cache_rc->getSingleInt(startPoint))
    {
		xt_log.error("get startPoint fail. venuename:%s\n", hlddz.game->m_venuename.c_str());
    }
    else
    {
        hlddz.game->ROOMSCORE = startPoint;
    }
    
    //台费
	ret = hlddz.cache_rc->command("hget %s accessFee", hlddz.game->m_venuename.c_str());
    long long accessFee = 0;
    if(ret < 0 || false == hlddz.cache_rc->getSingleInt(accessFee))
    {
		xt_log.error("get accessFee fail. venuename:%s\n", hlddz.game->m_venuename.c_str());
    }
    else
    {
        hlddz.game->ROOMTAX = accessFee;
    }
    
    //xt_log.debug("roomlimit:%d, roomscore:%d \n", hlddz.game->ROOMLIMIT, hlddz.game->ROOMSCORE);
}
