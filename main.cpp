#include <algorithm>
#include <cassert>
#include <vector>

struct node {
    bool m_is_free;
    unsigned int m_offset;
    unsigned int m_size;
};

struct memory_manager {
    std::vector<char> m_buffer;
    std::vector<node> m_list;
};

memory_manager initmem(unsigned int max_size) {
    return memory_manager{
            std::vector<char>(max_size),
            std::vector<node>{
                node{true, 0, max_size}
            }
        };
}
// Initialise les structures de données du gestionnaire. 
 
void* alloumem(memory_manager m, unsigned int size);
// Cette fonction alloue un nouveau bloc de mémoire. 
 
void liberemem(memory_manager m, void* buffer);
// Cette fonction libère un bloc de mémoire similaire à free(). 
 
unsigned int nbloclibres(memory_manager m) {
    // Cette fonction retourne le nombre de blocs de mémoire libres 
    return std::count_if(
            std::begin(m.m_list),
            std::end(m.m_list),
            [](node& n) { return n.m_is_free; });
}
 
unsigned int nblocalloues(memory_manager m) {
    // Cette fonction retourne le nombre de blocs alloués présentement 
    return m.m_list.size();
}
 
unsigned int memlibre(memory_manager m) {
    // Combien de mémoire libre (non allouée) au total. 
    unsigned int sum = 0;

    for (auto n : m.m_list) {
        if (n.m_is_free) {
            sum += n.m_size;
        }
    }

    return sum;
}

 
unsigned int mem_pgrand_libre(memory_manager m) {
    // Taille du plus grand bloc libre. 

    unsigned int max = 0;

    for (auto n : m.m_list) {
        if (n.m_is_free && n.m_size > max) {
            max = n.m_size;
        }
    }

    return max;
}
 
unsigned int mem_small_free(memory_manager m, unsigned int maxTaillePetit) {
    // Combien de petits blocs (plus petits que maxTaillePetit) non alloués y a-t-il présentement en mémoire?
    return std::count_if(
            std::begin(m.m_list),
            std::end(m.m_list),
            [maxTaillePetit](node& n) { return n.m_size < maxTaillePetit; });
}

bool mem_est_alloue(memory_manager m, void* pOctet);
// Est-ce qu’un octet (byte) particulier est alloué

int main() {
    auto manager = initmem(1024);
    assert(1024 == mem_pgrand_libre(manager));
    assert(1024 == memlibre(manager));
    assert(1 == nblocalloues(manager));
    assert(1 == nbloclibres(manager));
    assert(0 == mem_small_free(manager, 512));
    assert(1 == mem_small_free(manager, 2048));
}
