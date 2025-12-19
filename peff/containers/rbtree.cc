#include "rbtree.h"

using namespace peff;

PEFF_CONTAINERS_API RBTreeBase::AbstractNode::~AbstractNode() {}

PEFF_CONTAINERS_API RBTreeBase::AbstractNode* RBTreeBase::_getMinNode(AbstractNode* node) {
	if (!node)
		return nullptr;

	while (node->l)
		node = node->l;
	return node;
}

PEFF_CONTAINERS_API RBTreeBase::AbstractNode* RBTreeBase::_getMaxNode(AbstractNode* node) {
	if (!node)
		return nullptr;

	while (node->r)
		node = node->r;
	return node;
}

PEFF_CONTAINERS_API void RBTreeBase::_lRot(AbstractNode* x) {
	AbstractNode* y = x->r;
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

PEFF_CONTAINERS_API void RBTreeBase::_rRot(AbstractNode* x) {
	AbstractNode* y = x->l;
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

PEFF_CONTAINERS_API void RBTreeBase::_insertFixUp(AbstractNode* node) {
	AbstractNode* p, * gp = node, * u;  // Parent, grandparent and uncle

	while ((p = gp->p) && _isRed(p)) {
		gp = p->p;

		if (p == gp->l) {
			u = gp->r;

			if (_isRed(u)) {
				p->color = RBColor::Black;
				u->color = RBColor::Black;
				gp->color = RBColor::Red;
				node = gp;
				continue;
			}
			else {
				if (node == p->r) {
					_lRot(p);
					std::swap(node, p);
				}
				_rRot(gp);
				p->color = RBColor::Black;
				gp->color = RBColor::Red;
			}
		}
		else {
			u = gp->l;

			if (_isRed(u)) {
				p->color = RBColor::Black;
				u->color = RBColor::Black;
				gp->color = RBColor::Red;
				node = gp;
				continue;
			}
			else {
				if (node == p->l) {
					_rRot(p);
					std::swap(node, p);
				}
				_lRot(gp);
				p->color = RBColor::Black;
				gp->color = RBColor::Red;
			}
		}
	}

	_root->color = RBColor::Black;
}

PEFF_CONTAINERS_API RBTreeBase::AbstractNode* RBTreeBase::_removeFixUp(AbstractNode* node) {
	// From SGI STL's stl_tree, with some minor improvements.
	AbstractNode* y = node, * x, * p;

	if (!y->l)
		// The node has right child only.
		x = y->r;
	else if (!y->r) {
		// The node has left child only.
		x = y->l;
	}
	else {
		// The node has two children.
		y = _getMinNode(y->r);
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

	if (_isBlack(y)) {
		while (x != _root && _isBlack(x)) {
			if (x == p->l) {
				auto w = p->r;

				if (_isRed(w)) {
					w->color = RBColor::Black;
					p->color = RBColor::Red;
					_lRot(p);
					w = p->r;
				}

				if (_isBlack(w->l) && _isBlack(w->r)) {
					w->color = RBColor::Red;
					x = p;
					p = p->p;
				}
				else {
					if (_isBlack(w->r)) {
						if (w->l)
							w->l->color = RBColor::Black;
						w->color = RBColor::Red;
						_rRot(w);
						w = p->r;
					}
					w->color = p->color;
					p->color = RBColor::Black;
					if (w->r)
						w->r->color = RBColor::Black;
					_lRot(p);
					break;
				}
			}
			else {
				auto w = p->l;

				if (_isRed(w)) {
					w->color = RBColor::Black;
					p->color = RBColor::Red;
					_rRot(p);
					w = p->l;
				}

				if (_isBlack(w->r) && _isBlack(w->l)) {
					w->color = RBColor::Red;
					x = p;
					p = p->p;
				}
				else {
					if (_isBlack(w->l)) {
						if (w->r)
							w->r->color = RBColor::Black;
						w->color = RBColor::Red;
						_lRot(w);
						w = p->l;
					}
					w->color = p->color;
					p->color = RBColor::Black;
					if (w->l)
						w->l->color = RBColor::Black;
					_rRot(p);
					break;
				}
			}
		}
		if (x)
			x->color = RBColor::Black;
	}

	return y;
}

PEFF_CONTAINERS_API void RBTreeBase::_verify(AbstractNode* node, const size_t nBlack, size_t cntBlack) const {
	if (!node) {
		// We have reached a terminal node.
		if (nBlack != cntBlack)
			throw std::logic_error("Inequal black node counts detected");
		return;
	}

	if (_isRed(node) && _isRed(node->p))
		throw std::logic_error("Connected red nodes detected");

	if (_isBlack(node))
		++cntBlack;

	_verify(node->l, nBlack, cntBlack);
	_verify(node->r, nBlack, cntBlack);
}

PEFF_CONTAINERS_API void RBTreeBase::_verify() const {
	if (!_root)
		return;

	if (_isRed(_root))
		throw std::logic_error("Red root node detected");

	size_t nBlack = 0;
	for (AbstractNode* i = _root; i; i = i->l) {
		if (_isBlack(i))
			++nBlack;
	}

	_verify(_root, nBlack, 0);
}

PEFF_CONTAINERS_API RBTreeBase::AbstractNode* RBTreeBase::_getNextNode(const AbstractNode* node, const AbstractNode* lastNode) noexcept {
	assert(node);

	if (node != lastNode) {
		if (node->r) {
			return _getMinNode(node->r);
		}
		else {
			while (node->p && (node == node->p->r))
				node = node->p;
			return node->p;
		}
	}

	return nullptr;
}

PEFF_CONTAINERS_API RBTreeBase::AbstractNode* RBTreeBase::_getPrevNode(const AbstractNode* node, const AbstractNode* firstNode) noexcept {
	assert(node);

	if (node != firstNode) {
		if (node->l) {
			return _getMaxNode(node->l);
		}
		else {
			while (node->p && (node == node->p->l))
				node = node->p;
			return node->p;
		}
	}

	return nullptr;
}

PEFF_CONTAINERS_API RBTreeBase::RBTreeBase() {
}

PEFF_CONTAINERS_API RBTreeBase::~RBTreeBase() {
}
