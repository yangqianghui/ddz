#ifndef _TABLE_H_
#define _TABLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ev++.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <json/json.h>

#include <map>
#include <set>

#include "XtDeck.h"
#include "XtTypeDeck.h"
#include "XtHoleCards.h"
#include "jpacket.h"

class Player;
class Client;

class Table
{
public:
    int							tid;
    int             			vid;
	std::map<int, Player*>		players;

public:
    Table();
    virtual ~Table();
	int init(int my_tid, int my_vid, int my_zid, int my_type, int my_min_money,
				int my_max_money, int my_base_money, int my_min_round, int my_max_round, float my_fee,
				int my_lose_exp, int my_win_exp);
    int broadcast(Player *player, const std::string &packet);
    int unicast(Player *player, const std::string &packet);
	int random(int start, int end);
	void vector_to_json_array(std::vector<XtCard> &cards, Jpacket &packet, string key);
	void map_to_json_array(std::map<int, XtCard> &cards, Jpacket &packet, string key);
	void json_array_to_vector(std::vector<XtCard> &cards, Jpacket &packet, string key);
};

#endif
