#include <sys/time.h>
#include "CPN_Product.h"
#include "BA/SBA.h"
#include "BA/xml2ltl.h"
using namespace std;
#define VMRSS_LINE 17
#define TOTALTOOLTIME 3595
NUM_t placecount;
NUM_t transitioncount;
NUM_t varcount;
CPN *cpnet;
CPN_RG *cpnRG;
bool ready2exit = false;
//以MB为单位
short int total_mem;
short int total_swap;
pid_t mypid;

size_t  heap_malloc_total, heap_free_total,mmap_total, mmap_count;
void print_info() {
    struct mallinfo mi = mallinfo();
    printf("count by itself:\n");
    printf("\033[31m\theap_malloc_total=%lu heap_free_total=%lu heap_in_use=%lu\n\tmmap_total=%lu mmap_count=%lu\n",
           heap_malloc_total*1024, heap_free_total*1024, heap_malloc_total*1024-heap_free_total*1024,
           mmap_total*1024, mmap_count);
    printf("count by mallinfo:\n");
    printf("\theap_malloc_total=%lu heap_free_total=%lu heap_in_use=%lu\n\tmmap_total=%lu mmap_count=%lu\n\033[0m",
           mi.arena, mi.fordblks, mi.uordblks,
           mi.hblkhd, mi.hblks);
//    malloc_stats();
}
unsigned int get_proc_mem(unsigned int pid){

    char file_name[64]={0};
    FILE *fd;
    char line_buff[512]={0};
    sprintf(file_name,"/proc/%d/status",pid);

    fd =fopen(file_name,"r");
    if(nullptr == fd){
        return 0;
    }

    char name[64];
    int vmrss;
    for (int i=0; i<VMRSS_LINE-1;i++){
        fgets(line_buff,sizeof(line_buff),fd);
    }

    fgets(line_buff,sizeof(line_buff),fd);
    sscanf(line_buff,"%s %d",name,&vmrss);
    fclose(fd);

    return vmrss;
}
double get_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec / 1000000.0;
}

int main() {
    mypid = getpid();
    cout << "=================================================" << endl;
    cout << "=====This is our tool-enPAC for the MCC'2020=====" << endl;
    cout << "=================================================" << endl;

//    char Ffile[50] = "LTLFireability.xml";
//    char Cfile[50] = "LTLCardinality.xml";
//    convertc(Cfile);
//    convertf(Ffile);

    double starttime, endtime;

    /**************************NET*****************************/
    CPN *cpn = new CPN;
    char filename[] = "model.pnml";
    cpn->getSize(filename);
//    cpn->printSort();
    cpn->readPNML(filename);
    cpn->getRelVars();
//    cpn->printTransVar();
//    cpn->printVar();
//    cpn->printCPN();
    cpn->setGlobalVar();
    cpnet = cpn;

    CPN_RG *crg;
//    timeflag = true;
//    crg = new CPN_RG;
//    cpnRG = crg;
//    CPN_RGNode *rginit=crg->CPNRGinitialnode();
//    rginit->getFiringTrans();
//    CPN_Product *psinit=new CPN_Product;
//    psinit->RGname_ptr = rginit;
//    psinit->initiate();
//    CPN_Product_Automata *product = new CPN_Product_Automata(NULL);
//    product->Generate(psinit);
//    cout<<"Hash conflict times:"<<cpnRG->hash_conflict_times<<endl;
    string S, propertyid; //propertyid stores names of LTL formulae
    char *form = new char[200000];     //store LTL formulae
    ofstream outresult("boolresult.txt", ios::out);  //outresult export results to boolresult.txt

    ifstream read("LTLFireability.txt", ios::in);
    if (!read) {
        cout << "error!";
        getchar();
        exit(-1);
    }

    SHORTNUM ltlcount = 0;
    SHORTNUM total_left_time = TOTALTOOLTIME;
    //print_info();
    while (getline(read, propertyid, ':')) {

        SHORTNUM each_run_time = 0;
        SHORTNUM each_used_time = 0;
        if(ltlcount<6)
            each_run_time = 300;
        else
            each_run_time = total_left_time/(16-ltlcount);

        getline(read, S);
        int len = S.length();
        if (len >= 200000) {
            outresult << '?';
            cout << "FORMULA " + propertyid + " " + "CANNOT_COMPUTE" << endl;
            continue;
        }
        strcpy(form, S.c_str());
        //lexer
        Lexer *lex = new Lexer(form, S.length());
        //syntax analysis
        Syntax_Tree *ST;
        ST = new Syntax_Tree;
        formula_stack Ustack;
        ST->reverse_polish(*lex);
        ST->build_tree();

        ST->simplify_LTL(ST->root->left);

        ST->negconvert(ST->root->left, Ustack);
        ST->computeCurAP(ST->root->left);
        delete lex;

        crg = new CPN_RG;
        cpnRG = crg;

        TGBA *Tgba;
        Tgba = new TGBA;
        Tgba->CreatTGBA(Ustack, ST->root->left);
        delete ST;

        TBA *tba;
        tba = new TBA;
        tba->CreatTBA(*Tgba, Ustack);
        delete Tgba;
        string filename = propertyid + ".txt";

        SBA *sba;
        sba = new SBA;
        sba->CreatSBA(*tba);
        sba->Simplify();
        sba->Compress();
        sba->ChangeOrder();
        sba->PrintSBA();
        delete tba;

        ready2exit = false;
        starttime = get_time();
        CPN_Product_Automata *product = new CPN_Product_Automata(sba);
        each_used_time = product->ModelChecker(propertyid,each_run_time);
        endtime = get_time();
        cout<<" RUNTIME:"<<endtime-starttime<<" NODECOUNT:"<<crg->nodecount<<" DEPTH:"<<product->depth<<" MEM:"<<get_proc_mem(getpid())<<endl;
        int ret = product->getresult();
        outresult << (ret == -1 ? '?' : (ret == 0 ? 'F' : 'T'));
        delete product;
        delete sba;
        delete crg;
        total_left_time -= each_used_time;
        ltlcount++;
        //print_info();
    }

    delete cpn;
    return 0;
}