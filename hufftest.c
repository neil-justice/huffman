/* hufftest.c
 * 
 * Function: tests different approaches to building a huffman binary tree.
 * The approaches are: qsorted array, insertion sorted array, and 
 * binary search + memmove (the approach I use in the actual implementation).
 * Usage: filename
 * 
 * creates an array of ASIZE nodes with frequency rand() % RMOD, and then 
 * times how long each different approach takes to sort the same values.
 * Unfortunately I haven't had time to implement a linked-list insertion
 * sort, which would probably do quite well - but I don't think it would
 * be better than mine, since insertion sort cannot match the binary 
 * search's O(log n).  This would however depend totally on how memmove
 * has been implemented in the system used.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#define ASIZE 5000
#define PNODE -250 /* char field of the parent nodes */
#define RMOD 10000

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

node *createNode(int c, int freq, node *left, node *right);
node **createNodeIndex(int *a, int len);
int  calcNodeCnt(int *a);
void freeNodes(node *n);
void fillTestArray(int *a);

void testBinary(int *testarray);
int  getInsertionPoint(int key, nodeIndex *index, int start);
node *populateTreeBINARY(nodeIndex *index);

void testInsertion(int *testarray);
void insertionSort(nodeIndex *index, int start);
node *populateTreeINSERT(nodeIndex *index);

void testQsort(int *testarray);
node *populateTreeQSORT(nodeIndex *index);
int  nodeComp(const void * a, const void * b);

int main(void)
{
  int testarray[ASIZE] = {0};
  clock_t begin, end;
  double timespent;
  
  srand(time(NULL));
  fillTestArray(testarray);   
  
  printf("test array filled with %d random frequencies.\n", ASIZE);
  printf("with values from 0 - %d.\n\n", RMOD); 
    
  begin = clock();
  testQsort(testarray);
  end = clock();
  timespent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("time spent to execute qsort: %fs\n", timespent);
  
  begin = clock();
  testBinary(testarray);
  end = clock();
  timespent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("time spent to execute binary search + memmove: %fs\n", timespent);
  
  begin = clock();
  testInsertion(testarray);
  end = clock();
  timespent = (double)(end - begin) / CLOCKS_PER_SEC;
  printf("time spent to execute insertion sort: %fs\n", timespent);

  return 0;
}

void testInsertion(int *testarray)
{

  nodeIndex index = {NULL, 0};
  node *root;

  index.len = calcNodeCnt(testarray);
  index.a = createNodeIndex(testarray, index.len);
  qsort(index.a, index.len, sizeof(node *), nodeComp);
  root = populateTreeINSERT(&index);

  free(index.a);
  freeNodes(root);
}

void testBinary(int *testarray)
{
  nodeIndex index = {NULL, 0};
  node *root;

  index.len = calcNodeCnt(testarray);
  index.a = createNodeIndex(testarray, index.len);
  qsort(index.a, index.len, sizeof(node *), nodeComp);
  root = populateTreeBINARY(&index);

  free(index.a);
  freeNodes(root);
}

void testQsort(int *testarray)
{
  nodeIndex index = {NULL, 0};
  node *root;

  index.len = calcNodeCnt(testarray);
  index.a = createNodeIndex(testarray, index.len);
  qsort(index.a, index.len, sizeof(node *), nodeComp);
  root = populateTreeQSORT(&index);

  free(index.a);
  freeNodes(root);
}

node *populateTreeQSORT(nodeIndex *index)
{
  int newfreq, start = 0, len = index->len;
  node *temp;

  for (start = 0; start < index->len - 1; start++) {
    newfreq = index->a[start]->freq + index->a[start + 1]->freq;
    temp = createNode(PNODE, newfreq, index->a[start], index->a[start + 1]);
    index->a[start] = index->a[start + 1] = NULL;
    index->a[start + 1] = temp;
    len -= 1;
    qsort(index->a + start + 1, len, sizeof(node *), nodeComp);
  }
  return index->a[index->len - 1];
}

node *populateTreeINSERT(nodeIndex *index)
{
  int newfreq, start = 0;
  node *temp;

  for (start = 0; start < index->len - 1; start++) {
    /* create parent node of the 2 smallest */
    newfreq = index->a[start]->freq + index->a[start + 1]->freq;
    temp = createNode(PNODE, newfreq, index->a[start], index->a[start + 1]);

    index->a[start] = index->a[start + 1] = NULL;
    index->a[start + 1] = temp;

    insertionSort(index, start + 1);
  }
  return index->a[index->len - 1]; /* return pointer to root node */
}

void insertionSort(nodeIndex *index, int start)
{
  int i, j;
  node *temp;

  for (i = start + 1; i < index->len; i++) {
    j = i;
    if (index->a[i] == NULL) {
      printf("ohno\n");
    }
    while (j > 0 && index->a[j-1]->freq > index->a[j]->freq) {
      temp = index->a[j-1];
      index->a[j-1] = index->a[j];
      index->a[j] = temp;
    }
  }
}

node *populateTreeBINARY(nodeIndex *index)
{
  int insertpoint, newfreq, start = 0;
  node *temp;

  for (start = 0; start < index->len - 1; start++) {
    newfreq = index->a[start]->freq + index->a[start + 1]->freq;
    temp = createNode(PNODE, newfreq, index->a[start], index->a[start + 1]);

    insertpoint = getInsertionPoint(temp->freq, index, start);
    index->a[start] = index->a[start + 1] = NULL;

    if (insertpoint > start + 1) {
      memmove(index->a, index->a + 1, insertpoint * sizeof(node *));
    }
    index->a[insertpoint] = temp;
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

int nodeComp(const void * a, const void * b)
{
  const node **n1 = (const node **)a;
  const node **n2 = (const node **)b;

  return (int)((*n1)->freq - (*n2)->freq);
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

int calcNodeCnt(int *a)
{
  int i, cnt = 0;

  for (i = 0; i < ASIZE; i++) {
    if (a[i] != 0) {
      cnt++;
    }
  }

  return cnt;
}

void fillTestArray(int *a)
{
  int i;

  for (i = 0; i < ASIZE; i++) {
    a[i] = rand()% RMOD;
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
