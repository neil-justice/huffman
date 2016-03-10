/* huffsdl.c
 * 
 * Function: builds a binary huffman tree from the ASCII-encoded text file
 * given in argv[1], and draws the tree to an SDL window.
 * Usage: filename path/to/textfile
 * Make with makefile command 'make'.
 * 
 * See Huffman.c and huffvis.c for information about the shared processes.
 * The int array in huffvis.c has been converted to a char array to work
 * with Neill_SDL_DrawString.  It is read into an alloced buffer of size
 * d.xlen+1 one line at a time.  It no longer contains branch characters
 * and the parent node char is only left in as a marker for the line
 * drawing function (which checks the contents of the char grid for non-
 * empty spots).  The parent node is removed as the grid is read into
 * the buffer.  The colour of each node is a function of its height.
 * The window will close on any keypress or mouse click.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "neillsdl2.h"

#define ASIZE   128  /* size of array indexed by ASCII chars */
#define XOFFSET   2  /* offsets for printing binary tree */
#define YOFFSET   3   
#define PNODE   '#'  /* characters for printing binary tree */
#define EMPTY   ' '

#define FNTFILE   "m7fixed.fnt"
#define TOPOFFSET FNTHEIGHT * 3 /* space at top of screen for file info etc*/
#define PADDING   3 /* extra space around node char*/
#define NRADIUS   (FNTHEIGHT / 2) + PADDING /* radius of node */
#define NODEGREEN    120 /* colour values for nodes - red varies by depth*/
#define NODEBLUE    120

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
  char *grid;
  size_t xlen, ylen;
} display;

typedef struct buffer {
  char *str;
  short size;
} buffer;

typedef struct colour {
  uint8_t red, green, blue;
} colour;

/* drawing functions */
void handleDisplay(node *n, display *d, char **argv);
void drawTree(node *n, display *d, SDL_Simplewin *sw, 
  fntrow fontdata[FNTCHARS][FNTHEIGHT]);
void drawTreeRecursive(node *n, display *d, SDL_Simplewin *sw, 
  int y, int x);
int  getRightBranchOffset(node *n);
void drawInfo(SDL_Simplewin *sw, fntrow fontdata[FNTCHARS][FNTHEIGHT], 
  char **argv);
void drawBranches(node *n, display *d, SDL_Simplewin *sw, int x, int y);
void drawLeftBranch(int x, int y, SDL_Simplewin *sw );
void drawRightBranch(int x, int y, display *d, SDL_Simplewin *sw );
void drawNode(int x, int y, SDL_Simplewin *sw );
void drawDisplayGrid(display *d, SDL_Simplewin *sw, 
  fntrow fontdata[FNTCHARS][FNTHEIGHT]);
buffer createBuffer(int size);
int  treeHeight(node *n);
void initDisplayGrid(display *d, node *root);
int  setGridHeight(node *n, int height, int maxheight);  
  
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
  handleDisplay(root, &d, argv);
  free(index.a);
  freeNodes(root);
  free(d.grid);
 
  return 0;
}

void handleDisplay(node *n, display *d, char **argv)
{
  SDL_Simplewin sw;
  fntrow fontdata[FNTCHARS][FNTHEIGHT];
  
  Neill_SDL_Init(&sw);
  Neill_SDL_ReadFont(fontdata, FNTFILE);
  drawTree(n, d, &sw, fontdata);
  drawInfo(&sw, fontdata, argv);

  SDL_RenderPresent(sw.renderer);
  SDL_UpdateWindowSurface(sw.win);
  
  do {
    Neill_SDL_Events(&sw);
  }
  while (!sw.finished);

  SDL_Quit(); 
}

void drawTree(node *n, display *d, SDL_Simplewin *sw, 
  fntrow fontdata[FNTCHARS][FNTHEIGHT])
{
  initDisplayGrid(d, n);
  
  drawTreeRecursive(n, d, sw, 0 ,0);

  /* This blend mode stops the nodes and branches being covered 
   * over by the text layer. */
  SDL_SetRenderDrawBlendMode(sw->renderer, SDL_BLENDMODE_ADD);   
  drawDisplayGrid(d, sw, fontdata);
}

void drawInfo(SDL_Simplewin *sw, fntrow fontdata[FNTCHARS][FNTHEIGHT], 
  char **argv)
{
  char s[WWIDTH / FNTWIDTH] = "";

  strncpy(s, argv[1], WWIDTH / FNTWIDTH);
  /* doesn't bother loading any more than can fit on the screen */
  
  Neill_SDL_DrawString(sw, fontdata, "Huffman tree", 0, 0); 
  Neill_SDL_DrawString(sw, fontdata, s, 0, FNTHEIGHT);
}

