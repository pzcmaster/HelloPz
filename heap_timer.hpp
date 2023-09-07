#ifndef _LIB_SRC_HEAP_TIMER_H_
#define _LIB_SRC_HEAP_TIMER_H_


#include <iostream>
#include "timer_common.hpp"


/*
 * @Author: MGH
 * @Date: 2021-09-29 13:01:04
 * @Last Modified by: Author
 * @Last Modified time: 2021-09-29 13:01:04
 * @Description: Heap Timer
*/

#define HEAP_DEFAULT_SIZE 128



// 定时器数据结构的定义
template <typename _User_Data>
class HeapTimer
{
public:
    HeapTimer() = default;
    HeapTimer(int msec)
    {
        timer.setTimeout(msec);
    }

    ~HeapTimer()
    {

    }

    void setTimeout(time_t timeout)
    {
        timer.setTimeout(timeout);
    }

    time_t getExpire()
    {
        return timer.getExpire();
    }

    void setUserData(_User_Data *userData)
    {
        timer.setUserData(userData);
    }

    int getPos()
    {
        return _pos;
    }

    void setPos(int pos)
    {
        this->_pos = pos;
    }

    void handleTimeOut()
    {
        timer.handleTimeOut();    
    }

    using TimeOutCbFunc = void (*)(_User_Data *);
    void setCallBack(TimeOutCbFunc callBack)
    {
        timer.setCallBack(callBack);
    }

public:
    Timer<_User_Data> timer;  

private:
      
    int _pos;                          // 保存该定时器在数组中的位置，以便查找删除操作            
};


// 定时容器，使用最小堆实现
template <typename _UData>
class HeapTimerContainer : public ITimerContainer<_UData> 
{
public:
    HeapTimerContainer();
    HeapTimerContainer(int capacity);
    HeapTimerContainer(HeapTimer<_UData> **initArray, int arrSize, int capacity);
    virtual ~HeapTimerContainer() override;

public:
    virtual void tick() override;               
    Timer<_UData> *addTimer(time_t timeout)  override;
    void delTimer(Timer<_UData> *timer)  override;
    void resetTimer(Timer<_UData> *timer, time_t timeout)  override;
    int getMinExpire() override;
    Timer<_UData> *top();
    void popTimer();

private:
    void percolateDown(int hole);
    void percolateUp(int hole);
    void resize();
    bool isEmpty();

private:
    HeapTimer<_UData> **_array;              // 堆数据
    int _capacity;                           // 堆数组的容量
    int _size;                               // 当前包含的元素
};


template <typename _UData>
HeapTimerContainer<_UData>::HeapTimerContainer() : HeapTimerContainer(HEAP_DEFAULT_SIZE)
{

}

// 初始化一个大小为cap的空堆
template <typename _UData>
HeapTimerContainer<_UData>::HeapTimerContainer(int capacity)
{
    this->_capacity = capacity;
    this->_size = 0;

    _array = new HeapTimer<_UData> *[capacity]{nullptr};
}

// 用已有数组来初始化堆 
template <typename _UData>
HeapTimerContainer<_UData>::HeapTimerContainer(HeapTimer<_UData> **initArray, int arrSize, int capacity) :
    _size(arrSize)
{
    if(capacity < arrSize) 
    {
        this->_capacity = capacity = 2 * arrSize;
    }

    _array = new HeapTimer<_UData> *[capacity];
    for (int i = 0; i < capacity; i++)
    {
        _array[i] = nullptr;
    }

    if(arrSize > 0) 
    {
        for (int i = 0; i < arrSize; i++)
        {
           _array[i] = initArray[i]; 
        }
        
        for(int i = (_size - 1) / 2; i >= 0; i--)
        {
            percolateDown(i);       //对数组中的第(_size - 1) / 2 ~ 0个元素执行下滤操作
        }
    }
    
}

template <typename _UData>
HeapTimerContainer<_UData>::~HeapTimerContainer()
{
    if(_array)
    {
        for(int i = 0; i < _size; i++) 
        {
            delete _array[i];
        }
        delete []_array;
    }
}

template <typename _UData>
void HeapTimerContainer<_UData>::tick()
{
    std::cout << "----------tick----------" << std::endl;
    HeapTimer<_UData> *tmp = _array[0];
    time_t cur = getMSec();
    // 循环处理到期的定时器
    while(!isEmpty())
    {
        if(!tmp)
        {
            break;
        }

        // 如果定时器没到期，则退出循环
        if(tmp->getExpire() > cur)
        {
            break;
        }

        tmp->handleTimeOut();
        // 将堆顶元素删除，同时生成新的堆顶定时器
        popTimer();
        tmp = _array[0];
    }
}

