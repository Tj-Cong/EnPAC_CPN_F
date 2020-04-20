//
// Created by hecong on 19-11-28.
//

#include "condition_tree.h"

condition_tree::condition_tree() {
    root = new CTN;
    root->mytype = nodetype::Root;
}

condition_tree::~condition_tree() {
    destructor(this->root);
}

void condition_tree::build_step(TiXmlElement *elem, CTN *&curnode) {
    string value = elem->Value();
    if(value == "subterm")
    {
        elem = elem->FirstChildElement();
        value = elem->Value();
    }

    if(value == "and" ||
       value == "or" ||
       value == "not" ||
       value == "imply")
    {
        curnode = new CTN;
        curnode->myname = value;
        curnode->mytype = nodetype::Boolean;
        TiXmlElement *leftchild,*rightchild;
        leftchild = elem->FirstChildElement();
        rightchild = leftchild->NextSiblingElement();
        build_step(leftchild,curnode->left);
        build_step(rightchild,curnode->right);
    }
    else if(value == "equality" ||
            value == "inequality" ||
            value == "lessthan" ||
            value == "lessthanorequal" ||
            value == "greaterthan" ||
            value == "greaterthanorequal")
    {
        curnode = new CTN;
        curnode->myname = value;
        curnode->mytype = nodetype::Relation;
        TiXmlElement *leftchild,*rightchild;
        leftchild = elem->FirstChildElement();
        rightchild = leftchild->NextSiblingElement();
        build_step(leftchild,curnode->left);
        build_step(rightchild,curnode->right);
    }
    else if(value == "variable")
    {
        TiXmlAttribute *refvariable = elem->FirstAttribute();
        curnode = new CTN;
        curnode->myname = refvariable->Value();
        curnode->mytype = nodetype::variable;
    }
    else if(value == "useroperator")
    {
        TiXmlAttribute *declaration = elem->FirstAttribute();
        curnode = new CTN;
        curnode->myname = declaration->Value();
        curnode->mytype = nodetype::useroperator;
    }
    else
    {
        cerr<<"Can not recognize operator \'"<<value<<"\' in Transition's guard function!"<<endl;
        exit(0);
    }
}

void condition_tree::constructor(TiXmlElement *condition) {
    string value = condition->Value();
    if(value == "condition")
    {
        TiXmlElement *structure = condition->FirstChildElement("structure");
        TiXmlElement *firstoperator = structure->FirstChildElement();
        build_step(firstoperator,root->left);
    }
    else
        cerr<<"Guard function's entrance is XmlElement \'condition\'"<<endl;
}

void condition_tree::destructor(CTN *node) {
    if(node->left!=NULL)
        destructor(node->left);
    if(node->right!=NULL)
        destructor(node->right);
    delete node;
}

void condition_tree::computeEXP(CTN *node) {
    switch(node->mytype)
    {
        case Boolean:{
            if(node->myname == "and")
            {
                computeEXP(node->left);
                computeEXP(node->right);
                node->myexp = "("+node->left->myexp+")&&("+node->right->myexp+")";
            }
            else if(node->myname == "or")
            {
                computeEXP(node->left);
                computeEXP(node->right);
                node->myexp = "("+node->left->myexp+")||("+node->right->myexp+")";
            }
            else if(node->myname == "imply")
            {
                computeEXP(node->left);
                computeEXP(node->right);
                node->myexp = "("+node->left->myexp+")->("+node->right->myexp+")";
            }
            else if(node->myname == "not")
            {
                computeEXP(node->left);
                node->myexp = "!("+node->left->myexp+")";
            }
            break;
        }
        case Relation:{
            computeEXP(node->left);
            computeEXP(node->right);
            if(node->myname == "equality")
            {
                node->myexp = "("+node->left->myexp+")==("+node->right->myexp+")";
            }
            else if(node->myname == "inequality")
            {
                node->myexp = "("+node->left->myexp+")!=("+node->right->myexp+")";
            }
            else if(node->myname == "lessthan")
            {
                node->myexp = "("+node->left->myexp+")<("+node->right->myexp+")";
            }
            else if(node->myname == "lessthanorequal")
            {
                node->myexp = "("+node->left->myexp+")<=("+node->right->myexp+")";
            }
            else if(node->myname == "greaterthan")
            {
                node->myexp = "("+node->left->myexp+")>("+node->right->myexp+")";
            }
            else if(node->myname == "greaterthanorequal")
            {
                node->myexp = "("+node->left->myexp+")>=("+node->right->myexp+")";
            }
            break;
        }
        case variable:{
            node->myexp = node->myname;
            break;
        }
        case useroperator: {
            node->myexp = node->myname;
            break;
        }
    }
}

