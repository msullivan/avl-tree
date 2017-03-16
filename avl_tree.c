#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "avl_tree.h"

static inline avl_dir_t flip_dir(avl_dir_t dir) { return !dir; }

static inline int is_dummy(avl_node_t *node) { return !node->parent; }
static inline int is_root(avl_node_t *node) { return is_dummy(node->parent); }
static inline int tree_height(avl_node_t *n) { return n ? n->height : 0; }

static inline int max(int x, int y) { return x > y ? x : y; }

void avl_node_init(avl_node_t *node) {
	node->parent = NULL;
	node->pdir = -1;
	node->left = NULL;
	node->right = NULL;
	node->data = NULL;
	node->height = 1;
}
int avl_init(avl_tree_t *tree,
             avl_cmp_func *lookup_cmp, avl_cmp_func *insert_cmp,
             avl_combine_func *combine,
             void *arg) {
	avl_node_init(&tree->dummy);
	tree->lookup_cmp = lookup_cmp;
	tree->insert_cmp = insert_cmp;
	tree->combine = combine;
	tree->arg = arg;
	return 0;
}

// core manipulation/query routines
static void avl_update(avl_tree_t *tree, avl_node_t *node) {
	node->height = max(tree_height(node->left), tree_height(node->right)) + 1;
	if (tree->combine) tree->combine(node, tree->arg);
}

static inline void set_child(avl_node_t *root, avl_dir_t dir,
                             avl_node_t *child) {
	root->links[dir] = child;
	if (child) {
		child->parent = root;
		child->pdir = dir;
	}
}

// rotates around node in direction 'dir'
static avl_node_t *avl_rotate(avl_tree_t *tree,
                              avl_node_t *node, avl_dir_t dir) {
	avl_dir_t odir = flip_dir(dir);

	avl_node_t *replacement = node->links[odir];
	if (!replacement)
		return node;

	set_child(node, odir, replacement->links[dir]);
	set_child(replacement, dir, node);

	avl_update(tree, node);
	avl_update(tree, replacement);

	return replacement;
}

// lookup:
// Do a lookup, but also compute where the node would be
// inserted if it *doesn't* exist in the tree.
avl_node_t *avl_core_lookup(avl_tree_t *tree,
                            avl_cmp_func cmp,
                            const void *data,
                            avl_node_t **parent_out,
                            avl_dir_t *dir_out) {
	avl_node_t *parent = &tree->dummy;
	avl_dir_t dir = AVL_RIGHT;
	avl_node_t *node = avl_get_root(tree);
	void *arg = tree->arg;

	while (node) {
		int n = cmp(data, node->data, arg);

		if (n == 0) {
			break;
		} else if (n < 0) {
			dir = AVL_LEFT;
		} else {
			dir = AVL_RIGHT;
		}
		parent = node;
		node = node->links[dir];
	}

	if (parent_out) *parent_out = parent;
	if (dir_out) *dir_out = dir;

	return node;
}

avl_node_t *avl_lookup(avl_tree_t *tree, const void *data) {
	return avl_core_lookup(tree, tree->lookup_cmp, data, NULL, NULL);
}

// find the node that is the closest to where the value /would/ go,
// in direction 'dir'
avl_node_t *avl_lookup_closest(avl_tree_t *tree, const void *data,
                               avl_dir_t dir) {
	avl_node_t *parent;
	avl_dir_t insert_dir;
	avl_node_t *node = avl_core_lookup(tree, tree->lookup_cmp, data,
	                                   &parent, &insert_dir);
	if (node || is_dummy(parent)) return node;
	if (insert_dir != dir) return parent;
	return avl_step(parent, dir);
}
avl_node_t *avl_lookup_ge(avl_tree_t *tree, const void *data) {
	return avl_lookup_closest(tree, data, AVL_RIGHT);
}
avl_node_t *avl_lookup_le(avl_tree_t *tree, const void *data) {
	return avl_lookup_closest(tree, data, AVL_LEFT);
}

// node repair, used by insert and delete
static int balance_factor(avl_node_t *root) {
	return tree_height(root->left) - tree_height(root->right);
}

static void avl_node_repair(avl_tree_t *tree, avl_node_t *root) {
	avl_update(tree, root);
	int bal = balance_factor(root);
	if (abs(bal) <= 1) return;

	// Grab what we need to update parent of the rotation
	avl_dir_t pdir = root->pdir;
	avl_node_t *parent = root->parent;

	// We need to do repair. Which side is too tall?
	avl_dir_t dir = bal >= 1 ? AVL_LEFT : AVL_RIGHT;
	avl_node_t *subtree = root->links[dir];

	// We want to do a rotation so that the too-tall child replaces
	// the root with a too-tall grandchild as its child.
	// (After an insert there is at most one too-tall grandchild.
	// After a delete there might be two.)
	// If a too-tall grandchild is in the same direction as the
	// too-tall child, we can just do one rotation. If not we need to
	// do a rotation in the child tree to get it set up.
	if (tree_height(subtree->links[!dir])+1 == subtree->height) {
		set_child(root, dir, avl_rotate(tree, subtree, dir));
	}
	set_child(parent, pdir, avl_rotate(tree, root, flip_dir(dir)));
}

