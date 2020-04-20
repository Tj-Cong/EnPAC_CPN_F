//
// Created by hecong on 20-1-8.
//

#include "CPN_Product.h"

bool timeflag;
void sig_handler(int num)
{
    //printf("time out .\n");
    timeflag = false;
    ready2exit = true;
}
/*********************************************************/
bool CPN_Product::getNextBinding() {
    const CTransition &tt = cpnet->transition[this->RGname_ptr->fireQ[fireptr].tranid];
    for(int i=tt.relvararray.size()-1;i>=0;--i)
    {
        //i表示变迁tt的第i个变量
        const Variable &var = tt.relvararray[i];
        firebind[var.vid] = firebind[var.vid]+1;
        if(firebind[var.vid]>=var.upbound)
        {
            firebind[var.vid] = 0;
            continue;
        } else{
            return true;
        }
    }
    return false;
}

bool CPN_Product::getNextFireBinding() {
    while(getNextBinding())
    {
        if(!timeflag)
            return false;
        CTransition &tran = cpnet->transition[this->RGname_ptr->fireQ[fireptr].tranid];
        if(RGname_ptr->isFiringBinding(this->firebind,tran)) {
            return true;
        }
    }
    return false;
}

CPN_RGNode *CPN_Product::getNextRGNode(bool &exist) {
    while(fireptr<this->RGname_ptr->fireQ.size())
    {
        FiringTrans &ft = this->RGname_ptr->fireQ[fireptr];
        CTransition &tt = cpnet->transition[ft.tranid];
        if(virgin == true) {
            virgin = false;
            CPN_RGNode *child;
            child = getChild(exist);
            if(child!=NULL && !exist)
                child->getFiringTrans();
            return child;
        }
        else if(this->getNextFireBinding()) {
            CPN_RGNode *child;
            child = getChild(exist);
            if(child!=NULL && !exist)
                child->getFiringTrans();
            return child;
        }
        else {
            ++fireptr;
            memcpy(firebind,RGname_ptr->fireQ[fireptr].varvec, sizeof(COLORID)*MAXVARNUM);
            virgin = true;
        }
    }
    return NULL;
}

CPN_RGNode *CPN_Product::getChild(bool &exist) {
    CTransition &tran = cpnet->transition[RGname_ptr->fireQ[fireptr].tranid];
    CPN_RGNode *newmark = new CPN_RGNode;
    *newmark = *this->RGname_ptr;
    int i;
    vector<CSArc>::const_iterator front;
    for(i=0,front=tran.producer.begin();front!=tran.producer.end();++i,++front)
    {
        MarkingMeta ms_pre;
        type tid = cpnet->place[front->idx].tid;
        SORTID sid = cpnet->place[front->idx].sid;
        ms_pre.initiate(tid,sid);
        int psnum;
        if(tid==productsort) {
            psnum = sorttable.productsort[sid].sortnum;
        }
        else {
            psnum = 0;
        }
        computeArcEXP(front->arc_exp,ms_pre,firebind,psnum);
        MINUS(newmark->marking[front->idx],ms_pre);
    }

    vector<CSArc>::const_iterator rear;
    for(i=0,rear=tran.consumer.begin();rear!=tran.consumer.end();++i,++rear)
    {
        MarkingMeta ms_post;
        type tid = cpnet->place[rear->idx].tid;
        SORTID sid = cpnet->place[rear->idx].sid;
        ms_post.initiate(tid,sid);
        int psnum;
        if(tid == productsort) {
            psnum = sorttable.productsort[sid].sortnum;
        }
        else {
            psnum = 0;
        }
        computeArcEXP(rear->arc_exp,ms_post,firebind,psnum);
        PLUS(newmark->marking[rear->idx],ms_post);
    }

    for(int i=0;i<placecount;++i)
    {
        newmark->marking[i].Hash();
    }
    CPN_RGNode *existnode =NULL;
    exist = cpnRG->NodeExist(newmark,existnode);
    if(exist)
    {
//        cout<<"----------------------------------"<<endl;
//        cout<<"M"<<this->RGname_ptr->numid<<"[>"<<tran.id<<" M"<<existnode->numid<<endl;
//        cout<<"----------------------------------"<<endl;
        delete newmark;
        return existnode;
    } else{
        cpnRG->addRGNode(newmark);
//        cout<<"M"<<this->RGname_ptr->numid<<"[>"<<tran.id;
//        newmark->printMarking();
//        cout<<"***NODECOUNT:"<<cpnRG->nodecount<<"***"<<endl;
        return newmark;
    }

}

