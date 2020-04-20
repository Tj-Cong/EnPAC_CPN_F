//
// Created by hecong on 19-11-28.
//

#include <iostream>
#include "BA/tinyxml.h"
#include <map>

using namespace std;
class SortTable;
class CPN;
struct mapcolor_info;
extern SortTable sorttable;
extern CPN *cpnet;

extern int stringToNum(const string& str);
enum nodetype{Root,Boolean,Relation,variable,useroperator};
typedef unsigned short COLORID;
typedef unsigned short SORTID;
typedef unsigned short VARID;
/***********************************************************************/
/*=====Transition's guard function is an AST(Abstract Syntax Tree)=====*/
/*Here is an example                                                   *
 *                               and                                   *
 *                              /    \                                 *
 *                    greaterthan     equallity                        *
 *                    /     \           /    \                         *
 *                  var  color/var var/color var /color                */
/***********************************************************************/
typedef struct condition_tree_node
{
    //if this node is a operator, myname is the operator's name,
    //like and operator, myname is 'and';
    //if this node is a variable or useroperator,
    //myname is this variable or color's name.
    string myname;
    string myexp;
    COLORID cid;
    nodetype mytype;
    bool mytruth = false;
    condition_tree_node *left = NULL;
    condition_tree_node *right = NULL;
} CTN;

class condition_tree
{
public:
    CTN *root;
public:
    condition_tree();
    void constructor(TiXmlElement *condition);
    void build_step(TiXmlElement *elem,CTN *&curnode);
    void destructor(CTN *node);
    void computeEXP(CTN *node);
    void printEXP(string &str);
    ~condition_tree();
};

/***********************************************************************/
/*===================Arc expression is also an AST=====================*/
/*this abstract tree has this node type:
 * 1.numberof: his left child is a number,his right child can be a tuple,
 * a variable or a useroperator;
 * ----------------------------------------------------------------------
 * 2.tuple: his left child can be a variable or a useroperator, his right
 * child can be a variable or a useroperator or a tuple too(if this tuple's
 * members are over 2),it would be like this(it is like Huffman code tree):
 *             tuple
 *           /       \
 *          var     tuple
 *                 /     \
 *                var    ...
 * ----------------------------------------------------------------------
 * 3.variable
 * ----------------------------------------------------------------------
 * 4.useroperator(useroperator is a color in one colorset)
 * ----------------------------------------------------------------------
 * 5.operator:we just support 'add' and CyclicEnumOperator-'successor' and
 * 'predecessor'
 * */
/***********************************************************************/
enum arcnodetype{structure,operat,delsort,var,sortclass};
typedef struct multiset_node
{
    /*number has multi-explanations:
     * 1.as operator and structure node(number,numberof,tuple,add,successor,predecessor),
     * it represents the operator name itself
     * 2.as to usroperator, it represents the color's name
     * 3.as to var node, it represents the variable's name*/
    string myname;
    string myexp;
    /*number has multi-explanations:
     * 1.as number node, it represents the physical number
     * 2.as to usroperator, represents the color index
     * 3.as to var node, it represents the sort index*/
    int number = -1;
    arcnodetype mytype;
    multiset_node *leftnode = NULL;
    multiset_node *rightnode = NULL;
} meta;

class arc_expression
{
public:
    meta *root;
public:
    arc_expression();
    ~arc_expression();
    void constructor(TiXmlElement *hlinscription);
    void build_step(TiXmlElement *elem,meta *&curnode);
    void destructor(meta *&node);
    void computeEXP(meta *node);
    void printEXP(string &str);
};

