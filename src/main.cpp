#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <semaphore>
#include <atomic>
#include <chrono>
#include <random>

// Global variables for synchronization
constexpr int NUM_JOGADORES = 4;
std::counting_semaphore<NUM_JOGADORES> cadeira_sem(NUM_JOGADORES - 1); // Inicia com n-1 cadeiras, capacidade máxima n
std::condition_variable music_cv;
std::mutex music_mutex;
std::atomic<bool> musica_parada{false};
std::atomic<bool> jogo_ativo{true};

/*
 * Uso básico de um counting_semaphore em C++:
 * 
 * O `std::counting_semaphore` é um mecanismo de sincronização que permite controlar o acesso a um recurso compartilhado 
 * com um número máximo de acessos simultâneos. Neste projeto, ele é usado para gerenciar o número de cadeiras disponíveis.
 * Inicializamos o semáforo com `n - 1` para representar as cadeiras disponíveis no início do jogo. 
 * Cada jogador que tenta se sentar precisa fazer um `acquire()`, e o semáforo permite que até `n - 1` jogadores 
 * ocupem as cadeiras. Quando todos os assentos estão ocupados, jogadores adicionais ficam bloqueados até que 
 * o coordenador libere o semáforo com `release()`, sinalizando a eliminação dos jogadores.
 * O método `release()` também pode ser usado para liberar múltiplas permissões de uma só vez, por exemplo: `cadeira_sem.release(3);`,
 * o que permite destravar várias threads de uma só vez, como é feito na função `liberar_threads_eliminadas()`.
 *
 * Métodos da classe `std::counting_semaphore`:
 * 
 * 1. `acquire()`: Decrementa o contador do semáforo. Bloqueia a thread se o valor for zero.
 *    - Exemplo de uso: `cadeira_sem.acquire();` // Jogador tenta ocupar uma cadeira.
 * 
 * 2. `release(int n = 1)`: Incrementa o contador do semáforo em `n`. Pode liberar múltiplas permissões.
 *    - Exemplo de uso: `cadeira_sem.release(2);` // Libera 2 permissões simultaneamente.
 */

// Classes

class Cadeira {
    public:
        Cadeira() : id_jogador_(0) {}
        void set_id_jogador(int id_jogador) { this->id_jogador_ = id_jogador; }
        int get_id_jogador() { return this->id_jogador_; }
    private:
        // Um id_jogador_ igual a 0 significa que a cadeira está desocupada
        int id_jogador_;
};

class JogoDasCadeiras {
    public:
        JogoDasCadeiras(int num_jogadores);

        void iniciar_rodada();

        void comecar_musica();

        void parar_musica();

        void eliminar_jogador();

        void exibir_estado();

        bool pegar_cadeira(int id_jogador);

        void remover_cadeira();

        void esvaziar_cadeiras();

        int get_num_jogadores() { return num_jogadores_; }

        int get_id_vencedor() { return this->cadeiras_[0].get_id_jogador(); }

        void add_jogador_eliminado(int id_jogador);

    private:
        int num_jogadores_;
        std::vector<Cadeira> cadeiras_;
        std::vector<int> jogadores_eliminados_;
};

JogoDasCadeiras::JogoDasCadeiras(int num_jogadores) : num_jogadores_(num_jogadores) {
    // Inicializa o vetor de cadeiras desocupadas com "num_jogadores-1" cadeiras
    for (int i = 0; i < num_jogadores-1; i++) {
        this->cadeiras_.emplace_back();
    }
}

void JogoDasCadeiras::iniciar_rodada() {
    // Inicia uma nova rodada, removendo uma cadeira e ressincronizando o semáforo se não for a primeira rodada
    if (this->num_jogadores_ < NUM_JOGADORES) {
        this->esvaziar_cadeiras();
        this->remover_cadeira();
        cadeira_sem.release(this->num_jogadores_-1);

        std::cout << "Próxima rodada com " << this->num_jogadores_ << " jogadores e "
                  << this->cadeiras_.size() << " cadeiras." << std::endl;
        std::cout << "A música começou a tocar" << std::endl << std::endl;
    } else {
        
        std::cout << "Iniciando rodada com " << this->num_jogadores_ << " jogadores e "
                  << this->cadeiras_.size() << " cadeiras." << std::endl;
        std::cout << "A música está tocando" << std::endl << std::endl;
    }
}

// Simula o momento em que a música começa e notifica os jogadores via variável de condição
void JogoDasCadeiras::comecar_musica() {
    music_mutex.lock();
    musica_parada = false;
    music_cv.notify_all();
    music_mutex.unlock();
}

// Simula o momento em que a música para e notifica os jogadores via variável de condição
void JogoDasCadeiras::parar_musica() {
    music_mutex.lock();
    musica_parada = true;
    music_cv.notify_all();
    music_mutex.unlock();

    std::cout << "> A música parou" << std::endl << std::endl;
}

// Diminui a quantidade de jogadores no jogo
void JogoDasCadeiras::eliminar_jogador() {
    --this->num_jogadores_;
}

void JogoDasCadeiras::exibir_estado() {
    // Exibe o estado atual das cadeiras e dos jogadores

    for (size_t i = 1; i <= this->cadeiras_.size(); i++) {
        std::cout << "[Cadeira " << i << "]: Está ocupada por P" << this->cadeiras_[i-1].get_id_jogador() << std::endl;
    }
    std::cout << "Jogador P" << this->jogadores_eliminados_.back() << " está fora do jogo"<< std::endl;
}