void condition_tree::printEXP(string &str) {
    computeEXP(root->left);
    str = root->left->myexp;
}


/*********************************************************************************************************/
arc_expression::arc_expression() {
    root = new meta;
}

arc_expression::~arc_expression() {

}

void arc_expression::constructor(TiXmlElement *hlinscription) {
    string value = hlinscription->Value();
    if(value == "hlinscription" || value == "hlinitialMarking") {
        TiXmlElement *structure = hlinscription->FirstChildElement("structure");
        TiXmlElement *firstoperator = structure->FirstChildElement();
        build_step(firstoperator,root->leftnode);
    }
    else
        cerr<<"Arc expression's entrance is XmlElement \'hlinscription\' or \'hlinitialMarking\'"<<endl;
}

void arc_expression::build_step(TiXmlElement *elem, meta *&curnode) {
    string value = elem->Value();
    if(value == "subterm") {
        elem = elem->FirstChildElement();
        value = elem->Value();
    }

    if(value == "numberof") {
        curnode = new meta;
        curnode->myname = "numberof";
        curnode->mytype = arcnodetype::structure;

        //get numberconstant's value
        TiXmlElement *leftchild,*rightchild;
        leftchild = elem->FirstChildElement();
        rightchild = leftchild->NextSiblingElement();
        TiXmlElement *numconst = leftchild->FirstChildElement("numberconstant");
        string num = numconst->FirstAttribute()->Value();

        //create number node
        meta *numnode = new meta;
        numnode->mytype = arcnodetype::structure;
        numnode->myname = "number";
        numnode->number = stringToNum(num);
        curnode->leftnode = numnode;

        //create rightchild node
        build_step(rightchild,curnode->rightnode);
    }
    else if(value == "tuple") {
        curnode = new meta;
        curnode->myname = "tuple";
        curnode->mytype = arcnodetype ::structure;

        TiXmlElement *term = elem->FirstChildElement();
        build_step(term,curnode->leftnode);
        term = term->NextSiblingElement();
        build_step(term,curnode->rightnode);

        meta *prese,*prede;
        prede = curnode;
        term = term->NextSiblingElement();
        while(term)
        {
            build_step(term,prese);
            meta *tt = new meta;
            tt->myname = "tuple";
            tt->mytype = arcnodetype ::structure;
            tt->leftnode = prede->rightnode;
            tt->rightnode = prese;
            prede->rightnode = tt;
            prede = tt;
            term = term->NextSiblingElement();
        }
    }
    else if(value == "add") {
        curnode = new meta;
        curnode->myname = "add";
        curnode->mytype = operat;

        TiXmlElement *term = elem->FirstChildElement();
        build_step(term,curnode->leftnode);
        term = term->NextSiblingElement();
        if(term!=NULL)
        {
            build_step(term,curnode->rightnode);
        }
        else return;
        term = term->NextSiblingElement();

        meta *prese,*prede;
        prede = curnode;
        while(term)
        {
            build_step(term,prese);
            meta *aa = new meta;
            aa->myname = "add";
            aa->mytype = arcnodetype ::operat;
            aa->leftnode = prede->rightnode;
            aa->rightnode = prese;
            prede->rightnode = aa;
            prede = aa;
            term=term->NextSiblingElement();
        }
    }
    else if(value == "subtract") {
        curnode = new meta;
        curnode->myname = "subtract";
        curnode->mytype = operat;
        TiXmlElement *term=elem->FirstChildElement();
        build_step(term,curnode->leftnode);
        term = term->NextSiblingElement();
        build_step(term,curnode->rightnode);

        term = term->NextSiblingElement();
        meta *prese,*prede;
        prede = curnode;
        while(term)
        {
            build_step(term,prese);
            meta *aa = new meta;
            aa->myname = "add";
            aa->mytype = operat;
            aa->leftnode = prede->rightnode;
            aa->rightnode = prese;
            prede->rightnode = aa;
            prede = aa;
            term = term->NextSiblingElement();
        }
    }
    else if(value == "successor") {
        curnode = new meta;
        curnode->mytype = operat;
        curnode->myname = "successor";
        TiXmlElement *leftchild = elem->FirstChildElement();
        build_step(leftchild,curnode->leftnode);
    }
    else if(value == "predecessor") {
        curnode = new meta;
        curnode->mytype = operat;
        curnode->myname = "predecessor";
        TiXmlElement *leftchild = elem->FirstChildElement();
        build_step(leftchild,curnode->leftnode);
    }
    else if(value == "all") {
        curnode = new meta;
        curnode->mytype = operat;
        curnode->myname = "all";
        TiXmlElement *leftchild = elem->FirstChildElement();
        build_step(leftchild,curnode->leftnode);
    }
    else if(value == "variable") {
        curnode = new meta;
        curnode->mytype = var;
        TiXmlAttribute *refvariable = elem->FirstAttribute();
        curnode->myname = refvariable->Value();
    }
    else if(value == "useroperator") {
        curnode = new meta;
        curnode->mytype = delsort;
        TiXmlAttribute *declaration = elem->FirstAttribute();
        curnode->myname = declaration->Value();
    }
    else if(value == "usersort") {
        curnode = new meta;
        curnode->mytype = sortclass;
        curnode->myname = elem->FirstAttribute()->Value();
    }
    else if(value == "dotconstant") {
        curnode = new meta;
        curnode->mytype = delsort;
        curnode->myname = "dotconstant";
    }
    else {
        cerr<<"Can not recognize operator \'"<<value<<"\' in Arc's expression!"<<endl;
        exit(0);
    }
}

