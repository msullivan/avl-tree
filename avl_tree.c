#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "avl_tree.h"



#define TREE_HEIGHT(n) ((n) ? ((n)->height) : 0)

static inline int is_root(avl_node_t *node) { return !node->parent; }
static inline void set_root(avl_tree_t *root, avl_node_t *child);
static inline avl_dir_t get_parent_dir(avl_node_t *node);
static inline void set_child(avl_node_t *root, avl_dir_t dir,
                             avl_node_t *child);
static inline void set_left(avl_node_t *root, avl_node_t *child);
static inline void set_right(avl_node_t *root, avl_node_t *child);
static avl_node_t *avl_rotate_right(avl_node_t *node);
static avl_node_t *avl_rotate_left(avl_node_t *node);
static void avl_update_height(avl_node_t *node);
void avl_check_invariant(avl_node_t *root);

avl_node_t *avl_node_next(avl_node_t *node);

void avl_node_init(avl_node_t *node)
{
	node->parent = NULL;
	node->left = NULL;
	node->right = NULL;
	node->data = NULL;
	node->height = 1;
}
int avl_init(avl_tree_t *tree,
             avl_cmp_func lookup_cmp, avl_cmp_func insert_cmp)
{
	avl_node_init(&tree->dummy);
	tree->lookup_cmp = lookup_cmp;
	tree->insert_cmp = insert_cmp;
	return 0;
}

// lookup:
avl_node_t *avl_node_lookup(avl_node_t *root, void *data, avl_cmp_func cmp)
{
	while (root) {
		int n = cmp(data, root->data);
		if (n == 0) {
			break;
		} else if (n < 0) {
			root = root->left;
		} else {
			root = root->right;
		}
	}

	return root;
}

avl_node_t *avl_lookup(avl_tree_t *tree, void *data)
{
	return avl_node_lookup(avl_get_root(tree), data, tree->lookup_cmp);
}

// insert:
static avl_node_t *avl_node_repair(avl_node_t *root)
{
	int lh = TREE_HEIGHT(root->left);
	int rh = TREE_HEIGHT(root->right);

	// Needs repair.
	if (lh > rh+1) {
		avl_node_t *subtree = root->left;
		assert(subtree);
		int llh = TREE_HEIGHT(subtree->left);

		if (llh == subtree->height-1) {
			root = avl_rotate_right(root);
		} else {
			set_left(root, avl_rotate_left(subtree));
			root = avl_rotate_right(root);
		}
	} else if (rh > lh+1) {
		avl_node_t *subtree = root->right;
		assert(subtree);
		int rrh = TREE_HEIGHT(subtree->right);

		if (rrh == subtree->height-1) {
			root = avl_rotate_left(root);
		} else {
			set_right(root, avl_rotate_right(subtree));
			root = avl_rotate_left(root);
		}
	}

	return root;
}


avl_node_t *avl_node_insert(avl_node_t *root, avl_node_t *data,
                            avl_cmp_func cmp)
{
	if (!root) {
		return data;
	}

	int n = cmp(data->data, root->data);

	if (n == 0) {
		/* aw, shit. figure out way to signal this */
		return root;
	} else if (n < 0) {
		set_left(root, avl_node_insert(root->left, data, cmp));
	} else {
		set_right(root, avl_node_insert(root->right, data, cmp));
	}

	root = avl_node_repair(root);

	avl_update_height(root);
	return root;
}

void avl_insert(avl_tree_t *tree, avl_node_t *node, void *data) {
	avl_node_init(node); // should we do this??
	node->data = data;
	set_root(tree,
	         avl_node_insert(avl_get_root(tree), node, tree->insert_cmp));
}

// delete:
static void swap_nodes(avl_node_t *node1, avl_node_t *node2) {
	avl_node_t temp = *node1;
	set_left(node1, node2->left);
	set_right(node1, node2->right);
	set_left(node2, temp.left);
	set_right(node2, temp.right);

	avl_dir_t node1_pdir = get_parent_dir(node1);
	avl_dir_t node2_pdir = get_parent_dir(node2);

	set_child(node2->parent, node2_pdir, node1);
	set_child(temp.parent, node1_pdir, node2);
}
// replaces *in the parent*
static void replace_node(avl_node_t *old_node, avl_node_t *new_node) {
	avl_dir_t old_pdir = get_parent_dir(old_node);
	set_child(old_node->parent, old_pdir, new_node);
}

