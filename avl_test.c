#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "avl_tree.h"

#define INT_TO_P(x) ((void *)(long)(x))
#define P_TO_INT(p) ((int)(long)(p))


void avl_debug(avl_node_t *root)
{
	int l, r;
	if (!root)
		return;

	l = root->left ? P_TO_INT(root->left->data) : 0;
	r = root->right ? P_TO_INT(root->right->data) : 0;

	avl_debug(root->left);
	printf("%p-%d-%d(%d,%d) ",
	       root, P_TO_INT(root->data), root->height, l, r);
	avl_debug(root->right);
}

void avl_display(avl_node_t *root, int level)
{
	int c;
	if (!root)
		return;

	avl_display(root->left, level+1);
	for (c = 2*level; c; c--)
		putchar(' ');
//	printf("%02d\n", root->debug);
	printf("%02d\n", P_TO_INT(root->data));

	avl_display(root->right, level+1);
}

void avl_iterate(avl_tree_t *tree) {
	for (avl_node_t *node = avl_first(tree); node;
		 node = avl_node_next(node)) {

		printf("%d ", P_TO_INT(node->data));
	}
	printf("\n");
}

int test_print(void *p)
{
	printf("%d ", P_TO_INT(p));
	return 0;
}

int test_cmp(void *p, void *q)
{
	return P_TO_INT(p) - P_TO_INT(q);
}

#define NUM_ELEMS 100
#define NUM_DELS 10
#define MAX_VAL 100

void debug_crap(avl_tree_t *tree) {
	avl_node_t *root = avl_get_root(tree);
	avl_debug(root); printf("\n");
	avl_display(root, 0);
	avl_check_tree(tree);
	avl_iterate(tree);
}

int gen() {
//	static int num = 0; return ++num;
	return rand();
}

int main(void)
{
	int n, i;
	avl_tree_t stree;
	avl_tree_t *tree = &stree;
	avl_init(tree, test_cmp, test_cmp);

	for (i = 0; i < NUM_ELEMS; i++) {
		n = gen() % MAX_VAL;
		if (avl_lookup(tree, INT_TO_P(n))) continue;

		avl_node_t *node = calloc(1, sizeof(*node));
		printf("inserting %d\n", n);
		avl_insert(tree, node, INT_TO_P(n));

		debug_crap(tree);
	}

	for (i = 0; i < NUM_DELS; i++) {
		n = gen() % MAX_VAL;
		avl_node_t *node = avl_lookup(tree, INT_TO_P(n));
		if (!node) continue;

		printf("deleting %d\n", n);
		avl_node_delete(node);


		debug_crap(tree);
	}


	return 0;
}
