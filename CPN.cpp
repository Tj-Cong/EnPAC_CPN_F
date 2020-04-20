//
// Created by hecong on 19-11-23.
//
#include "CPN.h"
SortTable sorttable;
bool colorflag;

/*************************************************************************/
int stringToNum(const string& str)
{
    istringstream iss(str);
    int num;
    iss >> num;
    return num;
}

SORTID SortTable::getFIRid(string FIRname) {
    vector<FiniteIntRange>::iterator iter;
    unsigned short idx = 0;
    for(iter=finitintrange.begin();iter!=finitintrange.end();++iter,++idx)
    {
        if(iter->id == FIRname)
            return idx;
    }
    return MAXSHORT;
}

SORTID SortTable::getPSid(string PSname) {
    vector<ProductSort>::iterator iter;
    unsigned short idx = 0;
    for(iter=productsort.begin();iter!=productsort.end();++iter,++idx)
    {
        if(iter->id == PSname)
            return idx;
    }
    return MAXSHORT;
}

SORTID SortTable::getUSid(string USname) {
    vector<UserSort>::iterator iter;
    unsigned short idx = 0;
    for(iter=usersort.begin();iter!=usersort.end();++iter,++idx)
    {
        if(iter->id == USname)
            return idx;
    }
    return MAXSHORT;
}

void Tokens::initiate(SHORTNUM tc,type sort,int PSnum) {
    tokencount = tc;
    switch(sort)
    {
        case type::productsort:{
            if(PSnum!=0)
                tokens = new ProductSortValue(PSnum);
            break;
        }
        case type::usersort:{
            tokens = new UserSortValue;
            break;
        }
        case type::dot:{
            tokens = NULL;
            break;
        }
        case type::finiteintrange:{
            tokens = new FIRSortValue;
            break;
        }
        default:{
            cerr<<"Sorry! Can not support this sort."<<endl;
            exit(-1);
        }
    }
}

CPN::CPN() {
    placecount = 0;
    transitioncount = 0;
    arccount = 0;
    varcount = 0;
}
/*This function is to first scan the pnml file and do these things:
 * 1. get the number of places,transitions,arcs and variables;
 * 2. alloc space for place table, transition table, arc table,
 * P_sorttable and variable table;
 * 3. parse declarations, get all sorts;
 * */
void CPN::getSize(char *filename) {
    TiXmlDocument *mydoc = new TiXmlDocument(filename);
    if(!mydoc->LoadFile()) {
        cerr<<mydoc->ErrorDesc()<<endl;
    }

    TiXmlElement *root = mydoc->RootElement();
    if(root == NULL) {
        cerr<<"Failed to load file: no root element!"<<endl;
        mydoc->Clear();
    }

    TiXmlElement *net = root->FirstChildElement("net");
    TiXmlElement *page = net->FirstChildElement("page");

    //doing the first job;
    while(page)
    {
        TiXmlElement *pageElement = page->FirstChildElement();
        while(pageElement)
        {
            string value = pageElement->Value();
            if(value == "place") {
                ++placecount;
            }
            else if(value == "transition") {
                transitioncount++;
            }
            else if(value == "arc") {
                arccount++;
            }
            pageElement = pageElement->NextSiblingElement();
        }
        page = page->NextSiblingElement();
    }

    //doing the second job
    place = new CPlace[placecount];
    transition = new CTransition[transitioncount];
    arc = new CArc[arccount];

    //doing the third job
    TiXmlElement *decl = net->FirstChildElement("declaration");
    TiXmlElement *decls = decl->FirstChildElement()->FirstChildElement("declarations");
    TiXmlElement *declContent = decls->FirstChildElement();
    SORTID psptr=0,usptr=0,firptr=0;
    while(declContent)
    {
        string value = declContent->Value();
        if(value == "namedsort")
        {
            TiXmlElement *sort = declContent->FirstChildElement();
            string sortname = sort->Value();
            if(sortname == "cyclicenumeration")
            {
                UserSort uu;
                uu.mytype = usersort;
                uu.id = declContent->FirstAttribute()->Value();
                TiXmlElement *feconstant = sort->FirstChildElement();
                COLORID colorindex;
                for(uu.feconstnum=0,colorindex=0; feconstant; ++uu.feconstnum,++colorindex,feconstant=feconstant->NextSiblingElement())
                {
                    string feconstid = feconstant->FirstAttribute()->Value();
                    uu.cyclicenumeration.push_back(feconstid);
                    uu.mapValue.insert(pair<string,SORTID>(feconstid,colorindex));
                    MCI  mci;
                    mci.cid = colorindex;
                    mci.sid = usptr;
                    sorttable.mapColor.insert(pair<string,MCI>(feconstid,mci));
                }
                sorttable.usersort.push_back(uu);

                MSI msi;
                msi.tid = usersort;
                msi.sid = usptr++;
                sorttable.mapSort.insert(pair<string,MSI>(uu.id,msi));
            }
            else if(sortname == "productsort")
            {
                ProductSort pp;
                pp.id = declContent->FirstAttribute()->Value();
                pp.mytype = productsort;
                int sortcount = 0;
                TiXmlElement *usersort = sort->FirstChildElement();
                for(sortcount=0;usersort;++sortcount,usersort=usersort->NextSiblingElement())
                {
                    string ustname = usersort->Value();
                    if(ustname != "usersort") {
                        cerr<<"Unexpected sort \'"<<ustname<<"\'. We assumed that every sort of productsort is usersort."<<endl;
                        exit(-1);
                    }
                    string relatedsortname = usersort->FirstAttribute()->Value();
                    pp.sortname.push_back(relatedsortname);
                }
                pp.sortnum = sortcount;
                sorttable.productsort.push_back(pp);

                MSI m;
                m.sid = psptr++;
                m.tid = productsort;
                sorttable.mapSort.insert(pair<string,MSI>(pp.id,m));
            }
            else if(sortname == "finiteintrange")
            {
                cerr<<"Find you, finiteintrange!"<<endl;
                exit(-1);
            }
            else if(sortname == "dot") {
                sorttable.hasdot = true;
            }
        }
        else if(value == "variabledecl")
        {
            ++varcount;
        }
        else
        {
            cerr<<"Can not support XmlElement "<<value<<endl;
            exit(-1);
        }
        declContent = declContent->NextSiblingElement();
    }

    vector<ProductSort>::iterator psiter;
    for(psiter=sorttable.productsort.begin();psiter!=sorttable.productsort.end();++psiter)
    {
        vector<string>::iterator miter;
        for(miter=psiter->sortname.begin();miter!=psiter->sortname.end();++miter)
        {
            map<string,MSI>::iterator citer;
            citer = sorttable.mapSort.find(*miter);
            if(citer!=sorttable.mapSort.end())
            {
                psiter->sortid.push_back(citer->second);
            }
            else cerr<<"[CPN.cpp\\line223].ProductSort error."<<endl;
        }
    }
    vartable = new Variable[varcount];
}

