void ClearSubtree(Node*& subtree);

Node* CopyNode(Node* other);

void RenderPixel(PNG &img, Node* pixel, unsigned int scale) const;

void assignColor(Node*& subtree, string mode);

void FlipNode(Node*& subtree);

void positionNormal(Node*& subtree, int width1, int height1);

void RotateSubtree(Node*& subtree);

void coordinateHelper(Node*& subtree, int width1, int width2, int height1);

void PruneNode(Node*& subtree, double tolerance);

void DeleteChildren(Node*& subtree);

bool shouldPrune(Node* subtree, RGBAPixel a, double tolerance);