// 获取一个定时器
template <typename _UData>
Timer<_UData> *HeapTimerContainer<_UData>::addTimer(time_t timeout)
{
    if(_size >= _capacity)
    {
        this->resize();             //如果容量不够，则进行扩容
    }

    // hole是新建空穴的位置
    int hole = _size++;
    HeapTimer<_UData> *timer = new HeapTimer<_UData>(timeout);
    _array[hole] = timer;

    percolateUp(hole);

    return &timer->timer;
}

// 删除目标定时器
template <typename _UData>
void HeapTimerContainer<_UData>::delTimer(Timer<_UData> *timer)
{
    if(!timer) 
    {
        return ;
    }

    /* 仅仅将目标定时器的数据设置为空，延迟销毁
       等定时器超时再删除该定时器
     */
    timer->setCallBack(nullptr);
    timer->setUserData(nullptr);

}

// 重置一个定时器
template <typename _UData>
void HeapTimerContainer<_UData>::resetTimer(Timer<_UData> *timer, time_t timeout)
{
    // 类型强转
    HeapTimer<_UData> *htimer = reinterpret_cast< HeapTimer<_UData>* >(timer);

    // 找到该定时器在数组中的位置，将其与最后一个定时器的位置交换，然后先进行下滤操作，再进行上滤操作
    int pos = htimer->getPos();
    int lastPos = _size - 1;
    if(pos != lastPos)
    {
        HeapTimer<_UData> *temp = _array[pos];
        _array[pos] = _array[lastPos];
        _array[lastPos] = temp;
    }
    timer->setTimeout(timeout);

    // 下滤 上滤
    percolateDown(pos);
    percolateUp(lastPos);

}

// 获取容器中超时值最小的值
template <typename _UData>
int HeapTimerContainer<_UData>::getMinExpire()
{
    Timer<_UData> * timer = top();
    if(timer)
    {
        return timer->getExpire();
    }

    return -1;
}

// 获得顶部的定时器
template <typename _UData>
Timer<_UData> *HeapTimerContainer<_UData>::top()
{
    if(isEmpty())
    {
        return nullptr;
    }

    return &_array[0]->timer;
}

template <typename _UData>
void HeapTimerContainer<_UData>::popTimer()
{
    if(isEmpty())
    {
        return;
    }

    if(_array[0])
    {
        delete _array[0];
        // 将原来的堆顶元素替换为堆数组中最后一个元素
        _array[0] = _array[--_size];
        // 对新的堆顶元素执行下滤操作
        percolateDown(0);
    }
}

// 最小堆的下滤操作，它确保数组中以第hole个节点作为根的子树拥有最小堆性质
template <typename _UData>
void HeapTimerContainer<_UData>::percolateDown(int hole)
{
    if(_size == 0)
    {
        return ;
    }
    HeapTimer<_UData> *temp = _array[hole];
    int child = 0;
    for(; ((hole * 2 + 1) <= _size - 1); hole = child)
    {
        child = hole * 2 + 1;
        if((child < (_size - 1)) && (_array[child + 1]->getExpire() < _array[child]->getExpire()))
        {
            child++;
        }

        if(_array[child]->getExpire() < temp->getExpire())
        {
            _array[hole] = _array[child];
            _array[hole]->setPos(hole);             // 调整定时器的位置时，重新设置timer中pos保存的其在数组中的位置
        }
        else 
        {
            break;
        }
    }

    _array[hole] = temp;
    _array[hole]->setPos(hole);
}

template <typename _UData>
void HeapTimerContainer<_UData>::percolateUp(int hole)
{
    int parent = 0;
    HeapTimer<_UData> *temp = _array[hole];

    // 对从空穴到根节点的路径上的所有节点执行上滤操作
    for(; hole > 0; hole = parent)
    {
        parent = (hole - 1) / 2;
        // 将新插入节点的超时值与父节点比较，如果父节点的值小于等于该节点的值，那么就无需再调整了。否则将父节点下移，继续这个操作。
        if(_array[parent]->getExpire() <= temp->getExpire())   
        {
            break;
        }
        _array[hole] = _array[parent];
        _array[hole]->setPos(hole);
    }

    _array[hole] = temp;
    _array[hole]->setPos(hole);
    
}

// 将数组的容量扩大一倍
template <typename _UData>
void HeapTimerContainer<_UData>::resize()
{
    HeapTimer<_UData> **temp = new HeapTimer<_UData> *[2 * _capacity];
    _capacity = 2 * _capacity;
    

    for(int i = 0; i < _size; i++)
    {
        temp[i] = _array[i];
    }

    for(int i = _size; i < _capacity; i++)
    {
        temp[i] = nullptr;
    }

    delete []_array;
    _array = temp;
}

template <typename _UData>
bool HeapTimerContainer<_UData>::isEmpty()
{
    return _size == 0;
}

#endif
