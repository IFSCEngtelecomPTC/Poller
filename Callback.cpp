/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Callback.cpp
 * Author: msobral
 * 
 * Created on 20 de Setembro de 2018, 13:41
 */

#include "Callback.h"

Callback::Callback(int fd, long tout): fd(fd), base_tout(tout),tout(tout), enabled_to(true),finished(false) {
    if (tout < 0) throw -1;
    enabled = (fd >= 0);
}

Callback::Callback(long tout) : fd(-1), base_tout(tout),tout(tout), enabled_to(true),finished(false) {
    if (tout < 0) throw -1;
    enabled = false;
}

int Callback::filedesc() const { return fd;}

int Callback::timeout() const { return tout;}

void Callback::reload_timeout() {
    tout = base_tout;
}

void Callback::update(long dt) {
    tout -= dt;
    if (tout < 0) tout = 0;
}
    
bool Callback::operator==(const Callback & o) const {
  return fd == o.fd;
}

void Callback::disable_timeout() {
    enabled_to = false;
}

void Callback::disable() {
    enabled = false;
}

void Callback::enable() {
    enabled = true;
}

void Callback::enable_timeout() {
    enabled_to = true;
}

void Callback::set_timeout(long timeout) {
    tout = timeout;
}

void Callback::finish() {
    finished = true;
    disable();
    disable_timeout();
}
