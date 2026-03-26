#include "rbtree.h"

using namespace peff;

PEFF_CONTAINERS_API RBTreeBase::NodeBase *RBTreeBase::_get_min_node(NodeBase *node) noexcept {
	if (!node)
		return nullptr;

	while (node->l)
		node = node->l;
	return node;
}

PEFF_CONTAINERS_API RBTreeBase::NodeBase *RBTreeBase::_get_max_node(NodeBase *node) noexcept {
	if (!node)
		return nullptr;

	while (node->r)
		node = node->r;
	return node;
}

PEFF_CONTAINERS_API void RBTreeBase::_l_rot(NodeBase *x) noexcept {
	NodeBase* y = x->r;
	assert(y);

	x->r = y->l;
	if (y->l)
		y->l->p = x;

	y->p = x->p;

	if (!x->p)
		_root = y;
	else if (x->p->l == x)
		x->p->l = y;
	else
		x->p->r = y;

	y->l = x;
	x->p = y;
}

PEFF_CONTAINERS_API void RBTreeBase::_r_rot(NodeBase *x) noexcept {
	NodeBase* y = x->l;
	assert(y);

	x->l = y->r;
	if (y->r)
		y->r->p = x;

	y->p = x->p;
	if (!x->p)
		_root = y;
	else if (x->p->l == x)
		x->p->l = y;
	else
		x->p->r = y;

	y->r = x;
	x->p = y;
}

PEFF_CONTAINERS_API void RBTreeBase::_insert_fix_up(NodeBase *node) noexcept {
	NodeBase* p, * gp = node, * u;  // Parent, grandparent and uncle

	while ((p = gp->p) && _is_red(p)) {
		gp = p->p;

		if (p == gp->l) {
			u = gp->r;

			if (_is_red(u)) {
				p->color = RBColor::Black;
				u->color = RBColor::Black;
				gp->color = RBColor::Red;
				node = gp;
				continue;
			}
			else {
				if (node == p->r) {
					_l_rot(p);
					std::swap(node, p);
				}
				_r_rot(gp);
				p->color = RBColor::Black;
				gp->color = RBColor::Red;
			}
		}
		else {
			u = gp->l;

			if (_is_red(u)) {
				p->color = RBColor::Black;
				u->color = RBColor::Black;
				gp->color = RBColor::Red;
				node = gp;
				continue;
			}
			else {
				if (node == p->l) {
					_r_rot(p);
					std::swap(node, p);
				}
				_l_rot(gp);
				p->color = RBColor::Black;
				gp->color = RBColor::Red;
			}
		}
	}

	_root->color = RBColor::Black;
}

PEFF_CONTAINERS_API RBTreeBase::NodeBase *RBTreeBase::_remove_fix_up(NodeBase *node) noexcept {
	// From SGI STL's stl_tree, with some minor improvements.
	NodeBase* y = node, * x, * p;

	if (!y->l)
		// The node has right child only.
		x = y->r;
	else if (!y->r) {
		// The node has left child only.
		x = y->l;
	}
	else {
		// The node has two children.
		y = _get_min_node(y->r);
		x = y->r;
	}

	if (y != node) {
		node->l->p = y;
		y->l = node->l;

		if (y != node->r) {
			p = y->p;
			if (x)
				x->p = y->p;
			y->p->l = x;
			y->r = node->r;
			node->r->p = y;
		}
		else
			p = y;

		if (_root == node)
			_root = y;
		else if (node->p->l == node)
			node->p->l = y;
		else
			node->p->r = y;

		y->p = node->p;
		std::swap(y->color, node->color);
		y = node;
	}
	else {
		p = y->p;
		if (x)
			x->p = y->p;

		if (_root == node)
			_root = x;
		else if (node->p->l == node)
			node->p->l = x;
		else
			node->p->r = x;
	}

	if (_is_black(y)) {
		while (x != _root && _is_black(x)) {
			if (x == p->l) {
				auto w = p->r;

				if (_is_red(w)) {
					w->color = RBColor::Black;
					p->color = RBColor::Red;
					_l_rot(p);
					w = p->r;
				}

				if (_is_black(w->l) && _is_black(w->r)) {
					w->color = RBColor::Red;
					x = p;
					p = p->p;
				}
				else {
					if (_is_black(w->r)) {
						if (w->l)
							w->l->color = RBColor::Black;
						w->color = RBColor::Red;
						_r_rot(w);
						w = p->r;
					}
					w->color = p->color;
					p->color = RBColor::Black;
					if (w->r)
						w->r->color = RBColor::Black;
					_l_rot(p);
					break;
				}
			}
			else {
				auto w = p->l;

				if (_is_red(w)) {
					w->color = RBColor::Black;
					p->color = RBColor::Red;
					_r_rot(p);
					w = p->l;
				}

				if (_is_black(w->r) && _is_black(w->l)) {
					w->color = RBColor::Red;
					x = p;
					p = p->p;
				}
				else {
					if (_is_black(w->l)) {
						if (w->r)
							w->r->color = RBColor::Black;
						w->color = RBColor::Red;
						_l_rot(w);
						w = p->l;
					}
					w->color = p->color;
					p->color = RBColor::Black;
					if (w->l)
						w->l->color = RBColor::Black;
					_r_rot(p);
					break;
				}
			}
		}
		if (x)
			x->color = RBColor::Black;
	}

	return y;
}

PEFF_CONTAINERS_API void RBTreeBase::_verify(NodeBase *node, const size_t num_black, size_t black_count) const noexcept {
	if (!node) {
		// We have reached a terminal node.
		if (num_black != black_count)
			// Inequal black node counts detected
			std::terminate();
		return;
	}

	if (_is_red(node) && _is_red(node->p))
		// Connected red nodes detected
		std::terminate();

	if (_is_black(node))
		++black_count;

	_verify(node->l, num_black, black_count);
	_verify(node->r, num_black, black_count);
}

PEFF_CONTAINERS_API void RBTreeBase::_verify() const noexcept {
	if (!_root)
		return;

	if (_is_red(_root))
		// Red root node detected
		std::terminate();

	size_t num_black = 0;
	for (NodeBase* i = _root; i; i = i->l) {
		if (_is_black(i))
			++num_black;
	}

	_verify(_root, num_black, 0);
}

PEFF_CONTAINERS_API RBTreeBase::NodeBase* RBTreeBase::_get_next_node(const NodeBase* node, const NodeBase* last_node) noexcept {
	assert(node);

	if (node != last_node) {
		if (node->r) {
			return _get_min_node(node->r);
		}
		else {
			while (node->p && (node == node->p->r))
				node = node->p;
			return node->p;
		}
	}

	return nullptr;
}

PEFF_CONTAINERS_API RBTreeBase::NodeBase* RBTreeBase::_get_prev_node(const NodeBase* node, const NodeBase* first_node) noexcept {
	assert(node);

	if (node != first_node) {
		if (node->l) {
			return _get_max_node(node->l);
		}
		else {
			while (node->p && (node == node->p->l))
				node = node->p;
			return node->p;
		}
	}

	return nullptr;
}

PEFF_CONTAINERS_API RBTreeBase::RBTreeBase() noexcept {
}

PEFF_CONTAINERS_API RBTreeBase::~RBTreeBase() {
}
