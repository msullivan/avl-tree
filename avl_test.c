#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "avl_tree.h"

#define INT_TO_P(x) ((void *)(long)(x))
#define P_TO_INT(p) ((int)(long)(p))

///
// Some test routines that I can't decide whether are worth including.
// The main motivation here is that there should be some reasonable sort
// of way to deconstruct a binary tree in linear time.
// I try two approaches:
// 1. Turn the tree into a list that can be traversed and freed
// 2. Write code to do a postorder traversal so that we can
//    free children followed by the parent.

// Turn a tree into a singly linked list, linked through ->right.
// Leaves the old values in ->left and ->parent, which are now all
// nonsense. Clears out tree.
// Returns the head of the list (previously the leftmost element).
//
// This depends on knowing that the ->right pointer won't be
// inspected after a node has already been returned while
// traversing backwards.
avl_node_t *flatten_tree(avl_tree_t *tree) {
	avl_node_t *next = NULL;
	for (avl_node_t *node = avl_last(tree); node; node = avl_prev(node)) {
		node->right = next;
		next = node;
	}
	tree->dummy.right = NULL;
	return next;
}

//
static inline int is_root(avl_node_t *node) { return !node->parent->parent;}
avl_node_t *postorder_node_first(avl_node_t *node) {
	if (!node) return node;
	for (;;) {
		if (node->left) node = node->left;
		else if (node->right) node = node->right;
		else return node;
	}
}
avl_node_t *postorder_first(avl_tree_t *tree) {
	return postorder_node_first(avl_get_root(tree));
}
avl_node_t *postorder_next(avl_node_t *node) {
	if (is_root(node)) return NULL;
	if (node->pdir == AVL_LEFT) {
		avl_node_t *next = postorder_node_first(node->parent->right);
		if (next) return next;
	}
	return node->parent;
}

////////


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

	avl_display(root->right, level+1);

	for (c = 2*level; c; c--)
		putchar(' ');
	printf("%02d\n", P_TO_INT(root->data));

	avl_display(root->left, level+1);
}

void avl_iterate(avl_tree_t *tree) {
	printf("forward: ");
	for (avl_node_t *node = avl_first(tree); node; node = avl_next(node)) {
		printf("%d ", P_TO_INT(node->data));
	}
	printf("\n");

	printf("backward: ");
	for (avl_node_t *node = avl_last(tree); node; node = avl_prev(node)) {
		printf("%d ", P_TO_INT(node->data));
	}
	printf("\n");

	printf("postorder: ");
	for (avl_node_t *node = postorder_first(tree); node;
		 node = postorder_next(node)) {
		printf("%d ", P_TO_INT(node->data));
	}
	printf("\n");
}

int test_print(void *p)
{
	printf("%d ", P_TO_INT(p));
	return 0;
}

int test_cmp(const void *p, const void *q, void *arg)
{
	return P_TO_INT(p) - P_TO_INT(q);
}

void debug_crap(avl_tree_t *tree) {
	avl_node_t *root = avl_get_root(tree);
	avl_debug(root); printf("\n");
	avl_display(root, 0);
	avl_check_tree(tree);
	avl_iterate(tree);
	printf("\n");
}

int gen() {
//	static int num = 0; return ++num;
	return rand();
}

void lookup2_test(avl_tree_t *tree, int n) {
	avl_node_t *node = avl_lookup_ge(tree, INT_TO_P(n));
	printf("lge(%d) = %d\n", n, node ? P_TO_INT(node->data) : -1);
	node = avl_lookup_le(tree, INT_TO_P(n));
	printf("lle(%d) = %d\n", n, node ? P_TO_INT(node->data) : -1);
}
void delete_tree(avl_tree_t *tree) {
	avl_node_t *head = flatten_tree(tree);
	avl_node_t *next;
	for (avl_node_t *node = head; node; node = next) {
		next = node->right;
		free(node);
	}
}
void delete_tree2(avl_tree_t *tree) {
	avl_node_t *next;
	for (avl_node_t *node = postorder_first(tree); node; node = next) {
		next = postorder_next(node);
		free(node);
	}
}

int main(int argc, char **argv)
{
	int num_elems = 100;
	int num_dels = 50;
	int max_val = 1000;
	int debug = 1;

	int n, i;
	avl_tree_t stree;
	avl_tree_t *tree = &stree;
	avl_init(tree, test_cmp, test_cmp, NULL, NULL);

	for (i = 0; i < num_elems; i++) {
		n = gen() % max_val;
		if (avl_lookup(tree, INT_TO_P(n))) continue;

		avl_node_t *node = calloc(1, sizeof(*node));
		if (debug) printf("inserting %d\n", n);
		avl_insert(tree, node, INT_TO_P(n));

		if (debug) debug_crap(tree);
	}

	for (i = 0; i < num_dels; ) {
		n = gen() % max_val;
		avl_node_t *node = avl_lookup(tree, INT_TO_P(n));
		if (!node) continue;

		if (debug) printf("deleting %d\n", n);
		avl_node_delete(tree, node);
		free(node);

		if (debug) debug_crap(tree);
		i++;
	}

	lookup2_test(tree, 5);
	lookup2_test(tree, 327);
	lookup2_test(tree, 328);
	lookup2_test(tree, 1000);

	delete_tree(tree);

	return 0;
}
