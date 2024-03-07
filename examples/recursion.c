#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define FRAMED_IMPLEMENTATION
#include "public/framed.h"

typedef struct TreeNode TreeNode;
struct TreeNode
{
  TreeNode *left;
  TreeNode *right;
  int value;
};

void tree_print(TreeNode *node)
{
  framed_zone_begin("tree_print");

  if (node->left)
  {
    tree_print(node->left);
  }

  printf("%d ", node->value);

  if (node->right)
  {
    tree_print(node->right);
  }

  framed_zone_end();
}

TreeNode *tree_insert(TreeNode *tree, int value)
{
  framed_zone_begin("tree_insert");

  if (!tree)
  {
    tree = calloc(1, sizeof(TreeNode));
    tree->value = value;
  }
  else if (value < tree->value)
  {
    tree->left = tree_insert(tree->left, value);
  }
  else
  {
    tree->right = tree_insert(tree->right, value);
  }

  framed_zone_end();
  return tree;
}

int main(int argc, char **argv)
{
  framed_init(true);

  framed_zone_begin("main");

  TreeNode *tree = 0;
  for (int i = 0; i < 10000; ++i)
  {
    tree = tree_insert(tree, rand());
  }

  tree_print(tree);

  framed_zone_end();

  framed_flush();
}
