/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BlockQueue.h
 * Author: 10256
 *
 * Created on 2017年1月13日, 上午10:30
 */

#include <list>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include "DataFormat.h"

#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H
namespace blockqueue {

    template <class DataT>
    class BlockQueue {
    public:

        BlockQueue(size_t capacity) {
        };

        BlockQueue(const BlockQueue& orig) {
        };

        virtual ~BlockQueue() {
        };

        bool isFull() const {
            return m_queue.size() == m_capacity;
        }

        bool isEmpty() const {
            return m_queue.empty();
        }

        void put(DataT data) {
            std::unique_lock<std::mutex> lk(m_mutex);
            while (isFull()) {
//                std::cout << "BlockQueue::queue is full, put is blocking." << std::endl;
                m_notFull.wait(lk);
            }
            m_queue.push_back(data);
            m_notEmpty.notify_one();
        }

        DataT take(void) {
            std::unique_lock<std::mutex> lk(m_mutex);
            while (isEmpty()) {
//                std::cout << "BlockQueue::queue is empty, take is blocking." << std::endl;
                m_notEmpty.wait(lk);
            }
            DataT data = m_queue.front();
            m_queue.pop_front();
            m_notFull.notify_one();
            return data;
        }
        //void clearAndFreeElements(void);
    private:


        std::list<DataT> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_notEmpty;
        std::condition_variable m_notFull;
        size_t m_capacity;
    };
}
#endif /* BLOCKQUEUE_H */

