/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockQueue.cpp
 * Author: 10256
 * 
 * Created on 2017年1月13日, 上午10:30
 */

#include "BlockQueue.h"
#include "ByteBuffer.h"
#include <iostream>
using namespace std;
using namespace blockqueue;

template <class DataT>
BlockQueue<DataT>::BlockQueue(size_t capacity) : m_capacity(capacity) {}

template <class DataT>
BlockQueue<DataT>::BlockQueue(const BlockQueue& orig) {
}

template <class DataT>
BlockQueue<DataT>::~BlockQueue() {
    clearAndFreeElements();
}

template <class DataT>
void BlockQueue<DataT>::put(DataT data) {
    
    unique_lock<mutex> lk(m_mutex);
    while (isFull()) {
//        cout << "BlockQueue::queue is full, put is blocking." << endl;
        m_notFull.wait(lk);
    }
    m_queue.push_back(data);
    m_notEmpty.notify_one();
}
template <class DataT>
DataT BlockQueue<DataT>::take() {
    unique_lock<mutex> lk(m_mutex);
    while(isEmpty()) {
//        cout << "BlockQueue::queue is empty, take is blocking." << endl;
        m_notEmpty.wait(lk);        
    }
    DataT* data = m_queue.front();
    m_queue.pop_front();
    m_notFull.notify_one();    
    return data;
}
template <class DataT>
void BlockQueue<DataT>::clearAndFreeElements(void) {
    
    for(typename list<DataT>::iterator iter = m_queue.begin(); iter != m_queue.end();){
        delete *iter;
        iter = m_queue.erase(iter);
    }
    
}