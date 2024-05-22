#include "qtree.h"
#include <cmath>

/**
 * Constructor that builds a QTree out of the given PNG.
 * Every leaf in the tree corresponds to a pixel in the PNG.
 * Every non-leaf node corresponds to a rectangle of pixels
 * in the original PNG, represented by an (x,y) pair for the
 * upper left corner of the rectangle and an (x,y) pair for
 * lower right corner of the rectangle. In addition, the Node
 * stores a pixel representing the average color over the
 * rectangle.
 *
 * The average color for each node in your implementation MUST
 * be determined in constant time. HINT: this will lead to nodes
 * at shallower levels of the tree to accumulate some error in their
 * average color value, but we will accept this consequence in
 * exchange for faster tree construction.
 * Note that we will be looking for specific color values in our
 * autograder, so if you instead perform a slow but accurate
 * average color computation, you will likely fail the test cases!
 *
 * Every node's children correspond to a partition of the
 * node's rectangle into (up to) four smaller rectangles. The node's
 * rectangle is split evenly (or as close to evenly as possible)
 * along both horizontal and vertical axes. If an even split along
 * the vertical axis is not possible, the extra line will be included
 * in the left side; If an even split along the horizontal axis is not
 * possible, the extra line will be included in the upper side.
 * If a single-pixel-wide rectangle needs to be split, the NE and SE children
 * will be null; likewise if a single-pixel-tall rectangle needs to be split,
 * the SW and SE children will be null.
 *
 * In this way, each of the children's rectangles together will have coordinates
 * that when combined, completely cover the original rectangle's image
 * region and do not overlap.
 */
QTree::QTree(const PNG& imIn) {
	height = imIn.height();
	width = imIn.width();
	root = BuildNode(imIn, make_pair(0,0), make_pair(width - 1, height - 1));
}

/**
 * Overloaded assignment operator for QTrees.
 * Part of the Big Three that we must define because the class
 * allocates dynamic memory. This depends on your implementation
 * of the copy and clear funtions.
 *
 * @param rhs The right hand side of the assignment statement.
 */
QTree& QTree::operator=(const QTree& rhs) {
	Copy(rhs);
	return *this;
}

/**
 * Render returns a PNG image consisting of the pixels
 * stored in the tree. may be used on pruned trees. Draws
 * every leaf node's rectangle onto a PNG canvas using the
 * average color stored in the node.
 *
 * For up-scaled images, no color interpolation will be done;
 * each rectangle is fully rendered into a larger rectangular region.
 *
 * @param scale multiplier for each horizontal/vertical dimension
 * @pre scale > 0
 */
PNG QTree::Render(unsigned int scale) const {
	PNG img(width * scale, height * scale);
	RenderPixel(img, root, scale);
	return img;
}

void QTree::RenderPixel(PNG &img, Node* node, unsigned int scale) const {
	if (node == nullptr) {
		return;
	}

	int x = node->lowRight.first - node->upLeft.first + 1;
	int y = node->lowRight.second - node->upLeft.second + 1;

	if (scale == 1) {
		for (int i = node->upLeft.first; i<node->upLeft.first + x; i++) {
			for (int j = node->upLeft.second; j<node->upLeft.second + y; j++) {
				RGBAPixel* pixel = img.getPixel(i, j);
				*pixel = node->avg;
			}
		}

	} else {
		for (unsigned int i = 0; i < scale; i++) {
			for (unsigned int j = 0; j < scale; j++) {
				RGBAPixel* pixel = img.getPixel(node->upLeft.first * scale + i, node->upLeft.second * scale + j);
				*pixel = node->avg;
			}
		}
	}

	RenderPixel(img, node->NW, scale);
	RenderPixel(img, node->NE, scale);
	RenderPixel(img, node->SW, scale);
	RenderPixel(img, node->SE, scale);
}

/**
 *  Prune function trims subtrees as high as possible in the tree.
 *  A subtree is pruned (cleared) if all of the subtree's leaves are within
 *  tolerance of the average color stored in the root of the subtree.
 *  NOTE - you may use the distanceTo function found in RGBAPixel.h
 *  Pruning criteria should be evaluated on the original tree, not
 *  on any pruned subtree. (we only expect that trees would be pruned once.)
 *
 * @param tolerance maximum RGBA distance to qualify for pruning
 * @pre this tree has not previously been pruned, nor is copied from a previously pruned tree.
 */
