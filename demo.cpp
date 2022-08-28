#include <iostream>
#include <string>
#include "poller.h"

using namespace std;

class CallbackStdin: public Callback {
 public:
  CallbackStdin(long tout): Callback(0, tout) {}

  void handle() {
    string w;

    getline(cin, w);
    cout << "Lido: " << w << endl;
  }

  void handle_timeout() {
     cout << "Timeout !!!" << endl;
  }

};

int main() {
  // cria o objeto CallbackStdin, com timeout de 5000ms
  CallbackStdin cb(5000);
  //cria um poller: ele contém um loop de eventos !
  Poller sched;

  // registra o callback no poller
  sched.adiciona(&cb);

  // entrega o controle ao poller.
  // Ele irá esperar pelos eventos e executará
  // os respectivos callbacks
  sched.despache();
}