/*********************************************************/
index_t CHashtable::hashfunction(CPN_Product *node) {
    index_t hashvalue;
    index_t size = CHASHSIZE-1;
    hashvalue = node->RGname_ptr->Hash(cpnRG->weight);
    hashvalue = hashvalue & size;

    index_t Prohashvalue = hashvalue + node->BAname_id;
    Prohashvalue = Prohashvalue & size;
    return Prohashvalue;
}

index_t CHashtable::hashfunction(Small_CPN_Product *node) {
    index_t hashvalue;
    index_t size = CHASHSIZE-1;
    hashvalue = node->RGname_ptr->Hash(cpnRG->weight);
    hashvalue = hashvalue & size;

    index_t Prohashvalue = hashvalue + node->BAname_id;
    Prohashvalue = Prohashvalue & size;
    return Prohashvalue;
}

Small_CPN_Product *CHashtable::search(CPN_Product *node) {
    index_t idx = hashfunction(node);
    Small_CPN_Product *p = table[idx];
    while(p!=NULL)
    {
        if(p->BAname_id==node->BAname_id && p->RGname_ptr==node->RGname_ptr)
            return p;
        p=p->hashnext;
    }
    return NULL;
}

void CHashtable::remove(CPN_Product *node) {
    index_t idx = hashfunction(node);
    Small_CPN_Product *p=table[idx];
    Small_CPN_Product *q;
    if(p->BAname_id==node->BAname_id && p->RGname_ptr==node->RGname_ptr)
    {
        q=p->hashnext;
        table[idx] = q;
        delete p;
        return;
    }

    q=p;
    p=p->hashnext;
    while(p!=NULL)
    {
        if(p->BAname_id==node->BAname_id && p->RGname_ptr==node->RGname_ptr)
        {
            q->hashnext=p->hashnext;
            delete p;
            return;
        }

        q=p;
        p=p->hashnext;
    }
    cerr<<"Couldn't delete from hashtable!"<<endl;
}

void CHashtable::resetHash() {
    for(int i=0;i<CHASHSIZE;++i)
    {
        if(table[i]!=NULL) {
            Small_CPN_Product *p=table[i];
            Small_CPN_Product *q;
            while(p!=NULL) {
                q=p->hashnext;
                delete p;
                p=q;
            }
        }
    }
    memset(table,NULL,sizeof(CPN_Product *)*CHASHSIZE);
    nodecount = 0;
    hash_conflict_times = 0;
}

void CHashtable::insert(CPN_Product *node) {
    Small_CPN_Product *scp = new Small_CPN_Product(node);
    index_t idx = hashfunction(node);
    if(table[idx]!=NULL)
        hash_conflict_times++;
    scp->hashnext = table[idx];
    table[idx] = scp;
    nodecount++;
}


CPstack::CPstack() {
    toppoint = 0;
    hashlink = new index_t[CHASHSIZE];
    memset(hashlink,UNREACHABLE,sizeof(index_t)*CHASHSIZE);
    mydata = new CPN_Product* [CPSTACKSIZE];
    memset(mydata,NULL,sizeof(CPN_Product*)*CPSTACKSIZE);
}

CPstack::~CPstack() {
    delete [] hashlink;
    for(int i=0;i<toppoint;++i)
    {
        if(mydata[i]!=NULL)
            delete mydata[i];
    }
    delete [] mydata;
    MallocExtension::instance()->ReleaseFreeMemory();
}

index_t CPstack::hashfunction(CPN_Product *node) {
    index_t hashvalue = node->RGname_ptr->Hash(cpnRG->weight);
    index_t size = CHASHSIZE-1;
    hashvalue = hashvalue & size;
    index_t  Prohashvalue = (hashvalue+node->BAname_id)&size;
    return Prohashvalue;
}

