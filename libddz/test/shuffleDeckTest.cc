#include "XtShuffleDeck.h"
#include "XtHoleCards.h"
#include "table.h"

void testShuttle(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard[] = {XtCard(0x04), XtCard(0x35), XtCard(0x26), XtCard(0x0B), XtCard(0x3B), XtCard(0x2B), XtCard(0x1B), XtCard(0x1C), XtCard(0x0C), XtCard(0x2C), XtCard(0x3C)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    //vector<XtCard> cards;
    //deck.getHoleCards(cards, 17);
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    

    if(deck.isShuttle0(cards))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testAircraft(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard[] = {XtCard(0x14), XtCard(0x25), XtCard(0x04), XtCard(0x15), XtCard(0x0B), XtCard(0x3B), XtCard(0x2B), XtCard(0x1C), XtCard(0x0C), XtCard(0x2C)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    //vector<XtCard> cards;
    //deck.getHoleCards(cards, 17);
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    

    if(deck.isAircraft0(cards))
    {
        printf("true!\n");
    }
    else
    {
    
        printf("false!\n");
    }
}

void test4and2(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard[] = {XtCard(0x01), XtCard(0x11), XtCard(0x2D), XtCard(0x3D), XtCard(0x3C), XtCard(0x1C), XtCard(0x0C), XtCard(0x2C)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    

    if(deck.is4and22s(cards))
    {
        printf("true!\n");
    }
    else
    {
    
        printf("false!\n");
    }
}

void testDoubleStraight(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard[] = {XtCard(0x0A), XtCard(0x1A), XtCard(0x2A), XtCard(0x3A), XtCard(0x3B), XtCard(0x1B), XtCard(0x0C), XtCard(0x2C)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    

    if(deck.isDoubleStraight(cards))
    {
        printf("true!\n");
    }
    else
    {
    
        printf("false!\n");
    }
}

void testStraight(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard[] = {XtCard(0x03), XtCard(0x14), XtCard(0x25), XtCard(0x36), XtCard(0x37), XtCard(0x18), XtCard(0x09), XtCard(0x29)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    

    if(deck.isStraight(cards))
    {
        printf("true!\n");
    }
    else
    {
    
        printf("false!\n");
    }
}

void testThree(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard[] = { XtCard(0x19), XtCard(0x09), XtCard(0x29)};
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    

    if(deck.isThree0(cards))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testPair(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard[] = { XtCard(0x00), XtCard(0x10) };
    vector<XtCard> cards(initCard, initCard + sizeof(initCard) / sizeof(XtCard));
    XtCard::sortByDescending(cards);

    for(vector<XtCard>::const_iterator it = cards.begin(); it != cards.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    

    if(deck.isPair(cards))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testCompareBomb(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard1[] = { XtCard(0x02), XtCard(0x12), XtCard(0x22), XtCard(0x32) };
    vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    XtCard::sortByDescending(cards1);
    for(vector<XtCard>::const_iterator it = cards1.begin(); it != cards1.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x33) };
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    XtCard::sortByDescending(cards2);
    for(vector<XtCard>::const_iterator it = cards2.begin(); it != cards2.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    

    if(deck.compareBomb(cards1, cards2))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testCompareShuttle(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard1[] = { XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x34), XtCard(0x05), XtCard(0x15), XtCard(0x25), XtCard(0x35) };
    vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    XtCard::sortByDescending(cards1);
    for(vector<XtCard>::const_iterator it = cards1.begin(); it != cards1.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x33), XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x34) };
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    XtCard::sortByDescending(cards2);
    for(vector<XtCard>::const_iterator it = cards2.begin(); it != cards2.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    
    //不检测牌型
    if(deck.compareShuttle(cards1, cards2))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testCompareAircraft(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    XtCard initCard1[] = { XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x05), XtCard(0x15)};
    vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    XtCard::sortByDescending(cards1);
    for(vector<XtCard>::const_iterator it = cards1.begin(); it != cards1.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x04), XtCard(0x14)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    XtCard::sortByDescending(cards2);
    for(vector<XtCard>::const_iterator it = cards2.begin(); it != cards2.end(); ++it)
    {
        cout << it->getCardDescription() << " ";
    }
    cout << endl;
    
    //不检测牌型
    if(deck.compareAircraft(cards1, cards2))
    {
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

static string printStr;
void show(const vector<XtCard>& card)
{
    printStr.clear();
    for(vector<XtCard>::const_iterator it = card.begin(); it != card.end(); ++it)
    {
        //xt_log.debug("%s\n", it->getCardDescription());
        printStr.append(it->getCardDescriptionString());
        printStr.append(" ");
    }
    printf("%s\n", printStr.c_str());
}

void testBigPair(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    //XtCard initCard1[] = { XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x05), XtCard(0x15), XtCard(0x06), XtCard(0x16)};
    //vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    XtCard::sortByDescending(cards1);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    
    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigPair(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testBigThree2s(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    //XtCard initCard1[] = { XtCard(0x04), XtCard(0x14), XtCard(0x24), XtCard(0x05), XtCard(0x15), XtCard(0x06), XtCard(0x16)};
    //vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    XtCard::sortByDescending(cards1);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x04), XtCard(0x14)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    
    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigThree2s(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testBigThree1(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    /*
    XtCard initCard1[] = { XtCard(16), XtCard(0), XtCard(34), XtCard(49), XtCard(28), XtCard(27), XtCard(41), 
        XtCard(40), XtCard(24), XtCard(39), XtCard(6), XtCard(53), XtCard(5), XtCard(36), XtCard(35), XtCard(19), XtCard(3)};
    vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));
    */
    XtCard::sortByDescending(cards1);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23), XtCard(0x04)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    
    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigThree1(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testBigThree0(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    XtCard::sortByDescending(cards1);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x13), XtCard(0x23)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    
    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigThree0(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

void testBigStraight(void)
{
    XtShuffleDeck deck;
    deck.fill();
    deck.shuffle(0);

    vector<XtCard> cards1;
    deck.getHoleCards(cards1, 17);
    //XtCard initCard1[] = { XtCard(0x02), XtCard(0x12), XtCard(0x22), XtCard(0x32) };
    //vector<XtCard> cards1(initCard1, initCard1 + sizeof(initCard1) / sizeof(XtCard));

    XtCard::sortByDescending(cards1);

    XtCard initCard2[] = { XtCard(0x03), XtCard(0x14), XtCard(0x25), XtCard(0x06), XtCard(0x07)};
    vector<XtCard> cards2(initCard2, initCard2 + sizeof(initCard2) / sizeof(XtCard));
    
    show(cards1);
    show(cards2);
    vector<XtCard> result;
    if(deck.bigStraight(cards1, cards2, result))
    {
        show(result);
        printf("true!\n");
    }
    else
    {
        printf("false!\n");
    }
}

/*
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
 */


int main()
{
    //testDoubleStraight();
    //testStraight();
    //testPair();
    //testCompareBomb();
    //testCompareShuttle();
    //testCompareAircraft();
    //testBigPair();
    //testBigThree2s();
    //testBigThree1();
    //testBigThree0();
    testBigStraight();
    return 0;
}