/*This function is to second scan pnml file just after getSize();
 * It does these jobs:
 * 1.parse pnml file, get every place's information,especially initMarking,
 * get every transition's information,especially guard fucntion;
 * get every arc's information, especially arc expressiion
 * 2.get all variable's information, set up variable table;
 * 3.construct the CPN, complete every place and transition's producer and consumer
 * */
void CPN::readPNML(char *filename) {
    TiXmlDocument *mydoc = new TiXmlDocument(filename);
    if(!mydoc->LoadFile()) {
        cerr<<mydoc->ErrorDesc()<<endl;
    }

    TiXmlElement *root = mydoc->RootElement();
    if(root == NULL) {
        cerr<<"Failed to load file: no root element!"<<endl;
        mydoc->Clear();
    }

    TiXmlElement *net = root->FirstChildElement("net");
    TiXmlElement *page = net->FirstChildElement("page");

    index_t pptr = 0;
    index_t tptr = 0;
    index_t aptr = 0;
    index_t vpter = 0;
    while(page)
    {
        TiXmlElement *pageElement = page->FirstChildElement();
        for(;pageElement;pageElement=pageElement->NextSiblingElement())
        {
            string value = pageElement->Value();
            if(value == "place"){
                CPlace &pp = place[pptr];
                //get id
                TiXmlAttribute *attr = pageElement->FirstAttribute();
                pp.id = attr->Value();
                //get type(mysort,mysortid)
                TiXmlElement *ttype = pageElement->FirstChildElement("type")
                        ->FirstChildElement("structure")->FirstChildElement("usersort");
                string sortname = ttype->FirstAttribute()->Value();
                if(sortname == "dot")
                {
                    pp.tid=dot;
                }
                else{
                    map<string,MSI>::iterator sortiter;
                    sortiter = sorttable.mapSort.find(sortname);
                    if(sortiter!=sorttable.mapSort.end()) {
                        pp.sid = (sortiter->second).sid;
                        pp.tid = (sortiter->second).tid;
                    }
                    else
                    {
                        cerr<<"Can not recognize sort \'"<<sortname<<"\'."<<endl;
                        exit(-1);
                    }
                }
                pp.initM.initiate(pp.tid,pp.sid);
                //get initMarking

                TiXmlElement *initM = pageElement->FirstChildElement("hlinitialMarking");
                if(initM!=NULL){
                    TiXmlElement *firstoperator = initM->FirstChildElement("structure")->FirstChildElement();
                    string opstr = firstoperator->Value();
                    if(opstr == "all")
                    {
                        if(pp.tid!=usersort){
                            cerr<<"ERROR @ line340. The \'all\' operator is not used for cyclicenumeration"<<endl;
                            exit(-1);
                        }
                        SHORTNUM feconstnum = sorttable.usersort[pp.sid].feconstnum;
                        for(COLORID i=0;i<feconstnum;++i)
                        {
                            Tokens *p = new Tokens;
                            p->initiate(1,usersort);
                            p->tokens->setColor(i);
                            pp.initM.insert(p);
                        }
                    }
                    else{
                        getInitMarking(initM,pp);
                    }
                }
                mapPlace.insert(pair<string,index_t>(pp.id,pptr));
                ++pptr;
            }
            else if(value == "transition")
            {
                CTransition &tt = transition[tptr];
                tt.id = pageElement->FirstAttribute()->Value();
                TiXmlElement *condition = pageElement->FirstChildElement("condition");
                if(condition)
                {
                    tt.guard.constructor(condition);
                    tt.hasguard = true;
                }
                else
                {
                    tt.hasguard = false;
                }
                mapTransition.insert(pair<string,index_t>(tt.id,tptr));
                ++tptr;
            }
            else if(value == "arc")
            {
                CArc &aa = arc[aptr];
                TiXmlAttribute *attr = pageElement->FirstAttribute();
                aa.id = attr->Value();
                attr = attr->Next();
                aa.source_id = attr->Value();
                attr = attr->Next();
                aa.target_id = attr->Value();
                TiXmlElement *hlinscription = pageElement->FirstChildElement("hlinscription");
                aa.arc_exp.constructor(hlinscription);
                ++aptr;
            }
        }
        page = page->NextSiblingElement();
    }

    //doing the second job:
    TiXmlElement *declContent = net->FirstChildElement("declaration")
                                    ->FirstChildElement("structure")
                                    ->FirstChildElement("declarations")
                                    ->FirstChildElement();
    while(declContent)
    {
        string value = declContent->Value();
        if(value == "variabledecl")
        {
            vartable[vpter].id = declContent->FirstAttribute()->Value();
            vartable[vpter].vid = vpter;
            TiXmlElement *ust = declContent->FirstChildElement();
            string ustname = ust->Value();
            if(ustname != "usersort")
            {
                cerr<<"Unexpected sort\'"<<ustname<<"\'. We assumed that every member of productsort is usersort."<<endl;
                exit(-1);
            }
            string sortname = ust->FirstAttribute()->Value();
            map<string,MSI>::iterator iter;
            iter = sorttable.mapSort.find(sortname);
            vartable[vpter].sid = (iter->second).sid;
            vartable[vpter].tid = (iter->second).tid;
            vartable[vpter].upbound = sorttable.usersort[vartable[vpter].sid].feconstnum;
            mapVariable.insert(pair<string,VARID>(vartable[vpter].id,vpter));
            if((iter->second).tid != usersort)
            {
                cerr<<"Sorry, but by now, we only support variables whose type is \'cyclicenumeration\'."<<endl;
                exit(-1);
            }
            ++vpter;
        }
        declContent = declContent->NextSiblingElement();
    }

    //doing the third job
    for(int i=0;i<arccount;++i)
    {
        CArc &aa = arc[i];
        map<string,index_t>::iterator souiter;
        map<string,index_t>::iterator tagiter;
        souiter = mapPlace.find(aa.source_id);
        if(souiter == mapPlace.end())
        {
            //souiter是一个变迁
            aa.isp2t = false;
            souiter = mapTransition.find(aa.source_id);
            tagiter = mapPlace.find(aa.target_id);
            CSArc forplace,fortrans;

            forplace.idx = souiter->second;
            forplace.arc_exp = aa.arc_exp;
            place[tagiter->second].producer.push_back(forplace);

            fortrans.idx = tagiter->second;
            fortrans.arc_exp = aa.arc_exp;
            transition[souiter->second].consumer.push_back(fortrans);
        }
        else
        {
            aa.isp2t = true;
            tagiter = mapTransition.find(aa.target_id);
            CSArc forplace,fortrans;

            forplace.idx = tagiter->second;
            forplace.arc_exp = aa.arc_exp;
            place[souiter->second].consumer.push_back(forplace);

            fortrans.idx = souiter->second;
            fortrans.arc_exp = aa.arc_exp;
            transition[tagiter->second].producer.push_back(fortrans);
        }
    }
}

