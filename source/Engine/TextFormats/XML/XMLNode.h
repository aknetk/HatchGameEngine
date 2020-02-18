#ifndef XMLNODE_H
#define XMLNODE_H

struct XMLNode {
    Token            name;
    HashMap<Token>   attributes;
    vector<XMLNode*> children;
    XMLNode*         parent;
    Stream*          base_stream;
};

#endif /* XMLNODE_H */
