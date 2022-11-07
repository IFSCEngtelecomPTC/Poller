#include <sys/time.h>
#include <cstdlib>
#include <set>
#include <algorithm>
#include "poller.h"

using std::set;

// valor mto grande ...
const long MaxTimeout = 1000000000;


Poller::Poller() {
}

Poller::~Poller() {
    for (auto cb: cbs_to) delete cb;
    for (auto & par: cbs) delete par.second;
}


void Poller::adiciona(Callback * cb) {
    if (cb->filedesc() < 0) {
        // callback para timer:
        // verifica se já existe um callback equivalente cadastrado
        // se existir, sobrescreve-o com o novo callback
        for (auto & c : cbs_to) {
            if (*c == *cb) {
                c = cb;
                return;
            }
        }
        // adiciona novo callback timer
        cbs_to.push_back(cb);
    } else cbs[cb->filedesc()] = cb; // adiciona callback para descritor de arquivo
}

void Poller::remove(int fd) {
    cbs.erase(fd);
}

void Poller::remove(Callback * cb) {
    int fd = cb->filedesc();
    
    if (fd >= 0) remove(fd);
    else {
        for (auto it = cbs_to.begin(); it != cbs_to.end(); it++) {
            auto c = *it;
            if (*c == *cb) {
                cbs_to.erase(it);
                break;
            }
        }
    }
}

void Poller::limpa() {
    cbs.clear();
    cbs_to.clear();
}

void Poller::despache() {
    while (despache_simples());
}

bool Poller::despache_simples() {
    pollfd fds[MAX_FDS];
    int nfds = 0;
    
    // identifica o menor timeout dentre todos os timers
    int min_timeout = MaxTimeout;
    Callback * cb_tout = nullptr;
    bool has_tout = false;

    for (auto cb : cbs_to) {
        if (! cb->timeout_enabled()) continue;

        has_tout = true;
        int fd = cb->filedesc();
        int timeout = cb->timeout();
        
        if (timeout < min_timeout) {
            min_timeout = timeout;
            cb_tout = cb;
        }
    }
    
    // gera o vetor de descritores a ser usado na chamada poll
    // verifica tb se o timeout de algum callback desses é menor do que o
    // timer mais próximo 
    for (auto & par : cbs) {
        if (par.second->timeout_enabled()) {
            int timeout = par.second->timeout();

            has_tout = true;
            if (timeout < min_timeout) {
                min_timeout = timeout;
                cb_tout = par.second;
            }
        }
        
        if (par.second->is_enabled()) {
            int fd = par.second->filedesc();
            if (nfds == MAX_FDS) throw -1; // erro: excedeu qtde de descritores vigiados
            fds[nfds].fd = fd;
            fds[nfds].events = POLLIN;
            nfds++;            
        }
    }

    // lê o relógio, para saber o instante em que o poll iniciou
    timeval t1, t2;
    gettimeofday(&t1, NULL);
    
    if (! has_tout) {
        if (nfds > 0) min_timeout = -1;
        else return false;
    }
    int n = poll(fds, nfds, min_timeout);
    
    // lê novamente o relógio, para saber o instante em que o poll retornou
    gettimeofday(&t2, NULL);
        
    // determina o tempo decorrido até ocorrência do evento
    long dt = (t2.tv_usec - t1.tv_usec)/1000 + (t2.tv_sec-t1.tv_sec)*1000;
    
    // se nenhum descritor está pronto para ser lido, ocorreu timeout
    // Na verdade, um timer disparou: chama o callback desse timer
    set<Callback*> fired;
    if (n == 0) {
        cb_tout->handle_timeout(); // timeout        
        cb_tout->reload_timeout();
        fired.insert(cb_tout);
    } else {
        // caso contrário, chama o callkback de cada descritor pronto para ser acessado
        for (int i=0; i < nfds and n > 0; i++) {
            auto cb = cbs[fds[i].fd];
            if (fds[i].revents & POLLIN) {                
                fired.insert(cb);
                cb->handle();
                cb->reload_timeout();
                n--;
            }/* else {
		cb->update(dt);
	    }*/
        }
    }

    cleanup();
    
    for (auto & cb: cbs_to) {
      //if (cb != cb_tout) cb->update(dt);
      if (fired.count(cb) == 0) cb->update(dt);
    }
    for (auto & par: cbs) {
      if (fired.count(par.second) == 0) par.second->update(dt);
    }

    return true;
}

void Poller::cleanup() {
    std::list<Callback*> l_finished;

    std::copy_if(cbs_to.begin(), cbs_to.end(), std::back_inserter(l_finished), [](auto & cb) { return cb->is_finished();});
    for (auto & par: cbs) {
        if (par.second->is_finished()) {
            l_finished.push_back(par.second);
        }
    }

    for (auto & cb: l_finished) {
        remove(cb);
    }
}
