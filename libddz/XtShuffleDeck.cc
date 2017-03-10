#include <algorithm>

#include "XtShuffleDeck.h"

static int card_arr[] = {
	0x00, 0x10,                 //Joker 16: 0x00 little joker, 0x10 big joker
	0x01, 0x11, 0x21, 0x31,		//A 14 
	0x02, 0x12, 0x22, 0x32,		//2 15
	0x03, 0x13, 0x23, 0x33,		//3 3
	0x04, 0x14, 0x24, 0x34,		//4 4
	0x05, 0x15, 0x25, 0x35,		//5 5
	0x06, 0x16, 0x26, 0x36,		//6 6
	0x07, 0x17, 0x27, 0x37,		//7 7
	0x08, 0x18, 0x28, 0x38,		//8 8
	0x09, 0x19, 0x29, 0x39,		//9 9
	0x0A, 0x1A, 0x2A, 0x3A,		//10 10
	0x0B, 0x1B, 0x2B, 0x3B,		//J 11
	0x0C, 0x1C, 0x2C, 0x3C,		//Q 12
	0x0D, 0x1D, 0x2D, 0x3D,		//K 13
};

XtShuffleDeck::XtShuffleDeck()
{
}

XtShuffleDeck::~XtShuffleDeck()
{
}

void XtShuffleDeck::fill()
{
	m_cards.clear();

	for (int i = 0; i < 54; i++)
	{
		XtCard c(card_arr[i]);
		push(c);
	}
}

void XtShuffleDeck::empty()
{
	m_cards.clear();
}

int XtShuffleDeck::count() const
{
	return m_cards.size();
}

bool XtShuffleDeck::push(const XtCard& card)
{
	m_cards.push_back(card);
	return true;
}

bool XtShuffleDeck::pop(XtCard& card)
{
	if(m_cards.empty())
	{
		return false;
	}

    card.setValue(m_cards.back().m_value);
	m_cards.pop_back();
	return true;
}

void XtShuffleDeck::shuffle(int seed)
{
	srand(time(NULL) + seed);
	random_shuffle(m_cards.begin(), m_cards.end());
}

bool XtShuffleDeck::getHoleCards(std::vector<XtCard>& card, unsigned int num)
{
    if(m_cards.size() < num)
    {
        return false;
    }

	XtCard tmp;
	for (unsigned int i = 0; i < num; i++)
	{
		pop(tmp);
        card.push_back(tmp);
        
	}
	return true;
}

int XtShuffleDeck::changeHoleCards(int pos, XtHoleCards* holecards)
{
	if(m_cards.size()==0)
	{
		return -1;
	}

	XtCard card;
	holecards->m_cards.erase(holecards->m_cards.begin() + pos);
	pop(card);
	holecards->m_cards.push_back(card);

	return 0;
}

void XtShuffleDeck::showCards(void) const
{
    for(vector<XtCard>::const_iterator it = m_cards.begin(); it != m_cards.end(); ++it)
    {
       printf("%s\n", it->getCardDescription());
    }
}
        
int XtShuffleDeck::getCardType(const std::vector<XtCard>& card) 
{
    if(isRocket(card))
    {
        return CT_ROCKET;
    }

    if(isBomb(card))
    {
        return CT_BOMB;
    }

    if(isShuttle(card))
    {
        return CT_BOMB;
    }

    if(isAircraft(card))
    {
        return CT_AIRCRAFT;
    }

    if(is4and2(card))
    {
        return CT_4AND2;
    }

    if(isDoubleStraight(card))
    {
        return CT_DOUBLE_STRAIGHT;
    }

    if(isStraight(card))
    {
        return CT_STRAIGHT;
    }

    if(isThree(card))
    {
        return CT_THREE;
    }

    if(isPair(card))
    {
        return CT_PAIR;
    }

    if(isSingle(card))
    {
        return CT_SINGLE;
    }

    return CT_ERROR;
}
        
bool XtShuffleDeck::isRocket(const vector<XtCard>& card) const
{
    if(card.size() != 2)
    {
        return false;
    }

    if(card[0].m_face == card[1].m_face && card[0].m_face == 16)
    {
        return true;
    }

    return false;
}

bool XtShuffleDeck::isBomb(const vector<XtCard>& card) const
{
    if(card.size() != 4)
    {
        return false;
    }

    if(card[0].m_face == card[1].m_face && card[1].m_face == card[2].m_face && card[2].m_face == card[3].m_face)
    {
        return true;
    }

    return false;
}