void QTree::Prune(double tolerance) {
	PruneNode(root, tolerance);
}

void QTree::PruneNode(Node*& subtree, double tolerance) {

	if (subtree == nullptr || ((subtree->NW == nullptr) && (subtree->NE == nullptr) && (subtree->SW == nullptr) && (subtree->SE == nullptr))) {
		return;
	}

	if (shouldPrune(subtree, subtree->avg, tolerance)) {
		DeleteChildren(subtree);
	}

	PruneNode(subtree->NW, tolerance);
	PruneNode(subtree->NE, tolerance);
	PruneNode(subtree->SW, tolerance);
	PruneNode(subtree->SE, tolerance);
}

bool QTree::shouldPrune(Node* subtree, RGBAPixel a, double tolerance) {
	if (subtree == nullptr) {
		return true;
	}
	if ((subtree->NW == nullptr) && (subtree->NE == nullptr) && (subtree->SW == nullptr) && (subtree->SE == nullptr)) {  //leaf node
		return (subtree->avg.distanceTo(a) <= tolerance);
	}
	return shouldPrune(subtree->NW, a, tolerance) && shouldPrune(subtree->NE, a, tolerance) && shouldPrune(subtree->SW, a, tolerance) && shouldPrune(subtree->SE, a, tolerance); 
}

void QTree::DeleteChildren(Node*& subtree) {
	if (subtree == nullptr || ((subtree->NW == nullptr) && (subtree->NE == nullptr) && (subtree->SW == nullptr) && (subtree->SE == nullptr))) {
		return;
	}

	DeleteChildren(subtree->NW);
	DeleteChildren(subtree->NE);
	DeleteChildren(subtree->SW);
	DeleteChildren(subtree->SE);

	if (subtree->NW != nullptr) {
		delete subtree->NW;
		subtree->NW = NULL;
	}
	if (subtree->NE != nullptr) {
		delete subtree->NE;
		subtree->NE = NULL;
	}
	if (subtree->SW != nullptr) {
		delete subtree->SW;
		subtree->SW = NULL;
	}
	if (subtree->SE != nullptr) {
		delete subtree->SE;
		subtree->SE = NULL;
	}
}

/**
 *  FlipHorizontal rearranges the contents of the tree, so that
 *  its rendered image will appear mirrored across a vertical axis.
 *  This may be called on a previously pruned/flipped/rotated tree.
 *
 *  After flipping, the NW/NE/SW/SE pointers must map to what will be
 *  physically rendered in the respective NW/NE/SW/SE corners, but it
 *  is no longer necessary to ensure that 1-pixel wide rectangles have
 *  null eastern children
 */
void QTree::FlipHorizontal() {
	FlipNode(root);
}

void QTree::FlipNode(Node*& subtree) {
	if (subtree == nullptr || ((subtree->NW == nullptr) && (subtree->NE == nullptr) && (subtree->SW == nullptr) && (subtree->SE == nullptr))) {
		return;
	}
	int width1, height1;

	if (subtree->NW == nullptr && subtree->SW == nullptr) {
		width1 = subtree->lowRight.first - subtree->upLeft.first + 1 - (subtree->NE->lowRight.first - subtree->NE->upLeft.first + 1); //total width - NE width
		height1 = subtree->NE->lowRight.second - subtree->NE->upLeft.second + 1; //NE height
	} else if (subtree->NW == nullptr && subtree->NE == nullptr) {
		width1 = subtree->SW->lowRight.first - subtree->SW->upLeft.first + 1; //sw width
		height1 = subtree->lowRight.second - subtree->upLeft.second + 1 - (subtree->SW->lowRight.second - subtree->SW->upLeft.second + 1); //total height - SW height
	} else {
		width1 = subtree->NW->lowRight.first - subtree->NW->upLeft.first + 1; //nw width
		height1 = subtree->NW->lowRight.second - subtree->NW->upLeft.second + 1; //nw height
	}

	std::swap(subtree->NW, subtree->NE);
	std::swap(subtree->SW, subtree->SE);

	positionNormal(subtree, width1, height1);

	FlipNode(subtree->NW);
	FlipNode(subtree->NE);
	FlipNode(subtree->SW);
	FlipNode(subtree->SE);
}