CPN_Product *CPstack::top() {
    return mydata[toppoint-1];
}

CPN_Product *CPstack::pop() {
    CPN_Product *popitem = mydata[--toppoint];
    index_t hashpos = hashfunction(popitem);
    hashlink[hashpos] = popitem->stacknext;
    mydata[toppoint] = NULL;
    return popitem;
}

CPN_Product *CPstack::search(CPN_Product *node) {
    index_t hashpos = hashfunction(node);
    index_t pos = hashlink[hashpos];
    CPN_Product *p;
    while(pos!=UNREACHABLE)
    {
        p=mydata[pos];
        if(p->BAname_id==node->BAname_id && p->RGname_ptr==node->RGname_ptr)
        {
            return p;
        }
        pos = p->stacknext;
    }
    return NULL;
}

int CPstack::push(CPN_Product *node) {
    if(toppoint >= CPSTACKSIZE) {
        return ERROR;
    }
    index_t hashpos = hashfunction(node);
    node->stacknext = hashlink[hashpos];
    hashlink[hashpos] = toppoint;
    mydata[toppoint++] = node;
    return OK;
}

NUM_t CPstack::size() {
    return toppoint;
}

bool CPstack::empty() {
    return (toppoint==0)?true:false;
}

void CPstack::clear() {
    toppoint = 0;
    for(int i=0;i<toppoint;++i)
    {
        if(mydata[i]!=NULL) {
            delete mydata[i];
        }
    }
    memset(mydata,NULL, sizeof(CPN_Product*)*CPSTACKSIZE);
    memset(hashlink,UNREACHABLE,sizeof(index_t)*CHASHSIZE);
}


CPN_Product_Automata::CPN_Product_Automata(SBA *sba) {
    ret = -1;
    result = true;
    ba = sba;
    memory_flag = true;
    stack_flag = true;
    depth = 0;
}

CPN_Product_Automata::~CPN_Product_Automata() {
    MallocExtension::instance()->ReleaseFreeMemory();
}

/*function:判断是否为Fireability公式
 * */
bool CPN_Product_Automata::judgeF(string s) {
    int pos = s.find("<=");
    if (pos == string::npos)
        return true;            //LTLFireability category
    else
        return false;          //LTLCardinality category
}

/*function:判断F类型中的一个原子命题在状态state下是否成立
 * in: s是公式的一小部分，一个原子命题； state，状态
 * out: true(成立) or false(不成立)
 * */
bool CPN_Product_Automata::handleLTLF(string s, CPN_RGNode *state) {
    if(s[0] == '!')   //前面带有'!'的is-fireable{}
    {
        /*!{t1 || t2 || t3}：
         * true：t1不可发生 并且 t2不可发生 并且 t3不可发生
         * false： 只要有一个能发生
         * */
         s=s.substr(2,s.length()-2); //去掉“!{”
         while(true)
         {
             //取出一个变迁，看其是否在firetrans里
             int pos = s.find_first_of(",");
             if(pos < 0)
                 break;

             string transname = s.substr(0,pos);  //取出一个变迁

             if(state->isfirable(transname))
                 return false;

             s=s.substr(pos+1,s.length()-pos);
         }
         return true;
    }
    else              //单纯的is-fireable{}原子命题
    {

        /*{t1 || t2 || t3}:
	     * true: 只要有一个能发生
	     * false: 都不能发生
	     * */
        s.substr(1,s.length()-1);//去掉‘{’

        while(true)
        {
            int pos = s.find_first_of(",");
            if(pos < 0)
                break;

            string transname = s.substr(0,pos);

            if(state->isfirable(transname))
                return true;

            s=s.substr(pos+1,s.length()-pos);
        }
        return false;
    }
}

/*function: 判断C类型公式中的一个原子命题s在状态state下是否成立
 * in: s,原子命题； state,状态
 * out: true(成立) or false(不成立)
 * */
