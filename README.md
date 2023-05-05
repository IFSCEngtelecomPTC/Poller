# Poller

## Como usar esta biblioteca em seu projeto C++ com cmake

Inclua o seguinte no arquivo CMakeLists.txt de seu projeto:

```
include(FetchContent)
FetchContent_Declare(
        poller
        URL https://github.com/IFSCEngtelecomPTC/Poller/archive/refs/tags/v1.0.1.tar.gz
)
FetchContent_MakeAvailable(poller)
include_directories(${poller_SOURCE_DIR} .)

# OBS: aqui é apenas uma demonstração de como linkar a biblioteca poller ao seu executável

# A linha abaixo usualmente é gerada pelo próprio CLion (ou por você mesmo, se criar seu CMakeLists.txt manualmente)
add_executable(test_app main.cpp)

# O comando a seguir linka seu executável com a biblioteca poller
# Portanto, renomeie test_app para o nome do seu executável
target_link_libraries(test_app poller)
```