void QTree::positionNormal(Node*& subtree, int width1, int height1) {
	if (subtree->NE != nullptr) {
		subtree->NE->upLeft = make_pair(subtree->lowRight.first - width1 + 1, subtree->upLeft.second);
		subtree->NE->lowRight = make_pair(subtree->lowRight.first,  subtree->upLeft.second + height1 - 1);
	}
	if (subtree->NW != nullptr) {
		subtree->NW->upLeft = make_pair(subtree->upLeft.first, subtree->upLeft.second);
		subtree->NW->lowRight = make_pair(subtree->lowRight.first - width1, subtree->upLeft.second + height1 - 1);
	}
	if (subtree->SE != nullptr) {
		subtree->SE->upLeft = make_pair(subtree->lowRight.first - width1 + 1, subtree->upLeft.second + height1);
		subtree->SE->lowRight = make_pair(subtree->lowRight.first, subtree->lowRight.second);
	}
	if (subtree->SW != nullptr) {
		subtree->SW->upLeft = make_pair(subtree->upLeft.first, subtree->upLeft.second + height1);
		subtree->SW->lowRight = make_pair(subtree->lowRight.first - width1, subtree->lowRight.second);
	}
}


/**
 *  RotateCCW rearranges the contents of the tree, so that its
 *  rendered image will appear rotated by 90 degrees counter-clockwise.
 *  This may be called on a previously pruned/flipped/rotated tree.
 *
 *  After rotation, the NW/NE/SW/SE pointers must map to what will be
 *  physically rendered in the respective NW/NE/SW/SE corners, but it
 *  is no longer necessary to ensure that 1-pixel tall or wide rectangles
 *  have null eastern or southern children
 *  (i.e. after rotation, a node's NW and NE pointers may be null, but have
 *  non-null SW and SE, or it may have null NW/SW but non-null NE/SE)
 */
void QTree::RotateCCW() {
	std::swap(height, width);
	std::swap(root->lowRight.first, root->lowRight.second);
	RotateSubtree(root);
	
}

void QTree::RotateSubtree(Node*& subtree) {
	if (subtree == nullptr || ((subtree->NW == nullptr) && (subtree->NE == nullptr) && (subtree->SW == nullptr) && (subtree->SE == nullptr))) {
		return;
	}

	int width1, width2, height1;

	if (subtree->NW == nullptr && subtree->SW == nullptr) {
		width1 = subtree->lowRight.second - subtree->upLeft.second + 1 - (subtree->NE->lowRight.first - subtree->NE->upLeft.first + 1); //total width - NE width
		height1 = subtree->NE->lowRight.second - subtree->NE->upLeft.second + 1; //NE height
		width2 = subtree->NE->lowRight.first - subtree->NE->upLeft.first + 1; //ne width
	} else if (subtree->NW == nullptr && subtree->NE == nullptr) {
		width1 = subtree->SW->lowRight.first - subtree->SW->upLeft.first + 1; //sw width
		width2 = subtree->SE->lowRight.first - subtree->SE->upLeft.first + 1; //se width
		height1 = subtree->lowRight.first - subtree->upLeft.first + 1 - (subtree->SW->lowRight.second - subtree->SW->upLeft.second + 1); //total height - SW height
	} else if (subtree->NE == nullptr && subtree->SE == nullptr) {
		width2 = subtree->lowRight.second - subtree->upLeft.second + 1 - (subtree->NW->lowRight.first - subtree->NW->upLeft.first + 1); //total width - NW width
		width1 = subtree->NW->lowRight.first - subtree->NW->upLeft.first + 1; //nw width
		height1 = subtree->NW->lowRight.second - subtree->NW->upLeft.second + 1; //nw height
	} else {
		width1 = subtree->NW->lowRight.first - subtree->NW->upLeft.first + 1; //nw width
		width2 = subtree->NE->lowRight.first - subtree->NE->upLeft.first + 1; //ne width
		height1 = subtree->NW->lowRight.second - subtree->NW->upLeft.second + 1; //nw height
	}

	coordinateHelper(subtree, width1, width2, height1);
	
	Node* temp = subtree->SE;
	subtree->SE = subtree->SW;
	subtree->SW = subtree->NW;
	subtree->NW = subtree->NE;
	subtree->NE = temp;

	RotateSubtree(subtree->NW);
	RotateSubtree(subtree->NE);
	RotateSubtree(subtree->SW);
	RotateSubtree(subtree->SE);
}


