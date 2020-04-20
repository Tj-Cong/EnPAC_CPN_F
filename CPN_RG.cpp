//
// Created by hecong on 19-12-10.
//

#include "CPN_RG.h"

/*********************************************************/
/*对于变迁cpnet->transition[tranid]，寻找他的下一个绑定，
 * 如果没有绑定了则返回false；
 * */
bool FiringTrans::getNextBinding() {
    const CTransition &tt = cpnet->transition[tranid];
    for(int i=tt.relvararray.size()-1;i>=0;--i)
    {
        //i表示变迁tt的第i个变量
        const Variable &var = tt.relvararray[i];
        varvec[var.vid] = varvec[var.vid]+1;
        if(varvec[var.vid]>=var.upbound)
        {
            varvec[var.vid] = 0;
            continue;
        } else{
            return true;
        }
    }
    return false;
}

bool FiringTrans::getNextFireBinding(CPN_RGNode *state) {
    while(getNextBinding())
    {
        if(!timeflag)
            return false;
        CTransition &tran = cpnet->transition[tranid];
        if(state->isFiringBinding(this->varvec,tran)) {
            return true;
        }
    }
    return false;
}
/*****************************************************************/
CPN_RGNode::~CPN_RGNode() {
    delete [] marking;
    MallocExtension::instance()->ReleaseFreeMemory();
}

index_t CPN_RGNode::Hash(SHORTNUM *weight) {
    index_t hashvalue = 0;
    for(int i=0;i<placecount;++i)
    {
        hashvalue += weight[i]*marking[i].hashvalue;
    }
    return hashvalue;
}

/*function:判断两个状态CPN_RGNode是否为同一个状态；
 * */
bool CPN_RGNode::operator==(const CPN_RGNode &n1) {
    bool equal = true;
    for(int i=0;i<placecount;++i)
    {
        //检查库所i的tokenmetacount是否一样
        SHORTNUM tmc = this->marking[i].colorcount;
        if(tmc != n1.marking[i].colorcount)
        {
            equal = false;
            break;
        }

        //检查每一个tokenmeta是否一样
        Tokens *t1=this->marking[i].tokenQ->next;
        Tokens *t2=n1.marking[i].tokenQ->next;
        for(t1,t2; t1!=NULL && t2!=NULL; t1=t1->next,t2=t2->next)
        {
            //先检查tokencount
            if(t1->tokencount!=t2->tokencount)
            {
                equal = false;
                break;
            }

            //检查color
            type tid = cpnet->place[i].tid;
            if(tid == dot) {
                continue;
            }
            else if(tid == usersort){
                COLORID cid1,cid2;
                t1->tokens->getColor(cid1);
                t2->tokens->getColor(cid2);
                if(cid1!=cid2) {
                    equal = false;
                    break;
                }
            }
            else if(tid == productsort){
                COLORID *cid1,*cid2;
                SORTID sid = cpnet->place[i].sid;
                int sortnum = sorttable.productsort[sid].sortnum;
                cid1 = new COLORID[sortnum];
                cid2 = new COLORID[sortnum];
                t1->tokens->getColor(cid1,sortnum);
                t2->tokens->getColor(cid2,sortnum);
                for(int k=0;k<sortnum;++k)
                {
                    if(cid1[k]!=cid2[k])
                    {
                        equal = false;
                        delete [] cid1;
                        delete [] cid2;
                        break;
                    }
                }
                if(equal==false)
                    break;
                delete [] cid1;
                delete [] cid2;
            }
            else if(tid == finiteintrange){
                cerr<<"[CPN_RG\\line110]FiniteIntRange ERROR.";
                exit(-1);
            }
        }
        if(equal == false)
            break;
    }
    return equal;
}

/*function:复制一个状态
 * */
void CPN_RGNode::operator=(const CPN_RGNode &rgnode) {
    int i;
    for(i=0;i<placecount;++i)
    {
        const MarkingMeta &placemark = rgnode.marking[i];
        MarkingMetacopy(this->marking[i],placemark,placemark.tid,placemark.sid);
    }
}

bool CPN_RGNode::isfirable(string transname) {
    map<string,index_t>::iterator titer;
    titer = cpnet->mapTransition.find(transname);
    SHORTNUM tranid = titer->second;
    vector<FiringTrans>::const_iterator iter;
    for(iter=fireQ.begin();iter!=fireQ.end();++iter)
    {
        if(iter->tranid == tranid)
            return true;
    }
    return false;
}

void CPN_RGNode::getFiringTrans() {
    for(SHORTNUM i=0;i<transitioncount;++i)
    {
        if(!timeflag)
            return;
        //先进行预检测
        CTransition &tran = cpnet->transition[i];
        vector<CSArc>::iterator piter;
        bool possiblefire = true;
        for(piter=tran.producer.begin();piter!=tran.producer.end(); ++piter)
        {
            if(this->marking[piter->idx].colorcount == 0)
            {
                possiblefire = false;
                break;
            }
        }
        if(!possiblefire)
            continue;
        FiringTrans ft(i);
        if(this->isFiringBinding(ft.varvec,tran)) {
            this->fireQ.push_back(ft);
        }
        else if(ft.getNextFireBinding(this)) {
            this->fireQ.push_back(ft);
        }
    }
}

void CPN_RGNode::printMarking() {
    cout<<"[M"<<numid<<"]"<<endl;
    for(int i=0;i<placecount;++i)
    {
        cout<<setiosflags(ios::left)<<setw(15)<<cpnet->place[i].id<<":";
        this->marking[i].printToken();
    }
    cout<<"----------------------------------"<<endl;
}

