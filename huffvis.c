/* huffvis.c
 * 
 * Function: builds a binary huffman tree from the ASCII-encoded text file
 * given in argv[1], and prints the tree to stdout.
 * Usage: filename path/to/textfile
 * 
 * See Huffman.c for information about the shared processes.  Once the 
 * tree has been assembled, its width and height are calculated and a grid
 * is allocated to store it.  This is a 1D array indexed as if it were 2D.
 * Then the tree is recursively printed to this grid.  In order to keep it
 * as compact as possible, each node is shifted right by the total number 
 * of right-branch children of its parent's left child.  However, since this
 * can often return too-large results, this shift is 1) only applied if the
 * node has any children, and 2) compared with the maximum x-coord used
 * to print a node so far, and the minimum of the 2 used added to XOFFSET
 * (the minimum increment value).
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define ASIZE   128  /* size of array indexed by ASCII chars */
#define XOFFSET   2  /* offsets for printing binary tree */
#define YOFFSET   2   
#define PNODE   '#'  /* characters for printing binary tree */
#define HBRANCH '-' 
#define VBRANCH '|'
#define EMPTY   ' '

typedef struct node {
  int freq;
  int c;
  struct node *left;
  struct node *right;
} node;

typedef struct nodeIndex {
  node **a;
  int len;
} nodeIndex;

typedef struct display {
  int *grid;
  size_t xlen, ylen;
} display;

/* Tree printing functions */
void printTree(node *n, display *d);
void initDisplayGrid(display *d, node *root);
int  setGridHeight(node *n, int height, int maxheight);
void printTreeRecursive(node *n, display *d, int y, int x);
int  treeHeight(node *n);
void printBranches(node *n, display *d, int x, int y);
int  getRightBranchOffset(node *n);
void printDisplayGrid(display *d);

/* Tree-building functions */
int  *getFreqsFromFile(int argc, char **argv, int *arr);
int  calcNodeCnt(int *a);
node *createNode(int c, int freq, node *left, node *right);
node **createNodeIndex(int *a, int len);
int  nodeComp(const void * a, const void * b);
node *populateTree(nodeIndex *index);
int  getInsertionPoint(int key, nodeIndex *index, int start);

int  min(int a, int b);
int  max(int a, int b);
void freeNodes(node *n);

int main(int argc, char **argv)
{
  int ascii[ASIZE] = {0};
  nodeIndex index = {NULL, 0};
  node *root;
  display d;

  getFreqsFromFile(argc, argv, ascii);
  index.len = calcNodeCnt(ascii);
  index.a = createNodeIndex(ascii, index.len);
  qsort(index.a, index.len, sizeof(node *), nodeComp);
  root = populateTree(&index);
  printTree(root, &d);
  
  free(index.a);
  freeNodes(root);
  free(d.grid);
  return 0;
}

void printTree(node *n, display *d)
{
  initDisplayGrid(d, n);
  printTreeRecursive(n, d, 0 ,0);
  printDisplayGrid(d);
}

void initDisplayGrid(display *d, node *root)
{
  unsigned int x,y;
  
  d->ylen = setGridHeight(root, 0, 0) + YOFFSET;
  d->xlen = getRightBranchOffset(root) + XOFFSET;
  
  d->grid = (int *)malloc(sizeof(int) * d->xlen * d->ylen);
  if (d->grid == NULL) {
    fprintf(stderr,"ERROR: grid malloc failed\n");
    exit(EXIT_FAILURE);
  }
  
  for (y = 0; y < d->ylen; y++) {
    for (x = 0; x < d->xlen; x++) {
      d->grid[(y * d->xlen) + x] = EMPTY;
    }
  }
}

int setGridHeight(node *n, int height, int maxheight)
{
  /* Finds the route with most left branches from n down */
  if (n == NULL) {
    return 0;
  }

  maxheight = max(
    setGridHeight(n->left, height + YOFFSET, maxheight),
    setGridHeight(n->right, height,  maxheight));
  
  if (height > maxheight) {
    maxheight = height;
  } 

  return maxheight;
}

void printTreeRecursive(node *n, display *d, int y, int x)
{
  /* prints the tree to a 1D array (indexed as 2D) */
  static int xmax = 0;
  int dx = 0;

  if (n == NULL) {
    return;
  }
  
  if (x > xmax) {
    xmax = x;
  }

  if (treeHeight(n->right) > 0) {
    dx = getRightBranchOffset(n->left);
  }

  d->grid[(y * d->xlen) + x] = n->c;
  printTreeRecursive(n->left, d, y + YOFFSET, x);
  printTreeRecursive(n->right, d, y, min(x + dx,xmax) + XOFFSET);
  printBranches(n, d, x, y);
}

int treeHeight(node *n)
{
  int lh, rh;
  if (n == NULL) {
    return -1;
  }

  lh = treeHeight(n->left);
  rh = treeHeight(n->right);

  if (lh > rh) {
    return lh + 1;
  }
  else {
    return rh + 1;
  }
}

int getRightBranchOffset(node *n)
{
  /* calculates the draw distance between a node and its right child. */
  int cnt = 0;

  if (n == NULL) {
    return 0;
  }
  
  cnt = getRightBranchOffset(n->left) 
      + getRightBranchOffset(n->right);
  
  if (n->right != NULL) {
    return cnt + XOFFSET;
  }
  else {
    return 0;
  }
}

