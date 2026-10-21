#pragma once

// A binary search tree in one block of nodes (PS2 XSBTree<T>, which only the occluders' Build instantiates, with
// TRISIDE2, to find the faces that share each edge). T's first member is the key, unsigned; the nodes with equal keys
// make a ring. The tree doesn't grow: each node added is the next one of the block, so the block holds the nodes in
// the order they came. The member names are guesses.
template<class T>
class XSBTree {
public:
    struct _NODE {
        T data;
        _NODE *left, *right;            // the smaller and the larger keys
        _NODE *next;                    // the ring of equal keys
    };

    _NODE *root;                        // the block, the root first
    _NODE *free;                        // the next node of the block
    int size;                           // of the block
    bool ok;                            // the block was allocated
    T tmp;                              // what Add_r adds
    uint unknown_18[4];
    int unknown_28;
    uint unknown_2c;

    XSBTree(ushort n)
    {
        ok = true;
        root = new _NODE[n];
        if (!root)
            ok = false;
        size = n;
        free = root;
        unknown_28 = 0;
    }

    ~XSBTree()
    {
        if (root)
            delete[] root;
    }

    // Adds t; returns the node whose ring it joined, or NULL
    _NODE *Add(const T &t)
    {
        if (free == root) {
            root->data = t;
            root->left = NULL;
            root->right = NULL;
            root->next = free;
            free++;
            return NULL;
        }
        tmp = t;
        return Add_r(root);
    }

    _NODE *Add_r(_NODE *n);

    // The nodes added
    int NumNodes() const
    {
        return free - root;
    }
};

// The occluder file's out-of-line copies, after its functions: the destructor for Build's unwinding, and Add_r
// TEMPLATE: LS3DF 0x1003cae0
// ??1?$XSBTree@UTRISIDE2@@@@QAE@XZ

// TEMPLATE: LS3DF 0x1003caf0
// ?Add_r@?$XSBTree@UTRISIDE2@@@@QAEPAU_NODE@1@PAU21@@Z

// Adds tmp below n: a loop, where PS2's recursion is, with the new node's setup in each branch (VC6 merges them, and
// keeps the zero in a register for both)
template<class T>
typename XSBTree<T>::_NODE *XSBTree<T>::Add_r(_NODE *n)
{
    while (true) {
        if (tmp.key == n->data.key) {
            _NODE *p = n;
            while (p->next != n)
                p = p->next;
            p->next = free;
            free->data = tmp;
            free->left = root;
            free->right = NULL;
            free->next = n;
            free++;
            return n;
        }
        if (tmp.key < n->data.key) {
            if (!n->left) {
                n->left = free;
                free->data = tmp;
                free->left = NULL;
                free->right = NULL;
                free->next = free;
                free++;
                return NULL;
            }
            n = n->left;
        } else {
            if (!n->right) {
                n->right = free;
                free->data = tmp;
                free->left = NULL;
                free->right = NULL;
                free->next = free;
                free++;
                return NULL;
            }
            n = n->right;
        }
    }
}
