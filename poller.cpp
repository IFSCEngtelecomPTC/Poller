#include <sys/time.h>
#include <set>
#include <cstddef>
#include <iostream>
#include <range/v3/all.hpp>
#include "poller.h"

using std::set;

// valor mto grande ...
const long MaxTimeout = 1000000000;


Poller::Poller() {
}

Poller::~Poller() {
//    for (auto cb: cbs_to) delete cb;
//    for (auto & par: cbs) delete par.second;
}


void Poller::adiciona(Callback * cb) {
    if (cb->filedesc() < 0) {
        // callback para timer:
        // verifica se já existe um callback equivalente cadastrado
        // se existir, sobrescreve-o com o novo callback
        for (auto & c : cbs_to) {
            if (c == cb) {
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
    while (true) despache_simples();
}

void Poller::despache_simples() {
    pollfd fds[MAX_FDS];

    // concatena callbacks normais e tiemout
    // filtra os que estão com timeout desativado
    // e então obtém aquele com menor timeout
    auto v_cbs = cbs | ranges::views::transform([](auto const & par){return par.second;});
    auto v_to = ranges::views::concat(v_cbs, cbs_to)
             | ranges::views::filter([](auto const & cb) { return cb->timeout_enabled();});
    auto i_cb = ranges::min_element(v_to, [](const auto & c1, const auto & c2) {return c1->timeout() < c2->timeout();});

    // i_cb é um iterador !
    Callback * cb_tout = (i_cb != end(v_to))?*i_cb:nullptr;
    // identifica o menor timeout dentre todos os timers
    int min_timeout = cb_tout != nullptr?cb_tout->timeout():-1;
    auto p_fds = fds;

    // preenche o vetor de fds com os fds dos callbacks ativos
    cbs | ranges::views::transform([](auto const & par){return par.second->filedesc();})
        | ranges::views::filter([](auto fd) { return fd >= 0;})
        | ranges::views::for_each([&p_fds](auto fd){p_fds->fd = fd;
                                                                       p_fds->events = POLLIN;
                                                                       p_fds++;
                                                                       return ranges::views::empty<int>;})
        | ranges::to_vector; // para executar as views

    auto nfds = std::distance(fds, p_fds);

    // lê o relógio, para saber o instante em que o poll iniciou
    timeval t1, t2;
    gettimeofday(&t1, NULL);
    
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
    
    
    for (auto & cb: cbs_to) {
      //if (cb != cb_tout) cb->update(dt);
      if (fired.count(cb) == 0) cb->update(dt);
    }
    for (auto & par: cbs) {
      if (fired.count(par.second) == 0) par.second->update(dt);
    }
}