void CPN_RGNode::selfcheck() {
    for(int i=0;i<placecount;++i)
    {
        if(marking[i].tid != cpnet->place[i].tid)
        {
            cerr<<"TYPE ERROR!"<<endl;
            exit(-1);
        }
        int cc = 0;
        Tokens *p = marking[i].tokenQ->next;
        while(p)
        {
            cc++;
            p=p->next;
        }
        if(cc!=marking[i].colorcount)
        {
            cerr<<"COLORCOUNT ERROR!"<<endl;
            exit(-1);
        }
        if(marking[i].tid == dot)
        {
            if(cc>1)
            {
                cerr<<"DOT ERROR!"<<endl;
                exit(-1);
            }
        }
    }
}

/*function:判断对于变迁tran来说，在当前状态下对于变量的绑定varvec能否发生
 * Algorithm:1.对于变迁tran的全部前集弧,首先计算其多重集，然后和前继库所的
 * 多重集进行比较
 * 2.判断guard函数*/
bool CPN_RGNode::isFiringBinding(const COLORID *varvec,const CTransition &tran) {
    //先判断Guard函数
    if(tran.hasguard)
    {
        judgeGuard(tran.guard.root->left,varvec);
        if(!(tran.guard.root->left->mytruth))
            return false;
    }

    bool firebound = true;
    vector<CSArc>::const_iterator preiter;
    int j=0;
    for(j,preiter=tran.producer.begin();preiter!=tran.producer.end();++j,++preiter)
    {
        MarkingMeta mm;
        //计算每一个弧的多重集，并判断和前继库所的大小关系
        type tid = cpnet->place[preiter->idx].tid;
        SORTID sid = cpnet->place[preiter->idx].sid;
        mm.initiate(tid,sid);
        int psnum;
        if(tid == productsort) {
            psnum = sorttable.productsort[sid].sortnum;
        } else{
            psnum = 0;
        }
        computeArcEXP(preiter->arc_exp,mm,varvec,psnum);

        if(colorflag==false)
        {
            firebound = false;
            break;
        }

        //判断前继库所的多重集和弧的多重集的关系
        if(this->marking[preiter->idx] >= mm){
            continue;
        }
        else {
            firebound = false;
            break;
        }
    }

    return firebound;
}

/*****************************************************************/
CPN_RG::CPN_RG() {
    initnode = NULL;
    markingtable = new CPN_RGNode*[CPNRGTABLE_SIZE];
    for(int i=0;i<CPNRGTABLE_SIZE;++i) {
        markingtable[i] = NULL;
    }
    nodecount = 0;
    hash_conflict_times = 0;

    weight = new SHORTNUM[placecount];
    srand((int)time(NULL));
    for(int j=0;j<placecount;++j)
    {
        weight[j] = random(133)+1;
    }
}

CPN_RG::~CPN_RG() {
    for(int i=0;i<CPNRGTABLE_SIZE;++i)
    {
        if(markingtable[i]!=NULL)
        {
            CPN_RGNode *p=markingtable[i];
            CPN_RGNode *q;
            while(p)
            {
                q=p->next;
                delete p;
                p=q;
            }
        }
    }
    delete [] markingtable;
    MallocExtension::instance()->ReleaseFreeMemory();
}

/*function：得到一个新的节点后，把这个节点加入到哈希表中；
 * Logics:
 * 1.计算该节点的哈希值；
 * 2.根据哈希值得到索引值，索引值为：hashvalue & size；
 * 3.根据索引值，插入到链式哈希的相应链中；
 * 4.维护nodecount；
 * */
void CPN_RG::addRGNode(CPN_RGNode *mark) {
    mark->numid = nodecount;
    index_t hashvalue = mark->Hash(weight);
    index_t size = CPNRGTABLE_SIZE-1;
    hashvalue = hashvalue & size;

    if(markingtable[hashvalue]!=NULL) {
        hash_conflict_times++;
        //cerr<<"<"<<mark->numid<<","<<markingtable[hashvalue]->numid<<">"<<endl;
    }

    mark->next = markingtable[hashvalue];
    markingtable[hashvalue] = mark;
    nodecount++;
}

 /*function:得到可达图的初始节点
  * Logics:
  * 1.遍历库所表的每一个库所，对于每一个库所，将这个库所的initMarking链复制到initnode.marking[i]中:
  * 2.遍历initMarking链的每一个Tokens，复制这个Tokens并插入到initnode.marking[i]链中；
  * 3.得到新状态后，不要忘记立马计算每一个库所的哈希值；
  * 4.将初始状态加入到哈西表中
  * */
CPN_RGNode *CPN_RG::CPNRGinitialnode() {
    initnode = new CPN_RGNode;
    //遍历每一个库所

    for(int i=0;i<placecount;++i)
    {
        CPlace &pp = cpnet->place[i];
        MarkingMeta &mm = initnode->marking[i];
        MarkingMetacopy(mm,pp.initM,pp.tid,pp.sid);
        mm.Hash();
    }
    initnode->getFiringTrans();
    addRGNode(initnode);
    return initnode;
}

bool CPN_RG::NodeExist(CPN_RGNode *mark,CPN_RGNode *&existmark) {
    index_t hashvalue = mark->Hash(weight);
    index_t size = CPNRGTABLE_SIZE-1;
    hashvalue = hashvalue & size;

    bool exist = false;
    CPN_RGNode *p = markingtable[hashvalue];
    while(p)
    {
        if(*p == *mark)
        {
            exist = true;
            existmark = p;
            break;
        }
        p=p->next;
    }
    return exist;
}
