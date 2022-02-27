#include <iostream>
#include <ctime>
#include <mutex>

std::mutex mtx;	//用于保持线程安全

struct Node {
	int m_key;
	int m_val;
	int m_level;
	//传统链表的结点的设计是，指向下一个结点，跳表的设计是指向一个数组，数组的下标代表第i层的结点
	Node **next;
	Node() {}
	Node(int key, int val, int level) : m_key(key), m_val(val), m_level(level) {
		next = new Node*[level];
	}

	~Node() {
		delete []next;
	}
};

class SkipList
{
private:
	//跳表最大的层数
	int m_maxLevel;
	//跳表当前层数
	int m_skipListLevel;
	//链表表头，方便统一操作
	Node* m_head;
	//元素个数
	int m_size;

public:
	SkipList(int level);
	~SkipList();
	int getRandomLevel();
	Node* createNode(int key, int val, int level);
	void printList();
	int insertNode(int key, int val);
	bool searchElement(int key);
	void deleteElement(int key);
	int size();
};

SkipList::SkipList(int maxLevel)
{
	m_maxLevel = maxLevel;
	m_skipListLevel = 0;
	m_size = 0;

	m_head = new Node(-1, -1, maxLevel);
	//随机数种子，用来生成新插入元素的层数
	srand(time(nullptr));
}

//释放内存
SkipList::~SkipList()
{
	if (m_head == nullptr) {
		return;
	}
	Node* cur = m_head;
	while (cur) {
		Node* node = cur->next[0];
		delete cur->next[0];
		cur->next[0] = nullptr;
		cur = node;
		
	}
	
}

//随机生成一个层数，新加入的元素将被放置在这一层
int SkipList::getRandomLevel()
{
	int k = 0;

	while (rand() % 2) {
		k++;
	}
	k = (k < m_maxLevel) ? k : m_maxLevel;

	return k;
}

//创建一个新的节点
Node* SkipList::createNode(int key, int val, int level)
{
	Node* node = new Node(key, val, level);

	return node;
}

//将跳表按格式打印在终端
void SkipList::printList()
{
	std::cout << "\n******Skip List Begin******\n";
	//从当前最高层开始输出
	for (int i = 0; i < m_skipListLevel; i++) {
		Node *node = m_head->next[i];
		std::cout << "Level " << i << ":";
		while (node != nullptr) {
			std::cout << node->m_key << ":" << node->m_val << ";"; 
			//node走到这一层的下一个结点去，相当于普通单链表的node=node->next;
			node = node->next[i];
		}
		//每一层换一行
		std::cout << std::endl;
	}
}

//插入结点
int SkipList::insertNode(int key, int val) 
{
	//同一时刻只能有一个线程进行操作
	mtx.lock();
	
	Node *curNode = m_head;
	Node **newNextArr = new Node*[m_maxLevel];

	for (int i = m_skipListLevel - 1; i >=0; i--) {
		while (curNode->next[i] != nullptr && curNode->next[i]->m_key < key) {
			curNode = curNode->next[i];
		}
		//每一层都需要更新,但每一层更新的位置是不一样的，有可能是最后一个，也可能是中间位置
		//移动方向为向右，向下
		newNextArr[i] = curNode;
	}

	//当前结点第一层
	curNode = curNode->next[0];

	if (curNode != nullptr && curNode->m_key == key) {
		std::cout << "key: " << key << ", exists." << std::endl;
		mtx.unlock();
		//结点存在返回1
		//也可以改为结点存在时，更改元素val值
		return 1;	
	}

	//此时curNode->m_key一定小于key，并且大于前面一个结点的key
	if (curNode == nullptr || curNode->m_key != key) {
		//开始执行插入准备工作
		//产生被插入到的层数
		int randomLevel = getRandomLevel();

		//如果新产生的层数，大于当前的层数，则需要在高于当前层数的层开始为链表头赋上新值
		if (randomLevel > m_skipListLevel) {
			for (int i = m_skipListLevel; i < randomLevel; i++) {
				newNextArr[i] = m_head;
			}
			m_skipListLevel = randomLevel;
		}
		
		//创建新结点
		Node* newNode = createNode(key, val, randomLevel);
		//真正的插入操作
		for (int i = 0; i < randomLevel; i++) {
			newNode->next[i] = newNextArr[i]->next[i];
			newNextArr[i]->next[i] = newNode;
		}
		std::cout << "insert successfully key: " << key << ", val: " << val << std::endl;
		m_size++;

	}

	mtx.unlock();
	//插入成功返回0
	return 0;
}

//搜索是否存在某个元素
bool SkipList::searchElement(int key)
{
	//搜索可以多个线程同时操作
	std::cout << "start search elemnt...\n";
	Node *cur = m_head;

	for (int i = m_skipListLevel - 1; i >= 0; i--) {
		while (cur->next[i] != nullptr && cur->next[i]->m_key < key) {
			cur = cur->next[i];
		}
	}

	cur = cur->next[0];

	if (cur != nullptr && cur->m_key == key) {
		std::cout << "found key: " << cur->m_key << ", val: " << cur->m_val << std::endl;
		return true;
	}

	std::cout << "not found key: " << key << std::endl;
	return false;
}

//根据key值删除元素
void SkipList::deleteElement(int key)
{
	mtx.lock();

	Node *cur = m_head;
	Node **update = new Node*[m_maxLevel];

	for (int i = m_skipListLevel - 1; i >= 0; i++) {
		while (cur->next[i] && cur->next[i]->m_key < key) {
			cur = cur->next[i];
		}
		update[i] = cur;
	}

	//定位到需要删除的结点处
	cur = cur->next[0];

	if (cur && cur->m_key == key) {
		for (int i = 0; i < m_skipListLevel; i++) {
			//对于不是该节点的层，选择跳过
			if (update[i]->next[i] != cur) break;

			//删除结点
			update[i]->next[i] = cur->next[i];
			delete cur;
			cur = nullptr;
		}

		//如果最高层的结点数为0，删除层数
		while (m_skipListLevel > 0 && m_head->next[m_skipListLevel] == nullptr) {
			m_skipListLevel--;
		}

		std::cout << "successfully delete key: " << key << std::endl;
	}
	

	mtx.unlock();
}

//返回元素个数
int SkipList::size()
{
	return m_size;
}