void QTree::coordinateHelper(Node*& subtree, int width1, int width2, int height1) {
	if (subtree->NW != nullptr) {
		subtree->NW->upLeft = make_pair(subtree->upLeft.first, subtree->upLeft.second + width2);
		subtree->NW->lowRight = make_pair(subtree->upLeft.first + height1 - 1, subtree->lowRight.second);
	}
	if (subtree->NE != nullptr) {
		subtree->NE->upLeft = make_pair(subtree->upLeft.first, subtree->upLeft.second);
		subtree->NE->lowRight = make_pair(subtree->upLeft.first + height1 - 1, subtree->upLeft.second + width2 - 1);
	}
	if (subtree->SW != nullptr) {
		subtree->SW->upLeft = make_pair(subtree->upLeft.first + height1, subtree->upLeft.second + width2);
		subtree->SW->lowRight = make_pair(subtree->lowRight.first, subtree->lowRight.second);
	}
	if (subtree->SE != nullptr) {
		subtree->SE->upLeft = make_pair(subtree->upLeft.first + height1, subtree->upLeft.second);
		subtree->SE->lowRight = make_pair(subtree->lowRight.first, subtree->upLeft.second + width2 - 1);
	}
}


/**
 * Destroys all dynamically allocated memory associated with the
 * current QTree object.
 */
void QTree:: Clear() {
	ClearSubtree(root);
}

void QTree:: ClearSubtree(Node*& subtree) {
	if (subtree == nullptr) {
		return;
	} else {
		ClearSubtree(subtree->NW);
		ClearSubtree(subtree->NE);
		ClearSubtree(subtree->SW);
		ClearSubtree(subtree->SE);
		delete subtree;
		subtree = NULL;
	}
}

/**
 * Copies the parameter other QTree into the current QTree.
 * Does not free any memory. Called by copy constructor and operator=.
 * @param other The QTree to be copied.
 */
void QTree::Copy(const QTree& other) {
	height = other.height;
	width = other.width;
	root = CopyNode(other.root);
}

Node* QTree::CopyNode(Node* other) {
	if (other == nullptr) {
		return NULL;
	} else {
		Node* subtree = new Node(other->upLeft, other->lowRight, other->avg);
		subtree->NW = CopyNode(other->NW);
		subtree->NE = CopyNode(other->NE);
		subtree->SW = CopyNode(other->SW);
		subtree->SE = CopyNode(other->SE);
		return subtree;
	}
}



/**
 * Private helper function for the constructor. Recursively builds
 * the tree according to the specification of the constructor.
 * @param img reference to the original input image.
 * @param ul upper left point of current node's rectangle.
 * @param lr lower right point of current node's rectangle.
 */
Node* QTree::BuildNode(const PNG& img, pair<unsigned int, unsigned int> ul, pair<unsigned int, unsigned int> lr) {
	Node* subtree = new Node(ul, lr, RGBAPixel());
	int x = lr.first - ul.first + 1;
	int y = lr.second - ul.second + 1;

	double halfw = ceil(x/2.0);
	double halfh = ceil(y/2.0);

	if (x == 1 && y == 1) {               //if is a leaf node
		subtree->NW = NULL;
		subtree->NE = NULL;
		subtree->SW = NULL;
		subtree->SE = NULL;
		subtree->avg = *img.getPixel(ul.first, ul.second);
	} else if (x == 1 && y != 1) {        //vertical
		subtree->NW = BuildNode(img, ul, make_pair(ul.first + halfw - 1, ul.second + halfh - 1));
		subtree->SW = BuildNode(img, make_pair(ul.first, ul.second + halfh), make_pair(ul.first + halfw - 1, lr.second));
		subtree->NE = NULL;
		subtree->SE = NULL;

		assignColor(subtree, "v");	
	} else if (y == 1 && x != 1) {        //horizontal
		subtree->NW = BuildNode(img, ul, make_pair(ul.first + halfw - 1, ul.second + halfh - 1));
		subtree->NE = BuildNode(img, make_pair(ul.first + halfw, ul.second), make_pair(lr.first, ul.second + halfh - 1));
		subtree->SW = NULL;
		subtree->SE = NULL;

		assignColor(subtree, "h");
	} else {
		subtree->NW = BuildNode(img, ul, make_pair(ul.first + halfw - 1, ul.second + halfh - 1));
		subtree->NE = BuildNode(img, make_pair(ul.first + halfw, ul.second), make_pair(lr.first, ul.second + halfh - 1));
		subtree->SW = BuildNode(img, make_pair(ul.first, ul.second + halfh), make_pair(ul.first + halfw - 1, lr.second));
		subtree->SE = BuildNode(img, make_pair(ul.first + halfw, ul.second + halfh), lr);

		assignColor(subtree, "else");
	}

	return subtree;
}

