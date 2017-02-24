#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "avl_tree.h"

#define TREE_HEIGHT(n) ((n) ? ((n)->height) : 0)

static inline avl_dir_t flip_dir(avl_dir_t dir) { return !dir; }

static inline int is_dummy(avl_node_t *node) { return !node->parent; }
static inline int is_root(avl_node_t *node) { return is_dummy(node->parent); }

static inline int max(int x, int y) { return x > y ? x : y; }

void avl_node_init(avl_node_t *node)
{
	node->parent = NULL;
	node->left = NULL;
	node->right = NULL;
	node->data = NULL;
	node->height = 1;
}
int avl_init(avl_tree_t *tree,
             avl_cmp_func lookup_cmp, avl_cmp_func insert_cmp, void *arg)
{
	avl_node_init(&tree->dummy);
	tree->lookup_cmp = lookup_cmp;
	tree->insert_cmp = insert_cmp;
	tree->arg = arg;
	return 0;
}

// core manipulation/query routines
static void avl_update_height(avl_node_t *node) {
	node->height = max(TREE_HEIGHT(node->left), TREE_HEIGHT(node->right)) + 1;
}

static inline void set_child(avl_node_t *root, avl_dir_t dir,
                             avl_node_t *child) {
	root->links[dir] = child;
	if (child) child->parent = root;
	avl_update_height(root);
}
static inline avl_dir_t get_parent_dir(avl_node_t *node) {
	avl_node_t *parent = node->parent;
	if (parent->left == node) return AVL_LEFT;
	assert(parent->right == node);
	return AVL_RIGHT;
}

// rotates around node in direction 'dir'
static avl_node_t *avl_rotate(avl_node_t *node, avl_dir_t dir) {
	avl_dir_t odir = flip_dir(dir);

	avl_node_t *replacement = node->links[odir];
	if (!replacement)
		return node;

	set_child(node, odir, replacement->links[dir]);
	set_child(replacement, dir, node);

	return replacement;
}

// lookup:
/* Do a lookup, but also compute where the node would be
 * inserted if it *does't* exist in the tree. */
