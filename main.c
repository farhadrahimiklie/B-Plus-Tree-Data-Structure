#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
// farhad rhaimi klie
#define T 2
#define MIN_KEYS (T-1)
#define MAX_KEYS (2*T-1)

typedef struct Node{
    int keys[MAX_KEYS];
    int values[MAX_KEYS]; // only valid if leaf
    struct Node* children[2*T];
    struct Node* next; // leaf-level linked list
    int n;
    bool is_leaf;
}Node;

Node* CreateNode(bool leaf){
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->is_leaf = leaf;
    newNode->n = 0;
    newNode->next = NULL;

    for(int i = 0; i < 2*T-1; i++){
        newNode->keys[i] = 0;
    }

    for(int j = 0; j < 2*T; j++){
        newNode->children[j] = NULL;
    }

    return newNode;
}


Node* Search(Node* root, int key, int* value) {
    if(!root) return NULL;

    while(!root->is_leaf){
        int index = 0;
        while(index < root->n && key >= root->keys[index]){
            index++;
        }
        root = root->children[index];
    }
    for(int i = 0; i < root->n; i++){
        if(root->keys[i] == key){
            *value = root->values[i];
            return root;
        }
    }
    return NULL;
}

void SplitChild(Node* Parent, int level, Node* child){
    Node* newNode = CreateNode(child->is_leaf);
    newNode->n = T;

    for(int j = 0; j < T; j++){
        newNode->keys[j] = child->keys[j+T-1];
    }

    if(child->is_leaf){
        for(int j = 0; j < T; j++){
            newNode->values[j] = child->values[j+T-1];
        }

        child->n = T -1;
        // update leaf linked list
        newNode->next = child->next;
        child->next = newNode;

        // promote first key of new leaf
        for(int j = Parent->n; j > level; j--){
            Parent->keys[j] = Parent->keys[j-1];
        }
        Parent->keys[level] = newNode->keys[0];
    }else{
        for(int j = 0; j < T; j++){
            newNode->children[j] = child->children[j+T];
        }
        child->n = T -1;

        for(int j = Parent->n; j > level; j--){
            Parent->keys[j] = Parent->keys[j-1];
        }
        Parent->keys[level] = child->keys[T-1];
    }

    for(int j = Parent->n+1; j > level +1; j--){
        Parent->children[j] = Parent->children[j-1];
    }
    Parent->children[level+1] = newNode;
    Parent->n++;
}

void insertNonFull(Node* node, int key, int value){
    int index = node->n -1;

    if(node->is_leaf){
        while(index >= 0 && key < node->keys[index]){
            node->keys[index+1] = node->keys[index];
            node->values[index+1] = node->values[index];
            index--;
        }
        node->keys[index+1] = key;
        node->values[index+1] = value;
        node->n++;
    }else{
        while(index >= 0 && key < node->keys[index]){
            index--;
        }
        index++;
        if(node->children[index]->n == 2*T-1){
            SplitChild(node, index, node->children[index]);
            if(key > node->keys[index]){
                index++;
            }
        }
        insertNonFull(node->children[index], key, value);
    }
}

Node* insert(Node* root, int key, int value){
    if(!root){
        root = CreateNode(true); // leaf node
        root->keys[0] = key;
        root->values[0] = value;
        root->n = 1;
        return root;
    }

    if(root->n == MAX_KEYS){
        Node* newRoot = CreateNode(false); // internal node
        newRoot->children[0] = root;
        SplitChild(newRoot, 0, root);

        int index = (key >= newRoot->keys[0]) ? 1 : 0;
        insertNonFull(newRoot->children[index], key, value);
        return newRoot;
    }else{
        insertNonFull(root, key, value);
        return root;
    }
}

int findIndex(Node* node, int key){
    int i=0;
    while(i<node->n && node->keys[i] < key) i++;
    return i;
}

void borrowFromLeft(Node* parent, int idx){
    Node* node = parent->children[idx];
    Node* left = parent->children[idx-1];

    for(int i=node->n-1;i>=0;i--){
        node->keys[i+1]=node->keys[i];
        node->values[i+1]=node->values[i];
    }

    node->keys[0]=left->keys[left->n-1];
    node->values[0]=left->values[left->n-1];

    left->n--;
    node->n++;

    parent->keys[idx-1]=node->keys[0];
}


void borrowFromRight(Node* parent,int idx){
    Node* node = parent->children[idx];
    Node* right = parent->children[idx+1];

    node->keys[node->n]=right->keys[0];
    node->values[node->n]=right->values[0];
    node->n++;

    for(int i=1;i<right->n;i++){
        right->keys[i-1]=right->keys[i];
        right->values[i-1]=right->values[i];
    }
    right->n--;

    parent->keys[idx]=right->keys[0];
}

void merge(Node* parent,int idx){
    Node* left = parent->children[idx];
    Node* right = parent->children[idx+1];

    for(int i=0;i<right->n;i++){
        left->keys[left->n+i]=right->keys[i];
        left->values[left->n+i]=right->values[i];
    }

    left->n += right->n;
    left->next = right->next;

    for(int i=idx+1;i<parent->n;i++)
        parent->keys[i-1]=parent->keys[i];

    for(int i=idx+2;i<=parent->n;i++)
        parent->children[i-1]=parent->children[i];

    parent->n--;
    free(right);
}

void deleteKey(Node* node,int key){
    if(!node) return;

    if(node->is_leaf){
        int idx = findIndex(node,key);

        if(idx == node->n || node->keys[idx]!=key)
            return;

        for(int i=idx+1;i<node->n;i++){
            node->keys[i-1]=node->keys[i];
            node->values[i-1]=node->values[i];
        }
        node->n--;
        return;
    }

    int idx = findIndex(node,key);
    Node* child = node->children[idx];

    if(child->n == MIN_KEYS){
        if(idx>0 && node->children[idx-1]->n > MIN_KEYS)
            borrowFromLeft(node,idx);
        else if(idx<node->n && node->children[idx+1]->n > MIN_KEYS)
            borrowFromRight(node,idx);
        else{
            if(idx < node->n)
                merge(node,idx);
            else{
                merge(node,idx-1);
                idx--;
            }
        }
    }

    deleteKey(node->children[idx],key);
}


Node* delete(Node* root, int key) {
    if(!root)
        return NULL;

    deleteKey(root, key);

    if(root->n == 0) {
        Node* tmp = root;
        if(root->is_leaf)
            root = NULL;
        else
            root = root->children[0];
        free(tmp);
    }
    return root;
}

void Traversal(Node* root){
    if(!root) return;

    while(!root->is_leaf){
        root = root->children[0];
    }

    while(root){
        for(int i = 0; i < root->n; i++){
            printf("(%d:%d) ", root->keys[i], root->values[i]);
        }
        root = root->next;
    }
}

int main(){
    Node* root = NULL;
    int keys[] = {10, 20, 5, 6, 12, 30, 7, 17};
    int size = sizeof(keys) / sizeof(keys[0]);
    for(int i = 0; i < size; i++){
        root = insert(root, keys[i], keys[i]*10);
    }

    printf("Traversal B+ Tree: \n");
    Traversal(root);
    printf("\n");


    //int value;
    //if(Search(root, 12, &value)){
        //printf("Found 12-> %d\n", value);
    //}else{
        //printf("Not Found\n");
    //}

    printf("Before Delete: \n");
    Traversal(root); printf("\n");

    int del[] = {6,7,5,20};
    for(int i = 0; i < 4; i++){
        root = delete(root, del[i]);
    }
    printf("After Delete: \n");
    Traversal(root); printf("\n");
    return 0;
}
