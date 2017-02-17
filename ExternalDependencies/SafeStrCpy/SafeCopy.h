/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SafeCopy.h
 * Author: 10256
 *
 * Created on 2017年1月18日, 下午7:06
 */

#ifndef SAFECOPY_H
#define SAFECOPY_H
#include <stddef.h>
namespace safecopy{
    size_t strlcat(char *dst, const char *src, size_t siz);
    size_t strlcpy(char *dst, const char *src, size_t siz);
}

#endif /* SAFECOPY_H */

