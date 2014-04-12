#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <iterator>
#include <vector>
#include <type_traits>

struct node {
    bool m_is_free;
    unsigned int m_offset;
    unsigned int m_size;
};


bool operator==(const node& lhs, const node rhs) {
    static_assert(std::is_pod<node>::value, "The type 'node' should be a POD.");
    return 0 == std::memcmp(&lhs, &rhs, sizeof(node));
}

struct memory_manager {
    const std::vector<char> m_buffer;
    std::vector<node> m_list;
};

void show_nodes(const memory_manager& m) {
    std::cout << "[" << std::dec;
    for (auto n : m.m_list) {
        std::cout << "(" << n.m_is_free << "," << n.m_offset << "," << n.m_size << ")";
    }
    std::cout << "]\n";
}

void show_hexa(const memory_manager& m) {
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

struct first_fit {
    template<class InputIterator>
    InputIterator operator()(InputIterator first, InputIterator last, unsigned int size) {
        return std::find_if(
            first,
            last,
            [size](node n) { return n.m_is_free && n.m_size >= size; });
    }
};

struct best_fit {
    template<class ForwardIterator>
    ForwardIterator operator()(ForwardIterator first, ForwardIterator last, unsigned int size) {
        ForwardIterator min = last;

        while (first != last) {
            if (first->m_is_free && first->m_size >= size) {
                if (min == last || first->m_size < min->m_size) {
                    min = first;
                }
            }
            ++first;
        }

        return min;
    }
};

struct worst_fit {
    template<class ForwardIterator>
    ForwardIterator operator()(ForwardIterator first, ForwardIterator last, unsigned int size) {
        ForwardIterator max = last;

        while (first != last) {
            if (first->m_is_free && first->m_size >= size) {
                if (max == last || first->m_size > max->m_size) {
                    max = first;
                }
            }
            ++first;
        }

        return max;
    }
};

struct next_fit {
    node last_node = node{false, 0, 0};
    unsigned int last_offset = 0;

    template<class ForwardIterator>
    ForwardIterator operator()(ForwardIterator first, ForwardIterator last, unsigned int size) {
        auto it = std::find_if(first, last, [=](node n){
            return n.m_offset > last_offset;
        });

        auto it2 = first_fit()(it, last, size);
        if (it2 != last) {
            last_node = *it2;
            last_offset = it2->m_offset;
            return it2;
        } else {
            auto it3 = first_fit()(first, it, size);
            if (it3 != it) {
                last_node = *it3;
                last_offset = it3->m_offset;
            }
            return it3;
        }
    }
};

template<class Strategy>
void* alloumem(memory_manager& m, Strategy& s, unsigned int size, int fill = 0) {
    // Cette fonction alloue un nouveau bloc de mémoire.

    // alloumem [(false,0,1024), (true,1024,1024), (false,2048,1024)] 512
    //          [(false,0,1024), (false,1024,512), (true,1536,512), (false,2048,1024)]
    std::cout << "allocation" << std::endl; 
    auto it = s(std::begin(m.m_list), std::end(m.m_list), size);

    if (it != std::end(m.m_list)) {
        it->m_is_free = false;

        if (it->m_size != size) {
            auto old_size = it->m_size;
            it->m_size = size;
            m.m_list.insert(std::next(it), node{true, it->m_offset + size, old_size - size});
        }

        auto ptr = reinterpret_cast<void*>(const_cast<char*>(m.m_buffer.data() + it->m_offset));
        std::memset(ptr, fill, size);

        return ptr;
    } else {
        return nullptr;
    }
}

void liberemem(memory_manager& m, void* buffer, int fill = 0) {
    // Cette fonction libère un bloc de mémoire similaire à free().

    // liberemem [(false,0,1024), (false,1024,512), (true,1536,512), (false,2048,1024)]
    //           [(false,0,1024), (true,1024,1024), (false,2048,1024)]
    //
    std::cout << "libération" << std::endl; 
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
        if (a <= pOctet && pOctet < b) {
            return !n.m_is_free;
        }
    }

    return false;
}