void CPN::getInitMarking(TiXmlElement *elem,CPlace &pp) {
    pp.init_exp.constructor(elem);
    int psnum;
    if(pp.tid == productsort) {
        psnum = sorttable.productsort[pp.sid].sortnum;
    } else {
        psnum = 0;
    }
    computeArcEXP(pp.init_exp,pp.initM,NULL,psnum);
}

CPN::~CPN() {
    delete [] place;
    delete [] transition;
    delete [] arc;
    delete [] vartable;
}

void CPN::printCPN() {
    char placefile[]="place_info.txt";
//    char transfile[]="transition_info.txt";
//    char arcfile[]="arc_info.txt";
    ofstream outplace(placefile,ios::out);
//    ofstream outtrans(transfile,ios::out);
//    ofstream outarc(arcfile,ios::out);

    outplace<<"PLACE INFO {"<<endl;
    for(int i=0;i<placecount;++i)
    {
        string marking = "";
        place[i].initM.printToken(marking);
        outplace<<"\t"<<place[i].id<<" "<<marking<<endl;
    }
    outplace<<"}"<<endl;

    /********************print dot graph*********************/
    char dotfile[]="CPN.dot";
    ofstream outdot(dotfile,ios::out);
    outdot<<"digraph CPN {"<<endl;
    for(int i=0;i<placecount;++i)
    {
        outdot<<"\t"<<place[i].id<<" [shape=circle]"<<endl;
    }
    outdot<<endl;
    for(int i=0;i<transitioncount;++i)
    {
        CTransition &t = transition[i];
        if(t.hasguard)
        {
            string guardexp;
            t.guard.printEXP(guardexp);
            outdot<<"\t"<<transition[i].id<<" [shape=box,label=\""<<transition[i].id<<"//"<<guardexp<<"\"]"<<endl;
        }
        else outdot<<"\t"<<transition[i].id<<" [shape=box]"<<endl;
    }
    for(int i=0;i<arccount;++i)
    {
        CArc &a = arc[i];
        string arcexp;
        a.arc_exp.printEXP(arcexp);
        outdot<<"\t"<<a.source_id<<"->"<<a.target_id<<" [label=\""<<arcexp<<"\"]"<<endl;
    }
    outdot<<"}"<<endl;
}

void CPN::printSort() {
    char sortfile[]="sorts_info.txt";
    ofstream outsort(sortfile,ios::out);

    outsort<<"USERSORT:"<<endl;
    vector<UserSort>::iterator uiter;
    for(uiter=sorttable.usersort.begin();uiter!=sorttable.usersort.end();++uiter)
    {
        outsort<<"name: "<<uiter->id<<endl;
        outsort<<"constants: (";
        for(int i=0;i<uiter->feconstnum;++i)
        {
            outsort<<uiter->cyclicenumeration[i]<<",";
        }
        outsort<<")"<<endl;
        outsort<<"------------------------"<<endl;
    }
    outsort<<endl;

    outsort<<"PRODUCTSORT:"<<endl;
    vector<ProductSort>::iterator psiter;
    for(psiter=sorttable.productsort.begin();psiter!=sorttable.productsort.end();++psiter)
    {
        outsort<<"name: "<<psiter->id<<endl;
        outsort<<"member: (";
        for(int i=0;i<psiter->sortnum;++i)
        {
            outsort<<psiter->sortname[i]<<",";
        }
        outsort<<")"<<endl;
        outsort<<"------------------------"<<endl;
    }
    outsort<<endl;
}

