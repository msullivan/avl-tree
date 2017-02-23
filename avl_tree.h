#ifndef __AVL_TREE_H
#define __AVL_TREE_H

typedef int (*avl_cmp_func)(void *, void *);

//typedef enum avl_balance_t { LEFT_HIGHER, RIGHT_HIGHER, SIDES_EQUAL }
//	avl_balance_t;
typedef enum avl_dir_t { AVL_LEFT, AVL_RIGHT }
	avl_dir_t;

typedef struct avl_node_t {
	struct avl_node_t *parent;
	union {
		struct {
			struct avl_node_t *left;
			struct avl_node_t *right;
		};
		struct avl_node_t *links[2];
	};
	void *data;
	int height;
	int debug;
} avl_node_t;

typedef struct avl_tree_t {
	avl_node_t dummy;

	avl_cmp_func lookup_cmp;
	avl_cmp_func insert_cmp;
} avl_tree_t;

#define AVL_NODE_INIT {NULL, {NULL, NULL}, NULL, 1, -1}

//// Definite API things
void avl_node_init(avl_node_t *node);
int avl_init(avl_tree_t *tree,
             avl_cmp_func lookup_cmp, avl_cmp_func insert_cmp);
avl_node_t *avl_lookup(avl_tree_t *tree, void *data);
void avl_insert(avl_tree_t *tree, avl_node_t *node, void *data);

// node-level stuff that definitely needs exposed
avl_node_t *avl_node_first(avl_node_t *node);
avl_node_t *avl_node_next(avl_node_t *node);
static inline avl_node_t *avl_get_root(avl_tree_t *root) {
	return root->dummy.right; }

void avl_check_invariant(avl_node_t *root);



//// SHOULD THESE BE IN THE PUBLIC INTERFACE??
avl_node_t *avl_node_lookup(avl_node_t *root, void *data, avl_cmp_func cmp);
avl_node_t *avl_node_insert(avl_node_t *root, avl_node_t *data,
                            avl_cmp_func cmp);
void avl_node_delete(avl_node_t *node);


#endif