void drawTreeRecursive(node *n, display *d, SDL_Simplewin *sw, 
  int y, int x)
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
  drawTreeRecursive(n->left, d, sw, y + YOFFSET, x);
  drawTreeRecursive(n->right, d, sw, y, min(x + dx,xmax) + XOFFSET);
  drawBranches(n, d, sw, x, y);
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

void drawBranches(node *n, display *d, SDL_Simplewin *sw, int x, int y)
{
  int nodeheight = min(treeHeight(n) + 1,UINT8_MAX);
  colour nodeclr = {0 , NODEGREEN, NODEBLUE };
  
  nodeclr.red = UINT8_MAX - UINT8_MAX / nodeheight;
  Neill_SDL_SetDrawColour(sw, nodeclr.red, 
    nodeclr.blue, nodeclr.green);
  drawNode(x, y, sw);
  
  if (n->left != NULL) {
    nodeclr.red = UINT8_MAX - UINT8_MAX / (nodeheight - 1);
    Neill_SDL_SetDrawColour(sw, nodeclr.red, 
      nodeclr.blue, nodeclr.green);    
    drawLeftBranch(x, y, sw);
  }

  if (n->right != NULL) {
    nodeclr.red = UINT8_MAX - UINT8_MAX / (nodeheight - 1);
    Neill_SDL_SetDrawColour(sw, nodeclr.red, 
      nodeclr.blue, nodeclr.green);   
    drawRightBranch(x, y, d, sw);
  }

}

void drawLeftBranch(int x, int y, SDL_Simplewin *sw )
{
  int SDLx = (x + 1) * FNTWIDTH + FNTWIDTH / 2;
  int SDLy = (y + 1) * FNTHEIGHT  + PADDING + TOPOFFSET;
  int SDLdy = YOFFSET * FNTHEIGHT - FNTHEIGHT - PADDING * 2;
  
  if (SDL_RenderDrawLine(sw->renderer, 
    SDLx, SDLy, SDLx, SDLy + SDLdy ) != 0) {
      fprintf(stderr, "\nFailed to draw line:  %s\n", SDL_GetError());
      SDL_Quit();
      exit(1);
    }
}

void drawRightBranch(int x, int y, display *d, SDL_Simplewin *sw )
{
  int i = 0, SDLdx;
  int SDLx = (x + 2)* FNTWIDTH + PADDING;
  int SDLy = y * FNTHEIGHT + FNTHEIGHT / 2 + TOPOFFSET;   
  
  while (d->grid[(y * d->xlen) + x + i + 1] == EMPTY) {
    i++;
  }
  SDLdx = i * FNTWIDTH - PADDING * 2;
  
  if (SDL_RenderDrawLine(sw->renderer, 
    SDLx, SDLy, SDLx + SDLdx, SDLy) != 0) {
      fprintf(stderr, "\nFailed to draw line:  %s\n", SDL_GetError());
      SDL_Quit();
      exit(1);
    } 
}

void drawNode(int x, int y, SDL_Simplewin *sw )
{
  int SDLx = (x + 1) * FNTWIDTH + FNTWIDTH / 2;
  int SDLy = y * FNTHEIGHT + FNTHEIGHT / 2  + TOPOFFSET;
  
  Neill_SDL_RenderFillCircle(sw->renderer, SDLx, SDLy, NRADIUS);
}

void drawDisplayGrid(display *d, SDL_Simplewin *sw, 
  fntrow fontdata[FNTCHARS][FNTHEIGHT])
{
  unsigned int i, x = 0, y;
  buffer b = createBuffer(d->xlen);
  
  for (y = 0; y < d->ylen; y++) {
    strncpy(b.str,  d->grid + (y * d->xlen), d->xlen);
    b.str[d->xlen - 1] = '\0';
    for (i = 0; b.str[i]; i++) {
      if (b.str[i] == PNODE) {
        b.str[i] = EMPTY; /* doesn't display the parent node char */
      } 
    }
    Neill_SDL_DrawString(sw, fontdata, b.str, x, y * FNTHEIGHT + TOPOFFSET);
    memset(b.str,'\0',strlen(b.str));
  }
  free(b.str);
}

void initDisplayGrid(display *d, node *root)
{
  unsigned int x,y;
  
  d->ylen = setGridHeight(root, 0, 0) + YOFFSET;
  d->xlen = getRightBranchOffset(root) + XOFFSET;
  
  d->grid = (char *)malloc(sizeof(char) * d->xlen * d->ylen);
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
  
  if (cnt < 2) {
    fprintf(stderr,"ERROR: too few nodes to build tree\n");
    exit(EXIT_FAILURE); 
  }

  return cnt;
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