void QTree::assignColor(Node*& subtree, string mode) {
	int nwArea = 0;
	int neArea = 0;
	int swArea = 0;
	int seArea = 0;
	int totalArea = 0;
	if (mode == "v") {
		nwArea = (subtree->NW->lowRight.second - subtree->NW->upLeft.second + 1) * (subtree->NW->lowRight.first - subtree->NW->upLeft.first + 1);
		swArea = (subtree->SW->lowRight.second - subtree->SW->upLeft.second + 1) * (subtree->SW->lowRight.first - subtree->SW->upLeft.first + 1);
		totalArea = nwArea + swArea;

		subtree->avg.r = (nwArea * subtree->NW->avg.r + swArea * subtree->SW->avg.r) / totalArea;
		subtree->avg.g = (nwArea * subtree->NW->avg.g + swArea * subtree->SW->avg.g) / totalArea;
		subtree->avg.b = (nwArea * subtree->NW->avg.b + swArea * subtree->SW->avg.b) / totalArea;
	} else if (mode == "h") {
		nwArea = (subtree->NW->lowRight.second - subtree->NW->upLeft.second + 1) * (subtree->NW->lowRight.first - subtree->NW->upLeft.first + 1);
		neArea = (subtree->NE->lowRight.second - subtree->NE->upLeft.second + 1) * (subtree->NE->lowRight.first - subtree->NE->upLeft.first + 1);
		totalArea = nwArea + neArea;

		subtree->avg.r = (nwArea * subtree->NW->avg.r + neArea * subtree->NE->avg.r) / totalArea;
		subtree->avg.g = (nwArea * subtree->NW->avg.g + neArea * subtree->NE->avg.g) / totalArea;
		subtree->avg.b = (nwArea * subtree->NW->avg.b + neArea * subtree->NE->avg.b) / totalArea;
	} else {
		nwArea = (subtree->NW->lowRight.second - subtree->NW->upLeft.second + 1) * (subtree->NW->lowRight.first - subtree->NW->upLeft.first + 1);
		neArea = (subtree->NE->lowRight.second - subtree->NE->upLeft.second + 1) * (subtree->NE->lowRight.first - subtree->NE->upLeft.first + 1);
		swArea = (subtree->SW->lowRight.second - subtree->SW->upLeft.second + 1) * (subtree->SW->lowRight.first - subtree->SW->upLeft.first + 1);
		seArea = (subtree->SE->lowRight.second - subtree->SE->upLeft.second + 1) * (subtree->SE->lowRight.first - subtree->SE->upLeft.first + 1);
		totalArea = nwArea + neArea + swArea +seArea;
		
		subtree->avg.r = (nwArea * subtree->NW->avg.r + neArea * subtree->NE->avg.r + swArea * subtree->SW->avg.r + seArea * subtree->SE->avg.r) / totalArea;
		subtree->avg.g = (nwArea * subtree->NW->avg.g + neArea * subtree->NE->avg.g + swArea * subtree->SW->avg.g + seArea * subtree->SE->avg.g) / totalArea;
		subtree->avg.b = (nwArea * subtree->NW->avg.b + neArea * subtree->NE->avg.b + swArea * subtree->SW->avg.b + seArea * subtree->SE->avg.b) / totalArea;
	}
}
