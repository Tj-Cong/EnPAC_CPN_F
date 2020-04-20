//
// Created by hecong on 19-12-10.
//

#ifndef CPN_PNML_PARSE_CPN_RG_H
#define CPN_PNML_PARSE_CPN_RG_H

#include "CPN.h"
#include <cmath>
#include <ctime>
#include <iomanip>
using namespace std;

#define MAXVARNUM 30
#define CPNRGTABLE_SIZE 1048576
#define random(x) rand()%(x)

extern NUM_t placecount;
extern NUM_t transitioncount;
extern NUM_t varcount;
extern CPN *cpnet;
extern bool timeflag;
extern bool colorflag;

class MarkingMeta;
class CPN_RGNode;
class CPN_RG;
class CPN_Product;
class CPN_Product_Automata;

class FiringTrans
{
private:
    SHORTNUM tranid;
    COLORID varvec[MAXVARNUM];
public:
    FiringTrans(SHORTNUM tranid) {
        if(varcount>MAXVARNUM) {
            cerr<<"The number of variable is over "<<MAXVARNUM<<"("<<varcount<<")"<<endl;
            exit(-1);
        }
        this->tranid = tranid;
        memset(varvec,0, sizeof(COLORID)*varcount);
    }
    FiringTrans(SHORTNUM tranid,COLORID *bind) {
        if(varcount>MAXVARNUM) {
            cerr<<"The number of variable is over "<<MAXVARNUM<<"("<<varcount<<")"<<endl;
            exit(-1);
        }
        this->tranid = tranid;
        memcpy(this->varvec,bind, sizeof(COLORID)*varcount);
    }
    ~FiringTrans() {};
    bool getNextBinding();
    bool getNextFireBinding(CPN_RGNode *state);
    friend class CPN_Product;
    friend class CPN_RGNode;
};

class CPN_RGNode
{
public:
    MarkingMeta *marking;
    CPN_RGNode *next;
    vector<FiringTrans> fireQ;
public:
    int numid;
    CPN_RGNode(){
        numid = 0;
        marking=new MarkingMeta[placecount];
        for(int i=0;i<placecount;++i) {
            marking[i].initiate(cpnet->place[i].tid,cpnet->place[i].sid);
        }
        next=NULL;
    }
    ~CPN_RGNode();
    index_t Hash(SHORTNUM *weight);
    bool isFiringBinding(const COLORID *varvec,const CTransition &tran);
    void getFiringTrans();
    bool isfirable(string transname);
    void printMarking();
    void selfcheck();
    bool operator==(const CPN_RGNode &n1);
    void operator=(const CPN_RGNode &rgnode);
    friend class CPN_RG;
};

class CPN_RG
{
private:
    CPN_RGNode **markingtable;
    CPN_RGNode *initnode;
public:
    SHORTNUM *weight;
    NUM_t hash_conflict_times;
    NUM_t nodecount;
public:
    CPN_RG();
    ~CPN_RG();
    void addRGNode(CPN_RGNode *mark);
    CPN_RGNode *CPNRGinitialnode();
    bool NodeExist(CPN_RGNode *mark,CPN_RGNode *&existmark);
    friend class CPN_Product_Automata;
    friend class CPN_Product;
};
#endif //CPN_PNML_PARSE_CPN_RG_H