// Verifica se há alguma cadeira vazia e retorna se a cadeira foi pega ou não
bool JogoDasCadeiras::pegar_cadeira(int id_jogador) {
    bool pegou_cadeira = false;
    for (auto &cadeira : this->cadeiras_) {
        if (cadeira.get_id_jogador() == 0) {
            cadeira.set_id_jogador(id_jogador);
            pegou_cadeira = true;
            break;
        }
    }

    return pegou_cadeira;
}

// Remove uma cadeira do jogo
void JogoDasCadeiras::remover_cadeira() {
    this->cadeiras_.pop_back();
}

// Esvazia as cadeiras do jogo, zerando o id_jogador associado a ela
void JogoDasCadeiras::esvaziar_cadeiras() {
    for (auto &cadeira : this->cadeiras_) {
        cadeira.set_id_jogador(0);
    }
}

void JogoDasCadeiras::add_jogador_eliminado(int id_jogador) {
    this->jogadores_eliminados_.push_back(id_jogador);
}

class Jogador {
public:
    Jogador(int id, JogoDasCadeiras& jogo)
        : id_(id), jogo_(jogo) {}

    bool tentar_ocupar_cadeira();

    static void joga(Jogador *jogador);

private:
    int id_;
    JogoDasCadeiras& jogo_;
};

// Tenta ocupar uma cadeira utilizando o semáforo contador
bool Jogador::tentar_ocupar_cadeira() {

    cadeira_sem.acquire();

    music_mutex.lock();
    bool ret = this->jogo_.pegar_cadeira(this->id_);
    music_mutex.unlock();

    return ret;
}

void Jogador::joga(Jogador *jogador) {
    
    while (jogo_ativo) {

        // Aguarda a música parar usando a variavel de condição
        std::unique_lock music_lock(music_mutex);
        while (!musica_parada) {
            music_cv.wait(music_lock);
        }
        music_lock.unlock();

        // Tenta ocupar uma cadeira e verifica se foi eliminado
        if (jogador->tentar_ocupar_cadeira()) {

            // Aguarda a música recomeçar
            music_lock.lock();
            while (musica_parada) {
                music_cv.wait(music_lock);
            }
            music_lock.unlock();
        } else {

            // Foi eliminado
            jogador->jogo_.add_jogador_eliminado(jogador->id_);
            return;
        }
    }
}

class Coordenador {
public:
    Coordenador(JogoDasCadeiras& jogo)
        : jogo_(jogo) {}

    static void iniciar_jogo(Coordenador *coordenador);

    void liberar_threads_eliminadas();

private:
    JogoDasCadeiras& jogo_;
};

void Coordenador::iniciar_jogo(Coordenador *coordenador) {

    // Inicializa o gerador de números aleatórios com uma semente aleatória
    std::mt19937 gen(static_cast<unsigned int>(std::time(0)));

    // Define o intervalo de valores (distribuição uniforme)
    std::uniform_int_distribution<int> distrib(0, 10);

    while (jogo_ativo) {

        // Começa uma nova rodada
        coordenador->jogo_.iniciar_rodada();

        // Dorme por um período aleatório
        std::this_thread::sleep_for(std::chrono::milliseconds(distrib(gen)*1000));

        // Para a música, sinalizando os jogadores
        coordenador->jogo_.parar_musica();

        // Aguarda os jogadores se sentarem
        while (cadeira_sem.try_acquire()) {
            cadeira_sem.release();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        coordenador->jogo_.eliminar_jogador();

        coordenador->liberar_threads_eliminadas();

        // Aguarda o jogador ser eliminado
        while (cadeira_sem.try_acquire()) {
            cadeira_sem.release();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        coordenador->jogo_.exibir_estado();

        if (coordenador->jogo_.get_num_jogadores() == 1) {
            jogo_ativo = false;

            std::cout << " O vencedor foi o Jogador P" << coordenador->jogo_.get_id_vencedor() << "! Parabéns! 🏆" << std::endl;
        }

        coordenador->jogo_.comecar_musica();
    }
}

void Coordenador::liberar_threads_eliminadas() {
    // Libera o jogador que não conseguiu se sentar
    cadeira_sem.release();
}

// Main function
int main() {

    std::cout << "O jogo das cadeiras já vai começar" << std::endl;

    JogoDasCadeiras jogo(NUM_JOGADORES);
    Coordenador coordenador(jogo);
    std::vector<std::thread> jogadores;

    // Criação das threads dos jogadores
    std::vector<Jogador> jogadores_objs;
    for (int i = 1; i <= NUM_JOGADORES; ++i) {
        jogadores_objs.emplace_back(i, jogo);
    }

    for (int i = 0; i < NUM_JOGADORES; ++i) {
        jogadores.emplace_back(&Jogador::joga, &jogadores_objs[i]);
    }

    // Thread do coordenador
    std::thread coordenador_thread(&Coordenador::iniciar_jogo, &coordenador);

    // Esperar pelas threads dos jogadores
    for (auto& t : jogadores) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Esperar pela thread do coordenador
    if (coordenador_thread.joinable()) {
        coordenador_thread.join();
    }

    std::cout << "Jogo das Cadeiras finalizado." << std::endl;
    return 0;
}