bool CPN_Product_Automata::handleLTLC(string s, CPN_RGNode *state) {
    NUM_t front_sum,latter_sum;
    if(s[0]=='!')
    {
        /*!(front <= latter)
	     * true:front > latter
	     * false:front <= latter
	     * */
	     s=s.substr(2,s.length()-2);
	     handleLTLCstep(front_sum,latter_sum,s,state);
	     if(front_sum<=latter_sum)
	         return false;
	     else
	         return true;
    }
    else
    {
        /*(front <= latter)
        * true:front <= latter
        * false:front > latter
        * */
        s=s.substr(1,s.length()-1);
        handleLTLCstep(front_sum,latter_sum,s,state);
        if(front_sum<=latter_sum)
            return true;
        else
            return false;
    }
}

/*function:计算在状态state下，C公式"<="前面库所的token和front_sum和后面库所的token和latter_sum
 * in: s,公式； state,状态
 * out: front_sum前半部分的和, latter_sum后半部分的和
 * */
void CPN_Product_Automata::handleLTLCstep(NUM_t &front_sum, NUM_t &latter_sum, string s, CPN_RGNode *state) {
    if(s[0]=='t')    //前半部分是token-count的形式
    {
        int pos = s.find_first_of("<=");      //定位到<=前
        string s_token = s.substr(12,pos-13); //去除"token-count(" ")"  ֻ只剩p1,p2,
        front_sum = sumtoken(s_token,state);  //计算token和

        s=s.substr(pos+2,s.length()-pos-2);

        if(s[0]=='t')  //后半部分是token-count
        {
            s_token = s.substr(12,s.length()-14);
            latter_sum = sumtoken(s_token,state);
        }
        else           //后半部分是常数
        {
            s=s.substr(0,s.length()-1);
            latter_sum=atoi(s.c_str());
        }
    }
    else             //前半部分是常数，那后半部分肯定是token-count
    {
        int pos = s.find_first_of("<=");
        string num = s.substr(0,pos);
        front_sum = atoi(num.c_str());

        s=s.substr(pos+14,s.length()-pos-16);
        latter_sum = sumtoken(s,state);
    }
}

NUM_t CPN_Product_Automata::sumtoken(string s,CPN_RGNode *state) {
    NUM_t sum=0;
    while(true)
    {
        int pos = s.find_first_of(",");
        if(pos == string::npos)
            break;
        string placename = s.substr(0,pos);
        map<string,index_t>::iterator piter;
        piter = cpnet->mapPlace.find(placename);
        sum += state->marking[piter->second].Tokensum();
        s=s.substr(pos+1,s.length()-pos);
    }
    return sum;
}

/*function: 判断可达图的一个状态和buchi自动机的一个状态能否合成交状态
 * in: state,可达图状态指针，指向要合成的可达图状态
 * sj,buchi自动机状态在邻接表中的序号
 * out: true(可以合成交状态) or false(不能合成状态)
 * */
bool CPN_Product_Automata::isLabel(CPN_RGNode *state, int sj) {
    string str = ba->svertics[sj].label;
    if(str == "true")
        return true;

    bool mark = false;
    while(true)
    {
        int pos = str.find_first_of("&&");
        if(pos == string::npos)   //最后一个原子命题
        {
            if(judgeF(str))
            {
                //如果是F类型公式
                /*a && b && c:
                 * true: 都为true
                 * false： 只要有一个为false
                 * */

                mark=handleLTLF(str,state);
                if(mark == false)
                    return false;

            } else {
                //如果是C类型公式
                mark = handleLTLC(str,state);
                if(mark == false)
                    return false;
            }
            break;
        }

        string AP = str.substr(0,pos);

        if(judgeF(AP))
        {
            //如果是F类型公式
            /*a && b && c:
             * true: 都为true
             * false： 只要有一个为false
             * */
            mark = handleLTLF(AP,state);
            if(mark==false)
                return false;
        }
        else
        {
            mark = handleLTLC(AP,state);
            if(mark == false)
                return false;
        }
        str = str.substr(pos+2,str.length()-pos-2);
    }
    return true;
}
/*function:得到当前状交状态的下一个交状态
 * Algorithm:1.首先生成当前可达图的下一个状态
 * 2.若为NULL
 *  i.若dink=true；表示当前状态没有可发生变迁，他的下一个状态仍然为自己
 *  ii.若dink=false，则直接返回NULL
 * 3.若不为NULL，则再判断能否生成交状态*/
