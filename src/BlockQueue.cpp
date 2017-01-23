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

BlockQueue::BlockQueue(size_t capacity) : m_capacity(capacity) {}

BlockQueue::BlockQueue(const BlockQueue& orig) {
}

BlockQueue::~BlockQueue() {
    clearAndFreeElements();
}

void BlockQueue::put(CarData* carData) {
    
    unique_lock<mutex> lk(m_mutex);
    while (isFull()) {
        cout << "queue is full, put is blocking." << endl;
        m_notFull.wait(lk);
    }
    m_queue.push_back(carData);
    m_notEmpty.notify_one();
}

CarData* BlockQueue::take() {
    unique_lock<mutex> lk(m_mutex);
    while(isEmpty()) {
        cout << "queue is empty, take is blocking." << endl;
        m_notEmpty.wait(lk);        
    }
    CarData* carData = m_queue.front();
    m_queue.pop_front();
    m_notFull.notify_one();    
    return carData;
}

void BlockQueue::clearAndFreeElements(void) {
    for(std::list<CarData*>::iterator iter = m_queue.begin(); iter != m_queue.end();){
        delete *iter;
        iter = m_queue.erase(iter);
    }
    
}