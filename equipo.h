#ifndef EQUIPO_H
#define EQUIPO_H

#include <semaphore.h>
#include <vector>
#include <thread>
#include "definiciones.h"
#include "gameMaster.h"
#include <atomic>

using namespace std;

class Equipo {
	private:
		gameMaster *belcebu; 
		color contrario, equipo, bandera_contraria;
		estrategia strat;
		int cant_jugadores, quantum, quantum_restante;
		vector<thread> jugadores;
		int cant_jugadores_que_ya_jugaron = 0;
		vector<coordenadas> posiciones;
		coordenadas pos_bandera_contraria;
		bool hizo_naive_search = false;
		timespec tiempo_antes_threaded;
		timespec tiempo_despues_threaded;

		mutex moverse;
		mutex terminacion_de_ronda;
		sem_t barrier;
		atomic_int todos_terminaron{0};
		sem_t lejanos;

		bool bandera_found = false;

		vector<int> jugadores_ya_jugaron;
		vector<int> quantums_por_jugador;
		direccion apuntar_a(coordenadas pos2, coordenadas pos1);
		void jugador(int nro_jugador);
		coordenadas buscar_bandera_contraria();
		void reiniciar_quantums();
		void buscar_bandera_naive();
	public:
		Equipo(gameMaster *belcebu, color equipo, 
				estrategia strat, int cant_jugadores, int quantum, vector<coordenadas> posiciones);
		void comenzar();
		void terminar();

};
#endif // EQUIPO_H
