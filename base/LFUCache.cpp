#include "LFUCache.h"

void KeyList::init(int freq)
{
    freq_ = freq;
    // Dummyhead_ = tail_= new Node<Key>;
    Dummyhead_ = newElement<Node<Key>>();
    std::cerr << "Dummyhead_ is " << Dummyhead_ << std::endl;
    std::cerr << "memorypool size:" << sizeof(Node<Key>) << std::endl;
    tail_ = Dummyhead_;
    Dummyhead_->setNext(nullptr);
}
// 删除整个链表
void KeyList::destroy()
{
    while (Dummyhead_ != nullptr)
    {
        key_node pre = Dummyhead_;
        Dummyhead_ = Dummyhead_->getNext();
        // delete pre;
        deleteElement(pre);
    }
}
int KeyList::getFreq() { return freq_; }
// 将节点添加到链表头部
void KeyList::add(key_node &node)
{
    if (Dummyhead_->getNext() == nullptr)
    {
        // printf("set tail\n");
        tail_ = node;
        // printf("head_ is %p, tail_ is %p\n", Dummyhead_, tail_);
    }
    else
    {
        Dummyhead_->getNext()->setPre(node);
        // printf("this node is not the first\n");
    }
    node->setNext(Dummyhead_->getNext());
    node->setPre(Dummyhead_);
    Dummyhead_->setNext(node);
    assert(!isEmpty());
}
// 删除⼩链表中的节点
void KeyList::del(key_node &node)
{
    node->getPre()->setNext(node->getNext());
    if (node->getNext() == nullptr)
    {
        tail_ = node->getPre();
    }
    else
    {
        node->getNext()->setPre(node->getPre());
    }
}
bool KeyList::isEmpty()
{
    return Dummyhead_ == tail_;
}
key_node KeyList::getLast()
{
    return tail_;
}
LFUCache::LFUCache(int capacity) : capacity_(capacity)
{
    init();
}
LFUCache::~LFUCache()
{
    while (Dummyhead_)
    {
        freq_node pre = Dummyhead_;
        Dummyhead_ = Dummyhead_->getNext();
        pre->getValue().destroy();
        // delete pre;
        deleteElement(pre);
    }
}
void LFUCache::init()
{
    // FIXME:缓存的容量动态变化

    // Dummyhead_ = new Node<KeyList>();
    Dummyhead_ = newElement<Node<KeyList>>();
    Dummyhead_->getValue().init(0);
    Dummyhead_->setNext(nullptr);
}
// 更新节点频度：
// 如果不存在下⼀个频度的链表，则增加⼀个
// 然后将当前节点放到下⼀个频度的链表的头位置
void LFUCache::addFreq(key_node &nowk, freq_node &nowf)
{
    if (!nowk || !nowf)
    {
        std::cerr << "Error: nowk or nowf is null in addFreq" << std::endl;
        return;
    }

    // Print debug information
    std::cout << "addFreq called with key: " << nowk->getValue().key_
              << " and freq: " << nowf->getValue().getFreq() << std::endl;

    // Handle freq_node allocation and addition
    freq_node nxt;
    if (nowf->getNext() == nullptr || nowf->getNext()->getValue().getFreq() != nowf->getValue().getFreq() + 1) {
        nxt = newElement<Node<KeyList>>();
        // 查看申请内存池大小
        // std::cerr << "memorypool size:" << sizeof(Node<KeyList>) << std::endl;
        if (!nxt) {
            std::cerr << "Error: Failed to allocate memory for freq_node" << std::endl;
            return;
        }
        nxt->getValue().init(nowf->getValue().getFreq() + 1);
        if (nowf->getNext() != nullptr) {
            nowf->getNext()->setPre(nxt);
        }
        nxt->setNext(nowf->getNext());
        nowf->setNext(nxt);
        nxt->setPre(nowf);
    } else {
        nxt = nowf->getNext();
    }

    std::cout << "Adding key to fmap_: " << nowk->getValue().key_ << std::endl;
    fmap_[nowk->getValue().key_] = nxt;

    if (nowf != Dummyhead_) {
        nowf->getValue().del(nowk);
    }
    nxt->getValue().add(nowk);

    std::cout << "addFreq completed. New freq: " << nxt->getValue().getFreq() << std::endl;

    if (nowf != Dummyhead_ && nowf->getValue().isEmpty())
        del(nowf);
}



bool LFUCache::get(string &key, string &val)
{
    if (!capacity_)
        return false;
    MutexLockGuard lock(mutex_);
    if (fmap_.find(key) != fmap_.end())
    {
        // 缓存命中
        key_node nowk = kmap_[key];
        freq_node nowf = fmap_[key];
        val += nowk->getValue().value_;
        addFreq(nowk, nowf);
        return true;
    }
    // 未命中
    return false;
}

void LFUCache::set(string &key, string &val)
{
    if (!capacity_)
        return;

    MutexLockGuard lock(mutex_);

    std::cout << "Setting key: " << key << " with value: " << val << std::endl;

    if (kmap_.size() == capacity_)
    {
        freq_node head = Dummyhead_->getNext();
        key_node last = head->getValue().getLast();
        head->getValue().del(last);
        kmap_.erase(last->getValue().key_);
        fmap_.erase(last->getValue().key_);
        deleteElement(last);

        if (head->getValue().isEmpty())
        {
            del(head);
        }
    }

    key_node nowk = newElement<Node<Key>>();
    // 内存池大小
    std::cerr << "memorypool size:" << sizeof(Node<Key>) << std::endl;
    if (!nowk)
    {
        std::cerr << "Error: Failed to allocate memory for key_node" << std::endl;
        return;
    }
    std::cout << "Allocated new key_node at address: " << nowk << std::endl;

    nowk->getValue().key_ = key;
    nowk->getValue().value_ = val;

    std::cout << "Adding key to cache: " << key << std::endl;
    addFreq(nowk, Dummyhead_);
    kmap_[key] = nowk;
    fmap_[key] = Dummyhead_->getNext();

    std::cout << "Set completed for key: " << key << std::endl;
}


void LFUCache::del(freq_node &node)
{
    node->getPre()->setNext(node->getNext());
    if (node->getNext() != nullptr)
    {
        node->getNext()->setPre(node->getPre());
    }
    node->getValue().destroy();
    // delete node;
    deleteElement(node);
}
LFUCache &getCache()
{
    static LFUCache cache(LFU_CAPACITY); //单模
    return cache;
}