void CPN::printVar() {
    ofstream outvar("sorts_info.txt",ios::app);
    outvar<<"VARIABLES:"<<endl;
    for(int i=0;i<varcount;++i)
    {
        outvar<<"name: "<<vartable[i].id<<endl;
        outvar<<"related sort: "<<sorttable.usersort[vartable[i].sid].id<<endl;
        outvar<<"------------------------"<<endl;
    }
}

void CPN::setGlobalVar() {
    ::placecount = this->placecount;
    ::transitioncount = this->transitioncount;
    ::varcount = this->varcount;
}

void CPN::getRelVars() {
    for(int i=0;i<transitioncount;++i)
    {
        CPN_Transition &tt = transition[i];
        vector<CSArc>::iterator preiter;
        //遍历所有前继弧的抽象语法树，得到所有变量
        for(preiter=tt.producer.begin();preiter!=tt.producer.end();++preiter)
        {
            TraArcTreeforVAR(preiter->arc_exp.root,tt.relvars);
        }

        vector<CSArc>::iterator postiter;
        for(postiter=tt.consumer.begin();postiter!=tt.consumer.end();++postiter)
        {
            TraArcTreeforVAR(postiter->arc_exp.root,tt.relvars);
        }

        set<Variable>::iterator viter;
        for(viter=tt.relvars.begin();viter!=tt.relvars.end();++viter)
        {
            tt.relvararray.push_back(*viter);
        }
    }
}

void CPN::TraArcTreeforVAR(meta *expnode, set<Variable> &relvars) {
    if(expnode == NULL)
        return;
    if(expnode->mytype == var)
    {
        map<string,VARID>::iterator viter;
        viter = mapVariable.find(expnode->myname);
        VARID vid = viter->second;
        relvars.insert(vartable[vid]);
        return;
    }
    if(expnode->leftnode!=NULL)
    {
        TraArcTreeforVAR(expnode->leftnode,relvars);
    }
    if(expnode->rightnode!=NULL)
    {
        TraArcTreeforVAR(expnode->rightnode,relvars);
    }
}

void CPN::printTransVar() {
    ofstream outvar("trans_var.txt",ios::out);
    for(int i=0;i<transitioncount;++i)
    {
        CPN_Transition &tt = transition[i];
        outvar<<transition[i].id<<":";
        set<Variable>::iterator iter;
        for(iter=tt.relvars.begin();iter!=tt.relvars.end();++iter)
        {
            outvar<<" "<<iter->id;
        }
        outvar<<endl;
    }
}

void Tokenscopy(Tokens &t1,const Tokens &t2,type tid,int PSnum)
{
    t1.initiate(t2.tokencount,tid,PSnum);
    if(tid == usersort)
    {
        COLORID cid;
        t2.tokens->getColor(cid);
        t1.tokens->setColor(cid);
    }
    else if(tid == productsort)
    {
        COLORID *cid = new COLORID[PSnum];
        t2.tokens->getColor(cid,PSnum);
        t1.tokens->setColor(cid,PSnum);
        delete [] cid;
    }
    else if(tid == finiteintrange)
    {
        cerr<<"[CPN.h\\Tokenscopy()].FiniteIntRange ERROR"<<endl;
        exit(-1);
    }
}

void MarkingMetacopy(MarkingMeta &mm1,const MarkingMeta &mm2,type tid,SORTID sid)
{
    mm1.colorcount = mm2.colorcount;
    Tokens *q = mm2.tokenQ->next;
    Tokens *p,*ppre = mm1.tokenQ;
    if(mm2.tid == productsort)
    {
        while(q)
        {
            p = new Tokens;
            int sortnum = sorttable.productsort[mm2.sid].sortnum;
            Tokenscopy(*p,*q,tid,sortnum);
            ppre->next = p;
            ppre = p;
            q=q->next;
        }
    }
    else
    {
        while(q)
        {
            p = new Tokens;
            Tokenscopy(*p,*q,tid);
            ppre->next = p;
            ppre = p;
            q=q->next;
        }
    }
}
//void arrayToString(string &str,COLORID *cid,int num)
//{
//    str="";
//    for(int i=0;i<num;++i)
//    {
//        str += to_string(cid[i]);
//    }
//}