bool XtShuffleDeck::isShuttle(const vector<XtCard>& card)
{
    if(card.size() < 8 || card.size() > 20)
    {
        return false;
    }

    if((card.size() % 4 != 0) && (card.size() % 6 != 0))
    {
        return false;
    }

    //取出全4的组合
    vector<XtCard> vecFour;
    keepN(vecFour, card, 4);

    //翼数量
    unsigned int wingNu = card.size() - vecFour.size(); 
    if((wingNu != 0) && (wingNu != vecFour.size() / 2))
    {
        return false;
    }

    //是否连续
    if(!isNContinue(vecFour, 4))
    {
       return false;  
    }

    return true;
}
        
bool XtShuffleDeck::isAircraft(const vector<XtCard>& card)
{
    if(card.size() < 6 || card.size() > 20)
    {
        return false;
    }

    //取出全3的组合
    vector<XtCard> vecThree;
    keepN(vecThree, card, 3);

    //翼数量 0, +1, +2
    unsigned int wingNu = card.size() - vecThree.size(); 
    if((wingNu != 0) && (wingNu != vecThree.size() / 3) && (wingNu != vecThree.size() / 3 * 2))
    {
        return false;
    }

   //是否连续
    if(!isNContinue(vecThree, 3))
    {
       return false;  
    }

    return true;
}
        
bool XtShuffleDeck::is4and2(const vector<XtCard>& card)
{
    if(card.size() < 6 || card.size() > 8)
    {
        return false;
    }

    //取出全4的组合
    vector<XtCard> vecFour;
    keepN(vecFour, card, 4);

    //翼数量
    unsigned int wingNu = card.size() - vecFour.size(); 
    if(wingNu == 2)
    {
        return true; 
    }

    if(wingNu == 4)
    {
        //取出全2的组合
        vector<XtCard> vecTwo;
        keepN(vecTwo, card, 2);
        if(vecTwo.size() == 4)
        {
            return true;
        }
    }

    return false;
}
        
bool XtShuffleDeck::isDoubleStraight(const vector<XtCard>& card) 
{
    if(card.size() < 6 || card.size() > 20)
    {
        return false;
    }

    //取出全2的组合
    vector<XtCard> vecTwo;
    keepN(vecTwo, card, 2);

    if(vecTwo.size() != card.size())
    {
        return false; 
    }

    //连续
    if(isNContinue(vecTwo, 2))
    {
        return true; 
    }

    return false;
}
        
bool XtShuffleDeck::isStraight(const vector<XtCard>& card)
{
    if(card.size() < 5 || card.size() > 12)
    {
        return false;
    }

    //取出全1的组合
    vector<XtCard> vecOne;
    keepN(vecOne, card, 1);

    if(vecOne.size() != card.size())
    {
        return false; 
    }

    //连续
    if(isNContinue(vecOne, 1))
    {
        return true; 
    }

    return false;
}
        
bool XtShuffleDeck::isThree(const vector<XtCard>& card)
{
    if(card.size() < 3 || card.size() > 5)
    {
        return false;
    }

    //取出全3的组合
    vector<XtCard> vecThree;
    keepN(vecThree, card, 3);

    //翼数量 0, +1, +2
    unsigned int wingNu = card.size() - vecThree.size(); 
    if((wingNu != 0) && (wingNu != 1) && (wingNu != 2))
    {
        return false;
    }

    return true;
}

bool XtShuffleDeck::isPair(const vector<XtCard>& card)
{
    if(card.size() != 2)
    {
        return false;
    }

    if(card[0].m_face != card[1].m_face)
    {
        return false;
    }

    if(card[0].isJoker())
    {
        return false; 
    }

    return true;
}
        
bool XtShuffleDeck::isSingle(const vector<XtCard>& card) const
{
    return card.size() == 1;
}

void XtShuffleDeck::analyze(map<int, int>& result, const vector<XtCard>& card) const
{
    result.clear();
    map<int, int> faceCot;
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
       faceCot[it->m_face] += 1;  
    }

    for(map<int, int>::const_iterator it = faceCot.begin(); it != faceCot.end(); ++it)
    {
        result[it->second] += 1;
    }
}
        
void XtShuffleDeck::keepN(vector<XtCard>& result, const vector<XtCard>& card, int nu)
{
    result.clear();
    map<int, int> faceCot;
    map<int, int>::const_iterator findit;
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
       faceCot[it->m_face] += 1;  
    }

    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        findit = faceCot.find(it->m_face);
        if(findit != faceCot.end() && findit->second == nu) 
        {
            result.push_back(*it); 
        }
    }
}

bool XtShuffleDeck::isMN(const vector<XtCard>& card, int m , int n) const
{
    return false;
}
        
bool XtShuffleDeck::isNContinue(const vector<XtCard>& card, int n) const
{
    if(card.size() % n != 0)
    {
        return false;
    }

    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        //大小王和2不能算
        if(!it->isContinuCard())
        {
            return false;
        }
    }

    for(size_t i = 0; i + n < card.size(); i += n)
    {
        if(card[i].m_face != card[i + n].m_face + 1)
        {
            return false;
        }
    }
    return true;
}
