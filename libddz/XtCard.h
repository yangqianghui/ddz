#ifndef _CARD_H_
#define _CARD_H_

#include <string>
#include <iostream>
#include <cstdio>
#include <vector>
#include <map>
#include <algorithm>

#include "XtCardType.h"

using namespace std;

class XtCard
{
	public:
		XtCard();
		XtCard(int val);

	public:

		void setValue(int val);

		std::string getCardDescription() const;

		bool operator <  (const XtCard &c) const{ return (m_face < c.m_face); };
		bool operator >  (const XtCard &c) const { return (m_face > c.m_face); };
		bool operator == (const XtCard &c) const { return (m_face == c.m_face); };

		static int compare(const XtCard &a, const XtCard &b)
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


	public:
		int m_face;
		int m_suit;
		int m_value;

	public:
		static bool lesserCallback(const XtCard &a, const XtCard &b)
		{
			if (XtCard::compare(a, b) == -1)
				return true;
			else
				return false;
		}

		static bool greaterCallback(const XtCard &a, const XtCard &b)
		{
			if (XtCard::compare(a, b) == 1)
				return true;
			else
				return false;
		}

		static void sortByAscending(std::vector<XtCard> &v)
		{
			sort(v.begin(), v.end(), XtCard::lesserCallback);
		}

		static void sortByDescending(std::vector<XtCard> &v)
		{
			sort(v.begin(), v.end(), XtCard::greaterCallback);
		}

		static void dumpCards(std::vector<XtCard> &v, string str = "cards");
		static void dumpCards(std::map<int, XtCard> &m, string str = "cards");
};

#endif /* _CARD_H_ */