int main() {
    auto managerFF = initmem(16);
    auto first_fit_strategy = first_fit{};
    auto best_fit_strategy = best_fit{};
    auto worst_fit_strategy = worst_fit{};
    auto next_fit_strategy = next_fit{};

    assert(16 == mem_pgrand_libre(managerFF));
    assert(16 == memlibre(managerFF));
    assert(1 == nblocalloues(managerFF));
    assert(1 == nbloclibres(managerFF));
    assert(0 == mem_small_free(managerFF, 8));
    assert(1 == mem_small_free(managerFF, 32));

    auto ptr1 = alloumem(managerFF, first_fit_strategy, 5);
    auto ptr2 = alloumem(managerFF, first_fit_strategy, 4);
    auto ptr3 = alloumem(managerFF, first_fit_strategy, 3);
    auto ptr4 = alloumem(managerFF, first_fit_strategy, 2);

    std::cout << "first fit sequence" << std::endl;
    show_nodes(managerFF);

    assert(2 == mem_pgrand_libre(managerFF));
    assert(2 == memlibre(managerFF));
    assert(5 == nblocalloues(managerFF));
    assert(1 == nbloclibres(managerFF));
    assert(1 == mem_small_free(managerFF, 8));
    assert(1 == mem_small_free(managerFF, 32));
    assert(true == mem_est_alloue(managerFF, ptr1));
    assert(true == mem_est_alloue(managerFF, ptr2));
    assert(true == mem_est_alloue(managerFF, ptr3));
    assert(true == mem_est_alloue(managerFF, ptr4));

    liberemem(managerFF, ptr1);
    liberemem(managerFF, ptr3);

    show_nodes(managerFF);
    assert(10 == memlibre(managerFF));
    assert(5 == nblocalloues(managerFF));
    assert(3 == nbloclibres(managerFF));
    assert(3 == mem_small_free(managerFF, 8));
    assert(3 == mem_small_free(managerFF, 32));
    assert(false == mem_est_alloue(managerFF, ptr1));
    assert(true == mem_est_alloue(managerFF, ptr2));
    assert(false == mem_est_alloue(managerFF, ptr3));
    assert(true == mem_est_alloue(managerFF, ptr4));
    
    liberemem(managerFF, ptr2);

    show_nodes(managerFF);
    assert(12 == mem_pgrand_libre(managerFF));
    assert(14 == memlibre(managerFF));
    assert(3 == nblocalloues(managerFF));
    assert(2 == nbloclibres(managerFF));
    assert(1 == mem_small_free(managerFF, 8));
    assert(2 == mem_small_free(managerFF, 32));
    assert(false == mem_est_alloue(managerFF, ptr1));
    assert(false == mem_est_alloue(managerFF, ptr2));
    assert(false == mem_est_alloue(managerFF, ptr3));
    assert(true == mem_est_alloue(managerFF, ptr4));
    
   
    //best fit
    std::cout << "best fit sequence" << std::endl;
    auto managerBF = initmem(16);
    ptr1 = alloumem(managerBF, best_fit_strategy, 5);
    ptr2 = alloumem(managerBF, best_fit_strategy, 4);
    ptr3 = alloumem(managerBF, best_fit_strategy, 3);
    ptr4 = alloumem(managerBF, best_fit_strategy, 2);
    show_nodes(managerBF);

    assert(2 == mem_pgrand_libre(managerBF));
    assert(2 == memlibre(managerBF));
    assert(5 == nblocalloues(managerBF));
    assert(1 == nbloclibres(managerBF));
    assert(1 == mem_small_free(managerBF, 8));
    assert(1 == mem_small_free(managerBF, 32));
    assert(true == mem_est_alloue(managerBF, ptr1));
    assert(true == mem_est_alloue(managerBF, ptr2));
    assert(true == mem_est_alloue(managerBF, ptr3));
    assert(true == mem_est_alloue(managerBF, ptr4));

    liberemem(managerBF, ptr1);
    liberemem(managerBF, ptr3);

    show_nodes(managerBF);
    assert(10 == memlibre(managerBF));
    assert(5 == nblocalloues(managerBF));
    assert(3 == nbloclibres(managerBF));
    assert(3 == mem_small_free(managerBF, 8));
    assert(3 == mem_small_free(managerBF, 32));
    assert(false == mem_est_alloue(managerBF, ptr1));
    assert(true == mem_est_alloue(managerBF, ptr2));
    assert(false == mem_est_alloue(managerBF, ptr3));
    assert(true == mem_est_alloue(managerBF, ptr4));
    
    liberemem(managerBF, ptr2);
    assert(false == mem_est_alloue(managerBF, ptr2));

    show_nodes(managerBF);
    assert(12 == mem_pgrand_libre(managerBF));
    assert(14 == memlibre(managerBF));
    assert(3 == nblocalloues(managerBF));
    assert(2 == nbloclibres(managerBF));
    assert(1 == mem_small_free(managerBF, 8));
    assert(2 == mem_small_free(managerBF, 32));
    auto ptr5 = alloumem(managerBF, best_fit_strategy, 2);
    show_nodes(managerBF);

    //worst fit
    std::cout << "worst fit sequence" << std::endl;
    auto managerWF = initmem(16);
    ptr1 = alloumem(managerWF, worst_fit_strategy, 5);
    ptr2 = alloumem(managerWF, worst_fit_strategy, 4);
    ptr3 = alloumem(managerWF, worst_fit_strategy, 3);
    ptr4 = alloumem(managerWF, worst_fit_strategy, 2);
    show_nodes(managerWF);

    assert(2 == mem_pgrand_libre(managerWF));
    assert(2 == memlibre(managerWF));
    assert(5 == nblocalloues(managerWF));
    assert(1 == nbloclibres(managerWF));
    assert(1 == mem_small_free(managerWF, 8));
    assert(1 == mem_small_free(managerWF, 32));
    assert(true == mem_est_alloue(managerWF, ptr1));
    assert(true == mem_est_alloue(managerWF, ptr2));
    assert(true == mem_est_alloue(managerWF, ptr3));
    assert(true == mem_est_alloue(managerWF, ptr4));

    liberemem(managerWF, ptr1);
    liberemem(managerWF, ptr3);

    show_nodes(managerWF);
    assert(10 == memlibre(managerWF));
    assert(5 == nblocalloues(managerWF));
    assert(3 == nbloclibres(managerWF));
    assert(3 == mem_small_free(managerWF, 8));
    assert(3 == mem_small_free(managerWF, 32));
    assert(false == mem_est_alloue(managerWF, ptr1));
    assert(true == mem_est_alloue(managerWF, ptr2));
    assert(false == mem_est_alloue(managerWF, ptr3));
    assert(true == mem_est_alloue(managerWF, ptr4));
    
    liberemem(managerWF, ptr2);
    assert(false == mem_est_alloue(managerWF, ptr2));

    show_nodes(managerWF);
    assert(12 == mem_pgrand_libre(managerWF));
    assert(14 == memlibre(managerWF));
    assert(3 == nblocalloues(managerWF));
    assert(2 == nbloclibres(managerWF));
    assert(1 == mem_small_free(managerWF, 8));
    assert(2 == mem_small_free(managerWF, 32));
    ptr5 = alloumem(managerWF, worst_fit_strategy, 2);
    show_nodes(managerWF);

    //next fit
    std::cout << "next fit sequence" << std::endl;
    auto managerNF = initmem(16);
    ptr1 = alloumem(managerNF, next_fit_strategy, 5);
    ptr2 = alloumem(managerNF, next_fit_strategy, 4);
    ptr3 = alloumem(managerNF, next_fit_strategy, 3);
    ptr4 = alloumem(managerNF, next_fit_strategy, 2);
    show_nodes(managerNF);

    assert(2 == mem_pgrand_libre(managerNF));
    assert(2 == memlibre(managerNF));
    assert(5 == nblocalloues(managerNF));
    assert(1 == nbloclibres(managerNF));
    assert(1 == mem_small_free(managerNF, 8));
    assert(1 == mem_small_free(managerNF, 32));
    assert(true == mem_est_alloue(managerNF, ptr1));
    assert(true == mem_est_alloue(managerNF, ptr2));
    assert(true == mem_est_alloue(managerNF, ptr3));
    assert(true == mem_est_alloue(managerNF, ptr4));

    liberemem(managerNF, ptr1);
    liberemem(managerNF, ptr3);

    show_nodes(managerNF);
    assert(10 == memlibre(managerNF));
    assert(5 == nblocalloues(managerNF));
    assert(3 == nbloclibres(managerNF));
    assert(3 == mem_small_free(managerNF, 8));
    assert(3 == mem_small_free(managerNF, 32));
    assert(false == mem_est_alloue(managerNF, ptr1));
    assert(true == mem_est_alloue(managerNF, ptr2));
    assert(false == mem_est_alloue(managerNF, ptr3));
    assert(true == mem_est_alloue(managerNF, ptr4));
    
    liberemem(managerNF, ptr2);
    assert(false == mem_est_alloue(managerNF, ptr2));

    show_nodes(managerNF);
    assert(12 == mem_pgrand_libre(managerNF));
    assert(14 == memlibre(managerNF));
    assert(3 == nblocalloues(managerNF));
    assert(2 == nbloclibres(managerNF));
    assert(1 == mem_small_free(managerNF, 8));
    assert(2 == mem_small_free(managerNF, 32));
    ptr5 = alloumem(managerNF, next_fit_strategy, 2);
    show_nodes(managerNF);

    auto ptr6 = alloumem(managerNF, next_fit_strategy, 2);
    show_nodes(managerNF);

    auto ptr7 = alloumem(managerNF, next_fit_strategy, 2);
    show_nodes(managerNF);

    liberemem(managerNF, ptr6);
    show_nodes(managerNF);
    auto ptr8 = alloumem(managerNF, next_fit_strategy, 2);
    show_nodes(managerNF);
}