int MINUS(MarkingMeta &mm1,const MarkingMeta &mm2)
{
    Tokens *pp1,*pp2,*pp1p;
    pp1 = mm1.tokenQ->next;
    pp1p = mm1.tokenQ;
    pp2 = mm2.tokenQ->next;
    if(mm1.tid == dot)
    {
        if(pp1->tokencount<pp2->tokencount)
            return 0;
        pp1->tokencount = pp1->tokencount-pp2->tokencount;
        if(pp1->tokencount == 0)
        {
            delete pp1;
            mm1.tokenQ->next=NULL;
            mm1.colorcount = 0;
        }
        return 1;
    }
    else if(mm1.tid == usersort)
    {
        COLORID cid1,cid2;
        while(pp2)
        {
            pp2->tokens->getColor(cid2);
            while(pp1)
            {
                pp1->tokens->getColor(cid1);
                if(cid1<cid2)
                {
                    pp1p = pp1;
                    pp1=pp1->next;
                    continue;
                }
                else if(cid1==cid2)
                {
                    if(pp1->tokencount < pp2->tokencount)
                        return 0;
                    pp1->tokencount = pp1->tokencount - pp2->tokencount;
                    if(pp1->tokencount == 0)
                    {
                        pp1p->next = pp1->next;
                        mm1.colorcount--;
                        delete pp1;

                        pp2=pp2->next;
                        pp1=pp1p->next;
                        break;
                    } else{
                        pp2=pp2->next;
                        pp1p = pp1;
                        pp1=pp1->next;
                        break;
                    }
                }
                else if(cid1 > cid2)
                {
                    return 0;
//                    cerr<<"[MINUS]ERROR:The minued is not greater than the subtractor."<<endl;
//                    exit(-1);
                }
            }
            if(pp1==NULL && pp2!=NULL)
            {
                return 0;
            }
        }
        return 1;
    }
    else if(mm1.tid == productsort)
    {
        int sortnum = sorttable.productsort[mm1.sid].sortnum;
        COLORID *cid1,*cid2;
        cid1 = new COLORID[sortnum];
        cid2 = new COLORID[sortnum];
        while(pp2)
        {
            pp2->tokens->getColor(cid2,sortnum);
            while(pp1)
            {
                pp1->tokens->getColor(cid1,sortnum);
                if(array_less(cid1,cid2,sortnum))
                {
                    pp1p = pp1;
                    pp1=pp1->next;
                    continue;
                }
                else if(array_equal(cid1,cid2,sortnum))
                {
                    if(pp1->tokencount<pp2->tokencount)
                        return 0;
                    pp1->tokencount = pp1->tokencount-pp2->tokencount;
                    if(pp1->tokencount == 0)
                    {
                        pp1p->next = pp1->next;
                        mm1.colorcount--;
                        delete pp1;

                        pp1=pp1p->next;
                        pp2=pp2->next;
                        break;
                    }
                    else
                    {
                        pp1p=pp1;
                        pp1=pp1->next;
                        pp2=pp2->next;
                        break;
                    }
                }
                else if(array_greater(cid1,cid2,sortnum))
                {
                    delete [] cid1;
                    delete [] cid2;
                    return 0;
//                    cerr<<"[MINUS]ERROR:The minued is not greater than the subtractor."<<endl;
//                    exit(-1);
                }
            }
            if(pp1==NULL && pp2!=NULL)
            {
                delete [] cid1;
                delete [] cid2;
                return 0;
            }
        }
        delete [] cid1;
        delete [] cid2;
        return 1;
    }
}

void PLUS(MarkingMeta &mm1,const MarkingMeta &mm2)
{
    Tokens *p = mm2.tokenQ->next;
    Tokens *resp;
    if(mm1.tid == productsort)
    {
        int sortnum = sorttable.productsort[mm1.sid].sortnum;
        while(p)
        {
            resp = new Tokens;
            Tokenscopy(*resp,*p,mm1.tid,sortnum);
            mm1.insert(resp);
            p=p->next;
        }
    } else{
        while(p)
        {
            resp = new Tokens;
            Tokenscopy(*resp,*p,mm1.tid);
            mm1.insert(resp);
            p=p->next;
        }
    }
}

void computeArcEXP(const arc_expression &arcexp,MarkingMeta &mm,const COLORID *varcolors,int psnum) {
    colorflag = true;
    computeArcEXP(arcexp.root->leftnode,mm,varcolors,psnum);
}

/*function:将未绑定过的抽象语法树转化为MarkingMeta
 * */
