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
#if UseBoostMutex
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#else
#include <mutex>
#include <condition_variable>
#endif
#include <iostream>
#include "DataFormat.h"

#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H
namespace blockqueue {

    template <class DataT>
    class BlockQueue {
        // we should use refrence instead of copy
        BlockQueue(const BlockQueue& orig) {
        }

        std::list<DataT> m_queue;
#if UseBoostMutex
        boost::mutex m_mutex;
        boost::condition_variable m_notEmpty;
        boost::condition_variable m_notFull;
#else
        std::mutex m_mutex;
        std::condition_variable m_notEmpty;
        std::condition_variable m_notFull;
#endif
        size_t m_capacity;
    public:

        BlockQueue(size_t capacity) : m_capacity(capacity)  {
        }

        virtual ~BlockQueue() {
        }

        bool isFull() const {
            return m_queue.size() == m_capacity;
        }

        bool isEmpty() const {
            return m_queue.empty();
        }

        void put(const DataT& data) {
#if UseBoostMutex
            boost::unique_lock<boost::mutex> lk(m_mutex);
#else
            std::unique_lock<std::mutex> lk(m_mutex);
#endif
            while (isFull()) {
                //                std::cout << "BlockQueue::queue is full, put is blocking." << std::endl;
                m_notFull.wait(lk);
            }
            m_queue.push_back(data);
            m_notEmpty.notify_one();
        }

        DataT take(void) {
#if UseBoostMutex
            boost::unique_lock<boost::mutex> lk(m_mutex);
#else
            std::unique_lock<std::mutex> lk(m_mutex);
#endif
            while (isEmpty()) {
                //                std::cout << "BlockQueue::queue is empty, take is blocking." << std::endl;
                m_notEmpty.wait(lk);
            }
            DataT data = m_queue.front();
            m_queue.pop_front();
            m_notFull.notify_one();
            return data;
        }

        size_t remaining() {
            return m_queue.size();
        }

        size_t capacity() {
            return m_capacity;
        }
        
        void clear() {
#if UseBoostMutex
            boost::unique_lock<boost::mutex> lk(m_mutex);
#else
            std::unique_lock<std::mutex> lk(m_mutex);
#endif
            m_queue.clear();
//            typename std::list<DataT>::iterator it = m_queue.begin();
//            for (; it != m_queue.end();) {
//                delete *it;
//                it = m_queue.erase(it);
//            }
        }
    };
}
#endif /* BLOCKQUEUE_H */

