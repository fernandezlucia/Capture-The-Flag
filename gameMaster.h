#ifndef GAMEMASTER_H
#define GAMEMASTER_H
#include <tuple>
#include <cstdio>
#include <vector>
#include <mutex>
#include <semaphore.h>
#include "definiciones.h"
#include "config.h"

using namespace std;

class gameMaster {
private:
    int nro_ronda = 0;
    int x, y, jugadores_por_equipos;
    vector<vector<color>> tablero;
    vector<coordenadas> pos_jugadores_azules, pos_jugadores_rojos;
    color turno;
    estrategia strat;

    int jugadasRojo = 0;
    int jugadasAzul = 0;

    color obtener_coordenadas(coordenadas coord);
    void mover_jugador_tablero(coordenadas pos_anterior, coordenadas pos_nueva, color colorEquipo);
    coordenadas pos_bandera_roja, pos_bandera_azul;
 
public:
    gameMaster(Config config);
    void termino_ronda(color equipo); 
    int mover_jugador(direccion dir, int nro_jugador);
    color ganador = INDEFINIDO;
    bool soy_el_mas_cercano(int nro_jugador, color equipo);
    vector<sem_t> mutexes_rr_rojos;
    vector<sem_t> mutexes_rr_azules;
    bool termino_juego();
	int getTamx();
	int getTamy();
    static int distancia(coordenadas pair1, coordenadas pair2);
    coordenadas posicion_de(int nro_jugador, color equipo);
    int ronda_actual();
    
    sem_t turno_rojo, turno_azul;

    sem_t ronda_anterior_finalizada;

    color en_posicion(coordenadas coord);
    bool es_posicion_valida(coordenadas pos);
    bool es_color_libre(color color_tablero);
	coordenadas proxima_posicion(coordenadas anterior, direccion movimiento); // Calcula la proxima posición a moverse
    int proximo_cercano(color equipo);
    bool es_posicion_bandera(coordenadas coord, color bandera);
    vector<coordenadas> movimiento_alternativo(coordenadas posicion, direccion intento_movimiento, coordenadas objetivo);
    void realizar_movimiento_alternativo(coordenadas posicion,direccion dir_bloqueada, direccion dir_prioritaria, vector<coordenadas> &res);
    void termino_equipo_rr(color equipo);
    void liberar_proximos_restantes_rr(color equipo, int nro_jugador, int cant_jugadores);
    
    void play();
};

#endif // GAMEMASTER_H