void avl_node_delete(avl_node_t *node) {
	avl_node_t *replacement;
	for (;;) {
		if (!node->left) {
			replacement = node->right;
			break;
		} else if (!node->right) {
			replacement = node->left;
			break;
		} else {
			// This is the hard case!
			avl_node_t *next = avl_node_next(node);
			swap_nodes(node, next);
		}
	}

	avl_node_t *tofix = node->parent;
	replace_node(node, replacement);

	/* now we need to climb the tree repairing nodes */
	while (!is_root(tofix)) {
		avl_dir_t pdir = get_parent_dir(tofix);
		avl_node_t *parent = tofix->parent;
		avl_node_t *fixed = avl_node_repair(tofix);
		if (fixed != tofix) {
			//printf("rotated at %d\n", tofix->debug);
			set_child(parent, pdir, fixed);
		}
		avl_update_height(tofix);
		tofix = parent;
	}
}




//
avl_node_t *avl_node_first(avl_node_t *node) {
	if (!node) return node;
	while (node->left) {
		node = node->left;
	}
	return node;
}

avl_node_t *avl_node_next(avl_node_t *node) {
	if (!node) return NULL;
	if (node->right) {
		return avl_node_first(node->right);
	}
	while (node->parent && node == node->parent->right) {
		node = node->parent;
	}
	assert(!node->parent || node == node->parent->left);
	return node->parent;
}


void avl_check_invariant(avl_node_t *root) {
	int left_height, right_height;

	if (!root)
		return;

	assert(!root->left || root->left->parent == root);
	assert(!root->right || root->right->parent == root);

	left_height = TREE_HEIGHT(root->left);
	right_height = TREE_HEIGHT(root->right);

	if (abs(left_height - right_height) > 1) {
		fprintf(stderr,
		        "invariant violated at %p(%p/%zd)! heights: left=%d, right=%d\n",
		        root, root->data, (size_t)root->data,
		        left_height, right_height);
		abort();
	}

	avl_check_invariant(root->left);
	avl_check_invariant(root->right);
}

static void avl_update_height(avl_node_t *node)
{
	int left_height = TREE_HEIGHT(node->left);
	int right_height = TREE_HEIGHT(node->right);

	node->height =
		(left_height > right_height ? left_height : right_height) + 1;
}

static inline void set_root(avl_tree_t *tree, avl_node_t *child) {
	set_right(&tree->dummy, child);
}
static inline void set_child(avl_node_t *root, avl_dir_t dir,
                             avl_node_t *child) {
	root->links[dir] = child;
	if (child) child->parent = root;
}
static inline avl_dir_t get_parent_dir(avl_node_t *node) {
	avl_node_t *parent = node->parent;
	if (parent->left == node) return AVL_LEFT;
	assert(parent->right == node);
	return AVL_RIGHT;
}
static inline void set_left(avl_node_t *root, avl_node_t *child) {
	set_child(root, AVL_LEFT, child);
}
static inline void set_right(avl_node_t *root, avl_node_t *child) {
	set_child(root, AVL_RIGHT, child);
}

static avl_node_t *avl_rotate_right(avl_node_t *node)
{
	avl_node_t *replacement = node->left;
	if (!replacement)
		return node;

	set_left(node, replacement->right);
	set_right(replacement, node);

	avl_update_height(replacement->right);
	avl_update_height(replacement);

	return replacement;
}

static avl_node_t *avl_rotate_left(avl_node_t *node)
{
	avl_node_t *replacement = node->right;
	if (!replacement)
		return node;

	set_right(node, replacement->left);
	set_left(replacement, node);

	avl_update_height(replacement->left);
	avl_update_height(replacement);

	return replacement;
}