void arc_expression::destructor(meta *&node) {
    if(node!=NULL)
    {
        if(node->leftnode != NULL)
            destructor(node->leftnode);
        if(node->rightnode != NULL)
            destructor(node->rightnode);
        delete node;
        node = NULL;
    }
}

void arc_expression::computeEXP(meta *node) {
    if(node->mytype == structure)
    {
        if(node->myname == "numberof")
        {
            computeEXP(node->leftnode);
            computeEXP(node->rightnode);
            node->myexp = node->leftnode->myexp+"'["+node->rightnode->myexp+"]";
        }
        else if(node->myname == "number")
        {
            node->myexp = to_string(node->number);
        }
        else if(node->myname == "tuple")
        {
            computeEXP(node->leftnode);
            computeEXP(node->rightnode);
            node->myexp = node->leftnode->myexp+","+node->rightnode->myexp;
        }
    }
    else if(node->mytype == operat)
    {
        if(node->myname == "all")
        {
            computeEXP(node->leftnode);
            node->myexp = "["+node->leftnode->myexp+"].all";
        }
        else if(node->myname == "successor")
        {
            computeEXP(node->leftnode);
            node->myexp = node->leftnode->myexp+"++";
        }
        else if(node->myname =="predecessor")
        {
            computeEXP(node->leftnode);
            node->myexp = node->leftnode->myexp+"--";
        }
        else if(node->myname == "add")
        {
            computeEXP(node->leftnode);
            if(node->rightnode!=NULL)
            {
                computeEXP(node->rightnode);
                node->myexp = node->leftnode->myexp+"+"+node->rightnode->myexp;
            }
            else
                node->myexp = node->leftnode->myexp;
        }
        else if(node->myname == "subtract")
        {
            computeEXP(node->leftnode);
            computeEXP(node->rightnode);
            node->myexp = node->leftnode->myexp+"-("+node->rightnode->myexp+")";
        }
    }
    else if(node->mytype == delsort)
    {
        node->myexp = node->myname;
    }
    else if(node->mytype == sortclass)
    {
        node->myexp = node->myname;
    }
    else if(node->mytype == var)
    {
        node->myexp = node->myname;
    }
}

void arc_expression::printEXP(string &str) {
    computeEXP(root->leftnode);
    str = root->leftnode->myexp;
}