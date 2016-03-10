/* huffman.c
 * 
 * Function: builds a binary huffman tree from the ASCII-encoded text file
 * given in argv[1], and displays the bit encoding of each character and
 * total number of bytes required to encode. 
 * Usage: filename path/to/textfile
 * 
 * Uses a runtime-sized array of pointers to node structures, where each
 * node tracks the frequency of a particular char in argv[1].  This array
 * is qsorted once, and then to keep it sorted, the insertion point for a
 * parent is found with a binary search and the memmove function is used
 * to move elements within the array and make space for the parent (the 
 * pointers to the 2 child nodes having been set to NULL, there is 
 * spare room).  This avoids normal slow insertion times for arrays,
 * making a linked-list implementation unnecessary.
 * Once the tree is complete, the huffman encodings are found recursively.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define ASIZE 128 /* size of int array indexed by ASCII chars */
#define FORMATSTRLEN 128 /* length of format string for printing 
 * huffman encodings */
#define PNODE -250 /* arbitrary non-ascii char field for the 
 * parent nodes */
#define BITSPERBYTE 8

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

typedef struct buffer {
  char *str;
  short size;
} buffer;

/* Encoding calculation and printing functions */
void printHuffman(int *a, node *root);
int  treeHeight(node *n);
buffer createBuffer(int size);
void setFormatString(node *n, buffer *b, char *formatstr);
void reverseString(char *s);
node *findEncoding(node *root, int target, buffer *b);

/* Tree-building functions */
int  *getFreqsFromFile(int argc, char **argv, int *arr);
int  calcNodeCnt(int *a);
node *createNode(int c, int freq, node *left, node *right);
node **createNodeIndex(int *a, int len);
int  nodeComp(const void * a, const void * b);
node *populateTree(nodeIndex *index);
int  getInsertionPoint(int key, nodeIndex *index, int start);

void freeNodes(node *n);

int main(int argc, char **argv)
{
  int ascii[ASIZE] = {0};
  nodeIndex index = {NULL, 0};
  node *root;

  getFreqsFromFile(argc, argv, ascii);
  index.len = calcNodeCnt(ascii);
  index.a = createNodeIndex(ascii, index.len);
  qsort(index.a, index.len, sizeof(node *), nodeComp);
  root = populateTree(&index);
  printHuffman(ascii, root);

  free(index.a);
  freeNodes(root);
  return 0;
}

void printHuffman(int *a, node *root)
{
  int i;
  long unsigned int bits = 0;
  node *n;
  buffer b = createBuffer(treeHeight(root));
  char formatstr[FORMATSTRLEN] = "";

  for (i = 0; i < ASIZE; i++) {
    if (a[i] != 0) {
      n = findEncoding(root, i, &b);
      reverseString(b.str);
      setFormatString(n, &b, formatstr);
      fprintf(stdout, formatstr, n->c, b.str, strlen(b.str), n->freq);
      bits += (strlen(b.str) * n->freq);
      memset(formatstr,'\0',strlen(formatstr));
      memset(b.str,'\0',strlen(b.str));
    }
  }
  fprintf(stdout, "%lu Bytes\n\n", 
    bits / BITSPERBYTE + (bits % BITSPERBYTE != 0)); /* rounds up */
  free(b.str);
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

buffer createBuffer(int size)
{
  buffer b;

  b.str = (char *)calloc(size + 1,sizeof(char));
  if (b.str == NULL) {
    fprintf(stderr,"ERROR: buffer calloc failed\n");
    exit(EXIT_FAILURE);
  }

  b.size = size + 1;

  return b;
}

void setFormatString(node *n, buffer *b, char *formatstr)
{
  if (!isprint(n->c)) {
    sprintf(formatstr, "%%03d :%%%ds", b->size);
  }
  else {
    sprintf(formatstr,  "'%%c' :%%%ds", b->size);
  }
  /* Unfortunately snprintf is c99 only, but a buffer overflow
   * could only happen if b.size is longer than (FORMATSTRLEN - 8)
   * = 120 digits. */
  strcat(formatstr," (%3lu * %4d)\n\0");
}

void reverseString(char *s)
{
  /* the recursive method of finding the huffman encoding by traversing
   * the binary tree results in a backwards string.  */
  int i, mid = strlen(s) / 2;
  char *a = s, *b = s + (strlen(s) - 1), temp;

  for(i = 0 ; i < mid; i++, a++, b--) {
    temp = *a;
    *a = *b;
    *b = temp;
  }
}

node *findEncoding(node *root, int target, buffer *b)
{
  node *ln, *rn, *n = root;
  static int cnt = 0;

  if (n == root) {
    cnt = 0; /* reinitialise count for different nodes */
  }
  if (n == NULL) {
    return NULL;
  }
  if (n->c == target) {
    return n;
  }
  if ((ln = findEncoding(n->left, target, b)) != NULL) {
    b->str[cnt++] = '0';
    return ln;
  }
  if ((rn = findEncoding(n->right, target, b)) != NULL) {
    b->str[cnt++] = '1';
    return rn;
  }
  return NULL;
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
    fprintf(stderr, "This program only requires an ASCII textfile.\n");
    fclose(file);
    exit(1);
  }

  while((c = fgetc(file)) != EOF) {
    a[c]++;
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
  
  if (cnt < 2) {
    fprintf(stderr,"ERROR: too few nodes to build tree\n");
    exit(EXIT_FAILURE); 
  }
  
  return cnt;
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
  return index->a[index->len - 1]; /* return pointer to root node */
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

void freeNodes(node *n)
{
  if (n == NULL) {
    return;
  }
  freeNodes(n->left);
  freeNodes(n->right);
  free(n);
}