CPN_Product *CPN_Product_Automata::getNextChild(CPN_Product *curstate) {
    bool exist;
    if(curstate->cur_RGchild == NULL) {
        curstate->cur_RGchild = curstate->getNextRGNode(exist);
    }
    while(curstate->cur_RGchild)
    {
        while(curstate->pba)
        {
            if(isLabel(curstate->cur_RGchild,curstate->pba->adjvex))
            {
                //能够生成交状态
                CPN_Product *qs = new CPN_Product;
                qs->RGname_ptr = curstate->cur_RGchild;
                qs->BAname_id = curstate->pba->adjvex;
                qs->pba = ba->svertics[qs->BAname_id].firstarc;
                qs->initiate();
//                cout<<"("<<qs->RGname_ptr->numid<<","<<qs->BAname_id<<")";
//                qs->RGname_ptr->printMarking();
                curstate->pba = curstate->pba->nextarc;
                return qs;
            }
            else {
                curstate->pba = curstate->pba->nextarc;
//                delete qs;
            }
        }
        curstate->cur_RGchild = curstate->getNextRGNode(exist);
        curstate->pba = ba->svertics[curstate->BAname_id].firstarc;
    }

    if(!timeflag)
        return NULL;
    if(curstate->RGname_ptr->fireQ.size() == 0)
    {
        //i.若dink=true；表示当前状态没有可发生变迁，他的下一个状态仍然为自己
        CPN_RGNode *rgseed=NULL;
        rgseed = curstate->RGname_ptr;
        while(curstate->pba)
        {

            if(isLabel(rgseed,curstate->pba->adjvex))
            {
                //能够生成交状态
                CPN_Product *qs = new CPN_Product;
                qs->RGname_ptr = rgseed;
                qs->BAname_id = curstate->pba->adjvex;
                qs->pba = ba->svertics[qs->BAname_id].firstarc;
                qs->initiate();
//                cout<<"("<<qs->RGname_ptr->numid<<","<<qs->BAname_id<<")";
//                qs->RGname_ptr->printMarking();
                curstate->pba = curstate->pba->nextarc;
                return qs;
            }
            else {
                curstate->pba = curstate->pba->nextarc;
//                delete qs;
            }
        }
        return NULL;
    } else {
        return NULL;
    }
}

void CPN_Product_Automata::Generate(CPN_Product *curstate) {
    CPN_RGNode *rgseed=NULL;
    bool exist;
    while(rgseed=curstate->getNextRGNode(exist))
    {
        if(exist)
            continue;
        rgseed->getFiringTrans();
        CPN_Product *ps = new CPN_Product;
        ps->RGname_ptr = rgseed;
        ps->initiate();
        Generate(ps);
    }
}

void CPN_Product_Automata::TCHECK(CPN_Product *p0) {
    PUSH(p0);
    while(!dstack.empty() && !ready2exit)
    {
        CPN_Product *p = cstack.mydata[dstack.top()];
        CPN_Product *qs  = getNextChild(p);
        if(qs == NULL)
        {
            //没有后继了
            POP();
            continue;
        }
        else
        {
            //还有后继状态
            CPN_Product *existnode = cstack.search(qs);
            if(existnode != NULL)
            {
                UPDATE(existnode);
                delete qs;
                continue;
            }
            if(h.search(qs) == NULL)
            {
                if(PUSH(qs)==ERROR)
                    break;
                continue;
            }
            delete qs;
        }
    }
}

void CPN_Product_Automata::UPDATE(CPN_Product *p0) {
    if(p0 == NULL)
        return;
    index_t dtop = dstack.top();
    if(p0->id <= cstack.mydata[dtop]->id)
    {
        if(!astack.empty() && p0->id<=astack.top())
        {
            result = false;
            ready2exit = true;
        }
        cstack.mydata[dtop]->id = p0->id;
    }
}

int CPN_Product_Automata::PUSH(CPN_Product *p0) {
    p0->id = cstack.toppoint;
    dstack.push(cstack.toppoint);
    if(dstack.size()>depth)
        depth = dstack.size();
    if(ba->svertics[p0->BAname_id].isAccept)
        astack.push(cstack.toppoint);
    if(cstack.push(p0)==ERROR)
    {
        stack_flag = false;
        return ERROR;
    }
    return OK;
}