avl_node_t *avl_core_lookup(avl_tree_t *tree,
                            avl_cmp_func cmp,
                            void *data,
                            avl_node_t **parent_out,
                            avl_dir_t *dir_out)
{
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

avl_node_t *avl_lookup(avl_tree_t *tree, void *data) {
	return avl_core_lookup(tree, tree->lookup_cmp, data, NULL, NULL);
}

/* find the node that is the closest to where the value /would/ go,
 * in direction 'dir' */
avl_node_t *avl_lookup_close(avl_tree_t *tree, void *data, avl_dir_t dir) {
	avl_node_t *parent;
	avl_dir_t insert_dir;
	avl_node_t *node = avl_core_lookup(tree, tree->lookup_cmp, data,
	                                   &parent, &insert_dir);
	if (node || is_dummy(parent)) return node;
	if (insert_dir != dir) return parent;
	return avl_step(parent, dir);
}
avl_node_t *avl_lookup_ge(avl_tree_t *tree, void *data) {
	return avl_lookup_close(tree, data, AVL_RIGHT);
}
avl_node_t *avl_lookup_le(avl_tree_t *tree, void *data) {
	return avl_lookup_close(tree, data, AVL_LEFT);
}

// node repair, used by insert and delete
static int balance_factor(avl_node_t *root) {
	return TREE_HEIGHT(root->left) - TREE_HEIGHT(root->right);
}

static void avl_node_repair(avl_node_t *root)
{
	avl_update_height(root);
	int bal = balance_factor(root);
	if (abs(bal) <= 1) return;

	// Grab what we need to update parent of the rotation
	avl_dir_t pdir = get_parent_dir(root);
	avl_node_t *parent = root->parent;

	// We need to do repair. Which side is too tall?
	avl_dir_t dir = bal >= 1 ? AVL_LEFT : AVL_RIGHT;

	// Needs repair.
	avl_node_t *subtree = root->links[dir];
	// At most one grandchild subtree can be too tall.
	// If it is in the same direction as the too tall child,
	// we can just do one rotation. If not we need to do
	// a rotation in the child tree to get it set up.
	if (TREE_HEIGHT(subtree->links[dir])+1 != subtree->height) {
		set_child(root, dir, avl_rotate(subtree, dir));
	}
	set_child(parent, pdir, avl_rotate(root, flip_dir(dir)));
}

/* Update the structure back up to the root */
static void avl_chain_repair(avl_node_t *node) {
	while (!is_dummy(node)) {
		avl_node_t *parent = node->parent;
		avl_node_repair(node);
		node = parent;
	}
}

// insert:
void avl_insert(avl_tree_t *tree, avl_node_t *node, void *data) {
	avl_node_init(node); // should we do this??
	node->data = data;

	avl_node_t *parent;
	avl_dir_t dir;
	avl_node_t *existing = avl_core_lookup(tree, tree->insert_cmp, data,
	                                       &parent, &dir);
	assert(!existing); // XXX: do something less awful?

	set_child(parent, dir, node);
	avl_chain_repair(parent);
}

// delete:
static void swap_nodes(avl_node_t *node1, avl_node_t *node2) {
	// Swapping tree nodes is a lot easier when you don't care about
	// what object is in use and just can swap data pointers. Oh well.
	avl_node_t temp = *node1;
	set_child(node1, AVL_LEFT, node2->left);
	set_child(node1, AVL_RIGHT, node2->right);
	set_child(node2, AVL_LEFT, temp.left);
	set_child(node2, AVL_RIGHT, temp.right);

	avl_dir_t node1_pdir = get_parent_dir(node1);
	avl_dir_t node2_pdir = get_parent_dir(node2);

	set_child(node2->parent, node2_pdir, node1);
	set_child(temp.parent, node1_pdir, node2);
}
/* replaces *in the parent* */
static void replace_node(avl_node_t *old_node, avl_node_t *new_node) {
	avl_dir_t old_pdir = get_parent_dir(old_node);
	set_child(old_node->parent, old_pdir, new_node);
}

void avl_delete(avl_tree_t *tree, avl_node_t *node) {
	avl_node_t *replacement;
	for (;;) {
		if (!node->left) {
			replacement = node->right;
			break;
		} else if (!node->right) {
			replacement = node->left;
			break;
		} else {
			/* this is the hard case! */
			avl_node_t *next = avl_next(node);
			swap_nodes(node, next);
		}
	}

	avl_node_t *tofix = node->parent;
	replace_node(node, replacement);
	avl_chain_repair(tofix);
}

// tree traversal
/* find the leftmost or rightmost node, depending on dir */
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
/* step left or right in the tree */
avl_node_t *avl_step(avl_node_t *node, avl_dir_t dir) {
	avl_dir_t odir = flip_dir(dir);
	if (!node) return NULL;
	if (node->links[dir]) {
		return avl_node_end(node->links[dir], odir);
	}
	// there is some annoying fiddliness/redundancy in the stopping
	// condition, since it actually differs between left and right
	// traversal.
	while (node->parent && node == node->parent->links[dir]) {
		node = node->parent;
	}
	if (!node->parent || is_dummy(node->parent)) return NULL;
	assert(node == node->parent->links[odir]);
	return node->parent;
}
avl_node_t *avl_next(avl_node_t *node) {
	return avl_step(node, AVL_RIGHT);
}
avl_node_t *avl_prev(avl_node_t *node) {
	return avl_step(node, AVL_LEFT);
}

// tree consistency checkers
int avl_check_node(avl_node_t *node) {
	int left_height, right_height;

	if (!node) return 0;

	assert(!node->left || node->left->parent == node);
	assert(!node->right || node->right->parent == node);

	left_height = TREE_HEIGHT(node->left);
	right_height = TREE_HEIGHT(node->right);

	if (abs(left_height - right_height) > 1) {
		fprintf(stderr,
		        "invariant violated at %p(%p/%zd)! height: left=%d, right=%d\n",
		        node, node->data, (size_t)node->data,
		        left_height, right_height);
		abort();
	}

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
