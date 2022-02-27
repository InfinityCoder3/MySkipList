#include <iostream>

struct Node {
	int val;
	int key;
	Node **next;
	Node(int _val, int _key) : val(_val), key(_key) {}
};
