/*
  after all b+ tree we should jump here.
  1 - Disk pages
  2 - Buffer pool
  3 - WAL
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ORDER 5
#define MAX_KEYS (ORDER - 1)
#define MIN_KEYS (MAX_KEYS / 2)

// ================= NODE =================
typedef struct Node {
    int keys[MAX_KEYS];
    int values[MAX_KEYS];
    struct Node* children[ORDER];
    struct Node* next;
    int num_keys;
    bool is_leaf;
} Node;

// ================= CREATE =================
Node* CreateNode(bool leaf) {
    Node* n = (Node*)malloc(sizeof(Node));
    if (!n) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    n->is_leaf = leaf;
    n->num_keys = 0;
    n->next = NULL;

    for (int i = 0; i < ORDER; i++)
        n->children[i] = NULL;

    return n;
}

// ================= FIND LEAF =================
Node* find_leaf(Node* root, int key) {
    while (root && !root->is_leaf) {
        int i = 0;
        while (i < root->num_keys && key >= root->keys[i]) i++;
        root = root->children[i];
    }
    return root;
}

// ================= SEARCH =================
bool search(Node* root, int key, int* value) {
    Node* leaf = find_leaf(root, key);
    if (!leaf) return false;

    for (int i = 0; i < leaf->num_keys; i++) {
        if (leaf->keys[i] == key) {
            *value = leaf->values[i];
            return true;
        }
    }
    return false;
}

// ================= INSERT LEAF =================
void insert_leaf(Node* leaf, int key, int value) {
    for (int i = 0; i < leaf->num_keys; i++) {
        if (leaf->keys[i] == key) {
            leaf->values[i] = value;
            return;
        }
    }

    int i = leaf->num_keys - 1;
    while (i >= 0 && key < leaf->keys[i]) {
        leaf->keys[i+1] = leaf->keys[i];
        leaf->values[i+1] = leaf->values[i];
        i--;
    }

    leaf->keys[i+1] = key;
    leaf->values[i+1] = value;
    leaf->num_keys++;
}

// ================= SPLIT LEAF =================
Node* split_leaf(Node* leaf, int* up_key) {
    Node* new_leaf = CreateNode(true);

    int split = leaf->num_keys / 2;

    new_leaf->num_keys = leaf->num_keys - split;
    leaf->num_keys = split;

    for (int i = 0; i < new_leaf->num_keys; i++) {
        new_leaf->keys[i] = leaf->keys[split + i];
        new_leaf->values[i] = leaf->values[split + i];
    }

    new_leaf->next = leaf->next;
    leaf->next = new_leaf;

    *up_key = new_leaf->keys[0];
    return new_leaf;
}

// ================= SPLIT INTERNAL =================
Node* split_internal(Node* node, int* up_key) {
    Node* new_node = CreateNode(false);

    int mid = node->num_keys / 2;
    *up_key = node->keys[mid];

    int j = 0;
    for (int i = mid + 1; i < node->num_keys; i++, j++) {
        new_node->keys[j] = node->keys[i];
        new_node->children[j] = node->children[i];
    }
    new_node->children[j] = node->children[node->num_keys];
    new_node->num_keys = j;

    node->num_keys = mid;

    return new_node;
}

// ================= INSERT REC =================
Node* insert_rec(Node* node, int key, int value, int* up_key) {
    if (node->is_leaf) {
        insert_leaf(node, key, value);
        if (node->num_keys <= MAX_KEYS) return NULL;
        return split_leaf(node, up_key);
    }

    int i = 0;
    while (i < node->num_keys && key >= node->keys[i]) i++;

    int new_key;
    Node* new_child = insert_rec(node->children[i], key, value, &new_key);
    if (!new_child) return NULL;

    for (int j = node->num_keys; j > i; j--) {
        node->keys[j] = node->keys[j-1];
        node->children[j+1] = node->children[j];
    }

    node->keys[i] = new_key;
    node->children[i+1] = new_child;
    node->num_keys++;

    if (node->num_keys <= MAX_KEYS) return NULL;
    return split_internal(node, up_key);
}

// ================= INSERT =================
Node* insert(Node* root, int key, int value) {
    if (!root) {
        root = CreateNode(true);
        root->keys[0] = key;
        root->values[0] = value;
        root->num_keys = 1;
        return root;
    }

    int up_key;
    Node* new_child = insert_rec(root, key, value, &up_key);

    if (!new_child) return root;

    Node* new_root = CreateNode(false);
    new_root->keys[0] = up_key;
    new_root->children[0] = root;
    new_root->children[1] = new_child;
    new_root->num_keys = 1;

    return new_root;
}

// ================= BORROW LEFT =================
void borrow_left(Node* parent, int idx) {
    Node* node = parent->children[idx];
    Node* left = parent->children[idx-1];

    // shift node right
    for (int i = node->num_keys; i > 0; i--) {
        node->keys[i] = node->keys[i-1];
        if (node->is_leaf)
            node->values[i] = node->values[i-1];
    }

    if (node->is_leaf) {
        node->keys[0] = left->keys[left->num_keys - 1];
        node->values[0] = left->values[left->num_keys - 1];
        parent->keys[idx-1] = node->keys[0];
    } else {
        node->keys[0] = parent->keys[idx-1];
        parent->keys[idx-1] = left->keys[left->num_keys - 1];

        for (int i = node->num_keys + 1; i > 0; i--)
            node->children[i] = node->children[i-1];

        node->children[0] = left->children[left->num_keys];
    }

    left->num_keys--;
    node->num_keys++;
}

// ================= BORROW RIGHT =================
void borrow_right(Node* parent, int idx) {
    Node* node = parent->children[idx];
    Node* right = parent->children[idx+1];

    if (node->is_leaf) {
        node->keys[node->num_keys] = right->keys[0];
        node->values[node->num_keys] = right->values[0];

        for (int i = 1; i < right->num_keys; i++) {
            right->keys[i-1] = right->keys[i];
            right->values[i-1] = right->values[i];
        }

        parent->keys[idx] = right->keys[0];
    } else {
        node->keys[node->num_keys] = parent->keys[idx];
        parent->keys[idx] = right->keys[0];

        node->children[node->num_keys+1] = right->children[0];

        for (int i = 1; i <= right->num_keys; i++)
            right->children[i-1] = right->children[i];

        for (int i = 1; i < right->num_keys; i++)
            right->keys[i-1] = right->keys[i];
    }

    right->num_keys--;
    node->num_keys++;
}

// ================= MERGE =================
void merge(Node* parent, int idx) {
    Node* left = parent->children[idx];
    Node* right = parent->children[idx+1];

    if (!left->is_leaf) {
        left->keys[left->num_keys++] = parent->keys[idx];
    }

    for (int i = 0; i < right->num_keys; i++) {
        left->keys[left->num_keys + i] = right->keys[i];
        if (left->is_leaf)
            left->values[left->num_keys + i] = right->values[i];
    }

    if (!left->is_leaf) {
        for (int i = 0; i <= right->num_keys; i++)
            left->children[left->num_keys + i] = right->children[i];
    } else {
        left->next = right->next;
    }

    left->num_keys += right->num_keys;

    for (int i = idx + 1; i < parent->num_keys; i++)
        parent->keys[i-1] = parent->keys[i];

    for (int i = idx + 2; i <= parent->num_keys; i++)
        parent->children[i-1] = parent->children[i];

    parent->num_keys--;

    free(right);
}

// ================= DELETE REC =================
void delete_rec(Node* node, int key) {
    int idx = 0;
    while (idx < node->num_keys && key > node->keys[idx]) idx++;

    if (node->is_leaf) {
        if (idx == node->num_keys || node->keys[idx] != key) return;

        for (int i = idx + 1; i < node->num_keys; i++) {
            node->keys[i-1] = node->keys[i];
            node->values[i-1] = node->values[i];
        }
        node->num_keys--;
        return;
    }

    Node* child = node->children[idx];

    if (child->num_keys < MIN_KEYS) {
        if (idx > 0 && node->children[idx-1]->num_keys > MIN_KEYS)
            borrow_left(node, idx);
        else if (idx < node->num_keys && node->children[idx+1]->num_keys > MIN_KEYS)
            borrow_right(node, idx);
        else {
            if (idx < node->num_keys)
                merge(node, idx);
            else {
                merge(node, idx-1);
                idx--;
            }
        }
    }

    delete_rec(node->children[idx], key);
}

// ================= DELETE =================
Node* delete (Node* root, int key) {
    if (!root) return NULL;

    delete_rec(root, key);

    if (!root->is_leaf && root->num_keys == 0) {
        Node* tmp = root;
        root = root->children[0];
        free(tmp);
    }

    if (root && root->is_leaf && root->num_keys == 0) {
        free(root);
        return NULL;
    }

    return root;
}

// ================= TRAVERSE =================
void traverse(Node* root) {
    if (!root) return;

    while (!root->is_leaf)
        root = root->children[0];

    while (root) {
        for (int i = 0; i < root->num_keys; i++)
            printf("(%d:%d) ", root->keys[i], root->values[i]);
        root = root->next;
    }
}

// ================= MAIN =================
int main() {
    Node* root = NULL;

    int keys[] = {10,20,5,6,12,30,7,17};
    int size = sizeof(keys)/sizeof(keys[0]);

    for(int i=0;i<size;i++)
        root = insert(root,keys[i],keys[i]*10);

    printf("Initial:\n");
    traverse(root); printf("\n");

    int val;
    if(search(root,20,&val))
        printf("Found 20 -> %d\n",val);

    int del[] = {6,7,5,17,20};
    for(int i=0;i<5;i++)
        root = delete(root,del[i]);

    printf("After Delete:\n");
    traverse(root); printf("\n");

    return 0;
}