void computeArcEXP(meta *expnode,MarkingMeta &mm,const COLORID *varcolors,int psnum) {
    if(colorflag == false)
        return;
    if(expnode->myname == "add")
    {
        computeArcEXP(expnode->leftnode,mm,varcolors,psnum);
        if(expnode->rightnode!=NULL)
            computeArcEXP(expnode->rightnode,mm,varcolors,psnum);
    }
    else if(expnode->myname == "subtract")
    {
        MarkingMeta mm1,mm2;
        mm1.initiate(mm.tid,mm.sid);
        mm2.initiate(mm.tid,mm.sid);
        computeArcEXP(expnode->leftnode,mm1,varcolors,psnum);
        computeArcEXP(expnode->rightnode,mm2,varcolors,psnum);
        if(MINUS(mm1,mm2)==0)
        {
            colorflag = false;
            return;
        }
        else
        {
            MarkingMetacopy(mm,mm1,mm.tid,mm.sid);
        }
    }
    else if(expnode->myname == "numberof")
    {
        /*numberof节点等同于多重集合中的一个元，如1'a+2'b中的1'a
         * 处理分两步：
         * 1.取出左边节点的数,如1'a中的1;
         * 2.取出有边节点的color，如1'a中的a；
         * */

        //doing the first job;
        SHORTNUM num = expnode->leftnode->number;
        //doing the second job;
        meta *color = expnode->rightnode;
        if(color->myname == "tuple")
        {
            //tuple取出来的是一个数组cid
            COLORID *cid = new COLORID[psnum];
            getTupleColor(color,cid,varcolors,0);
            //创建一个colortoken，插入到mm中
            Tokens *t = new Tokens;
            t->initiate(num,productsort,psnum);
            t->tokens->setColor(cid,psnum);
            mm.insert(t);
            delete [] cid;
        }
        else if(color->myname == "all")
        {
            meta *sortname = color->leftnode;
            map<string,MSI>::iterator siter;
            siter = sorttable.mapSort.find(sortname->myname);
            SHORTNUM feconstnum = sorttable.usersort[siter->second.sid].feconstnum;
            for(COLORID i=0;i<feconstnum;++i)
            {
                Tokens *p = new Tokens;
                p->initiate(num,usersort);
                p->tokens->setColor(i);
                mm.insert(p);
            }
        }
        else if(color->myname == "successor")
        {
            SHORTNUM feconstnum;
            if(color->leftnode->mytype == var)
            {
                map<string,VARID>::iterator viter;
                viter = cpnet->mapVariable.find(color->leftnode->myname);
                SORTID sid = cpnet->vartable[viter->second].sid;
                feconstnum = sorttable.usersort[sid].feconstnum;

                COLORID cid = (varcolors[viter->second]+1)%feconstnum;
                Tokens *p = new Tokens;
                p->initiate(num,usersort);
                p->tokens->setColor(cid);
                mm.insert(p);
            }
            else
            {
                cerr<<"ERROR!CPN_RG::computeArcEXP"<<endl;
            }
        }
        else if(color->myname == "predecessor")
        {
            SHORTNUM feconstnum;
            if(color->leftnode->mytype == var)
            {
                map<string,VARID>::iterator viter;
                viter = cpnet->mapVariable.find(color->leftnode->myname);
                SORTID sid = cpnet->vartable[viter->second].sid;
                feconstnum = sorttable.usersort[sid].feconstnum;

                COLORID cid;
                if(varcolors[viter->second] == 0)
                    cid = feconstnum-1;
                else
                    cid = varcolors[viter->second]-1;
                Tokens *p = new Tokens;
                p->initiate(num,usersort);
                p->tokens->setColor(cid);
                mm.insert(p);
            }
            else
            {
                cerr<<"ERROR!CPN_RG::computeArcEXP"<<endl;
            }
        }
        else if(color->mytype == delsort)
        {
            Tokens *p = new Tokens;
            if(color->myname == "dotconstant")
            {
                p->initiate(num,dot);
                mm.insert(p);
                return;
            }

            COLORID cid;
            map<string,MCI>::iterator citer;
            citer = sorttable.mapColor.find(color->myname);
            cid = citer->second.cid;

            p->initiate(num,usersort);
            p->tokens->setColor(cid);
            mm.insert(p);
        }
        else if(color->mytype == var)
        {
            COLORID cid;
            map<string,VARID>::iterator viter;
            viter = cpnet->mapVariable.find(color->myname);
            cid = varcolors[viter->second];
            Tokens *p = new Tokens;
            p->initiate(num,usersort);
            p->tokens->setColor(cid);
            mm.insert(p);
        }
        else
            cerr<<"ERROR!CPN_RG::computeArcEXP @ line 1186"<<endl;
    }
    else{
        cerr<<"[CPN_RG::computeArcEXP] ERROR:Unexpected arc_expression node"<<expnode->myname<<endl;
        exit(-1);
    }
}

/*function：得到一个未绑定过的语法树中ProductSort类型的颜色（元祖）
 * */
void getTupleColor(meta *expnode,COLORID *cid,const COLORID *varcolors,int ptr) {
    if(expnode->mytype == var)
    {
        //1.首先检查该变量是否已经绑定
        //2.取出color，放在数组cid[ptr]；
        map<string,VARID>::iterator viter;
        viter = cpnet->mapVariable.find(expnode->myname);
        cid[ptr] = varcolors[viter->second];
    }
    else if(expnode->mytype == delsort)
    {
        //根据颜色的名字索引颜色的索引值
        map<string,MCI>::iterator citer;
        citer = sorttable.mapColor.find(expnode->myname);
        cid[ptr] = citer->second.cid;
    }
    else if(expnode->myname == "tuple")
    {
        getTupleColor(expnode->leftnode,cid,varcolors,ptr);
        getTupleColor(expnode->rightnode,cid,varcolors,ptr+1);
    }
    else if(expnode->myname == "successor")
    {
        SHORTNUM feconstnum;
        if(expnode->leftnode->mytype == var)
        {
            map<string,VARID>::iterator viter;
            viter = cpnet->mapVariable.find(expnode->leftnode->myname);  //根据变量找到变量的索引值
            SORTID sid = cpnet->vartable[viter->second].sid;
            feconstnum = sorttable.usersort[sid].feconstnum;

            cid[ptr] = (varcolors[viter->second]+1)%feconstnum;
        }
        else if(expnode->leftnode->mytype == delsort)
        {
            map<string,MCI>::iterator citer;
            citer = sorttable.mapColor.find(expnode->myname);
            feconstnum = sorttable.usersort[citer->second.sid].feconstnum;

            cid[ptr] = (citer->second.cid+1)%feconstnum;
        }
    }
    else if(expnode->myname == "predecessor")
    {
        SHORTNUM feconstnum;
        if(expnode->leftnode->mytype == var)
        {
            map<string,VARID>::iterator viter;
            viter = cpnet->mapVariable.find(expnode->leftnode->myname);
            SORTID sid = cpnet->vartable[viter->second].sid;
            feconstnum = sorttable.usersort[sid].feconstnum;

            COLORID ccc = varcolors[viter->second];

            if(ccc == MAXSHORT)
            {
                cerr<<"[CPN_RG::getTupleColor]ERROR：variable "<<expnode->leftnode->myname<<" is not bounded."<<endl;
                exit(-1);
            }

            if(ccc == 0)
                cid[ptr] = feconstnum-1;
            else
                cid[ptr] = ccc-1;
        }
        else if(expnode->leftnode->mytype == delsort)
        {
            map<string,MCI>::iterator citer;
            citer = sorttable.mapColor.find(expnode->myname);  //根据该变量名字找到该变量
            feconstnum = sorttable.usersort[citer->second.sid].feconstnum;

            if(citer->second.cid == 0)
                cid[ptr] = feconstnum-1;
            else
                cid[ptr] = citer->second.cid-1;
        }
    }
}