void CPN_Product_Automata::POP() {
    index_t p=dstack.pop();
    if(cstack.mydata[p]->id == p)
    {
        //强连通分量的根节点
        while(cstack.toppoint>p)
        {
            CPN_Product *popitem = cstack.pop();
            h.insert(popitem);
            delete popitem;
        }
    }
    if(!astack.empty() && astack.top()==p){
        astack.pop();
    }
    if(!dstack.empty())
        UPDATE(cstack.mydata[p]);
}

/*function: 生成交自动机的初始状态，并加入到initial_status数组中
 * in: initnode,可达图的初始节点
 * out: void
 * */
void CPN_Product_Automata::getinitial_status(CPN_RGNode *initnode) {
    for(int i=0;i<ba->svex_num;++i)
    {
        if(ba->svertics[i].isInitial)
        {
            if(isLabel(initnode,i))
            {
                CPN_Product ps;
                ps.BAname_id = i;
                ps.RGname_ptr = initnode;
                ps.initiate();
                initial_status.push_back(ps);
            }
        }
    }
}

void CPN_Product_Automata::getProduct() {
//    detect_mem_thread = thread(&CPN_Product_Automata::detect_memory,this);

    if(cpnRG->initnode==NULL)
        cpnRG->CPNRGinitialnode();
    getinitial_status(cpnRG->initnode);

    int i=0;
    int end = initial_status.size();
    for(i;i<end;++i)
    {
        CPN_Product *init = new CPN_Product;
        init->RGname_ptr = initial_status[i].RGname_ptr;
        init->BAname_id = initial_status[i].BAname_id;
        init->initiate();
        init->pba = ba->svertics[init->BAname_id].firstarc;
        init->id = 0;
        TCHECK(init);
        if(ready2exit)
        {
            break;
        }
    }
//    ready2exit = true;
//    detect_mem_thread.join();
}

SHORTNUM CPN_Product_Automata::ModelChecker(string propertyid,SHORTNUM each_run_time) {
    signal(SIGALRM, sig_handler);
    alarm(each_run_time);
    timeflag = true;
    memory_flag = true;
    stack_flag = true;
    result = true;
    getProduct();

    string re;
    if(timeflag && memory_flag && stack_flag)
    {
        if(result)
        {
            re="TRUE";

            cout << "FORMULA " << propertyid << " " << re;
            ret = 1;
        }
        else
        {
            re="FALSE";
            cout << "FORMULA " << propertyid + " " << re;
            ret = 0;
        }
    }
    else if(!memory_flag)
    {
        cout<<"FORMULA "<<propertyid<<" "<<"CANNOT_COMPUTE";
        ret = -1;
    }
    else if(!stack_flag)
    {
        cout<<"FORMULA "<<propertyid<<" "<<"CANNOT_COMPUTE";
        ret = -1;
    }
    else if(!timeflag)
    {
        cout<<"FORMULA "<<propertyid<<" "<<"CANNOT_COMPUTE";
        ret = -1;
    }
    unsigned short timeleft = alarm(0);
    return (each_run_time - timeleft);
}

void CPN_Product_Automata::detect_memory() {
    for(;;)
    {
        int size=0;
        char filename[64];
        sprintf(filename,"/proc/%d/status",mypid);
        FILE *pf = fopen(filename,"r");
        if(pf == nullptr)
        {
//            cout<<"未能检测到enPAC进程所占内存"<<endl;
            pclose(pf);
        } else{
            char line[128];
            while(fgets(line,128,pf) != nullptr)
            {
                if(strncmp(line,"VmRSS:",6) == 0)
                {
                    int len = strlen(line);
                    const char *p=line;
                    for(;std::isdigit(*p) == false;++p) {}
                    line[len-3]=0;
                    size = atoi(p);
                    break;
                }
            }
            fclose(pf);
            size = size/1024;
            if(100*size/16000 > 85)
            {
                memory_flag = false;
                ready2exit = true;
                //cout<<"detect memory over the size  given"<<endl;
                break;
            }

        }
        if(ready2exit)
        {
            break;
        }
        usleep(500000);
    }
}

