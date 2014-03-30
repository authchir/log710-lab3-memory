#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <iterator>
#include <vector>

struct node {
    bool m_is_free;
    unsigned int m_offset;
    unsigned int m_size;
};

struct memory_manager {
    const std::vector<char> m_buffer;
    std::vector<node> m_list;
};

void showNodes(const memory_manager& m) {
    std::cout << "[" << std::dec;
    for (auto n : m.m_list) {
        std::cout << "(" << n.m_is_free << "," << n.m_offset << "," << n.m_size << ")";
    }
    std::cout << "]\n";
}

void showHexa(const memory_manager& m) {
    for (auto n : m.m_buffer) {
        std::cout << std::hex << "[" << static_cast<short>(n) << "]";
    }
    std::cout << "\n";
}
memory_manager initmem(unsigned int max_size) {
    // Initialise les structures de données du gestionnaire.

    return memory_manager{
            std::vector<char>(max_size),
            std::vector<node>{
                node{true, 0, max_size}
            }
        };
}

void* alloumem(memory_manager& m, unsigned int size, int fill = 0) {
    // Cette fonction alloue un nouveau bloc de mémoire.

    // alloumem [(false,0,1024), (true,1024,1024), (false,2048,1024)] 512
    //          [(false,0,1024), (false,1024,512), (true,1536,512), (false,2048,1024)]
    for (auto& n : m.m_list) {
        if (n.m_is_free) {
            if (n.m_size >= size) {
                auto old_size = n.m_size;

                n.m_is_free = false;
                n.m_size = size;

                m.m_list.push_back(node{true, n.m_offset + size, old_size - size});

                auto ptr = reinterpret_cast<void*>(const_cast<char*>(m.m_buffer.data() + n.m_offset));

                std::memset(ptr, fill, n.m_size);

                return ptr;
            }
        }
    }

    return nullptr;
}

void liberemem(memory_manager& m, void* buffer, int fill = 0) {
    // Cette fonction libère un bloc de mémoire similaire à free().

    // liberemem [(false,0,1024), (false,1024,512), (true,1536,512), (false,2048,1024)]
    //           [(false,0,1024), (true,1024,1024), (false,2048,1024)]
    auto buffer_offset = reinterpret_cast<const char*>(buffer) - m.m_buffer.data();

    auto it = std::find_if(
        std::begin(m.m_list),
        std::end(m.m_list),
        [buffer_offset](node n){ return !n.m_is_free && n.m_offset == buffer_offset; });

    if (it != std::end(m.m_list)) {
        // Free the node
        it->m_is_free = true;

        // Merge with the prev and next nodes
        auto first = it->m_offset != 0 && std::prev(it)->m_is_free
            ? std::prev(it)
            : it;
        auto offset = first->m_offset;
        unsigned int size = 0;

        while (first != std::end(m.m_list) && first->m_is_free) {
            size += first->m_size;
            first = m.m_list.erase(first);
        }

        m.m_list.insert(first, node{true, offset, size});

        // Fill the memory with request value
        auto ptr = reinterpret_cast<void*>(const_cast<char*>(m.m_buffer.data() + offset));
        std::memset(ptr, fill, size);
    }
}

unsigned int nbloclibres(const memory_manager& m) {
    // Cette fonction retourne le nombre de blocs de mémoire libres
    return std::count_if(
            std::begin(m.m_list),
            std::end(m.m_list),
            [](const node& n) { return n.m_is_free; });
}
 
unsigned int nblocalloues(const memory_manager& m) {
    // Cette fonction retourne le nombre de blocs alloués présentement
    return m.m_list.size();
}
 
unsigned int memlibre(const memory_manager& m) {
    // Combien de mémoire libre (non allouée) au total.

    unsigned int sum = 0;

    for (auto n : m.m_list) {
        if (n.m_is_free) {
            sum += n.m_size;
        }
    }

    return sum;
}

unsigned int mem_pgrand_libre(const memory_manager& m) {
    // Taille du plus grand bloc libre.

    unsigned int max = 0;

    for (auto n : m.m_list) {
        if (n.m_is_free && n.m_size > max) {
            max = n.m_size;
        }
    }

    return max;
}

unsigned int mem_small_free(const memory_manager& m, unsigned int maxTaillePetit) {
    // Combien de petits blocs (plus petits que maxTaillePetit) non alloués y a-t-il présentement en mémoire?
    return std::count_if(
            std::begin(m.m_list),
            std::end(m.m_list),
            [maxTaillePetit](const node& n) { return n.m_is_free && n.m_size < maxTaillePetit; });
}

bool mem_est_alloue(const memory_manager& m, void* pOctet) {
    // Est-ce qu’un octet (byte) particulier est alloué

    for (auto n : m.m_list) {
        auto a = m.m_buffer.data() + n.m_offset;
        auto b = a + n.m_size;
        if (a <= pOctet && pOctet <= b) {
            return !n.m_is_free;
        }
    }

    return false;
}

int main() {
    auto manager = initmem(16);

    assert(16 == mem_pgrand_libre(manager));
    assert(16 == memlibre(manager));
    assert(1 == nblocalloues(manager));
    assert(1 == nbloclibres(manager));
    assert(0 == mem_small_free(manager, 8));
    assert(1 == mem_small_free(manager, 32));

    auto ptr1 = alloumem(manager, 5);
    auto ptr2 = alloumem(manager, 4);
    auto ptr3 = alloumem(manager, 3);
    auto ptr4 = alloumem(manager, 2);

    assert(2 == mem_pgrand_libre(manager));
    assert(2 == memlibre(manager));
    assert(5 == nblocalloues(manager));
    assert(1 == nbloclibres(manager));
    assert(1 == mem_small_free(manager, 8));
    assert(1 == mem_small_free(manager, 32));
    assert(true == mem_est_alloue(manager, ptr1));

    liberemem(manager, ptr1);
    liberemem(manager, ptr3);

    assert(10 == memlibre(manager));
    assert(5 == nblocalloues(manager));
    assert(3 == nbloclibres(manager));
    assert(3 == mem_small_free(manager, 8));
    assert(3 == mem_small_free(manager, 32));
    assert(false == mem_est_alloue(manager, ptr1));

    liberemem(manager, ptr2);

    assert(12 == mem_pgrand_libre(manager));
    assert(14 == memlibre(manager));
    assert(3 == nblocalloues(manager));
    assert(2 == nbloclibres(manager));
    assert(1 == mem_small_free(manager, 8));
    assert(2 == mem_small_free(manager, 32));
}