/*function：判断一个变迁的Guard函数（抽象语法树）是否为真
 * */
void judgeGuard(CTN *node,const COLORID *cid) {
    switch(node->mytype)
    {
        case Boolean:{
            if(node->myname == "and")
            {
                judgeGuard(node->left,cid);
                judgeGuard(node->right,cid);
                if(!node->left->mytruth)
                    node->mytruth = false;
                else
                    node->mytruth = node->right->mytruth;
            }
            else if(node->myname == "or")
            {
                judgeGuard(node->left,cid);
                judgeGuard(node->right,cid);
                if(node->left->mytruth)
                    node->mytruth = true;
                else
                    node->mytruth = node->right->mytruth;
            }
            else if(node->myname == "imply")
            {
                judgeGuard(node->left,cid);
                judgeGuard(node->right,cid);
                if(!node->left->mytruth)
                    node->mytruth = true;
                else if(node->left->mytruth && node->right->mytruth)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "not")
            {
                judgeGuard(node->left,cid);
                if(node->left->mytruth)
                    node->mytruth = false;
                else
                    node->mytruth = true;
            }
            break;
        }
        case Relation:{
            if(node->left->mytype!=variable && node->left->mytype!=useroperator)
                cerr<<"[condition_tree::judgeGuard]ERROR:Relation node's leftnode is not a variable or color."<<endl;
            if(node->right->mytype!=variable && node->right->mytype!=useroperator)
                cerr<<"[condition_tree::judgeGuard]ERROR:Relation node's rightnode is not a variable or color."<<endl;
            judgeGuard(node->left,cid);
            judgeGuard(node->right,cid);
            if(node->myname == "equality")
            {
                if(node->left->cid == node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "inequality")
            {
                if(node->left->cid != node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "lessthan")
            {
                if(node->left->cid < node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "lessthanorequal")
            {
                if(node->left->cid <= node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "greaterthan")
            {
                if(node->left->cid > node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "greaterthanorequal")
            {
                if(node->left->cid >= node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            break;
        }
        case variable:{
            map<string,VARID>::iterator viter;
            viter=cpnet->mapVariable.find(node->myname);
            if(cid[viter->second] == MAXSHORT)
            {
                cerr<<"[condition_tree::judgeGuard]ERROR:Variable "<<node->myname<<" is not bounded."<<endl;
                exit(-1);
            }
            node->cid = cid[viter->second];
            break;
        }
        case useroperator: {
            map<string,mapcolor_info>::iterator citer;
            citer = sorttable.mapColor.find(node->myname);
            node->cid = citer->second.cid;
            break;
        }
        default:{
            cerr<<"[judgeGuard] Unrecognized node in condition tree"<<endl;
            exit(-1);
        }
    }
}
/***********************************************************************/
index_t MarkingMeta::Hash() {
    index_t hv = 0;
    if(tid == usersort)
    {
        hv = colorcount*H1FACTOR*H1FACTOR*H1FACTOR;
        Tokens *p = tokenQ->next;
        COLORID i=3;
        for(i,p;p!=NULL;p=p->next,i=i*3)
        {
            COLORID cid;
            p->tokens->getColor(cid);
            hv += p->tokencount*H1FACTOR*H1FACTOR+(cid+1)*i;
        }
    }
    else if(tid == productsort)
    {
        hv = colorcount*H1FACTOR*H1FACTOR*H1FACTOR;
        Tokens *p;
        int sortnum = sorttable.productsort[sid].sortnum;
        COLORID *cid = new COLORID[sortnum];
        COLORID i=3;
        for(i,p=tokenQ->next;p!=NULL;p=p->next,i=i*3)
        {
            p->tokens->getColor(cid,sortnum);
            hv += p->tokencount*H1FACTOR*H1FACTOR;
            for(int j=0;j<sortnum;++j)
            {
                hv += (cid[j]+1)*i;
            }
        }
        delete [] cid;
    }
    else if(tid == dot)
    {
        hv = colorcount*H1FACTOR*H1FACTOR*H1FACTOR;
        Tokens *p = tokenQ->next;
        if(p!=NULL)
            hv += p->tokencount*H1FACTOR*H1FACTOR;
    }
    else if(tid == finiteintrange)
    {
        cerr<<"Sorry, we don't support finteintrange now."<<endl;
        exit(-1);
    }
    hashvalue = hv;
    return hv;
}
/*1.找到要插入的位置，分为三种：
 * i.末尾
 * ii.中间
 * iii.开头
 * 2.插入到该插入的位置，并设置colorcount++；
 * (3).如果是相同的color，仅更改tokencount；
 * */
void MarkingMeta::insert(Tokens *t) {
    if(tid == usersort)
    {
        Tokens *q=tokenQ,*p = tokenQ->next;
        COLORID pcid,tcid;
        t->tokens->getColor(tcid);
        while(p!=NULL)
        {
            p->tokens->getColor(pcid);
            if(tcid<=pcid)
                break;
            q=p;
            p=p->next;
        }
        if(p == NULL)
        {
            q->next = t;
            colorcount++;
            return;
        }
        if(tcid == pcid)
        {
            p->tokencount+=t->tokencount;
            delete t;
        }
        else if(tcid<pcid)
        {
            t->next = p;
            q->next = t;
            colorcount++;
        }
    }
    else if(tid == productsort)
    {
        Tokens *q=tokenQ,*p = tokenQ->next;
        COLORID *pcid,*tcid;
        int sortnum = sorttable.productsort[sid].sortnum;
        pcid = new COLORID[sortnum];
        tcid = new COLORID[sortnum];
        t->tokens->getColor(tcid,sortnum);
        while(p!=NULL)
        {
            p->tokens->getColor(pcid,sortnum);
            if(array_lessorequ(tcid,pcid,sortnum))
                break;
            q=p;
            p=p->next;
        }
        if(p == NULL)
        {
            q->next = t;
            colorcount++;
            delete [] pcid;
            delete [] tcid;
            return;
        }
        if(array_equal(tcid,pcid,sortnum))
        {
            p->tokencount += t->tokencount;
            delete t;
        }
        else if(array_less(tcid,pcid,sortnum))
        {
            t->next = p;
            q->next = t;
            colorcount++;
        }
        delete [] pcid;
        delete [] tcid;
    }
    else if(tid == dot)
    {
        if(tokenQ->next == NULL)
            tokenQ->next=t;
        else {
            tokenQ->next->tokencount+=t->tokencount;
            delete t;
        }
        colorcount = 1;
    }
}

bool MarkingMeta::operator>=(const MarkingMeta &mm) {
    bool greaterorequ = true;
    Tokens *p1,*p2;
    p1 = this->tokenQ->next;
    p2 = mm.tokenQ->next;

    if(mm.colorcount==0)
        return true;
    else if(this->colorcount == 0)
        return false;


    if(this->tid == dot)
    {
        if(p1->tokencount>=p2->tokencount)
            greaterorequ = true;
        else
            greaterorequ = false;
    }
    else if(this->tid == usersort)
    {
        COLORID cid1,cid2;
        while(p2)
        {
            //p2中的元素，p1中必须都有；遍历p2
            p2->tokens->getColor(cid2);
            while(p1)
            {
                p1->tokens->getColor(cid1);
                if(cid1<cid2)
                {
                    p1=p1->next;
                    continue;
                }
                else if(cid1==cid2)
                {
                    if(p1->tokencount>=p2->tokencount)
                    {
                        break;
                    }
                    else
                    {
                        greaterorequ = false;
                        break;
                    }
                }
                else if(cid1>cid2)
                {
                    greaterorequ = false;
                    break;
                }
            }
            if(p1 == NULL)
                greaterorequ = false;
            if(!greaterorequ)
                break;
            else{
                p2=p2->next;
            }
        }
    }
    else if(this->tid == productsort)
    {
        int sortnum = sorttable.productsort[sid].sortnum;
        COLORID *cid1,*cid2;
        cid1 = new COLORID[sortnum];
        cid2 = new COLORID[sortnum];
        while(p2)
        {
            p2->tokens->getColor(cid2,sortnum);
            while(p1)
            {
                p1->tokens->getColor(cid1,sortnum);
                if(array_less(cid1,cid2,sortnum)){
                    p1=p1->next;
                    continue;
                }
                else if(array_equal(cid1,cid2,sortnum))
                {
                    if(p1->tokencount>=p2->tokencount)
                    {
                        break;
                    }
                    else
                    {
                        greaterorequ = false;
                        break;
                    }
                }
                else if(array_greater(cid1,cid2,sortnum))
                {
                    greaterorequ = false;
                    break;
                }
            }
            if(p1 == NULL)
                greaterorequ = false;
            if(!greaterorequ)
                break;
            else {
                p2=p2->next;
            }
        }
        delete [] cid1;
        delete [] cid2;
    }
    return greaterorequ;
}

void MarkingMeta::printToken() {
    Tokens *p = tokenQ->next;
    if(p==NULL)
    {
        cout<<"NULL"<<endl;
        return;
    }
    while(p)
    {
        cout<<p->tokencount<<"\'";
        if(tid == usersort)
        {
            COLORID cid;
            p->tokens->getColor(cid);
            cout<<sorttable.usersort[sid].cyclicenumeration[cid];
        }
        else if(tid == productsort)
        {
            int sortnum = sorttable.productsort[sid].sortnum;
            COLORID *cid = new COLORID[sortnum];
            p->tokens->getColor(cid,sortnum);
            cout<<"[";
            for(int i=0;i<sortnum;++i)
            {
                SORTID ssid = sorttable.productsort[sid].sortid[i].sid;
                cout<<sorttable.usersort[ssid].cyclicenumeration[cid[i]];
            }
            cout<<"]";
            delete [] cid;
        }
        else if(tid == dot)
        {
            cout<<"dot";
        }
        p=p->next;
        cout<<'+';
    }
    cout<<endl;
}

void MarkingMeta::printToken(string &str) {
    Tokens *p = tokenQ->next;
    if(p==NULL)
    {
        str += "NULL";
        return;
    }
    while(p)
    {
        str += to_string(p->tokencount) + "\'";
        if(tid == usersort)
        {
            COLORID cid;
            p->tokens->getColor(cid);
            str += sorttable.usersort[sid].cyclicenumeration[cid];
        }
        else if(tid == productsort)
        {
            int sortnum = sorttable.productsort[sid].sortnum;
            COLORID *cid = new COLORID[sortnum];
            p->tokens->getColor(cid,sortnum);
            str += "[";
            for(int i=0;i<sortnum;++i)
            {
                SORTID ssid = sorttable.productsort[sid].sortid[i].sid;
                str += sorttable.usersort[ssid].cyclicenumeration[cid[i]];
            }
            str += "]";
            delete [] cid;
        }
        else if(tid == dot)
        {
            str += "dot";
        }
        p=p->next;
        str += '+';
    }
}

NUM_t MarkingMeta::Tokensum() {
    NUM_t sum = 0;
    Tokens *p = tokenQ->next;
    while(p!=NULL)
    {
        sum+=p->tokencount;
        p=p->next;
    }
    return sum;
}