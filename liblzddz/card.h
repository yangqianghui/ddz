#ifndef _CARD_H_
#define _CARD_H_

#include <string>
#include <iostream>
#include <cstdio>
#include <vector>
#include <map>
#include <algorithm>

#include "cardtype.h"

using namespace std;

class Card
{
	public:
		Card();
		Card(int val);

	public:
		void setValue(int val);
        //癞子变牌, val 变化后牌的value
        void change(int val);
        //只变点数，不变花色
        void changeFace(int face);
        //取消癞子变化
        void recover(void);

		const char* getCardDescription() const;
		string getCardDescriptionString(void) const;

		bool operator <  (const Card &c) const{ return (m_face < c.m_face); };
		bool operator >  (const Card &c) const { return (m_face > c.m_face); };
		bool operator == (const Card &c) const { return (m_face == c.m_face); };

		static int compare(const Card &a, const Card &b)
		{
			if (a.m_face > b.m_face)
			{
				return 1;
			}
			else if (a.m_face < b.m_face)
			{
				return -1;
			}
			else if (a.m_face == b.m_face)
			{
				if (a.m_suit > b.m_suit)
				{
					return 1;
				}
				else if (a.m_suit < b.m_suit)
				{
					return -1;
				}	
			}
			return 0;
		}

        //是否癞子牌
        bool isLZ(int lzface) const;
        //是否变化
        bool isChange(void) const {return m_oldvalue != m_value;}


	public:
        //有效量
		int m_face;
		int m_suit;
		int m_value;
        //原有量, 表示癞子牌原来的数值
        int m_oldvalue;

	public:
		static bool lesserCallback(const Card &a, const Card &b)
		{
			if (Card::compare(a, b) == -1)
				return true;
			else
				return false;
		}

		static bool greaterCallback(const Card &a, const Card &b)
		{
			if (Card::compare(a, b) == 1)
				return true;
			else
				return false;
		}

		static void sortByAscending(std::vector<Card> &v)
		{
			sort(v.begin(), v.end(), Card::lesserCallback);
		}

		static void sortByDescending(std::vector<Card> &v)
		{
			sort(v.begin(), v.end(), Card::greaterCallback);
		}

		static void dumpCards(std::vector<Card> &v, string str = "cards");
		static void dumpCards(std::map<int, Card> &m, string str = "cards");

        bool isContinuCard(void) const
        {
            //大小王和2不能作为连续的牌
            return (m_face != 16) && (m_face != 15);
        }

        bool isJoker(void) const
        {
            return m_face == 16; 
        }

        bool isBigJoker(void) const
        {
            return (m_value == 0x10);
        }
};

#endif /* _CARD_H_ */
