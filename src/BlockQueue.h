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

#include "DataFormat.h"

#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H
namespace blockqueue {

    class BlockQueue {
    public:
        BlockQueue(size_t capacity);
        BlockQueue(const BlockQueue& orig);
        virtual ~BlockQueue();

        bool isFull() const {
            return m_queue.size() == m_capacity;
        }

        bool isEmpty() const {
            return m_queue.empty();
        }

        void put(CarData* dataBuf);
        CarData* take(void);
        void clearAndFreeElements(void);
    private:


        std::list<CarData*> m_queue;
        std::mutex m_mutex;
        std::condition_variable m_notEmpty;
        std::condition_variable m_notFull;
        size_t m_capacity;
    };
}
#endif /* BLOCKQUEUE_H */