void printBranches(node *n, display *d, int x, int y)
{
  /* Fills in the branch tiles from n to its children */
  int i;

  if (n->left != NULL) {
    for (i = 1; i < YOFFSET; i++) {
      d->grid[((y + i) * d->xlen) + x] = VBRANCH;
    }
  }

  if (n->right != NULL) {
    for (i = 1; d->grid[(y * d->xlen) + x + i] == EMPTY; i++) {
      d->grid[(y * d->xlen) + x + i] = HBRANCH;
    }
  }
}

void printDisplayGrid(display *d)
{
  unsigned int x,y;
  
  for (y = 0; y < d->ylen; y++) {
    for (x = 0; x < d->xlen; x++) {
      fputc((char)d->grid[(y * d->xlen) + x], stdout);
    }
    fprintf(stdout,"\n");
  }
  fprintf(stdout,"\n");
}

int *getFreqsFromFile(int argc, char **argv, int *a)
{
  FILE *file = fopen(argv[1], "r");
  int c;

  if (file == NULL) {
    fprintf(stderr, "Error opening file - check name and directory.\n");
    exit(1);
  }
  if (argc != 2) {
    fprintf(stderr, "Invalid number of arguments supplied.\n");
    fclose(file);
    exit(1);
  }

  while((c = fgetc(file)) != EOF) {
    if (isalpha(c)) {
      c = toupper(c);
      a[c]++;
    }
  }

  fclose(file);
  return a;
}

int calcNodeCnt(int *a)
{
  int i, cnt = 0;

  for (i = 0; i < ASIZE; i++) {
    if (a[i] != 0) {
      cnt++;
    }
  }
  
  if (cnt <= 1) {
    fprintf(stderr,"ERROR: too few nodes to build tree\n");
    exit(EXIT_FAILURE); 
  }

  return cnt;
}

node **createNodeIndex(int *a, int len)
{
  int i, j;
  node **na = (node **)calloc(len,sizeof(node *));

  if (na == NULL) {
    fprintf(stderr,"ERROR: index alloc failed\n");
    exit(EXIT_FAILURE);
  }

  for (i = 0, j = 0; i < ASIZE; i++) {
    if (a[i]!= 0) {
      na[j] = createNode(i, a[i], NULL, NULL);
      j++;
    }
  }

  return na;
}

node *createNode(int c, int freq, node *left, node *right)
{
  node *p;

  p = (node *)malloc(sizeof(node));
  if (p == NULL) {
    fprintf(stderr,"ERROR: node malloc failed\n");
    exit(EXIT_FAILURE);
  }
  p->freq = freq;
  p->c = c;
  p->left = left;
  p->right = right;

  return p;
}

int nodeComp(const void * a, const void * b)
{
  const node **n1 = (const node **)a;
  const node **n2 = (const node **)b;

  return (int)((*n1)->freq - (*n2)->freq);
}

node *populateTree(nodeIndex *index)
{
  /* Sorts the array each time using a binary search to find the
  * insertion point, then memmove to create a space in the array
  * for the insertion.
  *
  * See hufftest.c for testing.  For different tree-building methods, 
  * (with a test case of 5000 nodes, starting from a sorted 
  * initial state, no optimisation flags)
  * representative results are:
  * time spent to execute qsort: 0.810000s
  * time spent to execute binary search + memmove: 0.010000s
  * time spent to execute insertion sort: 0.170000s
  */
  int insertpoint, newfreq, lchild, rchild, start = 0;
  node *parent;

  for (start = 0; start < index->len - 1; start++) {
    lchild = start; /* lchild and rchild are purely for readability */
    rchild = start + 1;
    newfreq = index->a[lchild]->freq + index->a[rchild]->freq;
    parent = createNode(PNODE, newfreq, index->a[lchild], index->a[rchild]);

    insertpoint = getInsertionPoint(parent->freq, index, start);
    index->a[lchild] = index->a[rchild] = NULL;

    if (insertpoint > start + 1) {
      /* if insertpoint is not at beginnning, create room for new node */
      memmove(index->a, index->a + 1, insertpoint * sizeof(node *));
    }
    index->a[insertpoint] = parent;
  }
  return index->a[index->len - 1]; /* returns pointer to root node */
}

int getInsertionPoint(int key, nodeIndex *index, int start)
{
  int end = index->len - 1, mid;

  while (start <= end) {
    mid = (start + end) / 2;
    if (key > index->a[mid]->freq) {
      start = mid + 1;
    }
    else if (key < index->a[mid]->freq){
      end = mid - 1;
    }
    else {
      return mid;
    }
  }
  return start - 1; /* still need an insertion point,
  even if no match was found. */
}

int min(int a, int b)
{
  if (a > b) {
    return b;
  }
  else {
    return a;
  }
}

int max(int a, int b)
{
  if (a < b) {
    return b;
  }
  else {
    return a;
  }
}

void freeNodes(node *n)
{
  if (n == NULL) {
    return;
  }
  freeNodes(n->left);
  freeNodes(n->right);
  free(n);
}