#ifndef LZSS_TREE_H
#define LZSS_TREE_H

struct LzssTreeNode {
    unsigned short parent;
    unsigned short smaller;
    unsigned short larger;
};

#endif