// Update the structure back up to the root
static void avl_chain_repair(avl_tree_t *tree, avl_node_t *node) {
	while (!is_dummy(node)) {
		avl_node_t *parent = node->parent;
		avl_node_repair(tree, node);
		node = parent;
	}
}

// insert:
// If the value is already in the tree, don't insert anything and
// return the existing node. Otherwise return NULL;
avl_node_t *avl_insert(avl_tree_t *tree, avl_node_t *node, void *data) {
	avl_node_init(node); // should we do this??
	node->data = data;

	avl_node_t *parent;
	avl_dir_t dir;
	avl_node_t *existing = avl_core_lookup(tree, tree->insert_cmp, data,
	                                       &parent, &dir);
	if (existing) return existing;

	set_child(parent, dir, node);
	avl_chain_repair(tree, parent);
	return NULL;
}

// delete:
static void swap_nodes(avl_node_t *node1, avl_node_t *node2) {
	// Swapping tree nodes is a lot easier when you don't care about
	// what object is in use and just can swap data pointers. Oh well.
	// Note that node2 might be node1's child, which means we need to do
	// the parent swap after the child swaps, in order to fix up that case.
	avl_node_t temp = *node1;
	set_child(node1, AVL_LEFT, node2->left);
	set_child(node1, AVL_RIGHT, node2->right);
	set_child(node2, AVL_LEFT, temp.left);
	set_child(node2, AVL_RIGHT, temp.right);

	set_child(node2->parent, node2->pdir, node1);
	set_child(temp.parent, temp.pdir, node2);
}

void avl_node_delete(avl_tree_t *tree, avl_node_t *node) {
	avl_node_t *replacement;
	for (;;) {
		if (!node->left) {
			replacement = node->right;
			break;
		} else if (!node->right) {
			replacement = node->left;
			break;
		} else {
			// this is the hard^Wfun case!
			swap_nodes(node, avl_next(node));
		}
	}

	avl_node_t *tofix = node->parent;
	set_child(tofix, node->pdir, replacement);
	avl_chain_repair(tree, tofix);
}
avl_node_t *avl_delete(avl_tree_t *tree, const void *data) {
	avl_node_t *node = avl_lookup(tree, data);
	if (node) avl_node_delete(tree, node);
	return node;
}

// tree traversal
// find the leftmost or rightmost node, depending on dir
avl_node_t *avl_node_end(avl_node_t *node, avl_dir_t dir) {
	if (!node) return node;
	while (node->links[dir]) {
		node = node->links[dir];
	}
	return node;
}
avl_node_t *avl_node_first(avl_node_t *node) {
	return avl_node_end(node, AVL_LEFT);
}
avl_node_t *avl_node_last(avl_node_t *node) {
	return avl_node_end(node, AVL_RIGHT);
}
// step left or right in the tree
avl_node_t *avl_step(avl_node_t *node, avl_dir_t dir) {
	avl_dir_t odir = flip_dir(dir);
	// Return the leftmost node in our right subtree (or vice versa)
	if (node->links[dir]) {
		return avl_node_end(node->links[dir], odir);
	}
	// No right subtree, so climb back up the tree until we find a node
	// that we are the left child of (or vice versa).
	for (;;) {
		if (is_root(node)) return NULL;
		if (dir != node->pdir) return node->parent;
		node = node->parent;
	}
}
avl_node_t *avl_next(avl_node_t *node) {
	return avl_step(node, AVL_RIGHT);
}
avl_node_t *avl_prev(avl_node_t *node) {
	return avl_step(node, AVL_LEFT);
}

// tree consistency checkers
int avl_check_node(avl_node_t *node) {
	if (!node) return 0;

	assert(node->parent->links[node->pdir] == node);
	assert(abs(balance_factor(node)) <= 1);

	int real_height = max(avl_check_node(node->left),
	                      avl_check_node(node->right)) + 1;
	assert(real_height == node->height);
	return real_height;
}
void avl_check_tree(avl_tree_t *tree) {
	avl_node_t *root = avl_get_root(tree);
	assert(!root || root->parent == &tree->dummy);
	avl_check_node(root);
}
